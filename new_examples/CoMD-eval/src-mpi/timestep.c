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

#if _CD2
//unsigned int preserveSimFlat(cd_handle_t *cdh, uint32_t knob, SimFlat *sim, int doeam); 
//unsigned int preserveAtoms(cd_handle_t *cdh, uint32_t knob, Atoms *atoms, int nTotalBoxes,
//                           unsigned int is_all, unsigned int is_gid,
//                           unsigned int is_r, unsigned int is_p,
//                           unsigned int is_f, unsigned int is_U,
//                           unsigned int is_iSpecies, unsigned int from, int to,
//                           unsigned int is_print, char *idx);
//unsigned int preserveSpeciesData(cd_handle_t *cdh, uint32_t knob, SpeciesData *species);
//unsigned int preserveLinkCell(cd_handle_t *cdh, uint32_t knob,
//                              LinkCell *linkcell,
//                              unsigned int is_all, 
//                              unsigned int is_nAtoms, unsigned int is_local,
//                              unsigned int is_nLocalBoxes,
//                              unsigned int is_nTotalBoxes);
//unsigned int preserveHaloAtom(cd_handle_t *cdh, uint32_t knob,
//                              AtomExchangeParms *xchange_parms,
//                              unsigned int is_cellList,
//                              unsigned int is_pbcFactor);
#endif

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
  for (int ii = 0; ii < nSteps; ++ii) {
#if _CD2
//*****************************************************************************
//            cd boundary: velocity (0.08%) (for both)
//*****************************************************************************
    cd_begin(lv2_cd, "advanceVelocity_start"); // lv2_cd starts
    // FIXME: should this be kRef?

    // Preserve local atoms->f (force)
    // TODO: Did this preserved in level1? Yes.
    //       Then, need to skip when both level1 and level2 are enabled
    char idx_advanceVelocity_start[256] = "-1"; // FIXME: it this always enough?
    sprintf(idx_advanceVelocity_start, "vel_start_%d", ii);
    int velocity_pre_size =
        preserveAtoms(lv2_cd, kCopy, s->atoms, 
                      s->boxes->nLocalBoxes, // not Total
                      0,  // is_all
                      0,  // is_gid
                      0,  // is_r
                      0,  // is_p
                      1,  // is_f
                      0,  // is_U
                      0,  // is_iSpecies
                      0,  // from (entire atoms)
                      -1, // to (entire atoms)
                      0,  // is_print
                      //NULL);
                      idx_advanceVelocity_start);
    // Preserve boxes->nLocalBoxes and boxes->nAtoms[0:nLocalBoxes-1]
    velocity_pre_size += preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/,
                                          1 /*nAtoms*/, 1 /*local*/,
                                          1 /*nLocalBoxes*/,
                                          0 /*nTotalBoxes*/);
    // dt is ignored since it's tiny and not changing.

#if DOPRV
    // Preserve loop index (ii)
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, 
                "advanceVelocity_start_ii", "advanceVelocity_start_ii");
#endif //DOPRV
    velocity_pre_size += sizeof(int);  // add the size of ii (loop index)
    //printf("\n preservation size for advanceVelocity(@beggining) %d\n",
    //       velocity_pre_size);
#endif //_CD2

#if _CD2
    // TODO: add kOutput     
    //       s->atoms->p
#endif //_CD2

    startTimer(velocityTimer);
    //------------------------------------------------
    advanceVelocity(s, s->boxes->nLocalBoxes, 0.5 * dt);
    //------------------------------------------------
    stopTimer(velocityTimer);
    
    // TODO: cd_preserve for output with kOutput(?)
    //       output: s->atoms->p
//FIXME:CD2 optimization
//#if _CD2
//    cd_detect(lv2_cd);
//    cd_complete(lv2_cd);
//#endif

//*****************************************************************************
//            cd boundary: position (0.09%)
//*****************************************************************************
#if _CD2
//FIXME:CD2 optimization
//    cd_begin(lv2_cd, "advancePosition");
    // Preserve atoms->p (momenta of local atoms)
    char idx_position[256] = "-1"; // FIXME: it this always enough?
    sprintf(idx_position, "position_%d", ii);
    int position_pre_size = preserveAtoms(lv2_cd, kCopy, s->atoms,
                                          s->boxes->nLocalBoxes, // not Total
                                          0,  // is_all
                                          0,  // is_gid
                                          0,  // is_r
                                          1,  // is_p
                                          0,  // is_f
                                          0,  // is_U
                                          1,  // is_iSpecies
                                          0,  // from (entire atoms)
                                          -1, // to (entire atoms)
                                          0,  // is_print
                                          idx_position);
                                          //NULL);
    // TODO: No need to preserve entier SpeciesData but only mass.
    // But this is tiny anyway so that let's leave it for now.
    position_pre_size += preserveSpeciesData(lv2_cd, kCopy, s->species);
    // Preserve nLocalBoxes and nAtoms[0:nLocalBoxes-1]
    position_pre_size += preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/, 
                                          1 /*only nAtoms*/, 1 /*local*/,
                                          1 /*nLocalBoxes*/,
                                          0 /*nTotalBoxes*/);
    // TODO: kOutput
    //       s->atoms->r
#if DOPRV
//FIXME:CD2 optimization
//    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, 
//                "advancePosition_ii", "advancePosition_ii");
#endif //DOPRV
    position_pre_size += sizeof(int);  // add the size of ii (loop index)
    //printf("\n preservation size for advancePosition %d\n", position_pre_size);
#endif
    startTimer(positionTimer);
    //------------------------------------------------
    advancePosition(s, s->boxes->nLocalBoxes, dt);
    //------------------------------------------------
    stopTimer(positionTimer);
#if _CD2
//    cd_detect(lv2_cd);
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
    sprintf(idx_redist, "redist_%d", ii);
    int redist_pre_size =
#if 0
        preserveAtoms(lv2_cd, kCopy, s->atoms, s->boxes->nTotalBoxes,
                      0, // is_all
                      1, // is_gid
                      1,  // TODO:should be preserved by reference
                      //0,  // is_r //assumed to be preserved by reference
                      1,  // is_p
                      1,  // is_f
                      1,  // is_U
                      0,  // is_iSpecies
                      0,  // from (entire atoms)
                      -1, // to (entire atoms)
                      0,  // is_print
                      idx_redist);
                      //NULL);
#else
        preserveAtoms(lv2_cd, kCopy, s->atoms, s->boxes->nTotalBoxes,
                      1, // is_all
                      0, // is_gid
                      0,  // is_r
                      //0,  // is_r //assumed to be preserved by reference
                      0,  // is_p
                      0,  // is_f
                      0,  // is_U
                      0,  // is_iSpecies
                      0,  // from (entire atoms)
                      -1, // to (entire atoms)
                      0,  // is_print
                      idx_redist);
                      //NULL);

#endif
    // Preserve (almost) all in boxes. Note that this is over-preservation 
    // because boxSize and nHaloBoxes are not required while tiny they are.
    redist_pre_size += preserveLinkCell(lv2_cd, kCopy, s->boxes, 1 /*all*/,
                                        0 /*nAtoms*/, 0 /*local*/, 
                                        0 /*nLocalBoxes*/,
                                        0 /*nTotalBoxes*/);

    // Preserve pbcFactor
    redist_pre_size = preserveHaloAtom(lv2_cd, kCopy, s->atomExchange->parms, 
                                       0 /*cellList*/, 
                                       1 /*pbcFactor*/);
    // TODO: kOutput
    //       s->atoms->r,p,f,U

#if DOPRV
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, 
                "redistributeAtoms_ii", "redistributeAtoms_ii");
#endif //DOPRV
    redist_pre_size += sizeof(int);  // add the size of ii (loop index)
    // printf("\n preservation size for redistributeAtoms %d\n",
    // redist_pre_size);
    // TODO: communication logging?
#endif

    startTimer(redistributeTimer);
    redistributeAtoms(s);
    stopTimer(redistributeTimer);

#if _CD2
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
    sprintf(idx_force, "force_%d", ii);
    int computeForce_pre_size =
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
                      //NULL); // is_print
#if DOPRV
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, 
                "computeForce_ii", "computeForce_ii");
#endif //DOPRV
    computeForce_pre_size += sizeof(int);  // add the size of ii (loop index)

#endif

#if _CD3
    //FIXME: 8 (or getNRanks()) is not working properly as it doesn't get
    //       roll backed.
    //TODO: evalute 1, 2, 4 children cases
    cd_handle_t *lv3_cd = cd_create(getcurrentcd(), /*1,*/ getNRanks(), 
                                    "ljForce", 
                                    kStrict | kDRAM, 0xC);
    //TODO: add interval to control lv3_cd
    // TODO: kOutput
    //       s->atoms->f, U


    const int CD3_INTERVAL = s->preserveRateLevel3;
    //FIXME: this doesn't make sense 
    //if ( ii % CD3_INTERVAL == 0) { 
      cd_begin(lv3_cd, "ljForce_in_timestep");
      // No need to preserve any since it's done already in the parent. 
      // cd_preserve( ... )
    //}
#endif
    startTimer(computeForceTimer);
    computeForce(s); // s->pot->force(s)
    stopTimer(computeForceTimer);
#if _CD3
    //if ( ii % CD3_INTERVAL == 0) { 
      cd_detect(lv3_cd);
      cd_complete(lv3_cd);
      cd_destroy(lv3_cd);
    //}
#endif
#if _CD2
    // Do I need cd_detect here when level2 is enabled? Yes, it won't
    // double detect here and in level3. (FIXME: should be verfified)
//    cd_detect(lv2_cd);
    cd_complete(lv2_cd);
#endif
//*****************************************************************************
//            cd boundary : advanceVelocity (@ end)
//*****************************************************************************
#if _CD2
    cd_begin(lv2_cd, "advanceVelocity_end");
    // Preserve local atoms->f (force)
    char idx_advanceVelocity_end[256] = "-1"; // FIXME: it this always enough?
    sprintf(idx_advanceVelocity_end, "vel_end__%d", ii);
    int velocity_end_pre_size =
        preserveAtoms(lv2_cd, kCopy,
                      // s->atoms, s->boxes->nTotalBoxes,
                      s->atoms, s->boxes->nLocalBoxes,
                      0,  // is_all
                      0,  // is_gid
                      0,  // is_r
                      0,  // is_p
                      1,  // is_f
                      0,  // is_U
                      0,  // is_iSpecies
                      0,  // from (entire atoms)
                      -1, // to (entire atoms)
                      0,  // is_print
                      idx_advanceVelocity_end);
    // Preserve boxes->nLocalBoxes and boxes->nAtoms[0:nLocalBoxes-1]
    velocity_end_pre_size += preserveLinkCell(lv2_cd, kCopy, s->boxes, 0 /*all*/,
                                              1 /*only nAtoms*/, 1 /*local*/,
                                              1 /*nLocalBoxes*/,
                                              0 /*nTotalBoxes*/);
    // TODO: kOutput
    //       s->atoms->p

#if DOPRV
    // Preserve loop index (ii)
    cd_preserve(lv2_cd, &ii, sizeof(int), kCopy, 
                "advanceVelocity_end_ii", "advanceVelocity_end_ii");
#endif //DOPRV
    velocity_end_pre_size += sizeof(int);  // add the size of ii (loop index)

    //printf("\n preservation size for advanceVelocity(@end) %d\n",
    //       velocity_end_pre_size);
#endif
    startTimer(velocityTimer);
    advanceVelocity(s, s->boxes->nLocalBoxes, 0.5 * dt);
    stopTimer(velocityTimer);
#if _CD2
//    cd_detect(lv2_cd);
    cd_complete(lv2_cd);

#endif
  }

  // This is out of level 2 cd
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
