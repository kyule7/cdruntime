/// \file
/// Leapfrog time integrator

#include "timestep.h"

#include "CoMDTypes.h"
#include "linkCells.h"
#include "parallel.h"
#include "performanceTimers.h"

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
double timestep(SimFlat* s, int nSteps, real_t dt)
{
#if _CD1
   cd_handle_t *cd_lv1 = getleafcd();
#endif
   for (int ii=0; ii<nSteps; ++ii)
   {
#if   _CD_loop
      cd_begin(cd_lv1, "timestep");
//      cdh->cd_preserve(&data, size, kCopy, "timestep_total");
#endif
      unsigned prv_size = preserveSimFlat(s);
      printf("Total Size:%u\n", prv_size);


      startTimer(velocityTimer);
      advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
      stopTimer(velocityTimer);

#if   _CD_advancePosition
      cd_detect(cd_lv1);
      cd_complete(cd_lv1);
      cd_begin(cd_lv1, "advancePosition");
//      cdh->cd_preserve(&data, size, kCopy, "timestep_total");
#endif 

      startTimer(positionTimer);
      advancePosition(s, s->boxes->nLocalBoxes, dt);
      stopTimer(positionTimer);

#if   _CD_redistributeAtoms
      cd_detect(cd_lv1);
      cd_complete(cd_lv1);
      cd_begin(cd_lv1, "redistributeAtoms");
//      cdh->cd_preserve(&data, size, kCopy, "timestep_total");
#endif 

      startTimer(redistributeTimer);
      redistributeAtoms(s);
      stopTimer(redistributeTimer);

#if   _CD_computeForce
      cd_detect(cd_lv1);
      cd_complete(cd_lv1);
      cd_begin(cd_lv1, "computeForce");
//      cdh->cd_preserve(&data, size, kCopy, "timestep_total");
#endif 

      startTimer(computeForceTimer);
      computeForce(s);
      stopTimer(computeForceTimer);

#if   _CD_advanceVelocity
      cd_detect(cd_lv1);
      cd_complete(cd_lv1);
      cd_begin(cd_lv1, "advanceVelocity");
//      cdh->cd_preserve(&data, size, kCopy, "timestep_total");
#endif 

      startTimer(velocityTimer);
      advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
      stopTimer(velocityTimer);
#if   _CD_loop
      cd_complete(cd_lv1);
#endif // _CD_loop
   }
#if   _CD_kineticEnergy 
   cd_detect(cd_lv1);
   cd_complete(cd_lv1);
   cd_begin(cd_lv1, "kineticEnergy"); 
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

// kyushick
// advanceVelocity : {atoms->p} <- {f,atoms->f,boxes->nLocalBoxes}
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
