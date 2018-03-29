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
  // Note that l2_cd, which is heterogeneous CDs with 5 children, can not be
  // optimized by adjusting interval but only done by merging 5 sequential CDs
  // ,guided by estimators.
  const int CD2_INTERVAL = s->preserveRateLevel2;
#endif
  // This will iterate as many as specified in "printRate (default:10)"
  for (int ii = 0; ii < nSteps; ++ii) {
#if _CD2_COMBINE
#if _CD2
    //************************************************************************//
    //            cd boundary: velocity (0.08%) (for both)
    //************************************************************************//
    cd_begin(lv2_cd, "lv2_combined");
    // FIXME: should this be kRef for the first iteration?
    // FIXME: this is not working corectly.
    int lv2_combined_pre_size = preserveSimFlat(lv2_cd, kCopy, s);
    //int lv2_combined_pre_size = preserveAtomsInLocalBox(
    //    lv2_cd, kCopy, s->atoms, s->boxes->nLocalBoxes, 0);
    //lv2_combined_pre_size +=
    //    preserveAtomsInHaloBox(lv2_cd, kCopy, s->atoms, s->boxes->nLocalBoxes,
    //                           s->boxes->nTotalBoxes, 0);
    //// TODO: No need to preserve entier SpeciesData but only mass.
    //// But this is tiny anyway so that let's leave it for now.
    //lv2_combined_pre_size += preserveSpeciesData(lv2_cd, kCopy, s->species);
    //lv2_combined_pre_size +=
    //    preserveLinkCell(lv2_cd, kCopy, s->boxes, 1 /*all*/, 0 /*nAtoms*/,
    //                     0 /*local*/, 0 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
    //// Preserve HaloAtom
    //lv2_combined_pre_size += preserveHaloAtom(
    //    lv2_cd, kCopy, s->atomExchange->parms, 1 /*cellList*/, 1 /*pbcFactor*/);

#if DO_PRV
    // Preserve loop index (ii)
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "timestep_ii", "timestep_ii");
#endif // DO_PRV
    lv2_combined_pre_size += sizeof(int); // add the size of ii (loop index)

#endif // _CD2
#else  // _CD2_COMBINE
#if _CD2
    //************************************************************************//
    //            cd boundary: velocity (0.08%) (for both)
    //************************************************************************//
    // Note that this is to be preserved at lv1_cd
    cd_begin(lv2_cd,
             "advanceVelocity_start"); // lv2_cd starts ( 1 / 5 sequential CDs)

    // FIXME: should this be kRef for the first iteration?
    // Note that we need to preserv atoms-> p as well
    // Preserve local atoms->f (force) and atoms->p (momentum) (read and
    // written)
    int velocity_pre_size = preserveAtoms(lv2_cd, kCopy, s->atoms,
                                          0, // is_gid
                                          0, // is_r
                                          1, // is_p
                                          1, // is_f
                                          0, // is_U
                                          0, // is_iSpecies
                                          0, // from (entire atoms)
                                          s->boxes->nLocalBoxes, // to (Local)
                                          0,                     // is_print
                                          "Local");
    // Preserve boxes->nLocalBoxes and boxes->nAtoms[0:nLocalBoxes-1]
    // TODO: this doesn't gets changed until redistribtueAtoms below.
    //       so that can be preserved via reference before it
    // TODO: also, preserveAtoms actually preserve maximum number of atoms
    //        in a box. But still we need nAtoms for each box?
    velocity_pre_size +=
        preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/, 1 /*nAtoms*/,
                         1 /*local*/, 1 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
    // dt is ignored since it's tiny and not changing. (assumed to be preserved
    // via Reference)

#if DO_PRV
    // Preserve loop index (ii)
    // TODO: this is not the most efficient way of preserving loop index
    //       also we don't have to distinguish this in the sequential level2
    //       CDs.
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "timestep_ii", "timestep_ii");
#endif // DO_PRV
    velocity_pre_size += sizeof(int); // add the size of ii (loop index)
#endif // _CD2
#endif // _CD2_COMBINE

    startTimer(velocityTimer);
    //------------------------------------------------
    advanceVelocity(s, s->boxes->nLocalBoxes, 0.5 * dt);
    //------------------------------------------------
    stopTimer(velocityTimer);

// TODO:add CD2 optimization: merge advanceVelocity_start and advancePosition
#if _CD2_COMBINE
    // nothing to do
#else // _CD2_COMBINE
#if _CD2
#if _CD2_OUTPUT
    int velocity_pre_out_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms,
                      0,                     // is_gid
                      0,                     // is_r
                      1,                     // is_p
                      0,                     // is_f
                      0,                     // is_U
                      0,                     // is_iSpecies
                      0,                     // from (entire atoms)
                      s->boxes->nLocalBoxes, // to (entire atoms)
                      0,                     // is_print
                      "Local");
#endif // _CD2_OUTPUT
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif // _CD2
#endif // _CD2_COMBINE

//*****************************************************************************
//            cd boundary: position (0.09%)
//*****************************************************************************
// TODO: CD2 optimization (combination of potential merges among 5 sequential
//       CDs shown below.
#if _CD2_COMBINE
#else // _CD2_COMBINE
#if _CD2
    cd_begin(lv2_cd, "advancePosition"); //  2nd (/ 5 sequential lv2_cd s)

    // Preserve atoms->p (momenta of local atoms) and atoms->r
    int position_pre_size =
        preserveAtoms(lv2_cd, kCopy, s->atoms,
                      0,                     // is_gid
                      1,                     // is_r
                      1,                     // is_p
                      0,                     // is_f
                      0,                     // is_U
                      1,                     // is_iSpecies
                      0,                     // from (entire atoms)
                      s->boxes->nLocalBoxes, // to (entire atoms)
                      0,                     // is_print
                      "Local");
    // TODO: No need to preserve entier SpeciesData but only mass.
    // But this is tiny anyway so that let's leave it for now.
    position_pre_size += preserveSpeciesData(lv2_cd, kCopy, s->species);
    // Preserve nLocalBoxes and nAtoms[0:nLocalBoxes-1]
    position_pre_size +=
        preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/, 1 /*only nAtoms*/,
                         1 /*local*/, 1 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
#if DO_PRV
    // FIXME:CD2 optimization
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "timestep_ii", "timestep_ii");

#endif // DO_PRV
    position_pre_size += sizeof(int); // add the size of ii (loop index)

#endif // _CD2
#endif // _CD2_COMBINE
    startTimer(positionTimer);
    //------------------------------------------------
    advancePosition(s, s->boxes->nLocalBoxes, dt);
    //------------------------------------------------
    stopTimer(positionTimer);
#if _CD2_COMBINE
    // nothing to do
#else // _CD2_COMBINE
#if _CD2
#if _CD2_OUTPUT
    int position_pre_out_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms,
                      0,                     // is_gid
                      1,                     // is_r
                      0,                     // is_p
                      0,                     // is_f
                      0,                     // is_U
                      0,                     // is_iSpecies
                      0,                     // from (entire atoms)
                      s->boxes->nLocalBoxes, // to (entire atoms)
                      0,                     // is_print
                      "Local");
#endif // _CD2_OUTPUT
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif // _CD2
#endif // _CD2_COMBINE

//-----------------------------------------------------------------------
//            Communication
//-----------------------------------------------------------------------
//*****************************************************************************
//            cd boundary: redistribution (6.88%)
//*****************************************************************************
#if _CD2_COMBINE
    // nothing to do
#else // _CD2_COMBINE
#if _CD2
    cd_begin(lv2_cd, "redistributeAtoms"); //  3rd (/ 5 sequential lv2_cd s)

    // TODO: For optimization,
    // only atoms->r for local cells needs to be preserved since it's update
    // right before this while r for halo cells still need to be preserved.
    // f and p are preserved in velocty_start and postion respectively,
    // meaning being able to be preserved by referece

    // For now, let's preserve everything required to evaluate from here
    // Preserve atoms->r, p, f, U
    int redist_pre_size = preserveAtomsInLocalBox(lv2_cd, kCopy, s->atoms,
                                                  s->boxes->nLocalBoxes, 0);
    redist_pre_size +=
        preserveAtomsInHaloBox(lv2_cd, kCopy, s->atoms, s->boxes->nLocalBoxes,
                               s->boxes->nTotalBoxes, 0);
    // Preserve (almost) all in boxes. Note that this is over-preservation
    // because boxSize and nHaloBoxes are not required while tiny they are.
    redist_pre_size +=
        preserveLinkCell(lv2_cd, kCopy, s->boxes, 1 /*all*/, 0 /*nAtoms*/,
                         0 /*local*/, 0 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
    // Preserve HaloAtom
    redist_pre_size += preserveHaloAtom(lv2_cd, kCopy, s->atomExchange->parms,
                                        1 /*cellList*/, 1 /*pbcFactor*/);
#if DO_PRV
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "timestep_ii", "timestep_ii");
#endif // DOPRV
    redist_pre_size += sizeof(int); // add the size of ii (loop index)
#endif // _CD2
#endif // _CD2_COMBINE
    startTimer(redistributeTimer);
    redistributeAtoms(s);
    stopTimer(redistributeTimer);
#if _CD2_COMBINE
    // nothing to do
#else // _CD2_COMBINE
#if _CD2
#if _CD2_OUTPUT
    // FIXME: not verified
    int redist_pre_out_size = preserveAtomsInLocalBox(lv2_cd, kOutput, s->atoms,
                                                      s->boxes->nLocalBoxes, 0);
    redist_pre_out_size +=
        preserveAtomsInHaloBox(lv2_cd, kOutput, s->atoms, s->boxes->nLocalBoxes,
                               s->boxes->nTotalBoxes, 0);
    // FIXME: not verified
    redist_pre_out_size =
        preserveLinkCell(lv2_cd, kOutput, s->boxes, 1 /*all*/, 0 /*nAtoms*/,
                         0 /*local*/, 0 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
#endif // _CD2_OUTPUT
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif // _CD2
#endif // _CD2_COMBINE
//-----------------------------------------------------------------------
//            Communication
//-----------------------------------------------------------------------

//*****************************************************************************
//            cd boundary: force (92.96%)
//*****************************************************************************
#if _CD2_COMBINE
    // nothing to do
#else // _CD2_COMBINE
#if _CD2
    cd_begin(lv2_cd, "computeForce"); // 4th (/ 5 sequential lv2_cd s)

    // Preserve atoms->r (postions)
    int computeForce_pre_lv2_size = preserveAtoms(lv2_cd, kCopy, s->atoms,
                                                  1, // is_gid
                                                  1, // is_r
                                                  0, // is_p
                                                  0, // is_f
                                                  0, // is_U
                                                  0, // is_iSpecies
                                                  0, // from
                                                  s->boxes->nLocalBoxes, // to
                                                  0, "Local");
#if DO_PRV
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "timestep_ii", "timestep_ii");
#endif // DO_PRV
    computeForce_pre_lv2_size += sizeof(int); // add the size of ii (loop index)
#endif // _CD2
#endif // _CD2_COMBINE

#if _CD3 && _CD2
    // FIXME: this can be either LJ potential or EAM potential
    // FIXME: eam force has cocmmunication in it so that we can't create
    // parallel children there
    // TODO: add switch to choose right CD either of LJ or EAM, depending on
    // doeam

    // TODO(estimator): will determine the optimal number of parallel children
    cd_handle_t *lv3_cd =
#if _CD3_NO_SPLIT
        cd_create(getcurrentcd(), 1 /*getNRanks(),*/, "computeForce_loop",
#else
        cd_create(getcurrentcd(), /*1,*/ getNRanks(), "computeForce_loop",
#endif
                  kStrict | kLocalMemory, 0x1);
#endif // _CD3 && _CD2
    startTimer(computeForceTimer);
    // call either eamForce or ljForce
    computeForce(s); // s->pot->force(s)
    stopTimer(computeForceTimer);
#if _CD3 && _CD2
    // cd_detect(lv3_cd);
    // cd_complete(lv3_cd);
    cd_destroy(lv3_cd);
#endif // _CD3 && _CD2

#if _CD2_COMBINE
    // nothing to do
#else // _CD2_COMBINE
#if _CD2
    // Do I need cd_detect here when level2 is enabled? Yes, it won't
    // double detect here and in level3.
#if _CD2_OUTPUT
    // TODO: kOutput (lv2_cd)
    //       s->atoms->f, U
    int computeForce_pre_output_lv2_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms,
                      0,                     // is_gid
                      0,                     // is_r
                      0,                     // is_p
                      1,                     // is_f
                      1,                     // is_U
                      0,                     // is_iSpecies
                      0,                     // from
                      s->boxes->nLocalBoxes, // to
                      0,                     // is_print
                      "Local");
#endif // _CD2_OUTPUT
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif // _CD2
#endif // _CD2_COMBINE
//*****************************************************************************
//            cd boundary : advanceVelocity (@ end)
//*****************************************************************************
#if _CD2_COMBINE
    // nothing to do
#else // _CD2_COMBINE
#if _CD2
    cd_begin(lv2_cd, "advanceVelocity_end"); // 5th (/ 5 sequential lv2_cd s)
    // Preserve local atoms->f (force) and atoms->p (momentum) (read and
    // written)
    int velocity_end_pre_size =
        preserveAtoms(lv2_cd, kCopy, s->atoms,
                      0,                     // is_gid
                      0,                     // is_r
                      1,                     // is_p
                      1,                     // is_f
                      0,                     // is_U
                      0,                     // is_iSpecies
                      0,                     // from (entire atoms)
                      s->boxes->nLocalBoxes, // to (entire atoms)
                      0,                     // is_print
                      "Local");
#if DO_PRV
    // Preserve loop index (ii)
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, "timestep_ii", "timestep_ii");
#endif // DO_PRV
    velocity_end_pre_size += sizeof(int); // add the size of ii (loop index)
#endif // _CD2
#endif // _CD2_COMBINE
    startTimer(velocityTimer);
    advanceVelocity(s, s->boxes->nLocalBoxes, 0.5 * dt);
    stopTimer(velocityTimer);
#if _CD2_COMBINE
    // same thing when not CD2 combined
#if _CD2
#if _CD2_OUTPUT
    int velocity_end_pre_out_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms,
                      0,                     // is_gid
                      0,                     // is_r
                      1,                     // is_p
                      0,                     // is_f
                      0,                     // is_U
                      0,                     // is_iSpecies
                      0,                     // from (entire atoms)
                      s->boxes->nLocalBoxes, // to (entire atoms)
                      0,                     // is_print
                      "Local");
#endif // _CD2_OUTPUT
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif // _CD2
#else  // _CD2_COMBINE
#if _CD2
#if _CD2_OUTPUT
    int velocity_end_pre_out_size =
        preserveAtoms(lv2_cd, kOutput, s->atoms,
                      0,                     // is_gid
                      0,                     // is_r
                      1,                     // is_p
                      0,                     // is_f
                      0,                     // is_U
                      0,                     // is_iSpecies
                      0,                     // from (entire atoms)
                      s->boxes->nLocalBoxes, // to (entire atoms)
                      0,                     // is_print
                      "Local");
#endif // _CD2_OUTPUT
    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif // _CD2
#endif // _CD2_COMBINE
  }    // for loop

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
