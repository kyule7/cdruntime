#include "CoMDTypes.h"
extern int is_eam;
//int preserveSimFlat(SimFlat *sim);
unsigned int preserveSimFlat(cd_handle_t *cdh, SimFlat *sim, int doeam);
unsigned int preserveDomain(cd_handle_t *cdh, Domain *domain);
unsigned int preserveLinkCell(cd_handle_t *cdh, 
                              LinkCell *linkcell, 
                              unsigned int all);
unsigned int preserveAtoms(cd_handle_t *cdh, 
                           Atoms *atoms, 
                           int nTotalBoxes,
                           unsigned int is_gid,
                           unsigned int is_r,
                           unsigned int is_p,
                           unsigned int is_f,
                           unsigned int is_U,
                           unsigned int is_iSpecies
                          );
unsigned int preserveSpeciesData(cd_handle_t *cdh, SpeciesData *species);
unsigned int preserveLjPot(cd_handle_t *cdh, LjPotential *pot);
unsigned int preserveInterpolationObject(cd_handle_t *cdh, 
                                         InterpolationObject *obj);
unsigned int preserveEamPot(cd_handle_t *cdh, 
                            EamPotential *pot, 
                            int nTotalBoxes);
unsigned int preserveHaloExchange(cd_handle_t *cdh, 
                                  HaloExchange *xchange, 
                                  int is_force);
unsigned int preserveHaloAtom(cd_handle_t *cdh, 
                              AtomExchangeParms *xchange_parms,
                              unsigned int is_cellList,
                              unsigned int is_pbcFactor);
unsigned int preserveHaloForce(cd_handle_t *chd, 
                               ForceExchangeParms *xchange);
unsigned int preserveForceData(cd_handle_t *chd, 
                               ForceExchangeData *forceData);

/// Domain decomposition information.
//typedef struct DomainSt
//{
//   // process-layout data
//   int procGrid[3];        //!< number of processors in each dimension
//   int procCoord[3];       //!< i,j,k for this processor
//
//   // global bounds data
//   real3 globalMin;        //!< minimum global coordinate (angstroms)
//   real3 globalMax;        //!< maximum global coordinate (angstroms)
//   real3 globalExtent;     //!< global size: globalMax - globalMin
//
//   // local bounds data
//   real3 localMin;         //!< minimum coordinate on local processor
//   real3 localMax;         //!< maximum coordinate on local processor
//   real3 localExtent;      //!< localMax - localMin
//} Domain;
//
//typedef struct LinkCellSt
//{
//   int gridSize[3];     
//   int nLocalBoxes;     
//   int nHaloBoxes;     
//   int nTotalBoxes;     
//                        
//   real3 localMin;      
//   real3 localMax;      
//   real3 boxSize;       
//   real3 invBoxSize;    
//
//   int* nAtoms;   // nAtoms = nTotalBoxes*sizeof(int)
//} LinkCell;
//
//
//typedef struct AtomsSt
//{
//   int nLocal;    
//   int nGlobal;   
//                  // int maxTotalAtoms = MAXATOMS*boxes->nTotalBoxes;
//   int* gid;      // maxTotalAtoms*sizeof(int));
//   int* iSpecies; // maxTotalAtoms*sizeof(int));
//   real3*  r;     // maxTotalAtoms*sizeof(real3));
//   real3*  p;     // maxTotalAtoms*sizeof(real3));
//   real3*  f;     // maxTotalAtoms*sizeof(real3));
//   real_t* U;     // maxTotalAtoms*sizeof(real_t));
//} Atoms;
//
//typedef struct SpeciesDataSt
//{
//   char  name[3];   //!< element name
//   int	 atomicNo;  //!< atomic number  
//   real_t mass;     //!< mass in internal units
//} SpeciesData;
//
//typedef struct BasePotentialSt 
//{
//   real_t cutoff;          //!< potential cutoff distance in Angstroms
//   real_t mass;            //!< mass of atoms in intenal units
//   real_t lat;             //!< lattice spacing (angs) of unit cell
//   char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
//   char  name[3];	   //!< element name
//   int	 atomicNo;	   //!< atomic number  
//   int  (*force)(struct SimFlatSt* s); //!< function pointer to force routine
//   void (*print)(FILE* file, struct BasePotentialSt* pot);
//   void (*destroy)(struct BasePotentialSt** pot); //!< destruction of the potential
//} BasePotential;
//typedef struct LjPotentialSt
//{
//   real_t cutoff;          //!< potential cutoff distance in Angstroms
//   real_t mass;            //!< mass of atoms in intenal units
//   real_t lat;             //!< lattice spacing (angs) of unit cell
//   char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
//   char  name[3];	   //!< element name
//   int	 atomicNo;	   //!< atomic number  
//   int  (*force)(SimFlat* s); //!< function pointer to force routine
//   void (*print)(FILE* file, BasePotential* pot);
//   void (*destroy)(BasePotential** pot); //!< destruction of the potential
//   real_t sigma;
//   real_t epsilon;
//} LjPotential;
//typedef struct EamPotentialSt 
//{
//   real_t cutoff;          //!< potential cutoff distance in Angstroms
//   real_t mass;            //!< mass of atoms in intenal units
//   real_t lat;             //!< lattice spacing (angs) of unit cell
//   char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
//   char  name[3];	   //!< element name
//   int	 atomicNo;	   //!< atomic number  
//   int  (*force)(SimFlat* s); //!< function pointer to force routine
//   void (*print)(FILE* file, BasePotential* pot);
//   void (*destroy)(BasePotential** pot); //!< destruction of the potential
//   InterpolationObject* phi;  //!< Pair energy
//   InterpolationObject* rho;  //!< Electron Density
//   InterpolationObject* f;    //!< Embedding Energy
//
//   real_t* rhobar;        //maxTotalAtoms*sizeof(real_t)
//   real_t* dfEmbed;       //maxTotalAtoms*sizeof(real_t)
//   HaloExchange* forceExchange;
//   ForceExchangeData* forceExchangeData; // shallow copy
//} EamPotential;
//
//
//typedef struct InterpolationObjectSt 
//{
//   int n;          //
//   real_t x0;      //
//   real_t invDx;   //
//   real_t* values; // (real_t*)comdCalloc(1, (n+3)*sizeof(real_t));
//} InterpolationObject;
//typedef struct ForceExchangeDataSt
//{
//   real_t* dfEmbed; //<! derivative of embedding energy
//   struct LinkCellSt* boxes;
//}ForceExchangeData;
//
//typedef struct SimFlatSt
//{
//   int nSteps;            
//   int printRate;         
//   double dt;             
//   
//   Domain* domain;       // flat 
//
//   LinkCell* boxes;      // nAtoms = nTotalBoxes*sizeof(int)
//
//   Atoms* atoms;         //
//
//   SpeciesData* species;  
//   
//   real_t ePotential;     
//   real_t eKinetic;       
//
//   BasePotential *pot;	  // depends on cmd.doeam
//
//   HaloExchange* atomExchange; // always atomExchange
//   
//} SimFlat;
//
//typedef struct HaloExchangeSt
//{
//   int nbrRank[6];
//   int bufCapacity;
//   int  (*loadBuffer)(void* parms, void* data, int face, char* buf);
//   void (*unloadBuffer)(void* parms, void* data, int face, int bufSize, char* buf);
//   void (*destroy)(void* parms);
//   void* parms;
//} 
//
//// each chunk in cellList is defined in nCells
//// each chunk in pbcFactor is always 3
//typedef struct AtomExchangeParmsSt
//{
//   int nCells[6];        
//   int* cellList[6];    
//   real_t* pbcFactor[6]; 
//}
//AtomExchangeParms;
//// sendCells = nCells*sizeof(int));
//// recvCells = nCells*sizeof(int));
//typedef struct ForceExchangeParmsSt
//{
//   int nCells[6];     
//   int* sendCells[6]; 
//   int* recvCells[6]; 
//}
//ForceExchangeParms;

//extern int is_eam;
//int preserveSimFlat(SimFlat *sim);
//
//int is_eam = false;
//
//int preserveSimFlat(SimFlat *sim)
//{
//    printf("Preserve SimFlat: %zu\n", sizeof(SimFlat));
//    
//    preserveDomain(sim->domain);       // flat 
//    preserveLinkCell(sim->boxes);      // nAtoms = nTotalBoxes*sizeof(int)
//    preserveAtoms(sim->atoms);         // 
//    preserveSpeciesData(sim->species);   // flat
//    if(is_eam) {
//        preserveEamPot(sim->pot);	  
//    } else {
//        preserveLjPot(sim->pot);	// flat 
//    }
//    preserveHaloExchange(sim->atomExchange, 0);
//}
//
//int preserveDomain(Domain *domain)
//{
//    printf("Preserve Domain: %zu\n", sizeof(Domain));
//}
//
//int preserveLinkCell(LinkCell *linkcell)
//{
//    uint32_t nAtoms_size = linkcell->nTotalBoxes*sizeof(int);
//    printf("Preserven LinkCell %zu, nAtoms: %u\n", sizeof(LinkCell), nAtoms_size);
//}
//
//int preserveAtoms (Atoms *atoms, int nTotalBoxes)
//{
//    uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
//    uint32_t gid_size      = maxTotalAtoms*sizeof(int);
//    uint32_t iSpecies_size = maxTotalAtoms*sizeof(int);
//    uint32_t r_size        = maxTotalAtoms*sizeof(real3);
//    uint32_t p_size        = maxTotalAtoms*sizeof(real3);
//    uint32_t f_size        = maxTotalAtoms*sizeof(real3);
//    uint32_t U_size        = maxTotalAtoms*sizeof(real_t);
//
//    printf("Preserve Atoms: %zu, totAtoms:%u, gid:%u, species:%u, r:%u, p:%u, f:%u, U:%u\n"
//         , sizeof(Atoms)
//         , maxTotalAtoms
//         , gid_size     
//         , iSpecies_size
//         , r_size       
//         , p_size       
//         , f_size       
//         , U_size       
//        );
//
//}
//
//int preserveSpeciesData(SpeciesData *species)
//{
//    printf("Preserve Species %zu\n", sizeof(SpeciesData));
//}
//
//int preserveLjPot(LjPotential *pot)
//{
//    printf("Preserve LjPotential %zu\n", sizeof(LjPotential));
//}
//
//int preserveInterpolationObject(InterpolationObject *obj)
//{
//    uint32_t values_size = (obj->n + 3)*sizeof(real_t);
//    printf("Preserve LjPotential %zu, values: %u\n", sizeof(InterpolationObject), values_size);
//}
//
//int preserveEamPot(EamPotential *pot, int nTotalBoxes)
//{
//    assert(is_eam);
//    uint32_t maxTotalAtoms = MAXATOMS * nTotalBoxes;
//    uint32_t rhobar_size = maxTotalAtoms*sizeof(real_t);
//    uint32_t dfEmbed_size= maxTotalAtoms*sizeof(real_t);
//    printf("Preserve EamPotential %zu, rhobar:%u, dfEmbed:%u\n"
//        , sizeof(EamPotential)
//        , rhobar_size 
//        , dfEmbed_size
//        );
//    preserveInterpolationObject(pot->phi);
//    preserveInterpolationObject(pot->rho);
//    preserveInterpolationObject(pot->f);
//    preserveHaloExchange(pot->forceExchange, is_eam);
//    preserveForceData(pot->forceExchangeData); // shallow copy
//}
//
//int preserveHaloExchange(HaloExchange *xchange, int is_force)
//{
//    printf("Preserve HaloExchange: %zu\n", sizeof(HaloExchange));
//    if(is_force) {
//        preserveHaloForce(xchange->parms);
//    } else {
//        preserveHaloAtom(xchange->parms);
//    }
//}
//
//int preserveHaloAtom(AtomExchangeParms *xchange)
//{
//    printf("Preserve Halo Atom:%zu\n", sizeof(AtomExchangeParms));
//    for(int i=0; i<6; i++) {
//        printf("Preserve cellList[%d]:%u\n", i, xchange->nCells[i] * sizeof(int));
//    }
//    for(int i=0; i<6; i++) {
//        printf("Preserve pbfFactor[%d]:%u\n", i, sizeof(real_t) * 3);
//    }
//}
//
//int preserveHaloForce(ForceExchangeParms *xchange)
//{
//    assert(is_eam);
//    printf("Preserve Halo Force:%zu\n", sizeof(ForceExchangeParms));
//    for(int i=0; i<6; i++) {
//        printf("Preserve buffer[%d]:%u\n", i, xchange->nCells[i] * sizeof(int));
//    }
//}
//
//int preserveForceData(ForceExchangeData *forceData)
//{
//    printf("Preserve ForceExchangeData: %zu\n", sizeof(ForceExchangeData));
//
//    // Preserve these two
////    forceData->dfEmbed;
////    forceData->boxes;
//
//}
//
//int (*prvDomain       )(void);    
//int (*prvLinkCell     )(void);     
//int (*prvAtoms        )(void);   
//int (*prvSpeciesData  )(void);
//int (*prvBasePotential)(void);
//int (*prvHaloExchange )(void);
