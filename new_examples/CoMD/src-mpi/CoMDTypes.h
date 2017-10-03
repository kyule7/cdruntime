/// \file
/// CoMD data structures.

#ifndef __COMDTYPES_H_
#define __COMDTYPES_H_

#include <stdio.h>
#include "mytype.h"
#include "haloExchange.h"
#include "linkCells.h"
#include "decomposition.h"
#include "initAtoms.h"

extern int myRank;
extern int nRanks;
struct SimFlatSt;
struct ForceExchangeDataSt;
/// The base struct from which all potentials derive.  Think of this as an
/// abstract base class.
///
/// CoMD uses the following units:
///  - distance is in Angstroms
///  - energy is in eV
///  - time is in fs
///  - force in in eV/Angstrom
///
///  The choice of distance, energy, and time units means that the unit
///  of mass is eV*fs^2/Angstrom^2.  Hence, we must convert masses that
///  are input in AMU (atomic mass units) into internal mass units.
typedef struct BasePotentialSt 
{
   real_t cutoff;          //!< potential cutoff distance in Angstroms
   real_t mass;            //!< mass of atoms in intenal units
   real_t lat;             //!< lattice spacing (angs) of unit cell
   char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
   char  name[3];	   //!< element name
   int	 atomicNo;	   //!< atomic number  
   int  (*force)(struct SimFlatSt* s); //!< function pointer to force routine
   void (*print)(FILE* file, struct BasePotentialSt* pot);
   void (*destroy)(struct BasePotentialSt** pot); //!< destruction of the potential
} BasePotential;

/// Derived struct for a Lennard Jones potential.
/// Polymorphic with BasePotential.
/// \see BasePotential
typedef struct LjPotentialSt
{
   real_t cutoff;          //!< potential cutoff distance in Angstroms
   real_t mass;            //!< mass of atoms in intenal units
   real_t lat;             //!< lattice spacing (angs) of unit cell
   char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
   char  name[3];	   //!< element name
   int	 atomicNo;	   //!< atomic number  
   int  (*force)(struct SimFlatSt* s); //!< function pointer to force routine
   void (*print)(FILE* file, BasePotential* pot);
   void (*destroy)(BasePotential** pot); //!< destruction of the potential
   real_t sigma;
   real_t epsilon;
} LjPotential;
typedef struct InterpolationObjectSt 
{
   int n;          //!< the number of values in the table
   real_t x0;      //!< the starting ordinate range
   real_t invDx;   //!< the inverse of the table spacing
   real_t* values; //!< the abscissa values
} InterpolationObject;
/// Derived struct for an EAM potential.
/// Uses table lookups for function evaluation.
/// Polymorphic with BasePotential.
/// \see BasePotential
typedef struct EamPotentialSt 
{
   real_t cutoff;          //!< potential cutoff distance in Angstroms
   real_t mass;            //!< mass of atoms in intenal units
   real_t lat;             //!< lattice spacing (angs) of unit cell
   char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
   char  name[3];	   //!< element name
   int	 atomicNo;	   //!< atomic number  
   int  (*force)(struct SimFlatSt* s); //!< function pointer to force routine
   void (*print)(FILE* file, BasePotential* pot);
   void (*destroy)(BasePotential** pot); //!< destruction of the potential
   InterpolationObject* phi;  //!< Pair energy
   InterpolationObject* rho;  //!< Electron Density
   InterpolationObject* f;    //!< Embedding Energy

   real_t* rhobar;        //!< per atom storage for rhobar
   real_t* dfEmbed;       //!< per atom storage for derivative of Embedding
   HaloExchange* forceExchange;
   struct ForceExchangeDataSt* forceExchangeData; 
} EamPotential;

/// species data: chosen to match the data found in the setfl/funcfl files
typedef struct SpeciesDataSt
{
   char  name[3];   //!< element name
   int	 atomicNo;  //!< atomic number  
   real_t mass;     //!< mass in internal units
} SpeciesData;

/// Simple struct to store the initial energy and number of atoms. 
/// Used to check energy conservation and atom conservation. 
typedef struct ValidateSt
{
   double eTot0; //<! Initial total energy
   int nAtoms0;  //<! Initial global number of atoms
} Validate;

/// 
/// The fundamental simulation data structure with MAXATOMS in every
/// link cell.
/// 
typedef struct SimFlatSt
{
   int nSteps;            //<! number of time steps to run
   int printRate;         //<! number of steps between output
   double dt;             //<! time step
   
   Domain* domain;        //<! domain decomposition data

   LinkCell* boxes;       //<! link-cell data

   Atoms* atoms;          //<! atom data (positions, momenta, ...)

   SpeciesData* species;  //<! species data (per species, not per atom)
   
   real_t ePotential;     //!< the total potential energy of the system
   real_t eKinetic;       //!< the total kinetic energy of the system

   BasePotential *pot;	  //!< the potential

   HaloExchange* atomExchange;
   
} SimFlat;

/// Extra data members that are needed for the exchange of atom data.
/// For an atom exchange, the HaloExchangeSt::parms will point to a
/// structure of this type.
typedef struct AtomExchangeParmsSt
{
   int nCells[6];        //!< Number of cells in cellList for each face.
   int* cellList[6];     //!< List of link cells from which to load data for each face.
   real_t* pbcFactor[6]; //!< Whether this face is a periodic boundary.
}
AtomExchangeParms;

/// Extra data members that are needed for the exchange of force data.
/// For an force exchange, the HaloExchangeSt::parms will point to a
/// structure of this type.
typedef struct ForceExchangeParmsSt
{
   int nCells[6];     //!< Number of cells to send/recv for each face.
   int* sendCells[6]; //!< List of link cells to send for each face.
   int* recvCells[6]; //!< List of link cells to recv for each face.
}
ForceExchangeParms;

#endif
