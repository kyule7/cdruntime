/// \file
/// Initialize the atom configuration.

#ifndef __INIT_ATOMS_H
#define __INIT_ATOMS_H

#include "mytype.h"
#include "cd.h"
struct SimFlatSt;
struct LinkCellSt;

/// Atom data
typedef struct AtomsSt
{
   // atom-specific data
   int nLocal;    //!< total number of atoms on this processor
   int nGlobal;   //!< total number of atoms in simulation

   int* gid;      //!< A globally unique id for each atom
   int* iSpecies; //!< the species index of the atom

   real3*  r;     //!< positions
   real3*  p;     //!< momenta of atoms
   real3*  f;     //!< forces 
   real_t* U;     //!< potential energy per atom
} Atoms;

#if 0
// FIXME
void serializeAtoms(int maxTotalAtoms)
{
  cd_handle_t *cdh = getleafcd();
  cdh->cd_preserve(atoms, sizeof(Atoms), kCopy, "Atoms");
  cdh->cd_preserve(gid, , kCopy, "Atoms");

  Atoms* atoms = (Atoms*)comdMalloc(sizeof(Atoms));

  int maxTotalAtoms = MAXATOMS*boxes->nTotalBoxes;

  cdh->cd_preserve(atoms->gid, maxTotalAtoms*sizeof(int), kCopy, "AtomsGid");
  cdh->cd_preserve(atoms->iSpecies, maxTotalAtoms*sizeof(int), kCopy, "AtomsSpecies");
  cdh->cd_preserve(atoms->r, maxTotalAtoms*sizeof(real3), kCopy, "AtomsR");
  cdh->cd_preserve(atoms->p, maxTotalAtoms*sizeof(real3), kCopy, "AtomsP");
  cdh->cd_preserve(atoms->f, maxTotalAtoms*sizeof(real3), kCopy, "AtomsF");
  cdh->cd_preserve(atoms->U, maxTotalAtoms*sizeof(real_t), kCopy, "AtomsU");
}
#endif
/// Allocates memory to store atom data.
Atoms* initAtoms(struct LinkCellSt* boxes);
void destroyAtoms(struct AtomsSt* atoms);

void createFccLattice(int nx, int ny, int nz, real_t lat, struct SimFlatSt* s);

void setVcm(struct SimFlatSt* s, real_t vcm[3]);
void setTemperature(struct SimFlatSt* s, real_t temperature);
void randomDisplacements(struct SimFlatSt* s, real_t delta);
#endif
