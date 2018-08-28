#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cd.h"
#include "cd_comd.h"
#include "cd_def_common.h"

#define PRINTON 0

unsigned int preserveSimFlat(cd_handle_t *cdh, uint32_t knob, SimFlat *sim) {
  uint32_t size = sizeof(SimFlat);
#ifdef DO_PRV
  // Preserve SimFlat object with shallow copy first.
  // This preserves the values of nSteps, printRate, dt, ePotential, eKinetic.
  // Also, it does the pointeres for domain, boxes, atoms, species, pot, and
  // atomExchange arrays.
  cd_preserve(cdh, sim, size, knob, "SimFlat", "SimFlat");
#endif
  if (PRINTON == 1)
    printf("Preserve SimFlat: %zu\n", sizeof(SimFlat));
  // Further preserve by chasing pointers of *domain, *boxes, *species, *pot
  // and *atomExchange.
  size += preserveDomain(cdh, knob, sim->domain); // Flat struct
  size += preserveLinkCell(cdh, knob, sim->boxes, 1 /*all*/, 0 /*nAtoms*/,
                           0 /*local*/, 0 /*nLocalBoxes*/, 0 /*nTotalBoxes*/);
  size += preserveAtomsInLocalBox(cdh, knob, sim->atoms,
                                  sim->boxes->nLocalBoxes, 0);
  size += preserveAtomsInHaloBox(cdh, knob, sim->atoms, sim->boxes->nLocalBoxes,
                                 sim->boxes->nTotalBoxes, 0);
  size += preserveSpeciesData(cdh, knob, sim->species); // Flat struct
  // There are two implementtaion for LJ force and EAM method
  if (sim->doeam) {
    // If it is EAM method (FIXME: not verified)
    size += preserveEamPot(cdh, knob, sim->doeam, (EamPotential *)sim->pot,
                           sim->boxes->nTotalBoxes);
    // preserve HaloForce by setting 1(=sim->doeam) at the end
    size += preserveHaloExchange(cdh, knob, sim->doeam, sim->atomExchange, 1);
  } else {
    // If it is LJ force
    // Note that there are some function pointers.
    size += preserveLjPot(cdh, knob, (LjPotential *)sim->pot); // Flat
    // preserve HaloAtom by setting 0 at the end
    size += preserveHaloExchange(cdh, knob, sim->doeam, sim->atomExchange, 0);
  }

  return size;
}

// perserve Domain object about domain decomposition information
unsigned int preserveDomain(cd_handle_t *cdh, uint32_t knob, Domain *domain) {
  uint32_t size = sizeof(Domain);

#ifdef DO_PRV
  // There is no pointer in Domain object but flat.
  cd_preserve(cdh, domain, sizeof(Domain), knob, "Domain", "Domain");
#endif
  if (PRINTON == 1)
    printf("Preserve Domain: %zu\n", sizeof(Domain));
  return size;
}

unsigned int preserveLinkCell(cd_handle_t *cdh, uint32_t knob,
                              LinkCell *linkcell, unsigned int is_all,
                              unsigned int is_nAtoms,
                              unsigned int is_local, // 0, 1, and 2
                              unsigned int is_nLocalBoxes,
                              unsigned int is_nTotalBoxes) {
  uint32_t size = sizeof(LinkCell);

  // nAtoms_size: The amount of data for nAtoms[] to be preserved
  // if is_local = 0 : then, preserve nAtoms[0:nTotalBoxes]
  // if is_local = 1 : then, preserve nAtoms[0:nLocalBoxes]
  // if is_local = 2 : then, preserve nAtoms[nLocalBoxes:nTotalBoxes]
  unsigned int nAtoms_size = 0;
  if (is_local == 1)
    // preserve atoms in local boxes
    nAtoms_size = (linkcell->nLocalBoxes) * sizeof(int);
  else if (is_local == 0)
    // preserve all atoms in total boxes
    nAtoms_size = (linkcell->nTotalBoxes) * sizeof(int);
  else if (is_local == 2)
    // preserve atoms only in halo boxes
    nAtoms_size =
        (linkcell->nTotalBoxes - linkcell->nLocalBoxes - 1) * sizeof(int);
  else
    assert(1); // shouldn't be the case.

  // Preserve an entire linkcell struct
  if (is_all) {
    // When is_all is set to 1, all the others should be set to 0 for sanity
    assert(is_nAtoms == 0);
    assert(is_local == 0); // nAtoms_size = nTotalBoxes
    assert(is_nLocalBoxes == 0);
    assert(is_nTotalBoxes == 0);
#ifdef DO_PRV
    // Preserve all but nAtoms array
    assert(size > 0);
    cd_preserve(cdh, linkcell, size, knob, "LinkCell", "LinkCell");
#endif
    size += sizeof(LinkCell);
#ifdef DO_PRV
    // Preserve nAtom int array
    assert(nAtoms_size > 0);
    cd_preserve(cdh, linkcell->nAtoms, nAtoms_size, knob, "LinkCell_nAtoms",
                "LinkCell_nAtoms");

#endif
    size += nAtoms_size;
    if (PRINTON == 1)
      printf("Preserven LinkCell %zu, nAtoms: %u\n", sizeof(LinkCell),
             nAtoms_size);
  }

  // Preserve int array being pointed by linkcell->nAtoms
  if (is_nAtoms) {
    // prevent from preserving this twice or more
    assert(is_all == 0);
#ifdef DO_PRV
    // Preserve nAtom int array starting from 1st index
    if (is_local == 1 || is_local == 0) {
      // TODO: This is required only when the pointer is used somewhere in the
      //      codes. How do I know if it's called somewhere or not?
      // FIXME: Should I remove this?
      cd_preserve(cdh, &(linkcell->nAtoms), sizeof(int *), knob,
                  "LinkCell_nAtoms_ptr", "LinkCell_nAtoms_ptr");
      cd_preserve(cdh, linkcell->nAtoms, nAtoms_size, knob, "LinkCell_nAtoms",
                  "LinkCell_nAtoms");
    }
    // preserve nAtoms[nLocalBoxes:nTotalBoxes], i.e. atoms in halo boxes
    else if (is_local == 2) {
      cd_preserve(cdh, &(linkcell->nAtoms), sizeof(int *), knob,
                  "LinkCell_nAtoms_ptr", "LinkCell_nAtoms_ptr");
      cd_preserve(cdh, &(linkcell->nAtoms[linkcell->nLocalBoxes]), nAtoms_size,
                  knob, "LinkCell_nAtoms", "LinkCell_nAtoms");
    } else
      assert(1); // shouldn't be the case.

#endif
    size += nAtoms_size;
    size += sizeof(int *);
    if (PRINTON == 1)
      printf("Preserven LinkCell_nAtoms: %u\n", nAtoms_size);
  }

  // Preserve nLocalBoxes
  if (is_nLocalBoxes) {
    assert(is_all == 0);
#ifdef DO_PRV
    cd_preserve(cdh, &(linkcell->nLocalBoxes), sizeof(int), knob,
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
    cd_preserve(cdh, &(linkcell->nTotalBoxes), sizeof(int), knob,
                "LinkCell_nTotalBoxes", "LinkCell_nTotalBoxes");
#endif
    size += sizeof(int);
    if (PRINTON == 1)
      printf("Preserven LinkCell_nTotalBoxes: %u\n", sizeof(int));
  }
  return size;
}

unsigned int
preserveAtoms(cd_handle_t *cdh, uint32_t knob, Atoms *atoms,
              unsigned int is_gid,      // globally unique id for each atom
              unsigned int is_r,        // positions
              unsigned int is_p,        // momenta of atoms
              unsigned int is_f,        // forces
              unsigned int is_U,        // potenial energy per atom
              unsigned int is_iSpecies, // the species index of the atom
              unsigned int from,
              int to, // indices for starting and ending boxes
              unsigned int is_print, char *idx) {
  // TODO: is_all is no longer used.

  // Each box can contain atoms up to as many as MAXATOMS.
  // const uint32_t maxTotalAtoms = MAXATOMS * nBoxes;

  // starting index for the atom to be preserved, given starting box number
  assert(from >= 0);
  const uint32_t prvAtomStartIdx = MAXATOMS * from;

  assert(to - from + 1 > 0);
  // Total number of atoms to be preserved
  const uint32_t szPreservedAtoms = MAXATOMS * (to - from + 1);

  uint32_t atoms_size = sizeof(Atoms);
  uint32_t gid_size = szPreservedAtoms * sizeof(int);
  uint32_t iSpecies_size = szPreservedAtoms * sizeof(int);
  uint32_t r_size = szPreservedAtoms * sizeof(real3);
  uint32_t p_size = szPreservedAtoms * sizeof(real3);
  uint32_t f_size = szPreservedAtoms * sizeof(real3);
  uint32_t U_size = szPreservedAtoms * sizeof(real_t);
  uint32_t size = 0; // total size of preserved data (to be returend)

  // create label(name) for preservation
  char atoms_name[16] = "";
  char gid_name[16] = "";
  char iSpecies_name[32] = "";
  char r_name[16] = "";
  char p_name[16] = "";
  char f_name[16] = "";
  char U_name[16] = "";

  if (strcmp(idx, "Local") == 0 && from == 0) {
    // Local boxes
    sprintf(atoms_name, "Atoms_Local");
    sprintf(gid_name, "Atoms_gid_Local");
    sprintf(iSpecies_name, "Atoms_iSpecies_Local");
    sprintf(r_name, "Atoms_r_Local");
    sprintf(p_name, "Atoms_p_Local");
    sprintf(f_name, "Atoms_f_Local");
    sprintf(U_name, "Atoms_U_Local");
  } else if (strcmp(idx, "Halo") == 0 && from > 0) {
    // Halo boxes
    sprintf(atoms_name, "Atoms_Halo");
    sprintf(gid_name, "Atoms_gid_Halo");
    sprintf(iSpecies_name, "Atoms_iSpecies_Halo");
    sprintf(r_name, "Atoms_r_Halo");
    sprintf(p_name, "Atoms_p_Halo");
    sprintf(f_name, "Atoms_f_Halo");
    sprintf(U_name, "Atoms_U_Halo");
  } else {
    // TODO: For now, support only two granurarity for preserveAtoms of either
    //      all atoms in all Local boxes or all Halo boxes
    assert(1);
  }

  // FIXME :Let's preserve all the pointers regardless of what needed actually
  // since it's not super expensive.
  cd_preserve(cdh, atoms, atoms_size, knob, atoms_name, atoms_name);
  // Preserve array for gids of atoms
  if (is_gid == 1) {
#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->gid[prvAtomStartIdx]), gid_size, knob, gid_name,
                gid_name);
#endif
    size += gid_size;
  }
  // Preserve array for iSpecies of atoms
  if (is_iSpecies == 1) {
#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->iSpecies[prvAtomStartIdx]), iSpecies_size, knob,
                iSpecies_name, iSpecies_name);
#endif
    size += iSpecies_size;
  }
  // Preserve array for atoms->r
  if (is_r == 1) {
#ifdef DO_PRV
    cd_preserve(cdh, atoms->r[prvAtomStartIdx], r_size, knob, r_name, r_name);
#endif
    size += r_size;
  }
  // Preserve array for atoms->p
  if (is_p == 1) {
#ifdef DO_PRV
    cd_preserve(cdh, atoms->p[prvAtomStartIdx], p_size, knob, p_name, p_name);
#endif
    size += p_size;
  }
  // Preserve array for atoms->f
  if (is_f == 1) {
#ifdef DO_PRV
    cd_preserve(cdh, atoms->f[prvAtomStartIdx], f_size, knob, f_name, f_name);
#endif
    size += f_size;
  }
  // Preserve array for atoms->U
  if (is_U == 1) {
#ifdef DO_PRV
    cd_preserve(cdh, &(atoms->U[prvAtomStartIdx]), U_size, knob, U_name,
                U_name);
#endif
    size += U_size;
  } else {
    // Someting went wrong. Never needed to call this in this case.
    assert(1);
  }
  if (is_print == 1) {
    printf("Preserve Atoms: %zu, #totAtoms:%u, gid:%u, species:%u, r:%u, p:%u, "
           "f:%u, U:%u\n",
           sizeof(Atoms), szPreservedAtoms, gid_size, iSpecies_size, r_size,
           p_size, f_size, U_size);
  }
  return size;
}

// wrapper function to preserve all atoms in Local box
unsigned int preserveAtomsInLocalBox(cd_handle_t *cdh, uint32_t knob,
                                     Atoms *atoms, int nBoxes,
                                     unsigned int is_print) {
  uint32_t size = 0;
  size += preserveAtoms(cdh, knob, atoms,
                        1,        // is_gid
                        1,        // is_r
                        1,        // is_p
                        1,        // is_f
                        1,        // is_U
                        1,        // is_iSpecies
                        0,        // from
                        nBoxes,   // to (entire atoms)
                        is_print, // is_print
                        "Local"); // idx
  return size;
}
// wrapper function to preserve all atoms in Halo box
unsigned int preserveAtomsInHaloBox(cd_handle_t *cdh, uint32_t knob,
                                    Atoms *atoms, int nLocalBoxes,
                                    int nTotalBoxes, unsigned int is_print) {
  uint32_t size = 0;
  size += preserveAtoms(cdh, knob, atoms,
                        1,           // is_gid
                        1,           // is_r
                        1,           // is_p
                        1,           // is_f
                        1,           // is_U
                        1,           // is_iSpecies
                        nLocalBoxes, // from
                        nTotalBoxes, // to
                        is_print,    // is_print
                        "Halo");     // idx
  return size;
}

unsigned int preserveSpeciesData(cd_handle_t *cdh, uint32_t knob,
                                 SpeciesData *species) {
  uint32_t size = sizeof(SpeciesData);
#ifdef DO_PRV
  cd_preserve(cdh, species, size, knob, "SpeciesData", "SpeciesData");
#endif
  if (PRINTON == 1)
    printf("Preserve SpeciesData %zu\n", sizeof(SpeciesData));
  return size;
}

unsigned int preserveLjPot(cd_handle_t *cdh, uint32_t knob, LjPotential *pot) {
  uint32_t size = sizeof(LjPotential);
#ifdef DO_PRV
  // There are function pointers called force(), print() and destroty()
  cd_preserve(cdh, pot, size, knob, "LjPotential", "LjPotential");
#endif
  if (PRINTON == 1)
    printf("Preserve LjPotential %zu\n", sizeof(LjPotential));
  return size;
}

// defined for preserveEamPot()
unsigned int preserveInterpolationObject(cd_handle_t *cdh, uint32_t knob,
                                         InterpolationObject *obj) {
  uint32_t size = sizeof(InterpolationObject);
#ifdef DO_PRV
  cd_preserve(cdh, obj, size, knob, "InterpolationObject",
              "InterpolationObject");

#endif
  uint32_t values_size = (obj->n + 3) * sizeof(real_t);
#ifdef DO_PRV
  // FIXME: not verifed
  cd_preserve(cdh, obj->values, values_size, knob, "InterpolationObject_value",
              "InterpolationObject_value");
#endif
  size += values_size;
  if (PRINTON == 1)
    printf("Preserve EamPotential %zu, values: %u\n",
           sizeof(InterpolationObject), values_size);
  return size;
}

unsigned int preserveEamPot(cd_handle_t *cdh, uint32_t knob, int doeam,
                            EamPotential *pot, int nTotalBoxes) {
  assert(doeam);
  uint32_t size = sizeof(EamPotential);
#ifdef DO_PRV
  cd_preserve(cdh, pot, size, knob, "EamPotential", "EamPotential");
#endif
  // preserving rhobar and dfEmbed
  // FIXME: not verifed
  uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
  uint32_t rhobar_size = maxTotalAtoms * sizeof(real_t);
#ifdef DO_PRV
  cd_preserve(cdh, pot->rhobar, size, knob, "EamPotential_rhobar",
              "EamPotential_rhobar");
#endif
  uint32_t dfEmbed_size = maxTotalAtoms * sizeof(real_t);
#ifdef DO_PRV
  cd_preserve(cdh, pot->dfEmbed, size, knob, "EamPotential_dfEmbed",
              "EamPotential_dfEmbed");
#endif
  if (PRINTON == 1)
    printf("Preserve EamPotential %zu, rhobar:%u, dfEmbed:%u\n",
           sizeof(EamPotential), rhobar_size, dfEmbed_size);
  size += rhobar_size + dfEmbed_size;
  // preserving phi, rho, and f in a type of InterpolationObject
  size += preserveInterpolationObject(cdh, knob, pot->phi);
  size += preserveInterpolationObject(cdh, knob, pot->rho);
  size += preserveInterpolationObject(cdh, knob, pot->f);
  // preserving HaloExchange. Note that doeam must be 1 to get here.
  // FIXME: this is done separatelly in preserveSimFlat
  // size += preserveHaloExchange(cdh, knob, doeam, pot->forceExchange, doeam);
  // preserving ForceExchangeDataSt
  size += preserveForceData(cdh, knob, pot->forceExchangeData); // shallow copy
  return size;
}

// Preserving HaloExchange strcuture. Note that LJ potential and EAM potential
// will be preserved in different manner since they do different stuffs.
// TODO: doeam and is_force are redundant
unsigned int preserveHaloExchange(cd_handle_t *cdh, uint32_t knob, int doeam,
                                  HaloExchange *xchange, int is_force) {
  uint32_t size = sizeof(HaloExchange);
#ifdef DO_PRV
  cd_preserve(cdh, xchange, size, knob, "HaloExchange", "HaloExchange");
#endif
  if (PRINTON == 1)
    printf("Preserve HaloExchange: %zu\n", sizeof(HaloExchange));
  if (is_force) {
    // This name is confusing. If is_force is set to 1, then this is about
    // EAM method and is calling ForceExchangeParms.
    size += preserveHaloForce(cdh, knob, doeam, xchange->parms);
  } else {
    // LJ potential is being used. This is calling AtomExchangeParms
    size += preserveHaloAtom(cdh, knob, xchange->parms, 1, 1);
  }
  return size;
}

unsigned int preserveHaloAtom(cd_handle_t *cdh, uint32_t knob,
                              AtomExchangeParms *xchange_parms,
                              unsigned int is_cellList,
                              unsigned int is_pbcFactor) {
  uint32_t size = 0;
  // FIXME: This should be only used when both is_cellList and is_pbcFactor are
  //        set to 1, meaning preserving entire structure in deep copy manner.
  //        If there is any other case such as only preserving cellList, then
  //        should be fixed to perform correctly.
  if (is_cellList == 1 && is_pbcFactor == 1) {
    size += sizeof(AtomExchangeParms);
#ifdef DO_PRV
    // preserve nCells[6], and pointers for *cellList[6] and *pbcFactor[6].
    cd_preserve(cdh, xchange_parms, size, knob, "AtomExchangeParms",
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
    // FIXME: The below brings restore failure for the next cd_preserve for
    // pbcFactor.
    //       Somehow it doesn't give a proper TableStore pointer after growth
    //       but gives a outdated one before growh of the table.
    // cd_preserve(cdh, xchange_parms->cellList, cellList_size, knob,
    //            "AtomExchangeParms_cellList", "AtomExchangeParms_cellList");
    cd_preserve(cdh, xchange_parms->cellList[0],
                (xchange_parms->nCells[0] * sizeof(int)), knob,
                "AtomExchangeParms_cellList0", "AtomExchangeParms_cellList0");
    cd_preserve(cdh, xchange_parms->cellList[1],
                (xchange_parms->nCells[1] * sizeof(int)), knob,
                "AtomExchangeParms_cellList1", "AtomExchangeParms_cellList1");
    cd_preserve(cdh, xchange_parms->cellList[2],
                (xchange_parms->nCells[2] * sizeof(int)), knob,
                "AtomExchangeParms_cellList2", "AtomExchangeParms_cellList2");
    cd_preserve(cdh, xchange_parms->cellList[3],
                (xchange_parms->nCells[3] * sizeof(int)), knob,
                "AtomExchangeParms_cellList3", "AtomExchangeParms_cellList3");
    cd_preserve(cdh, xchange_parms->cellList[4],
                (xchange_parms->nCells[4] * sizeof(int)), knob,
                "AtomExchangeParms_cellList4", "AtomExchangeParms_cellList4");
    cd_preserve(cdh, xchange_parms->cellList[5],
                (xchange_parms->nCells[5] * sizeof(int)), knob,
                "AtomExchangeParms_cellList5", "AtomExchangeParms_cellList5");
#endif
  }
  uint32_t pbcFactor_size = 3 * sizeof(real_t);
  if (is_pbcFactor == 1) {
#ifdef DO_PRV
    // cd_preserve(cdh, &(xchange_parms->pbcFactor[0][0]), pbcFactor_size, knob,
    //            "AtomExchangeParms_pbcFactor0",
    //            "AtomExchangeParms_pbcFactor0");
    cd_preserve(cdh, xchange_parms->pbcFactor[0], pbcFactor_size, knob,
                "AtomExchangeParms_pbcFactor0", "AtomExchangeParms_pbcFactor0");
    cd_preserve(cdh, xchange_parms->pbcFactor[1], pbcFactor_size, knob,
                "AtomExchangeParms_pbcFactor1", "AtomExchangeParms_pbcFactor1");
    cd_preserve(cdh, xchange_parms->pbcFactor[2], pbcFactor_size, knob,
                "AtomExchangeParms_pbcFactor2", "AtomExchangeParms_pbcFactor2");
    cd_preserve(cdh, xchange_parms->pbcFactor[3], pbcFactor_size, knob,
                "AtomExchangeParms_pbcFactor3", "AtomExchangeParms_pbcFactor3");
    cd_preserve(cdh, xchange_parms->pbcFactor[4], pbcFactor_size, knob,
                "AtomExchangeParms_pbcFactor4", "AtomExchangeParms_pbcFactor4");
    cd_preserve(cdh, xchange_parms->pbcFactor[5], pbcFactor_size, knob,
                "AtomExchangeParms_pbcFactor5", "AtomExchangeParms_pbcFactor5");
#endif
    // TODO: add PRINTON part
  }
  size += cellList_size;
  size += pbcFactor_size;
  return size;
}

// TODO: doeam is not necessarily required here.
unsigned int preserveHaloForce(cd_handle_t *cdh, uint32_t knob, int doeam,
                               ForceExchangeParms *xchange_parms) {
  assert(doeam);
  uint32_t size = sizeof(ForceExchangeParms);
#ifdef DO_PRV
  cd_preserve(cdh, xchange_parms, size, knob, "ForceExchangeParms",
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
  cd_preserve(cdh, xchange_parms->sendCells, buffer_size, knob,
              "ForceExchangeParms_sendCells", "ForceExchangeParms_sendCells");
  cd_preserve(cdh, xchange_parms->recvCells, buffer_size, knob,
              "ForceExchangeParms_recvCells", "ForceExchangeParms_recvCells");

#endif
  size += buffer_size; // sendCells
  size += buffer_size; // recvCells
  return size;
}

// defined in eam.h and called from preserveEamPot()
// FIXME: not verifed
unsigned int preserveForceData(cd_handle_t *cdh, uint32_t knob,
                               ForceExchangeData *forceData) {
  uint32_t size = sizeof(ForceExchangeData);
#ifdef DO_PRV
  cd_preserve(cdh, forceData, size, knob, "ForceExchangeData",
              "ForceExchangeData");
#endif
  if (PRINTON == 1)
    printf("Preserve ForceExchangeData: %zu\n", sizeof(ForceExchangeData));
  // preserve dfEmbed and boxes
  uint32_t dfEmbed_size = sizeof(forceData->dfEmbed);
#ifdef DO_PRV
  cd_preserve(cdh, forceData->dfEmbed, dfEmbed_size, knob,
              "ForceExchangeData_dfEmbed", "ForceExchangeData_dfEmbed");
#endif
  uint32_t boxes_size = sizeof(forceData->boxes);
#ifdef DO_PRV
  cd_preserve(cdh, forceData->boxes, boxes_size, knob,
              "ForceExchangeData_boxes", "ForceExchangeData_boxes");
#endif
  size += dfEmbed_size + boxes_size;
  return size;
}

void destroyAtomInReexecution(SimFlat *sim, int ranks, unsigned int is_r,
                              unsigned int is_p, unsigned int is_f,
                              unsigned int is_U) {
  if (is_reexec()) {
    // allranks
    if (ranks == -1) {
      // remove pointeres
      // sim->domain = NULL;
      // sim->boxes = NULL;
      for (int i = 0; i < MAXATOMS; i++) {
        if (is_r == 1)
          sim->atoms->r[i][0] = 0;
        if (is_p == 1)
          sim->atoms->p[i][0] = 0;
        if (is_f == 1)
          sim->atoms->f[i][0] = 0;
        if (is_U == 1)
          sim->atoms->U[i] = 0;
      }
    }
    // the specifed rank
    else if (ranks >= 0 && ranks <= getNRanks()) {
      if (ranks == getMyRank()) {
        // remove pointeres
        // sim->domain = NULL;
        // sim->boxes = NULL;
        for (int i = 0; i < MAXATOMS; i++) {
          if (is_r == 1)
            sim->atoms->r[i][0] = 0;
          if (is_p == 1)
            sim->atoms->p[i][0] = 0;
          if (is_f == 1)
            sim->atoms->f[i][0] = 0;
          if (is_U == 1)
            sim->atoms->U[i] = 0;
        }
      }
    }
  }
}
