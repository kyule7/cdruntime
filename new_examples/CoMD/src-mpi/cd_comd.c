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

int is_eam = 0;

unsigned int preserveSimFlat(cd_handle_t *cdh, SimFlat *sim)
{
    cd_preserve(cdh, sim, sizeof(SimFlat), kCopy, "SimFlat", "");
    printf("Preserve SimFlat: %zu\n", sizeof(SimFlat));
    uint32_t size = sizeof(SimFlat);    
    size += preserveDomain(cdh, sim->domain);       // flat 
    size += preserveLinkCell(sim->boxes);      // nAtoms = nTotalBoxes*sizeof(int)
    size += preserveAtoms(sim->atoms, sim->boxes->nTotalBoxes);         // 
    size += preserveSpeciesData(sim->species);   // flat
    if(is_eam) {
        size += preserveEamPot((EamPotential *)sim->pot, sim->boxes->nTotalBoxes);	  
    } else {
        size += preserveLjPot((LjPotential *)sim->pot);	// flat 
    }
    size += preserveHaloExchange(sim->atomExchange, 0);
    return size;
}

unsigned int preserveDomain(cd_handle_t *cdh, Domain *domain)
{
    uint32_t size = sizeof(Domain);
    cd_preserve(cdh, domain, sizeof(Domain), kCopy, "Domain", "");

    printf("Preserve Domain: %zu\n", sizeof(Domain));
    return size;
}

unsigned int preserveLinkCell(LinkCell *linkcell)
{
    uint32_t size = sizeof(LinkCell);
    uint32_t nAtoms_size = linkcell->nTotalBoxes*sizeof(int);
    size += nAtoms_size;
    printf("Preserven LinkCell %zu, nAtoms: %u\n", sizeof(LinkCell), nAtoms_size);
    return size;
}

unsigned int preserveAtoms (Atoms *atoms, int nTotalBoxes)
{
    uint32_t size = sizeof(Atoms);
    uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
    uint32_t gid_size      = maxTotalAtoms*sizeof(int);
    uint32_t iSpecies_size = maxTotalAtoms*sizeof(int);
    uint32_t r_size        = maxTotalAtoms*sizeof(real3);
    uint32_t p_size        = maxTotalAtoms*sizeof(real3);
    uint32_t f_size        = maxTotalAtoms*sizeof(real3);
    uint32_t U_size        = maxTotalAtoms*sizeof(real_t);
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

unsigned int preserveSpeciesData(SpeciesData *species)
{
    uint32_t size = sizeof(SpeciesData);
    printf("Preserve Species %zu\n", sizeof(SpeciesData));
    return size;
}

unsigned int preserveLjPot(LjPotential *pot)
{
    uint32_t size = sizeof(LjPotential);
    printf("Preserve LjPotential %zu\n", sizeof(LjPotential));
    return size;
}

unsigned int preserveInterpolationObject(InterpolationObject *obj)
{
    uint32_t size = sizeof(InterpolationObject);
    uint32_t values_size = (obj->n + 3)*sizeof(real_t);
    size += values_size;
    printf("Preserve LjPotential %zu, values: %u\n", sizeof(InterpolationObject), values_size);
    return size;
}

unsigned int preserveEamPot(EamPotential *pot, int nTotalBoxes)
{
    assert(is_eam);
    uint32_t size = sizeof(EamPotential);
    uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
    uint32_t rhobar_size = maxTotalAtoms*sizeof(real_t);
    uint32_t dfEmbed_size= maxTotalAtoms*sizeof(real_t);
    printf("Preserve EamPotential %zu, rhobar:%u, dfEmbed:%u\n"
        , sizeof(EamPotential)
        , rhobar_size 
        , dfEmbed_size
        );
    size += rhobar_size + dfEmbed_size;
    size += preserveInterpolationObject(pot->phi);
    size += preserveInterpolationObject(pot->rho);
    size += preserveInterpolationObject(pot->f);
    size += preserveHaloExchange(pot->forceExchange, is_eam);
    size += preserveForceData(pot->forceExchangeData); // shallow copy
    return size;
}

unsigned int preserveHaloExchange(HaloExchange *xchange, int is_force)
{
    uint32_t size = sizeof(HaloExchange);
    printf("Preserve HaloExchange: %zu\n", sizeof(HaloExchange));
    if(is_force) {
        size += preserveHaloForce(xchange->parms);
    } else {
        size += preserveHaloAtom(xchange->parms);
    }
    return size;
}

unsigned int preserveHaloAtom(AtomExchangeParms *xchange)
{
    uint32_t size = sizeof(AtomExchangeParms);
    printf("Preserve Halo Atom:%zu\n", sizeof(AtomExchangeParms));
    for(int i=0; i<6; i++) {
        printf("Preserve cellList[%d]:%u\n", i, xchange->nCells[i] * sizeof(int));
        size += xchange->nCells[i] * sizeof(int);
    }
    for(int i=0; i<6; i++) {
        printf("Preserve pbfFactor[%d]:%u\n", i, sizeof(real_t) * 3);
        size += 3 * sizeof(real_t);
    }
    return size;
}

unsigned int preserveHaloForce(ForceExchangeParms *xchange)
{
    assert(is_eam);
    uint32_t size = sizeof(ForceExchangeParms);
    printf("Preserve Halo Force:%zu\n", sizeof(ForceExchangeParms));
    for(int i=0; i<6; i++) {
        printf("Preserve buffer[%d]:%u\n", i, xchange->nCells[i] * sizeof(int));
        size += xchange->nCells[i] * sizeof(int);
    }
}

unsigned int preserveForceData(ForceExchangeData *forceData)
{
    uint32_t size = sizeof(ForceExchangeData);
    printf("Preserve ForceExchangeData: %zu\n", sizeof(ForceExchangeData));
    size += sizeof(forceData->dfEmbed) + sizeof(forceData->boxes);
    return size;

    // Preserve these two
//    forceData->dfEmbed;
//    forceData->boxes;

}

