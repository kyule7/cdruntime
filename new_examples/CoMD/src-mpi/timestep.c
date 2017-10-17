/// \file
/// Leapfrog time integrator

#include "timestep.h"

#include "CoMDTypes.h"
#include "linkCells.h"
#include "parallel.h"
#include "performanceTimers.h"

#if _CD
#include "cd.h"
#endif

static void advanceVelocity(SimFlat* s, int nBoxes, real_t dt);
static void advancePosition(SimFlat* s, int nBoxes, real_t dt);

#if _CD
unsigned int preserveSimFlat(cd_handle_t *cdh, SimFlat *sim, int doeam);
unsigned int preserveAtoms(cd_handle_t *cdh, 
                           Atoms *atoms, 
                           int nTotalBoxes,
                           unsigned int is_gid,
                           unsigned int is_r,
                           unsigned int is_p,
                           unsigned int is_f,
                           unsigned int is_U,
                           unsigned int is_iSpecies,
                           unsigned int from,
                           int to
                          );
unsigned int preserveSpeciesData(cd_handle_t *cdh, SpeciesData *species);
unsigned int preserveLinkCell(cd_handle_t *cdh, 
                              LinkCell *linkcell, 
                              unsigned int is_all,
                              unsigned int is_nAtoms,
                              unsigned int is_nTotalBoxes);
unsigned int preserveHaloAtom(cd_handle_t *cdh, 
                              AtomExchangeParms *xchange_parms,
                              unsigned int is_cellList,
                              unsigned int is_pbcFactor);
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
double timestep(SimFlat* s, int nSteps, real_t dt)
{

   cd_handle_t *cdh = getleafcd(); //still cdh points at root_cd since hasn't begin yet
   for (int ii=0; ii<nSteps; ++ii)
   {  
      //TODO
      //who preserve nStpes? Do current CDs need to preserve it as well?
      //************************************
      //            cd boundary: velocity (0.08%) (for both)
      //************************************
      cd_begin(cdh, "advanceVelocity_start"); // cd_lv1 starts
      //FIXME: need to pass cmd.doeam
      //FIXME: should this be kRef?
      int velocity_pre_size = preserveAtoms(cdh, s->atoms, s->boxes->nTotalBoxes, 
                                   0,  // is_gid
                                   0,  // is_r
                                   0,  // is_p
                                   1,  // is_f
                                   0,  // is_U
                                   0,  // is_iSpecies
                                   0,  // from (entire atoms)
                                  -1); // to (entire atoms)
      //TODO: Do I need to preserve dt and nTotalBoxes? 
      //      Aren't they already preserved at the  begging at the root CD?
      //int cd_preserve(cd_handle_t *c_handle, 
      //             void *data_ptr,
      //             uint64_t len,
      //             uint32_t preserve_mask,
      //             const char *my_name, 
      //             const char *ref_name)
      cd_preserve(cdh, &dt, sizeof(real_t), kCopy, "advanceVelocity_dt", "advanceVelocity_dt");
      cd_preserve(cdh, &ii, sizeof(int), kCopy, "advanceVelocity_ii", "advanceVelocity_ii");
      velocity_pre_size += sizeof(real_t);  // add the size of dt
      velocity_pre_size += sizeof(int);  // add the size of ii (loop index)
      velocity_pre_size += preserveLinkCell(cdh, s->boxes, 
                                            0/*all*/, 
                                            1/*only nAtoms*/,
                                            0/*nLocalBoxes*/);      
      printf("\n preservation size for advanceVelocity(@beggining) %d\n", velocity_pre_size);
      startTimer(velocityTimer);
      //------------------------------------------------
      advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
      //------------------------------------------------
      stopTimer(velocityTimer);
      //TODO: cd_preserve for output with kOutput(?)
      //      output: s->atoms->p
      cd_detect(cdh);
      cd_complete(cdh); 

      //************************************
      //            cd boundary: position (0.09%)
      //************************************
      cd_begin(cdh, "advancePosition"); 
      //TODO: preserve dt and nAtoms by *kRef* or KCopy? 
      //TODO: for now, no way to refer only dt in SimFlat preserved at the root
      //cd_preserve(cdh, &dt, sizeof(real_t), kRef, "advancePosition_dt", "advancePosition_dt");
      int position_pre_size = preserveAtoms(cdh, s->atoms, s->boxes->nTotalBoxes, 
                                   0,  // is_gid
                                   0,  // is_r
                                   1,  // is_p
                                   0,  // is_f
                                   0,  // is_U
                                   1,  // is_iSpecies
                                   0,  // from (entire atoms)
                                  -1); // to (entire atoms)

      //TODO: no need to preserve entier SpeciesData but only mass
      position_pre_size += preserveSpeciesData(cdh, s->species);
      printf("\n preservation size for advancePosition %d\n", position_pre_size);
      startTimer(positionTimer);
      //------------------------------------------------
      advancePosition(s, s->boxes->nLocalBoxes, dt);
      //------------------------------------------------
      stopTimer(positionTimer);

      cd_detect(cdh);
      cd_complete(cdh); 


      //-----------------------------------------------------------------------
      //            Communication
      //-----------------------------------------------------------------------
      //************************************
      //            cd boundary: redistribution (6.88%)
      //************************************
      cd_begin(cdh, "redistributeAtoms"); 
      //TODO: preserve nAtoms by kRef
      // I{atoms->[iSpecies, gid, r, p, f, U]}
      int redist_pre_size = preserveAtoms(cdh, s->atoms, s->boxes->nTotalBoxes, 
                                   1,  // is_gid
                                   1,  // is_r
                                   1,  // is_p
                                   1,  // is_f
                                   1,  // is_U
                                   1,  // is_iSpecies 
                                   0,  // from (entire atoms)
                                  -1); // to (entire atoms)
      // I{parms->pbcFactor}
      redist_pre_size = preserveHaloAtom(cdh, s->atomExchange->parms, 0, 1);
      printf("\n preservation size for redistributeAtoms %d\n", redist_pre_size);

      //TODO: what to preserve here?
      //      //---------------
      // |- O{boxes->nAtoms, atoms->[iSpecies, gid, r, p, f, U], }
      //     <- I{boxes->nAtoms, atoms->[iSpecies, gid, r, p, f, U]}
      //     <- I{parms->pbcFactor}
      //1. updateLinkCells(sim->boxes, sim->atoms);
      //   |+ moveAtom(boxes, atoms, ii, iBox, jBox);
      //   |    |- O{boxes->nAtoms } <- I{boxes->nAtoms}
      //   |+ copyAtom(boxes, atoms, iId, iBox, nj, jBox);
      //        |- O{ atoms->iSpecies, atoms->gid, r, p, f, U} 
      //        |-         <- I{ atoms->iSpecies, atoms->gid, r, p, f, U}
      //2. haloExchange(sim->atomExchange, sim);
      //   |+ exchangeData(haloExchange, data, iAxis[0,1,2]);
      //      |+loadBuffer(haloExchange->parms, data, faceM, sendBufM)
      //        |- O{remote data} <- I{s->atoms->[gid, iSpecies, r, p], parms->pbcFactor}
      //      |+unloadBuffer(haloExchange->parms, data, faceM, nRecvM, recvBufM);
      //        |+putAtomInBox
      //           |- O{boxes->nAtoms, atoms->[gid, iSpecies, r, p]} <- I{remote data}
      //3. sortAtomsInCell(sim->atoms, sim->boxes, ii);
      //   |- O{ atoms->ISpecies, atoms->gid, r,p} 
      //   |-         <- I{ atoms->ISpecies, atoms->gid, r, p}
      //---------------

      //FIXME: communication logging?
      startTimer(redistributeTimer);
      redistributeAtoms(s);
      stopTimer(redistributeTimer);

      //cd_detect(cdh);
      //cd_complete(cdh); 
      //-----------------------------------------------------------------------
      //            Communication
      //-----------------------------------------------------------------------


      //************************************
      //            cd boundary: force (92.96%)
      //************************************
      cd_handle_t *cd_lv2 = cd_create(cdh, 
                                      getNRanks(), 
                                      "timestep_after_comm", 
                                      kStrict, 0xE);
      cd_begin(cd_lv2, "computeForce"); 

      startTimer(computeForceTimer);
      computeForce(s); // s->pot->force(s)
      stopTimer(computeForceTimer);

      cd_detect(cd_lv2);
      cd_complete(cd_lv2); 

      //************************************
      //            cd boundary : advanceVelocity (@ end)
      //************************************
      cd_begin(cd_lv2, "advanceVelocity_end"); 
      //TODO: preserve dt and nAtoms by kRef
      int velocity_end_pre_size = preserveAtoms(cdh, 
                                    s->atoms, s->boxes->nTotalBoxes, 
                                    0,  // is_gid
                                    0,  // is_r
                                    0,  // is_p
                                    1,  // is_f
                                    0,  // is_U
                                    0,  // is_iSpecies
                                    0,  // from (entire atoms)
                                   -1); // to (entire atoms)

      printf("\n preservation size for advanceVelocity(@end) %d\n", 
            velocity_end_pre_size);
      startTimer(velocityTimer);
      advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
      stopTimer(velocityTimer);

      cd_detect(cd_lv2);
      cd_complete(cd_lv2); 

      cd_destroy(cd_lv2);

      //---
      cd_detect(cdh);
      cd_complete(cdh); 

   }

   //TODO: which CD to put this?
   kineticEnergy(s);

   return s->ePotential;
}

void computeForce(SimFlat* s)
{
   s->pot->force(s);
}


void advanceVelocity(SimFlat* s, int nBoxes, real_t dt)
{
   //printf("[DEBUG] task[%d] is calling advanceVelocity with %d nBoxes at time %f\n", 
   //         getMyRank(), nBoxes, dt);
   for (int iBox=0; iBox<nBoxes; iBox++)
   {
      for (int iOff=MAXATOMS*iBox,ii=0; ii<s->boxes->nAtoms[iBox]; ii++,iOff++)
      {
         s->atoms->p[iOff][0] += dt*s->atoms->f[iOff][0];
         s->atoms->p[iOff][1] += dt*s->atoms->f[iOff][1];
         s->atoms->p[iOff][2] += dt*s->atoms->f[iOff][2];
      }
   }
}

void advancePosition(SimFlat* s, int nBoxes, real_t dt)
{
   for (int iBox=0; iBox<nBoxes; iBox++)
   {
      for (int iOff=MAXATOMS*iBox,ii=0; ii<s->boxes->nAtoms[iBox]; ii++,iOff++)
      {
         int iSpecies = s->atoms->iSpecies[iOff];
         real_t invMass = 1.0/s->species[iSpecies].mass;
         s->atoms->r[iOff][0] += dt*s->atoms->p[iOff][0]*invMass;
         s->atoms->r[iOff][1] += dt*s->atoms->p[iOff][1]*invMass;
         s->atoms->r[iOff][2] += dt*s->atoms->p[iOff][2]*invMass;
      }
   }
}

/// Calculates total kinetic and potential energy across all tasks.  The
/// local potential energy is a by-product of the force routine.
void kineticEnergy(SimFlat* s)
{
   real_t eLocal[2];
   eLocal[0] = s->ePotential;
   eLocal[1] = 0;
   for (int iBox=0; iBox<s->boxes->nLocalBoxes; iBox++)
   {
      for (int iOff=MAXATOMS*iBox,ii=0; ii<s->boxes->nAtoms[iBox]; ii++,iOff++)
      {
         int iSpecies = s->atoms->iSpecies[iOff];
         real_t invMass = 0.5/s->species[iSpecies].mass;
         eLocal[1] += ( s->atoms->p[iOff][0] * s->atoms->p[iOff][0] +
         s->atoms->p[iOff][1] * s->atoms->p[iOff][1] +
         s->atoms->p[iOff][2] * s->atoms->p[iOff][2] )*invMass;
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
void redistributeAtoms(SimFlat* sim)
{
   updateLinkCells(sim->boxes, sim->atoms);

   startTimer(atomHaloTimer);
   haloExchange(sim->atomExchange, sim);
   stopTimer(atomHaloTimer);

   for (int ii=0; ii<sim->boxes->nTotalBoxes; ++ii)
      sortAtomsInCell(sim->atoms, sim->boxes, ii);
}
