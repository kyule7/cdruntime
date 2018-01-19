#include "CoMDTypes.h"
#include "eam.h" // for ForceExchangeData
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "cd.h"
#include "cd_comd.h"

#define PRINTON 0

unsigned int preserveSimFlat(cd_handle_t *cdh, SimFlat *sim) {
  uint32_t size = sizeof(SimFlat);
#ifdef DO_PRV
  // Preserve SimFlat object with shallow copy first
  // This preserve the values of nSteps, printRate, dt, ePotential, eKinetic.
  // Also, it does the pointeres of domain, boxes, atoms, species, pot, and
  // atomExchange.
  cd_preserve(cdh, sim, size, kCopy, "SimFlat", "SimFlat");
#endif
  if (PRINTON == 1)
    printf("Preserve SimFlat: %zu\n", sizeof(SimFlat));

  // Further preserve by chasing pointers of *domain, *boxes, *species, *pot
  // and *atomExchange.
  size += preserveDomain(cdh, sim->domain); // Flat struct
  size += preserveLinkCell(cdh, sim->boxes, 1 /*all*/, 0 /*nAtoms*/, 0/*local*/,
                           0 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
  size += preserveAtoms(cdh, sim->atoms, sim->boxes->nTotalBoxes,
                        1,                        // is_all
                        0,                        // is_gid
                        0,                        // is_r
                        0,                        // is_p
                        0,                        // is_f
                        0,                        // is_U
                        0,                        // is_iSpecies
                        0,                        // from
                        -1,                       // to (entire atoms)
                        0,                        // is_print
                        NULL);                    // idx
  size += preserveSpeciesData(cdh, sim->species); // Flat struct
  if (sim->doeam) {
    // FIXME: not verified
    size += preserveEamPot(cdh, sim->doeam, (EamPotential *)sim->pot,
                           sim->boxes->nTotalBoxes);
  } else {
    // There are some function pointers
    size += preserveLjPot(cdh, (LjPotential *)sim->pot); // Flat
  }
  // FIMXE: why always 0?
  size += preserveHaloExchange(cdh, sim->doeam, sim->atomExchange, 0);
  return size;
}

unsigned int preserveDomain(cd_handle_t *cdh, Domain *domain) {
  uint32_t size = sizeof(Domain);
// there is no pointer in Domain object
#ifdef DO_PRV
  cd_preserve(cdh, domain, sizeof(Domain), kCopy, "Domain", "Domain");
#endif
  if (PRINTON == 1)
    printf("Preserve Domain: %zu\n", sizeof(Domain));
  return size;
}

unsigned int preserveLinkCell(cd_handle_t *cdh, LinkCell *linkcell,
                              unsigned int is_all, 
                              unsigned int is_nAtoms, 
                              unsigned int is_local, // nAtoms[0:nLocalBox-1]
                              unsigned int is_nLocalBoxes,
                              unsigned int is_nTotalBoxes) {
  uint32_t size = sizeof(LinkCell);
  
  // nAtoms_size: The amount of data for nAtoms[] to be preserved
  // either nAtoms[0:nLocalBoxes-1] or nAtoms[0:nTotalBoxes-] with is_local 
  // 1 or 0 respectively.
  unsigned int nAtoms_size = 0;
  if (is_local == 1) nAtoms_size = linkcell->nLocalBoxes; 
  else if (is_local == 0) nAtoms_size = linkcell->nTotalBoxes;
  else assert(1); // shouldn't be the case.

  // Preserve entire linkcell struct
  if (is_all) {
    // When is_all is set to 1, all the others should be set to 0.
    assert(is_nAtoms == 0);
    assert(is_local == 0);
    assert(is_nLocalBoxes == 0);
    assert(is_nTotalBoxes == 0);
#ifdef DO_PRV
    // Preserve all but nAtoms array
    cd_preserve(cdh, linkcell, size, kCopy, "LinkCell", "LinkCell");
#endif
    size += sizeof(LinkCell);
#ifdef DO_PRV
    // Preserve nAtom int array
    cd_preserve(cdh, linkcell->nAtoms, nAtoms_size, kCopy, "LinkCell_nAtoms",
                "LinkCell_nAtoms");
#endif
    size += nAtoms_size;
    if (PRINTON == 1)
      printf("Preserven LinkCell %zu, nAtoms: %u\n", sizeof(LinkCell),
             nAtoms_size);
  }

  // Preserve int array being pointed by linkcell->nAtoms
  if (is_nAtoms) {
    assert(is_all == 0);
#ifdef DO_PRV
    // Preserve nAtom int array
    cd_preserve(cdh, &(linkcell->nAtoms), sizeof(int*), kCopy,
                "LinkCell_nAtoms_ptr", "LinkCell_nAtoms_ptr");
    cd_preserve(cdh, linkcell->nAtoms, nAtoms_size, kCopy, 
                "LinkCell_nAtoms", "LinkCell_nAtoms");
#endif
    size += nAtoms_size;
    size += sizeof(int*);
    if (PRINTON == 1)
      printf("Preserven LinkCell_nAtoms: %u\n", 0, nAtoms_size);
  }

  // Preserve nLocalBoxes
  if (is_nLocalBoxes) {
    assert(is_all == 0);
#ifdef DO_PRV
    cd_preserve(cdh, &(linkcell->nLocalBoxes), sizeof(int), kCopy,
                "LinkCell_nLocalBoxes", "LinkCell_nLocalBoxes");
#endif
    size += sizeof(int);
    if (PRINTON == 1)
      printf("Preserven LinkCell_nLocalBoxes: %u\n", sizeof(int));
  }

  // Preserve nTotalBoxes
  if (is_nTotalBoxes) {
    assert(is_all == 0);
#ifdef DO_PRV
    cd_preserve(cdh, &(linkcell->nTotalBoxes), sizeof(int), kCopy,
                "LinkCell_nTotalBoxes", "LinkCell_nTotalBoxes");
#endif
    size += sizeof(int);
    if (PRINTON == 1)
      printf("Preserven LinkCell_nTotalBoxes: %u\n", sizeof(int));
  }
  return size;
}

// TODO: need to add preserve by reference
unsigned int
preserveAtoms(cd_handle_t *cdh, Atoms *atoms, int nTotalBoxes,
              unsigned int is_all, // all the members
              unsigned int is_gid, // globally unique id for each atom
              unsigned int is_r, // positions
              unsigned int is_p, // momenta of atoms
              unsigned int is_f, // forces
              unsigned int is_U, // potenial energy per atom
              unsigned int is_iSpecies, // the species index of the atom
              // number of atoms to be preserved: from - to + 1
              unsigned int from, int to, unsigned int is_print, char *idx) {
  // TODO: Need to implement to concatenate idx.
  //       done only with r for now
  uint32_t size = 0; // total size of preserved data (to be returend)
  // Total number of atoms (allocated)
  uint32_t atoms_size = 0;
  uint32_t gid_size = 0;
  uint32_t iSpecies_size = 0;
  uint32_t r_size = 0;
  uint32_t p_size = 0;
  uint32_t f_size = 0;
  uint32_t U_size = 0;

  const uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
  // Total number of atoms to be preserved
  uint32_t maxPreservedAtoms = 0;

  // All allocated atom is to be preserved.
  if (to == -1) {
    assert(from == 0);
    maxPreservedAtoms = maxTotalAtoms;
  }
  // Given amount of atoms is to be preserved
  // For now, this is used only for aotms->r (positions)
  else if (to >= 1) { // at least 1 element
    // FIXME: This is now depreciated
    //        Need to take the number of boxes
    // the number of boxes to be preserved
    maxPreservedAtoms = to - from + 1;
    assert(1); // FIXME: to be fixed
  } else {
    // No atom to be preserved
    assert(1); // shoudn't fall down here.
  }

  atoms_size = sizeof(Atoms);
  gid_size = maxPreservedAtoms * sizeof(int);
  iSpecies_size = maxPreservedAtoms * sizeof(int);
  r_size = maxPreservedAtoms * sizeof(real3);
  p_size = maxPreservedAtoms * sizeof(real3);
  f_size = maxPreservedAtoms * sizeof(real3);
  U_size = maxPreservedAtoms * sizeof(real_t);

  // Preserve entire strcut of AtomSt
  if (is_all) {
    assert(is_gid == 0);
    assert(is_iSpecies == 0);
    assert(is_r == 0); 
    assert(is_p == 0);
    assert(is_f == 0);
    assert(is_U == 0);
    assert(from == 0);
    assert(to == -1);
#ifdef DO_PRV
    //TODO: add idx
    // Preserve the values of nLocal and nGLobal and the pointers of
    // *gid, *iSpecies, *r, *p, *f and *U. (just pointers themselvs)
    cd_preserve(cdh, atoms, atoms_size, kCopy, "Atoms", "Atoms");
    // Preserve arrays of gid, iSpecies, r, p, f, and U
    cd_preserve(cdh, atoms->gid, gid_size, kCopy, "Atoms_gid", "Atoms_gid");
    cd_preserve(cdh, atoms->iSpecies, iSpecies_size, kCopy, "Atoms_iSpecies",
                "Atoms_iSpecies");
    cd_preserve(cdh, atoms->r, r_size, kCopy, "Atoms_r", "Atoms_r");
    cd_preserve(cdh, atoms->p, p_size, kCopy, "Atoms_p", "Atoms_p");
    cd_preserve(cdh, atoms->f, f_size, kCopy, "Atoms_f", "Atoms_f");
    cd_preserve(cdh, atoms->U, U_size, kCopy, "Atoms_U", "Atoms_U");
#endif
    size = atoms_size + gid_size + iSpecies_size + r_size + p_size + f_size +
           U_size;
  }

  // Preserve array for gids of atoms
  else if (is_gid == 1) {
    // Be careful not to preserve twice
    assert(is_all != 1);
#ifdef DO_PRV
    // FIXME: check whether this works or not
    cd_preserve(cdh, &(atoms->gid), sizeof(int *), kCopy, "Atoms_gid_ptr",
                "Atoms_gid_ptr");
    cd_preserve(cdh, atoms->gid, gid_size, kCopy, "Atoms_gid", "Atoms_gid");
#endif
    size += gid_size;
  }

  // Preserve array for iSpecies of atoms
  else if (is_iSpecies == 1) {
    assert(is_all != 1);
    char tmp_atoms_iSpecies[256] = "-1"; // FIXME: it this always enough?
    char tmp_atoms_iSpecies_ptr[256] = "-1"; // FIXME: it this always enough?
    if (idx == NULL) {
      sprintf(tmp_atoms_iSpecies, "Atoms_iSpecies");
      sprintf(tmp_atoms_iSpecies_ptr, "Atoms_iSpecies_ptr");
    } else {
      sprintf(tmp_atoms_iSpecies, "Atoms_iSpecies%s", idx);
      sprintf(tmp_atoms_iSpecies_ptr, "Atoms_iSpecies_ptr%s", idx);
    }

#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->iSpecies), sizeof(int *), kCopy,
                tmp_atoms_iSpecies_ptr, tmp_atoms_iSpecies_ptr);
    cd_preserve(cdh, atoms->iSpecies, iSpecies_size, kCopy, tmp_atoms_iSpecies,
                tmp_atoms_iSpecies);
#endif
    size += iSpecies_size;
  }

  // Preserve array for atoms->r
  else if (is_r == 1) {
    assert(is_all != 1);
    char tmp_atoms_r[256] = "-1"; // FIXME: it this always enough?
    char tmp_atoms_r_ptr[256] = "-1"; // FIXME: it this always enough?
    if (idx == NULL) {
      sprintf(tmp_atoms_r, "Atoms_r");
      sprintf(tmp_atoms_r_ptr, "Atoms_r_ptr");
    } else {
      sprintf(tmp_atoms_r, "Atoms_r%s", idx);
      sprintf(tmp_atoms_r_ptr, "Atoms_r_ptr%s", idx);
    }
#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->r), sizeof(real3 *), kCopy, tmp_atoms_r_ptr,
                tmp_atoms_r_ptr);
    cd_preserve(cdh, atoms->r, r_size, kCopy, tmp_atoms_r, tmp_atoms_r);
#endif
    size += r_size;
  }

  // Preserve array for atoms->p
  else if (is_p == 1) {
    assert(is_all != 1);
    char tmp_atoms_p[256] = "-1"; // FIXME: it this always enough?
    char tmp_atoms_p_ptr[256] = "-1"; // FIXME: it this always enough?
    if (idx == NULL) {
      sprintf(tmp_atoms_p, "Atoms_p");
      sprintf(tmp_atoms_p_ptr, "Atoms_p_ptr");
    } else {
      sprintf(tmp_atoms_p, "Atoms_p%s", idx);
      sprintf(tmp_atoms_p_ptr, "Atoms_p_ptr%s", idx);
    }
#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->p), sizeof(real3 *), kCopy, tmp_atoms_p_ptr,
                tmp_atoms_p_ptr);
    cd_preserve(cdh, atoms->p, p_size, kCopy, tmp_atoms_p, tmp_atoms_p);
#endif
    size += p_size;
  }

  // Preserve array for atoms->f
  else if (is_f == 1) {
    assert(is_all != 1);
    char tmp_atoms_f[256] = "-1"; // FIXME: it this always enough?
    char tmp_atoms_f_ptr[256] = "-1"; // FIXME: it this always enough?
    if (idx == NULL) {
      sprintf(tmp_atoms_f, "Atoms_f");
      sprintf(tmp_atoms_f_ptr, "Atoms_f_ptr");
    } else {
      sprintf(tmp_atoms_f, "Atoms_f%s", idx);
      sprintf(tmp_atoms_f_ptr, "Atoms_f_ptr%s", idx);
    }
#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->f), sizeof(real3 *), kCopy, tmp_atoms_f_ptr,
                tmp_atoms_f_ptr);
    cd_preserve(cdh, atoms->f, f_size, kCopy, tmp_atoms_f, tmp_atoms_f);
#endif
    size += f_size;
  }

  // Preserve array for atoms->U
  else if (is_U == 1) {
    assert(is_all != 1);
    char tmp_atoms_U[256] = "-1"; // FIXME: it this always enough?
    char tmp_atoms_U_ptr[256] = "-1"; // FIXME: it this always enough?
    if (idx == NULL) {
      sprintf(tmp_atoms_U, "Atoms_U");
      sprintf(tmp_atoms_U_ptr, "Atoms_U_ptr");
    } else {
      sprintf(tmp_atoms_U, "Atoms_U%s", idx);
      sprintf(tmp_atoms_U_ptr, "Atoms_U_ptr%s", idx);
    }
#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->U), sizeof(real3 *), kCopy, tmp_atoms_U_ptr,
                tmp_atoms_U_ptr);
    cd_preserve(cdh, atoms->U, U_size, kCopy, tmp_atoms_U, tmp_atoms_U);
#endif
    size += U_size;
  } else {
    // Someting went wrong. No need to call this in this case.
    assert(1);
  }

  if (is_print == 1) {
    printf("Preserve Atoms: %zu, #totAtoms:%u, gid:%u, species:%u, r:%u, p:%u, "
           "f:%u, U:%u\n",
           sizeof(Atoms), maxPreservedAtoms, gid_size, iSpecies_size, r_size,
           p_size, f_size, U_size);
  }
  return size;
}

unsigned int preserveSpeciesData(cd_handle_t *cdh, SpeciesData *species) {
  uint32_t size = sizeof(SpeciesData);
#ifdef DO_PRV
  cd_preserve(cdh, species, size, kCopy, "SpeciesData", "SpeciesData");
#endif
  if (PRINTON == 1)
    printf("Preserve SpeciesData %zu\n", sizeof(SpeciesData));
  return size;
}

unsigned int preserveLjPot(cd_handle_t *cdh, LjPotential *pot) {
  uint32_t size = sizeof(LjPotential);
#ifdef DO_PRV
  cd_preserve(cdh, pot, size, kCopy, "LjPotential", "LjPotential");
#endif
  // force function pointer
  // print function pointer
  // destroy function pointer
  if (PRINTON == 1)
    printf("Preserve LjPotential %zu\n", sizeof(LjPotential));
  return size;
}

// defined for preservedEamPot()
unsigned int preserveInterpolationObject(cd_handle_t *cdh,
                                         InterpolationObject *obj) {
  uint32_t size = sizeof(InterpolationObject);
#ifdef DO_PRV
  cd_preserve(cdh, obj, size, kCopy, "InterpolationObject",
              "InterpolationObject");
#endif
  uint32_t values_size = (obj->n + 3) * sizeof(real_t);
#ifdef DO_PRV
  cd_preserve(cdh, obj->values, values_size, kCopy, "InterpolationObject_value",
              "");
#endif
  size += values_size;
  // FIXME: isn't this called from EamPot instead of LJPotential?
  if (PRINTON == 1)
    printf("Preserve LjPotential %zu, values: %u\n",
           sizeof(InterpolationObject), values_size);
  return size;
}

unsigned int preserveEamPot(cd_handle_t *cdh, int doeam, EamPotential *pot,
                            int nTotalBoxes) {
  assert(doeam);
  uint32_t size = sizeof(EamPotential);
#ifdef DO_PRV
  cd_preserve(cdh, pot, size, kCopy, "EamPotential", "EamPotential");
#endif
  // preserving rhobar and dfEmbed
  uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
  uint32_t rhobar_size = maxTotalAtoms * sizeof(real_t);
#ifdef DO_PRV
  cd_preserve(cdh, pot->rhobar, size, kCopy, "EamPotential_rhobar",
              "EamPotential_rhobar");
#endif
  uint32_t dfEmbed_size = maxTotalAtoms * sizeof(real_t);
#ifdef DO_PRV
  cd_preserve(cdh, pot->dfEmbed, size, kCopy, "EamPotential_dfEmbed",
              "EamPotential_dfEmbed");
#endif
  if (PRINTON == 1)
    printf("Preserve EamPotential %zu, rhobar:%u, dfEmbed:%u\n",
           sizeof(EamPotential), rhobar_size, dfEmbed_size);
  size += rhobar_size + dfEmbed_size;
  // preserving InterpolationObject phi, rho, and f
  size += preserveInterpolationObject(cdh, pot->phi);
  size += preserveInterpolationObject(cdh, pot->rho);
  size += preserveInterpolationObject(cdh, pot->f);
  // preserving HaloExchange
  size += preserveHaloExchange(cdh, doeam, pot->forceExchange, doeam);
  // preserving ForceExchangeDataSt
  size += preserveForceData(cdh, pot->forceExchangeData); // shallow copy
  return size;
}

unsigned int preserveHaloExchange(cd_handle_t *cdh, int doeam,
                                  HaloExchange *xchange, int is_force) {
  uint32_t size = sizeof(HaloExchange);
#ifdef DO_PRV
  cd_preserve(cdh, xchange, size, kCopy, "HaloExchange", "HaloExchange");
#endif
  if (PRINTON == 1)
    printf("Preserve HaloExchange: %zu\n", sizeof(HaloExchange));
  if (is_force) {
    // TODO: This name is confusing.
    //      This is preserveing ForceExchangeParms
    size += preserveHaloForce(cdh, doeam, xchange->parms);
  } else {
    // TODO: This name is confusing.
    //      This is preserveing AtomExchangeParms
    size += preserveHaloAtom(cdh, xchange->parms, 1, 1);
  }
  return size;
}

unsigned int preserveHaloAtom(cd_handle_t *cdh,
                              AtomExchangeParms *xchange_parms,
                              unsigned int is_cellList,
                              unsigned int is_pbcFactor) {
  uint32_t size = 0;
  if (is_cellList && is_pbcFactor) {
    size += sizeof(AtomExchangeParms);
#ifdef DO_PRV
    cd_preserve(cdh, xchange_parms, size, kCopy, "AtomExchangeParms",
                "AtomExchangeParms");
#endif
    if (PRINTON == 1)
      printf("Preserve Halo Atom:%zu\n", sizeof(AtomExchangeParms));
  }
  uint32_t cellList_size = 0;
  if (is_cellList == 1) {
    for (int i = 0; i < 6; i++) {
      if (PRINTON == 1)
        printf("Preserve cellList[%d]:%u\n", i,
               xchange_parms->nCells[i] * sizeof(int));
      cellList_size += xchange_parms->nCells[i] * sizeof(int);
    }
#ifdef DO_PRV
    cd_preserve(cdh, xchange_parms->cellList, cellList_size, kCopy,
                "AtomExchangeParms_cellList", "AtomExchangeParms_cellList");
#endif
  }
  uint32_t pbcFactor_size = 3 * sizeof(real_t);
  if (is_pbcFactor == 1) {
//    for (int i = 0; i < 6; i++) {
//      if (PRINTON == 1)
//        printf("Preserve pbfFactor[%d]:%u\n", i, sizeof(real_t) * 3);
//      pbcFactor_size += 3 * sizeof(real_t);
//    }
#ifdef DO_PRV
    cd_preserve(cdh, xchange_parms->pbcFactor[0], pbcFactor_size, kCopy,
                "AtomExchangeParms_pbcFactor0", "AtomExchangeParms_pbcFactor0");
    cd_preserve(cdh, xchange_parms->pbcFactor[1], pbcFactor_size, kCopy,
                "AtomExchangeParms_pbcFactor1", "AtomExchangeParms_pbcFactor1");
    cd_preserve(cdh, xchange_parms->pbcFactor[2], pbcFactor_size, kCopy,
                "AtomExchangeParms_pbcFactor2", "AtomExchangeParms_pbcFactor2");
    cd_preserve(cdh, xchange_parms->pbcFactor[3], pbcFactor_size, kCopy,
                "AtomExchangeParms_pbcFactor3", "AtomExchangeParms_pbcFactor3");
    cd_preserve(cdh, xchange_parms->pbcFactor[4], pbcFactor_size, kCopy,
                "AtomExchangeParms_pbcFactor4", "AtomExchangeParms_pbcFactor4");
    cd_preserve(cdh, xchange_parms->pbcFactor[5], pbcFactor_size, kCopy,
                "AtomExchangeParms_pbcFactor5", "AtomExchangeParms_pbcFactor5");
#endif
  }
  size += cellList_size;
  size += pbcFactor_size;
  return size;
}

unsigned int preserveHaloForce(cd_handle_t *cdh, int doeam,
                               ForceExchangeParms *xchange_parms) {
  assert(doeam);
  uint32_t size = sizeof(ForceExchangeParms);
#ifdef DO_PRV
  cd_preserve(cdh, xchange_parms, size, kCopy, "ForceExchangeParms",
              "ForceExchangeParms");
#endif
  if (PRINTON == 1)
    printf("Preserve Halo Force:%zu\n", sizeof(ForceExchangeParms));
  uint32_t buffer_size = 0;
  for (int i = 0; i < 6; i++) {
    if (PRINTON == 1)
      printf("Preserve buffer[%d]:%u\n", i,
             xchange_parms->nCells[i] * sizeof(int));
    buffer_size += xchange_parms->nCells[i] * sizeof(int);
  }
#ifdef DO_PRV
  cd_preserve(cdh, xchange_parms->sendCells, buffer_size, kCopy,
              "ForceExchangeParms_sendCells", "ForceExchangeParms_sendCells");
  cd_preserve(cdh, xchange_parms->recvCells, buffer_size, kCopy,
              "ForceExchangeParms_recvCells", "ForceExchangeParms_recvCells");
#endif
  size += buffer_size; // TODO: sendCells
  size += buffer_size; // TODO: recvCells
  return size;
}

// defined in eam.h
unsigned int preserveForceData(cd_handle_t *cdh, ForceExchangeData *forceData) {
  uint32_t size = sizeof(ForceExchangeData);
#ifdef DO_PRV
  cd_preserve(cdh, forceData, size, kCopy, "ForceExchangeData",
              "ForceExchangeData");
#endif
  if (PRINTON == 1)
    printf("Preserve ForceExchangeData: %zu\n", sizeof(ForceExchangeData));
  // preserve dfEmbed and boxes
  uint32_t dfEmbed_size = sizeof(forceData->dfEmbed);
#ifdef DO_PRV
  cd_preserve(cdh, forceData->dfEmbed, dfEmbed_size, kCopy,
              "ForceExchangeData_dfEmbed", "ForceExchangeData_dfEmbed");
#endif
  uint32_t boxes_size = sizeof(forceData->boxes);
#ifdef DO_PRV
  cd_preserve(cdh, forceData->boxes, boxes_size, kCopy,
              "ForceExchangeData_boxes", "ForceExchangeData_boxes");
#endif
  size += dfEmbed_size + boxes_size;
  return size;
}
