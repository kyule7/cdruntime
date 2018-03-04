#include "CoMDTypes.h"
#include "eam.h" // for ForceExchangeData

#ifndef CD_COMD_H_
#define CD_COMD_H_

unsigned int preserveSimFlat(cd_handle_t *cdh, uint32_t knob, SimFlat *sim);
unsigned int preserveDomain(cd_handle_t *cdh, uint32_t knob, Domain *domain);
unsigned int preserveLinkCell(cd_handle_t *cdh, uint32_t knob,
                              LinkCell *linkcell, unsigned int is_all,
                              unsigned int is_nAtoms, unsigned int is_local,
                              unsigned int is_nLocalBoxes,
                              unsigned int is_nTotalBoxes);
unsigned int preserveAtoms(cd_handle_t *cdh, uint32_t knob, Atoms *atoms,
                           int nTotalBoxes, unsigned int is_all,
                           unsigned int is_gid, unsigned int is_r,
                           unsigned int is_p, unsigned int is_f,
                           unsigned int is_U, unsigned int is_iSpecies,
                           unsigned int from, int to, unsigned int is_print,
                           char *idx);
unsigned int preserveSpeciesData(cd_handle_t *cdh, uint32_t knob,
                                 SpeciesData *species);
unsigned int preserveLjPot(cd_handle_t *cdh, uint32_t knob, LjPotential *pot);
unsigned int preserveInterpolationObject(cd_handle_t *cdh, uint32_t knob,
                                         InterpolationObject *obj);
unsigned int preserveEamPot(cd_handle_t *cdh, uint32_t knob, int doeam,
                            EamPotential *pot, int nTotalBoxes);
unsigned int preserveHaloExchange(cd_handle_t *cdh, uint32_t knob, int doeam,
                                  HaloExchange *xchange, int is_force);
unsigned int preserveHaloAtom(cd_handle_t *cdh, uint32_t knob,
                              AtomExchangeParms *xchange_parms,
                              unsigned int is_cellList,
                              unsigned int is_pbcFactor);
unsigned int preserveHaloForce(cd_handle_t *cdh, uint32_t knob, int doeam,
                               ForceExchangeParms *xchange);
unsigned int preserveForceData(cd_handle_t *cdh, uint32_t knob,
                               ForceExchangeData *forceData);

#endif
