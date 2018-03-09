/// \file
/// Leapfrog time integrator

#include "timestep.h"

#include "CoMDTypes.h"
#include "linkCells.h"
#include "parallel.h"
#include "performanceTimers.h"

#if _CD2
#include "cd.h"
#include "cd_comd.h"
#endif

static void advanceVelocity(SimFlat *s, int nBoxes, real_t dt);
static void advancePosition(SimFlat *s, int nBoxes, real_t dt);

/// Advance the simulation time to t+dt using a leap frog method
/// (equivalent to velocity verlet).
///
/// Forces must be computed before calling the integrator the first time.
///
///  - Advance velocities half time step using forces
///  - Advance positions full time step using velocities
///  - Update link cells and exchange remote particles
///  - Compute forces
///  - Update velocities half time step using forces
///
/// This leaves positions, velocities, and forces at t+dt, with the
/// forces ready to perform the half step velocity update at the top of
/// the next call.
///
/// After nSteps the kinetic energy is computed for diagnostic output.
double timestep(SimFlat *s, int nSteps, real_t dt) {

#if _CD2
  cd_handle_t *lv2_cd = getleafcd();
#endif
  // This will iterate as many as specified in "printRate (default:10)"
  for (int ii = 0; ii < nSteps; ++ii) {
#if _CD2
    //*****************************************************************************
    //            cd boundary: velocity (0.08%) (for both)
    //*****************************************************************************
    cd_begin(lv2_cd, "advanceVelocity_start"); // lv2_cd starts
    // FIXME: should this be kRef for the first iteration?
    // Note that we need to preserv atoms-> p as well 
    // Preserve local atoms->f (force) and atoms->p (momentum) (read and written)
    // TODO: Did this preserved in level1? Yes.
    //       Then, need to skip when both level1 and level2 are enabled
    //char idx_advanceVelocity_start[256] = "-1"; // FIXME: it this always enough?
    //sprintf(idx_advanceVelocity_start, "_vel_start_%d", ii);

    destroyDataDuringReexecution(s, -1); // all ranks destroy pointers and atom

    int velocity_pre_size = preserveAtoms(lv2_cd, kCopy, s->atoms,
                                          s->boxes->nLocalBoxes, // not Total
                                          //s->boxes->nTotalBoxes, // not Total
                                          0,                     // is_all
                                          0,                     // is_gid
                                          0,                     // is_r
                                          1,                     // is_p
                                          1,                     // is_f
                                          0,                     // is_U
                                          0,                     // is_iSpecies
                                          0,  // from (entire atoms)
                                          -1, // to (entire atoms)
                                          0,  // is_print
                                          NULL);
                                          //idx_advanceVelocity_start);
    // Preserve boxes->nLocalBoxes and boxes->nAtoms[0:nLocalBoxes-1]
    // TODO: this doesn't gets changed until redistribtueAtoms below.
    //       so that can be preserved via reference before it
    // TODO: also, preserveAtoms actually preserve maximum number of atoms 
    //        in a box. But still we need nAtoms for each box?
    velocity_pre_size +=
        preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/, 1 /*nAtoms*/,
                         1 /*local*/, 1 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
    // dt is ignored since it's tiny and not changing.

#if DO_PRV
    // Preserve loop index (ii)
    // TODO: this is not the most efficient way of preserving loop index
    //       also we don't have to distinguish this in the sequential level2 CDs.
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "advanceVelocity_start_ii",
                "advanceVelocity_start_ii");
#endif                                // DO_PRV
    velocity_pre_size += sizeof(int); // add the size of ii (loop index)
    // printf("\n preservation size for advanceVelocity(@beggining) %d\n",
    //       velocity_pre_size);
#endif //_CD2

    startTimer(velocityTimer);
    //------------------------------------------------
    advanceVelocity(s, s->boxes->nLocalBoxes, 0.5 * dt);
    //------------------------------------------------
    stopTimer(velocityTimer);

// FIXME:CD2 optimization: merge advanceVelocity_start and advancePosition
// TODO: restore this to have finer grained CD for estimator to discover optimal
// mapping
#if _CD2
#if DO_OUTPUT
    // TODO: cd_preserve for output with kOutput(?)
    //       output: s->atoms->p
    //
    int velocity_pre_out_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms,
                      s->boxes->nLocalBoxes,      // not Total
                      0,                          // is_all
                      0,                          // is_gid
                      0,                          // is_r
                      1,                          // is_p
                      0,                          // is_f
                      0,                          // is_U
                      0,                          // is_iSpecies
                      0,                          // from (entire atoms)
                      -1,                         // to (entire atoms)
                      0,                          // is_print
                      idx_advanceVelocity_start); // FIXME: should this be same
                                                  // as one at position?
//    // s->boxes are not updated but just read in advanceVelocity()
#endif
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif

//*****************************************************************************
//            cd boundary: position (0.09%)
//*****************************************************************************
#if _CD2
    // FIXME:CD2 optimization
    cd_begin(lv2_cd, "advancePosition");
    // Preserve atoms->p (momenta of local atoms)
    char idx_position[256] = "-1"; // FIXME: it this always enough?
    sprintf(idx_position, "_position_%d", ii);
    int position_pre_size = preserveAtoms(lv2_cd, kCopy, s->atoms,
                                          s->boxes->nLocalBoxes, // not Total
                                          0,                     // is_all
                                          0,                     // is_gid
                                          0,                     // is_r
                                          1,                     // is_p
                                          0,                     // is_f
                                          0,                     // is_U
                                          1,                     // is_iSpecies
                                          0,  // from (entire atoms)
                                          -1, // to (entire atoms)
                                          0,  // is_print
                                          idx_position);
    // NULL);
    // TODO: No need to preserve entier SpeciesData but only mass.
    // But this is tiny anyway so that let's leave it for now.
    position_pre_size += preserveSpeciesData(lv2_cd, kCopy, s->species);
    // Preserve nLocalBoxes and nAtoms[0:nLocalBoxes-1]
    position_pre_size +=
        preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/, 1 /*only nAtoms*/,
                         1 /*local*/, 1 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
#if DO_PRV
    // FIXME:CD2 optimization
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "advancePosition_ii",
                "advancePosition_ii");
#endif                                // DO_PRV
    position_pre_size += sizeof(int); // add the size of ii (loop index)
    // printf("\n preservation size for advancePosition %d\n",
    // position_pre_size);
#endif
    startTimer(positionTimer);
    //------------------------------------------------
    advancePosition(s, s->boxes->nLocalBoxes, dt);
    //------------------------------------------------
    stopTimer(positionTimer);
#if _CD2
#if DO_OUTPUT
    // TODO: kOutput
    //       s->atoms->r
    int position_pre_out_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms,
                      s->boxes->nLocalBoxes, // not Total
                      0,                     // is_all
                      0,                     // is_gid
                      1,                     // is_r
                      0,                     // is_p
                      0,                     // is_f
                      0,                     // is_U
                      0,                     // is_iSpecies
                      0,                     // from (entire atoms)
                      -1,                    // to (entire atoms)
                      0,                     // is_print
                      idx_position); // FIXME: what name should be given?
#endif
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif

//-----------------------------------------------------------------------
//            Communication
//-----------------------------------------------------------------------
//*****************************************************************************
//            cd boundary: redistribution (6.88%)
//*****************************************************************************
#if _CD2
    cd_begin(lv2_cd, "redistributeAtoms");
    // TODO: preserve nAtoms by kRef
    // TODO: For optimization,
    // only atoms->r for local cells needs to be preserved since it's update
    // right before this while r for halo cells still need to be preserved.
    // f and p are preserved in velocty_start and postion respectively, meaning
    // being able to be preserved by referece

    // For now, let's preserve everything required to evaluate from here
    // Preserve atoms->r, p, f, U
    char idx_redist[256] = "-1"; // FIXME: it this always enough?
    sprintf(idx_redist, "_redist_%d", ii);
    int redist_pre_size =
        // There is a possibility that any atome can be moved and it is not
        // known statically. Therefore, we may have to preserve all atoms
        // conservativelly.
        preserveAtoms(lv2_cd, kCopy, s->atoms, s->boxes->nTotalBoxes,
                      1, // is_all
                      0, // is_gid
                      0, // is_r
                      // 0,  // is_r //assumed to be preserved by reference
                      0,  // is_p
                      0,  // is_f
                      0,  // is_U
                      0,  // is_iSpecies
                      0,  // from (entire atoms)
                      -1, // to (entire atoms)
                      0,  // is_print
                      idx_redist);
    // NULL);
    // Preserve (almost) all in boxes. Note that this is over-preservation
    // because boxSize and nHaloBoxes are not required while tiny they are.
    // TODO: preserve nAtoms[nLocalBoxes:nTotalBoxes] as shown below
    // redist_pre_size += preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/,
    //                                    1 /*nAtoms*/, 2 /*local*/,
    //                                    0 /*nLocalBoxes*/,
    //                                    0 /*nTotalBoxes*/);
    redist_pre_size +=
        preserveLinkCell(lv2_cd, kCopy, s->boxes, 1 /*all*/, 0 /*nAtoms*/,
                         0 /*local*/, 0 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
    // Preserve pbcFactor
    redist_pre_size = preserveHaloAtom(lv2_cd, kCopy, s->atomExchange->parms,
                                       1 /*cellList*/, 1 /*pbcFactor*/);
//    int redist_pre_size = preserveSimFlat(lv2_cd, kCopy, s);
#if DO_PRV
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "redistributeAtoms_ii",
                "redistributeAtoms_ii");
#endif                              // DOPRV
    redist_pre_size += sizeof(int); // add the size of ii (loop index)
    // printf("\n preservation size for redistributeAtoms %d\n",
    // redist_pre_size);
    // TODO: communication logging?
#endif

    startTimer(redistributeTimer);
    redistributeAtoms(s);
    stopTimer(redistributeTimer);

#if _CD2
#if DO_OUTPUT
    // TODO: kOutput
    //       s->atoms->r,p,f,U
    //       s->boxes->nTotalBoxes?
    int redist_pre_out_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms, s->boxes->nTotalBoxes,
                      0,  // is_all
                      0,  // is_gid
                      1,  // is_r //assumed to be preserved by reference
                      1,  // is_p
                      1,  // is_f
                      1,  // is_U
                      0,  // is_iSpecies
                      0,  // from (entire atoms)
                      -1, // to (entire atoms)
                      0,  // is_print
                      idx_redist); // FIXME: correct name
    // TODO: kOutput
    //       boxes->nAtoms[nLocalBoxes:nTotalBoxes] (only HaloCells)
    redist_pre_out_size =
        preserveLinkCell(lv2_cd, kOutput, s->boxes, 0 /*all*/, 1 /*nAtoms*/,
                         0 /*local*/, 0 /*nLocalBoxes*/, 1 /*nTotalBoxes*/);
#endif
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif
//-----------------------------------------------------------------------
//            Communication
//-----------------------------------------------------------------------

//*****************************************************************************
//            cd boundary: force (92.96%)
//*****************************************************************************
#if _CD2
    cd_begin(lv2_cd, "computeForce");

    // Preserve atoms->r (postions)
    char idx_force[256] = "-1"; // FIXME: it this always enough?
    sprintf(idx_force, "_force_%d", ii);
    int computeForce_pre_lv2_size =
        preserveAtoms(lv2_cd, kCopy, s->atoms, s->boxes->nLocalBoxes,
                      0, // is_all
                      1, // is_gid
                      1, // is_r
                      0, // is_p
                      0, // is_f
                      0, // is_U
                      0, // is_iSpecies
                      // MAXATOMS*jBox,          // from
                      // MAXATOMS*jBox+nJBox-1,  // to
                      0,  // from
                      -1, // to
                      0,
                      idx_force); // is_print
                                  // NULL); // is_print
#if DO_PRV
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "computeForce_ii",
                "computeForce_ii");
#endif                                        // DO_PRV
    computeForce_pre_lv2_size += sizeof(int); // add the size of ii (loop index)

#endif

#if _CD3
    // FIXME: eam force has cocmmunication in it so that we can't create siblings there
    // TODO: add switch to choose right CD either of LJ or EAM, depending on doeam
    cd_handle_t *lv3_cd = cd_create(getcurrentcd(), /*1,*/ getNRanks(),
                                    "ljForce", kStrict | kLocalMemory, 0xC);
    // kStrict | kDRAM, 0xC);
    // TODO: add interval to control lv3_cd
    const int CD3_INTERVAL = s->preserveRateLevel3;
    // FIXME: this doesn't make sense
    // if ( ii % CD3_INTERVAL == 0) {
    // FIXME: this can be either LJ potential or EAM potential
    cd_begin(lv3_cd, "ljForce_in_timestep");
    // FIXME: check kRef semantic
    // FIXME: This is not correct implementation. In level 4 CD, it has finer
    //       grained than level 3 and the ref names should match with level 4
    //       Begin/Complete interval
    // Okay to reuse the same index. actually should
    // char idx_force[256] = "-1"; // FIXME: it this always enough?
    // sprintf(idx_force, "force_%d", ii);
    int computeForce_pre_lv3_size =
        preserveAtoms(lv2_cd, kRef, s->atoms, s->boxes->nLocalBoxes,
                      0, // is_all
                      1, // is_gid
                      1, // is_r
                      0, // is_p
                      0, // is_f
                      0, // is_U
                      0, // is_iSpecies
                      // MAXATOMS*jBox,          // from
                      // MAXATOMS*jBox+nJBox-1,  // to
                      0,  // from
                      -1, // to
                      0,  // is_print
                      idx_force);
    // NULL);

    // FIXME: why do I need lv3_cd here? to create parallel children?
    // No need to preserve any since it's done already in the parent (lv2_cd).
    // cd_preserve( ... )
    //}
#endif
    startTimer(computeForceTimer);
    // call either eamForce or ljForce
    computeForce(s); // s->pot->force(s) 
    stopTimer(computeForceTimer);
#if _CD3
    // if ( ii % CD3_INTERVAL == 0) {
    // TODO: kOutput (lv3_cd)
    //       s->atoms->f, U
#if DO_OUTPUT
    int computeForce_pre_output_lv3_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms, s->boxes->nLocalBoxes,
                      0, // is_all
                      0, // is_gid
                      0, // is_r
                      0, // is_p
                      1, // is_f
                      1, // is_U
                      0, // is_iSpecies
                      // MAXATOMS*jBox,          // from
                      // MAXATOMS*jBox+nJBox-1,  // to
                      0,          // from
                      -1,         // to
                      0,          // is_print
                      idx_force); // FIXME: correct name?
#endif
    cd_detect(lv3_cd);
    cd_complete(lv3_cd);
    cd_destroy(lv3_cd);
    //}
#endif
#if _CD2
    // Do I need cd_detect here when level2 is enabled? Yes, it won't
    // double detect here and in level3. (FIXME: should be verfified)
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif
//*****************************************************************************
//            cd boundary : advanceVelocity (@ end)
//*****************************************************************************
#if _CD2
    cd_begin(lv2_cd, "advanceVelocity_end");
    // Preserve local atoms->f (force) and atoms->p (momentum) (read and written)
    char idx_advanceVelocity_end[256] = "-1"; // FIXME: it this always enough?
    sprintf(idx_advanceVelocity_end, "_vel_end_%d", ii);
    int velocity_end_pre_size =
        preserveAtoms(lv2_cd, kCopy,
                      // s->atoms, s->boxes->nTotalBoxes,
                      s->atoms, s->boxes->nLocalBoxes,
                      0,  // is_all
                      0,  // is_gid
                      0,  // is_r
                      1,  // is_p
                      1,  // is_f
                      0,  // is_U
                      0,  // is_iSpecies
                      0,  // from (entire atoms)
                      -1, // to (entire atoms)
                      0,  // is_print
                      NULL);
                      //idx_advanceVelocity_end);
#if DO_PRV
    // Preserve loop index (ii)
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "advanceVelocity_end_ii",
                "advanceVelocity_end_ii");
#endif                                    // DO_PRV
    velocity_end_pre_size += sizeof(int); // add the size of ii (loop index)

    // printf("\n preservation size for advanceVelocity(@end) %d\n",
    //       velocity_end_pre_size);
#endif
    startTimer(velocityTimer);
    advanceVelocity(s, s->boxes->nLocalBoxes, 0.5 * dt);
    stopTimer(velocityTimer);
#if _CD2
#if DO_OUTPUT
    // TODO: kOutput
    //       s->atoms->p
    int velocity_end_pre_out_size =
        preserveAtoms(lv2_cd, kOutput,
                      s->atoms, s->boxes->nTotalBoxes,
                      //s->atoms, s->boxes->nLocalBoxes,
                      0,                        // is_all
                      0,                        // is_gid
                      0,                        // is_r
                      1,                        // is_p
                      0,                        // is_f
                      0,                        // is_U
                      0,                        // is_iSpecies
                      0,                        // from (entire atoms)
                      -1,                       // to (entire atoms)
                      0,                        // is_print
                      idx_advanceVelocity_end); // FIXME: what should be given?
#endif

    cd_detect(lv2_cd);
    cd_complete(lv2_cd);

#endif
  } // for loop

  // This is out of level 2 cd and a part of level 1 cd
  // TODO: eKinteic should be preserved
  kineticEnergy(s);

  return s->ePotential;
}

void computeForce(SimFlat *s) { s->pot->force(s); }

void advanceVelocity(SimFlat *s, int nBoxes, real_t dt) {
  for (int iBox = 0; iBox < nBoxes; iBox++) {
    for (int iOff = MAXATOMS * iBox, ii = 0; ii < s->boxes->nAtoms[iBox];
         ii++, iOff++) {
      s->atoms->p[iOff][0] += dt * s->atoms->f[iOff][0];
      s->atoms->p[iOff][1] += dt * s->atoms->f[iOff][1];
      s->atoms->p[iOff][2] += dt * s->atoms->f[iOff][2];
    }
  }
}

void advancePosition(SimFlat *s, int nBoxes, real_t dt) {
  for (int iBox = 0; iBox < nBoxes; iBox++) {
    for (int iOff = MAXATOMS * iBox, ii = 0; ii < s->boxes->nAtoms[iBox];
         ii++, iOff++) {
      int iSpecies = s->atoms->iSpecies[iOff];
      real_t invMass = 1.0 / s->species[iSpecies].mass;
      s->atoms->r[iOff][0] += dt * s->atoms->p[iOff][0] * invMass;
      s->atoms->r[iOff][1] += dt * s->atoms->p[iOff][1] * invMass;
      s->atoms->r[iOff][2] += dt * s->atoms->p[iOff][2] * invMass;
    }
  }
}

/// Calculates total kinetic and potential energy across all tasks.  The
/// local potential energy is a by-product of the force routine.
void kineticEnergy(SimFlat *s) {
  real_t eLocal[2];
  eLocal[0] = s->ePotential;
  eLocal[1] = 0;
  for (int iBox = 0; iBox < s->boxes->nLocalBoxes; iBox++) {
    for (int iOff = MAXATOMS * iBox, ii = 0; ii < s->boxes->nAtoms[iBox];
         ii++, iOff++) {
      int iSpecies = s->atoms->iSpecies[iOff];
      real_t invMass = 0.5 / s->species[iSpecies].mass;
      eLocal[1] += (s->atoms->p[iOff][0] * s->atoms->p[iOff][0] +
                    s->atoms->p[iOff][1] * s->atoms->p[iOff][1] +
                    s->atoms->p[iOff][2] * s->atoms->p[iOff][2]) *
                   invMass;
    }
  }

  real_t eSum[2];
  startTimer(commReduceTimer);
  addRealParallel(eLocal, eSum, 2);
  stopTimer(commReduceTimer);

  s->ePotential = eSum[0];
  s->eKinetic = eSum[1];
}

/// \details
/// This function provides one-stop shopping for the sequence of events
/// that must occur for a proper exchange of halo atoms after the atom
/// positions have been updated by the integrator.
///
/// - updateLinkCells: Since atoms have moved, some may be in the wrong
///   link cells.
/// - haloExchange (atom version): Sends atom data to remote tasks.
/// - sort: Sort the atoms.
///
/// \see updateLinkCells
/// \see initAtomHaloExchange
/// \see sortAtomsInCell
void redistributeAtoms(SimFlat *sim) {
  updateLinkCells(sim->boxes, sim->atoms);

  startTimer(atomHaloTimer);
  haloExchange(sim->atomExchange, sim);
  stopTimer(atomHaloTimer);

  for (int ii = 0; ii < sim->boxes->nTotalBoxes; ++ii)
    sortAtomsInCell(sim->atoms, sim->boxes, ii);
}
