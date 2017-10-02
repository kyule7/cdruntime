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
    cd_preserve(cdh, sim, size, kCopy, "SimFlat", "");
    printf("Preserve SimFlat: %zu\n", sizeof(SimFlat));

    //further preserve by chasing pointers
    size += preserveDomain(cdh, sim->domain);       // flat 
    size += preserveLinkCell(cdh, sim->boxes);      // nAtoms = nTotalBoxes*sizeof(int)
    size += preserveAtoms(cdh, sim->atoms, sim->boxes->nTotalBoxes);         // 
    size += preserveSpeciesData(cdh, sim->species);   // flat
    if(doeam) {
        size += preserveEamPot(cdh, (EamPotential *)sim->pot, sim->boxes->nTotalBoxes);	  
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
    cd_preserve(cdh, domain, sizeof(Domain), kCopy, "Domain", "");
    printf("Preserve Domain: %zu\n", sizeof(Domain));
    return size;
}

unsigned int preserveLinkCell(cd_handle_t *cdh, LinkCell *linkcell)
{
    uint32_t size = sizeof(LinkCell);
    cd_preserve(cdh, linkcell, size, kCopy, "LinkCell", "");
    uint32_t nAtoms_size = linkcell->nTotalBoxes*sizeof(int);
    //prerserving data being  pointed by linkcell->nAtoms
    cd_preserve(cdh, linkcell->nAtoms, nAtoms_size, kCopy, "LinkCell_nAtoms", "");
    size += nAtoms_size;
    printf("Preserven LinkCell %zu, nAtoms: %u\n", sizeof(LinkCell), nAtoms_size);
    return size;
}

//TODO: add switch to pick what to preserve 
//TODO: minimize the number of cd_preserve
unsigned int preserveAtoms (cd_handle_t *cdh, Atoms *atoms, int nTotalBoxes)
{
    uint32_t size = sizeof(Atoms);
    cd_preserve(cdh, atoms, size, kCopy, "Atoms", "");
    uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
    uint32_t gid_size      = maxTotalAtoms*sizeof(int);
    cd_preserve(cdh, atoms->gid, gid_size, kCopy, "Atoms_gid", "");
    uint32_t iSpecies_size = maxTotalAtoms*sizeof(int);
    cd_preserve(cdh, atoms->iSpecies, iSpecies_size, kCopy, "Atoms_iSpecies", "");
    uint32_t r_size        = maxTotalAtoms*sizeof(real3);
    cd_preserve(cdh, atoms->r, r_size, kCopy, "Atoms_r", "");
    uint32_t p_size        = maxTotalAtoms*sizeof(real3);
    cd_preserve(cdh, atoms->p, p_size, kCopy, "Atoms_p", "");
    uint32_t f_size        = maxTotalAtoms*sizeof(real3);
    cd_preserve(cdh, atoms->f, f_size, kCopy, "Atoms_f", "");
    uint32_t U_size        = maxTotalAtoms*sizeof(real_t);
    cd_preserve(cdh, atoms->U, U_size, kCopy, "Atoms_u", "");
    size += gid_size + iSpecies_size + r_size + p_size + f_size + U_size;

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
    cd_preserve(cdh, species, size, kCopy, "SpeciesData", "");
    printf("Preserve SpeciesData %zu\n", sizeof(SpeciesData));
    return size;
}

unsigned int preserveLjPot(cd_handle_t *cdh, LjPotential *pot)
{
    uint32_t size = sizeof(LjPotential);
    cd_preserve(cdh, pot, size, kCopy, "LjPotential", "");
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
    cd_preserve(cdh, obj, size, kCopy, "InterpolationObject", "");
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
    cd_preserve(cdh, pot, size, kCopy, "EamPotential", "");
    //preserving rhobar and dfEmbed
    uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
    uint32_t rhobar_size = maxTotalAtoms*sizeof(real_t);
    cd_preserve(cdh, pot->rhobar, size, kCopy, "EamPotential_rhobar", "");
    uint32_t dfEmbed_size= maxTotalAtoms*sizeof(real_t);
    cd_preserve(cdh, pot->dfEmbed, size, kCopy, "EamPotential_dfEmbed", "");
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
    cd_preserve(cdh, xchange, size, kCopy, "HaloExchange", "");
    printf("Preserve HaloExchange: %zu\n", sizeof(HaloExchange));
    if(is_force) {
        //TODO: This name is confusing.
        //      This is preserveing ForceExchangeParms
        size += preserveHaloForce(cdh, xchange->parms);
    } else {
        //TODO: This name is confusing.
        //      This is preserveing AtomExchangeParms
        size += preserveHaloAtom(cdh, xchange->parms);
    }
    return size;
}

unsigned int preserveHaloAtom(cd_handle_t *cdh, AtomExchangeParms *xchange_parms)
{
    uint32_t size = sizeof(AtomExchangeParms);
    cd_preserve(cdh, xchange_parms, size, kCopy, "AtomExchangeParms", "");
    printf("Preserve Halo Atom:%zu\n", sizeof(AtomExchangeParms));
    //printf("Preserve AtomExchangeParms:%zu\n", sizeof(AtomExchangeParms));
    uint32_t cellList_size;
    for(int i=0; i<6; i++) {
        printf("Preserve cellList[%d]:%u\n", i, xchange_parms->nCells[i] * sizeof(int));
        cellList_size += xchange_parms->nCells[i] * sizeof(int);
    }
    cd_preserve(cdh, xchange_parms->cellList, cellList_size, kCopy, 
                "AtomExchangeParms_cellList", "");
    uint32_t pbcFactor_size;
    for(int i=0; i<6; i++) {
        printf("Preserve pbfFactor[%d]:%u\n", i, sizeof(real_t) * 3);
        pbcFactor_size += 3 * sizeof(real_t);
    }
    cd_preserve(cdh, xchange_parms->pbcFactor, pbcFactor_size, kCopy, 
                "AtomExchangeParms_pbcFactor", "");
    size += cellList_size;
    size += pbcFactor_size;
    return size;
}

unsigned int preserveHaloForce(cd_handle_t *cdh, ForceExchangeParms *xchange_parms)
{
    //FIXME
    assert(is_eam);
    uint32_t size = sizeof(ForceExchangeParms);
    cd_preserve(cdh, xchange_parms, size, kCopy, "ForceExchangeParms", "");
    printf("Preserve Halo Force:%zu\n", sizeof(ForceExchangeParms));
    uint32_t buffer_size=0;
    for(int i=0; i<6; i++) {
        printf("Preserve buffer[%d]:%u\n", i, xchange_parms->nCells[i] * sizeof(int));
        buffer_size += xchange_parms->nCells[i] * sizeof(int);
    }
    cd_preserve(cdh, xchange_parms->sendCells, buffer_size, kCopy, 
                "ForceExchangeParms_sendCells", "");
    cd_preserve(cdh, xchange_parms->recvCells, buffer_size, kCopy, 
                "ForceExchangeParms_recvCells", "");
    size += buffer_size; //TODO: sendCells
    size += buffer_size; //TODO: recvCells
    return size;
}

//defined in eam.h
unsigned int preserveForceData(cd_handle_t *cdh, ForceExchangeData *forceData)
{
    uint32_t size = sizeof(ForceExchangeData);
    cd_preserve(cdh, forceData, size, kCopy, "ForceExchangeData", "");
    printf("Preserve ForceExchangeData: %zu\n", sizeof(ForceExchangeData));
    //preserve dfEmbed and boxes
    uint32_t dfEmbed_size = sizeof(forceData->dfEmbed);
    cd_preserve(cdh, forceData->dfEmbed, dfEmbed_size, kCopy, "ForceExchangeData_dfEmbed", "");
    uint32_t boxes_size = sizeof(forceData->boxes);
    cd_preserve(cdh, forceData->boxes, boxes_size, kCopy, "ForceExchangeData_boxes", "");
    size += dfEmbed_size + boxes_size;
    return size;
}

