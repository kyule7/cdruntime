#include "CoMDTypes.h"
#include "eam.h" // for ForceExchangeData
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "cd.h"
#include "cd_comd.h"
/*
 * nTotalBoxes is constant
advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
        {s->atoms->p} <- {s->atoms->f, s->boxes->nAtoms, dt}
advancePosition(s, s->boxes->nLocalBoxes, dt);
        {s->atoms->r} <- {atoms->iSpecies, s->atoms->p, dt, s->boxes->nAtoms}
redistributeAtoms(s);
        {s->atoms, s->boxes} <- {s->atoms, s->boxes}
computeForce(s);
ljForce: { ePotential, atoms->f, atoms->U} <- {pot, atoms->r, linkCells}
                                            //s->boxes->nTotalBoxes, s->boxes->nAtoms, nLocalBoxes, grid_size} 
eamForce: { ePotential, atoms->f, atoms->U, pot->rhobar, pot->dfEmbed } <- {pot, atoms->r, linkCells}
// rhobar and dfEmbed are temporary
advanceVelocity(s, s->boxes->nLocalBoxes, 0.5*dt); 
        {s->atoms->p} <- {s->atoms->f, s->boxes->nAtoms, dt}
kineticEnergy(s);
*/

//FIXME: replace with doeam (=cmd.doeam)
int is_eam = 0;
//extern int doeam;

unsigned int preserveSimFlat(cd_handle_t *cdh, SimFlat *sim, int doeam)
{   
    uint32_t size = sizeof(SimFlat); 
    //preserve object with shallow copy first
    //the below preserve nSteps, printRate, dt, ePotential and eKinetic;
    cd_preserve(cdh, sim, size, kCopy, "SimFlat", "SimFlat");
    printf("Preserve SimFlat: %zu\n", sizeof(SimFlat));

    //further preserve by chasing pointers
    size += preserveDomain(cdh, sim->domain);       // flat 
    size += preserveLinkCell(cdh, sim->boxes, 
                              1/*all*/, 0/*nAtoms*/, 0/*nLocalBoxes*/);      // nAtoms = nTotalBoxes*sizeof(int)
    size += preserveAtoms(cdh, sim->atoms, sim->boxes->nTotalBoxes, 
                          1,  // is_gid
                          1,  // is_r
                          1,  // is_p
                          1,  // is_f
                          1,  // is_U
                          1,  // is_iSpecies
                          0, // from
                         -1);// to
    size += preserveSpeciesData(cdh, sim->species);   // flat
    if(doeam) {
        printf("I am calling preserveEampot since doeam is %d\n", doeam);
        size += preserveEamPot(cdh, 
                               (EamPotential *)sim->pot, 
                               sim->boxes->nTotalBoxes);	  
    } else {
        size += preserveLjPot(cdh, (LjPotential *)sim->pot);	// flat 
    }
    //FIMXE: why always 0?
    size += preserveHaloExchange(cdh, sim->atomExchange, 0);
    return size;
}

unsigned int preserveDomain(cd_handle_t *cdh, Domain *domain)
{
  uint32_t size = sizeof(Domain);
  //there is no pointer in Domain object
  cd_preserve(cdh, domain, sizeof(Domain), kCopy, "Domain", "Domain");
  printf("Preserve Domain: %zu\n", sizeof(Domain));
  return size;
}


   int gridSize[3];     //!< number of boxes in each dimension on processor
   int nLocalBoxes;     //!< total number of local boxes on processor
   int nHaloBoxes;      //!< total number of remote halo/ghost boxes on processor
   int nTotalBoxes;     //!< total number of boxes on processor
                        //!< nLocalBoxes + nHaloBoxes
   real3 localMin;      //!< minimum local bounds on processor
   real3 localMax;      //!< maximum local bounds on processor
   real3 boxSize;       //!< size of box in each dimension
   real3 invBoxSize;    //!< inverse size of box in each dimension

   int* nAtoms;         //!< total number of atoms in each box

unsigned int preserveLinkCell(cd_handle_t *cdh, LinkCell *linkcell, 
                              unsigned int is_all,
                              unsigned int is_nAtoms,
                              unsigned int is_nTotalBoxes)
{
  uint32_t size = 0;
  uint32_t nAtoms_size = linkcell->nTotalBoxes*sizeof(int);
  if(is_all) {
    //FIXME: when is_all is set to 1, all others should be set to 0.
    assert( is_nAtoms == 0);
    assert( is_nTotalBoxes == 0);
    size += sizeof(LinkCell);
    cd_preserve(cdh, linkcell, size, kCopy, "LinkCell", "LinkCell");
    size += nAtoms_size;
    cd_preserve(cdh, linkcell->nAtoms, nAtoms_size, kCopy, 
                "LinkCell_nAtoms", "LinkCell_nAtoms");
    printf("Preserven LinkCell %zu, nAtoms: %u\n", sizeof(LinkCell), nAtoms_size);
  }
  //prerserving data being pointed by linkcell->nAtoms
  if(is_nAtoms) {
    assert( is_all == 0);
    size += nAtoms_size;
    cd_preserve(cdh, linkcell->nAtoms, nAtoms_size, kCopy, 
                "LinkCell_nAtoms", "LinkCell_nAtoms");
    printf("Preserven LinkCell_nAtoms: %u\n", 0, nAtoms_size);
  }
  //prerserving only nTotalBoxes 
  if(is_nTotalBoxes) {
    assert( is_all == 0);
    size += sizeof(unsigned int);
    cd_preserve(cdh, &(linkcell->nLocalBoxes), sizeof(unsigned int), kCopy, 
                "LinkCell_nLocalBoxes", "LinkCell_nLocalBoxes");
    printf("Preserven LinkCell_nLocalBoxes: %u\n", sizeof(unsigned int));
  }
  return size;
}



//TODO: add switch to pick what to preserve 
//TODO: minimize the number of cd_preserve
unsigned int preserveAtoms (cd_handle_t *cdh, 
                            Atoms *atoms, 
                            int nTotalBoxes,
                            unsigned int is_gid,//globally unique id for each atom
                            unsigned int is_r,  //positions
                            unsigned int is_p,  //momenta of atoms
                            unsigned int is_f,  //forces
                            unsigned int is_U,  //potenial energy per atom
                            unsigned int is_iSpecies, //the species index of the atom
                            unsigned int from,
                            int to 
                            )
{
    uint32_t size = sizeof(Atoms);
    //TODO: this is really not efficient nor readable
    //when want to preserve entire data 
    if( is_gid && is_iSpecies && is_r && is_p && is_f && is_U) {
      cd_preserve(cdh, atoms, size, kCopy, "Atoms", "Atoms");
    }
    //const uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
    uint32_t maxTotalAtoms = 0;
    if( to == -1) {
      //TODO: is this the right safe guard?
      maxTotalAtoms = MAXATOMS * nTotalBoxes;
    }
    else if( to >= 1 ) { //at least 1 element
      maxTotalAtoms = to - from + 1;
    }
    else {
      assert(1); //shoudn't fall down here.
    }
    uint32_t gid_size=0; 
    uint32_t iSpecies_size=0; 
    uint32_t r_size=0; 
    uint32_t p_size=0; 
    uint32_t f_size=0; 
    uint32_t U_size=0; 
    if(is_gid == 1) {
      gid_size = maxTotalAtoms*sizeof(int);
      cd_preserve(cdh, atoms->gid, gid_size, kCopy, "Atoms_gid", "Atoms_gid");
    }
    if(is_iSpecies == 1) {
      iSpecies_size = maxTotalAtoms*sizeof(int);
      cd_preserve(cdh, atoms->iSpecies+to*sizeof(int), iSpecies_size, kCopy, 
                  "Atoms_iSpecies", "Atoms_iSpecies");
    }
    //TODO: implement from and to
    if(is_r == 1) {
      r_size = maxTotalAtoms*sizeof(real3);
      //TODO: need to check pointer address
      cd_preserve(cdh, atoms->r+from*sizeof(real3*), r_size, kCopy, 
                  "Atoms_r", "Atoms_r");
    }
    if(is_p == 1) {
      p_size = maxTotalAtoms*sizeof(real3);
      cd_preserve(cdh, atoms->p+from*sizeof(real3*), p_size, kCopy, 
                  "Atoms_p", "Atoms_p");
    }
    if(is_f == 1) {
      f_size = maxTotalAtoms*sizeof(real3);
      cd_preserve(cdh, atoms->f+from*sizeof(real3*), f_size, kCopy, 
                  "Atoms_f", "Atoms_f");
    }
    if(is_U == 1) {
      U_size = maxTotalAtoms*sizeof(real_t);
      cd_preserve(cdh, atoms->U+from*sizeof(real3*), U_size, kCopy, 
                  "Atoms_u", "Atoms_u");
    }
    size += gid_size + iSpecies_size + r_size + p_size + f_size + U_size;
    //if(is_gid == 1) size += gid_size;
    //if(is_iSpecies == 1) size += iSpecies_size;
    //if(is_r == 1) size += r_size;
    //if(is_p == 1) size += p_size;
    //if(is_f == 1) size += f_size;
    //if(is_U == 1) size += U_size;

    printf("Preserve Atoms: %zu, totAtoms:%u, gid:%u, species:%u, r:%u, p:%u, f:%u, U:%u\n"
         , sizeof(Atoms)
         , maxTotalAtoms
         , gid_size     
         , iSpecies_size
         , r_size       
         , p_size       
         , f_size       
         , U_size       
        );
    return size;
}

unsigned int preserveSpeciesData(cd_handle_t *cdh, SpeciesData *species)
{
    uint32_t size = sizeof(SpeciesData);
    cd_preserve(cdh, species, size, kCopy, "SpeciesData", "SpeciesData");
    printf("Preserve SpeciesData %zu\n", sizeof(SpeciesData));
    return size;
}

unsigned int preserveLjPot(cd_handle_t *cdh, LjPotential *pot)
{
    uint32_t size = sizeof(LjPotential);
    cd_preserve(cdh, pot, size, kCopy, "LjPotential", "LjPotential");
    //force function pointer
    //print function pointer
    //destroy function pointer
    printf("Preserve LjPotential %zu\n", sizeof(LjPotential));
    return size;
}

//defined for preservedEamPot()
unsigned int preserveInterpolationObject(cd_handle_t *cdh, 
                                         InterpolationObject *obj)
{
    uint32_t size = sizeof(InterpolationObject);
    cd_preserve(cdh, obj, size, kCopy, 
                "InterpolationObject", "InterpolationObject");
    uint32_t values_size = (obj->n + 3)*sizeof(real_t);
    cd_preserve(cdh, obj->values, values_size, kCopy, "InterpolationObject_value", "");
    size += values_size;
    //FIXME: isn't this called from EamPot instead of LJPotential?
    printf("Preserve LjPotential %zu, values: %u\n", sizeof(InterpolationObject), values_size);
    return size;
}

unsigned int preserveEamPot(cd_handle_t *cdh, EamPotential *pot, int nTotalBoxes)
{
    assert(is_eam);
    uint32_t size = sizeof(EamPotential);
    cd_preserve(cdh, pot, size, kCopy, "EamPotential", "EamPotential");
    //preserving rhobar and dfEmbed
    uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
    uint32_t rhobar_size = maxTotalAtoms*sizeof(real_t);
    cd_preserve(cdh, pot->rhobar, size, kCopy, 
                "EamPotential_rhobar", "EamPotential_rhobar");
    uint32_t dfEmbed_size= maxTotalAtoms*sizeof(real_t);
    cd_preserve(cdh, pot->dfEmbed, size, kCopy, 
                "EamPotential_dfEmbed", "EamPotential_dfEmbed");
    printf("Preserve EamPotential %zu, rhobar:%u, dfEmbed:%u\n"
        , sizeof(EamPotential)
        , rhobar_size 
        , dfEmbed_size
        );
    size += rhobar_size + dfEmbed_size;
    //preserving InterpolationObject phi, rho, and f
    size += preserveInterpolationObject(cdh, pot->phi);
    size += preserveInterpolationObject(cdh, pot->rho);
    size += preserveInterpolationObject(cdh, pot->f);
    //preserving HaloExchange
    size += preserveHaloExchange(cdh, pot->forceExchange, is_eam);
    //preserving ForceExchangeDataSt
    size += preserveForceData(cdh, pot->forceExchangeData); // shallow copy
    return size;
}

unsigned int preserveHaloExchange(cd_handle_t *cdh, HaloExchange *xchange, int is_force)
{
    uint32_t size = sizeof(HaloExchange);
    cd_preserve(cdh, xchange, size, kCopy, "HaloExchange", "HaloExchange");
    printf("Preserve HaloExchange: %zu\n", sizeof(HaloExchange));
    if(is_force) {
        //TODO: This name is confusing.
        //      This is preserveing ForceExchangeParms
        size += preserveHaloForce(cdh, xchange->parms);
    } else {
        //TODO: This name is confusing.
        //      This is preserveing AtomExchangeParms
        size += preserveHaloAtom(cdh, xchange->parms, 1, 1);
    }
    return size;
}

unsigned int preserveHaloAtom(cd_handle_t *cdh, 
                              AtomExchangeParms *xchange_parms,
                              unsigned int is_cellList,
                              unsigned int is_pbcFactor)
{
    uint32_t size = 0;
    if( is_cellList && is_pbcFactor) {
      size += sizeof(AtomExchangeParms);
      cd_preserve(cdh, xchange_parms, size, kCopy, 
                  "AtomExchangeParms", "AtomExchangeParms");
      printf("Preserve Halo Atom:%zu\n", sizeof(AtomExchangeParms));
    }
    uint32_t cellList_size=0;
    if( is_cellList == 1) {
      for(int i=0; i<6; i++) {
        printf("Preserve cellList[%d]:%u\n", i, xchange_parms->nCells[i] * sizeof(int));
        cellList_size += xchange_parms->nCells[i] * sizeof(int);
      }
      cd_preserve(cdh, xchange_parms->cellList, cellList_size, kCopy, 
          "AtomExchangeParms_cellList", "AtomExchangeParms_cellList");
    }
    uint32_t pbcFactor_size=0;
    if( is_pbcFactor == 1) {
      for(int i=0; i<6; i++) {
        printf("Preserve pbfFactor[%d]:%u\n", i, sizeof(real_t) * 3);
        pbcFactor_size += 3 * sizeof(real_t);
      }
      cd_preserve(cdh, xchange_parms->pbcFactor, pbcFactor_size, kCopy, 
          "AtomExchangeParms_pbcFactor", "AtomExchangeParms_pbcFactor");
    }
    size += cellList_size;
    size += pbcFactor_size;
    return size;
}

unsigned int preserveHaloForce(cd_handle_t *cdh, ForceExchangeParms *xchange_parms)
{
    //FIXME
    assert(is_eam);
    uint32_t size = sizeof(ForceExchangeParms);
    cd_preserve(cdh, xchange_parms, size, kCopy, 
                "ForceExchangeParms", "ForceExchangeParms");
    printf("Preserve Halo Force:%zu\n", sizeof(ForceExchangeParms));
    uint32_t buffer_size=0;
    for(int i=0; i<6; i++) {
        printf("Preserve buffer[%d]:%u\n", i, xchange_parms->nCells[i] * sizeof(int));
        buffer_size += xchange_parms->nCells[i] * sizeof(int);
    }
    cd_preserve(cdh, xchange_parms->sendCells, buffer_size, kCopy, 
                "ForceExchangeParms_sendCells", "ForceExchangeParms_sendCells");
    cd_preserve(cdh, xchange_parms->recvCells, buffer_size, kCopy, 
                "ForceExchangeParms_recvCells", "ForceExchangeParms_recvCells");
    size += buffer_size; //TODO: sendCells
    size += buffer_size; //TODO: recvCells
    return size;
}

//defined in eam.h
unsigned int preserveForceData(cd_handle_t *cdh, ForceExchangeData *forceData)
{
    uint32_t size = sizeof(ForceExchangeData);
    cd_preserve(cdh, forceData, size, kCopy, 
                "ForceExchangeData", "ForceExchangeData");
    printf("Preserve ForceExchangeData: %zu\n", sizeof(ForceExchangeData));
    //preserve dfEmbed and boxes
    uint32_t dfEmbed_size = sizeof(forceData->dfEmbed);
    cd_preserve(cdh, forceData->dfEmbed, dfEmbed_size, kCopy, 
                "ForceExchangeData_dfEmbed", "ForceExchangeData_dfEmbed");
    uint32_t boxes_size = sizeof(forceData->boxes);
    cd_preserve(cdh, forceData->boxes, boxes_size, kCopy, 
                "ForceExchangeData_boxes", "ForceExchangeData_boxes");
    size += dfEmbed_size + boxes_size;
    return size;
}

