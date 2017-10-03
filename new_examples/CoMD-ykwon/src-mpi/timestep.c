/// \file
/// Leapfrog time integrator

#include "timestep.h"

#include "CoMDTypes.h"
#include "linkCells.h"
#include "parallel.h"
#include "performanceTimers.h"
#ifdef _CD
#include "cd.h"
#endif

static void advanceVelocity(SimFlat* s, int nBoxes, real_t dt);
static void advancePosition(SimFlat* s, int nBoxes, real_t dt);


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

// statistics with ibrun -n 8 -o 0 ./CoMD-mpi -i2 -j2 -k2 -x 100 -y 100 -z 100 -N 200 -n 10
//Timings for Rank 0
//        Timer        # Calls    Avg/Call (s)   Total (s)    % Loop
//___________________________________________________________________
//total                      1     253.5756      253.5756      100.72
//loop                       1     251.7697      251.7697      100.00
//timestep                  20      12.5881      251.7615      100.00
//  position               200       0.0118        2.3597        0.94
//  velocity               400       0.0062        2.4818        0.99
//  redistribute           201       0.0518       10.4207        4.14
//    atomHalo             201       0.0121        2.4302        0.97
//  force                  201       1.1283      226.7958       90.08
//commHalo                 603       0.0006        0.3326        0.13
//commReduce                69       0.0002        0.0147        0.01

double timestep(SimFlat* s, int nSteps, real_t dt)
{
#if   _CD1
  cd_handle_t *cd_lv1 = getleafcd();
#endif
  for (int ii=0; ii<nSteps; ++ii)
   {
#if   _CD1
      cd_begin(cd_lv1, "1_timestep_advanceVelocity"); 
      //not need to preserve "nSteps" and "dt" since they are stored in s(im).
      //cd_preserve(cd_lv1, &nSteps, sizeof(int), kCopy, "timestep_nSteps", NULL);
      //FIXME should be verified to be okay with kRef
      //FIXME what's the size when with kRef preservation? same as kCopy
      cd_preserve(cd_lv1, &dt, sizeof(real_t), kRef, NULL, "timestep_dt");
      cd_preserve(cd_lv1, &(s->boxes->nLocalBoxes), sizeof(int), kRef, NULL, "timestep_s_boxes_nLocalBoxes");
      cd_preserve(cd_lv1, s->atoms->p, MAXATOMS*(s->boxes->nTotalBoxes)*sizeof(real3), kRef, NULL, "timestep_s_atoms_p");
#endif
      startTimer(velocityTimer);
      // O{s->atoms->f} <- I{s->atoms->p, s->boxes->nLocalBoxes, dt}
      // less than 1% time of execution in this loop
      advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
      stopTimer(velocityTimer);
#if   _CD1
      //TODO: once kOuput is ready 
      //cd_preserve(cd_lv1, s->atoms->f, MAXATOMS*(s->boxes->nTotalBoxes)*sizeof(real3), kCopy|kOutput, NULL, "timestep_s_atoms_f");
      cd_detect(cd_lv1);
      cd_complete(cd_lv1);
#endif

#if   _CD1
      cd_begin(cd_lv1, "1_timestep_advancePosition"); 
      //FIXME all inputs are already preserved before advanceVelocity()
#endif
      startTimer(positionTimer);
      // O{s->atoms->r} <- I{s->atoms->p, s->boxes->nLocalBoxes, dt}
      // less than 1% time of execution in this loop
      advancePosition(s, s->boxes->nLocalBoxes, dt);
      stopTimer(positionTimer);
#if   _CD1
      //TODO: once kOuput is ready 
      //cd_preserve(cd_lv1, s->atoms->r, MAXATOMS*(s->boxes->nTotalBoxes)*sizeof(real3), kCopy|kOutput, NULL, "timestep_s_atoms_f");
      cd_detect(cd_lv1);
      cd_complete(cd_lv1);
#endif

#if   _CD1
      cd_begin(cd_lv1, "1_timestep_redistributeAtoms"); 
      //FIXME should be verified to be okay with kRef
      cd_preserve(cd_lv1, s->boxes->nAtoms, (s->boxes->nTotalBoxes)*sizeof(int), kRef, NULL, "timestep_boxes_nAtoms");
      int maxTotalAtoms=MAXATOMS*(s->boxes->nTotalBoxes);
      cd_preserve(cd_lv1, s->atoms->gid, maxTotalAtoms*sizeof(int), kRef, NULL, "Atoms_gid");
      cd_preserve(cd_lv1, s->atoms->iSpecies, maxTotalAtoms*sizeof(int), kRef, NULL, "Atoms_iSpecies");
      cd_preserve(cd_lv1, s->atoms->p, maxTotalAtoms*sizeof(real3), kRef, NULL, "Atoms_p");
      cd_preserve(cd_lv1, s->atoms->U, maxTotalAtoms*sizeof(real_t), kRef, NULL, "Atoms_U");
     
      //FIXME the below two are preseved as kCopy since
      //the below two are output of (produced from) previous functions 
      cd_preserve(cd_lv1, s->atoms->f, maxTotalAtoms*sizeof(real3), kCopy, "Atoms_f", NULL);
      cd_preserve(cd_lv1, s->atoms->r, maxTotalAtoms*sizeof(real3), kCopy, "Atoms_r", NULL);
#endif
      startTimer(redistributeTimer);
      //---------------
      // communication only in this function
      // around 5% time of execution in this loop
      redistributeAtoms(s);
      //---------------
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
      stopTimer(redistributeTimer);

#if   _CD2
      cd_handle_t *cd_lv2 = cd_create(cd_lv1, getNRanks(), "timestep (after communication)", kStrict, 0xE);
      cd_begin(cd_lv2, "2_computeForce_in_timestep"); 
#endif
      startTimer(computeForceTimer);
      //---------------
      //the most intensive computation: accounts for more than 90% of this loop
      //this eventually calls ljForce
      computeForce(s);
      //---------------
      stopTimer(computeForceTimer);

#if   _CD2
      cd_detect(cd_lv2);
      cd_complete(cd_lv2);

      // FIXME 
      // cd_destroy(cd_lv2);
      // cd_lv2 = cd_create(cd_lv1, 1, "timestep after computeForce", kStrict, 0x3);
      cd_begin(cd_lv2, "2_advanceVelocity_in_timestep"); 
#endif
      startTimer(velocityTimer);
      advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
      stopTimer(velocityTimer);
#if   _CD2
      cd_detect(cd_lv2);
      cd_complete(cd_lv2);
#endif
#if   _CD2
      cd_destroy(cd_lv2);
#endif

#if   _CD1   
      cd_detect(cd_lv1);
      cd_complete(cd_lv1);
#endif
   }
#if   _CD1   
   cd_begin(cd_lv1, "1_kineticEnergy"); 
#endif
   kineticEnergy(s);
#if   _CD1   
   cd_detect(cd_lv1);
   cd_complete(cd_lv1);
#endif

   return s->ePotential;
}

void computeForce(SimFlat* s)
{
   s->pot->force(s);
}


void advanceVelocity(SimFlat* s, int nBoxes, real_t dt)
{
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
