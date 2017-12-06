/// \file
/// Computes forces for the 12-6 Lennard Jones (LJ) potential.
///
/// The Lennard-Jones model is not a good representation for the
/// bonding in copper, its use has been limited to constant volume
/// simulations where the embedding energy contribution to the cohesive
/// energy is not included in the two-body potential
///
/// The parameters here are taken from Wolf and Phillpot and fit to the
/// room temperature lattice constant and the bulk melt temperature
/// Ref: D. Wolf and S.Yip eds. Materials Interfaces (Chapman & Hall
///      1992) Page 230.
///
/// Notes on LJ:
///
/// http://en.wikipedia.org/wiki/Lennard_Jones_potential
///
/// The total inter-atomic potential energy in the LJ model is:
///
/// \f[
///   E_{tot} = \sum_{ij} U_{LJ}(r_{ij})
/// \f]
/// \f[
///   U_{LJ}(r_{ij}) = 4 \epsilon
///           \left\{ \left(\frac{\sigma}{r_{ij}}\right)^{12}
///           - \left(\frac{\sigma}{r_{ij}}\right)^6 \right\}
/// \f]
///
/// where \f$\epsilon\f$ and \f$\sigma\f$ are the material parameters in the potential.
///    - \f$\epsilon\f$ = well depth
///    - \f$\sigma\f$   = hard sphere diameter
///
///  To limit the interation range, the LJ potential is typically
///  truncated to zero at some cutoff distance. A common choice for the
///  cutoff distance is 2.5 * \f$\sigma\f$.
///  This implementation can optionally shift the potential slightly
///  upward so the value of the potential is zero at the cuotff
///  distance.  This shift has no effect on the particle dynamics.
///
///
/// The force on atom i is given by
///
/// \f[
///   F_i = -\nabla_i \sum_{jk} U_{LJ}(r_{jk})
/// \f]
///
/// where the subsrcipt i on the gradient operator indicates that the
/// derivatives are taken with respect to the coordinates of atom i.
/// Liberal use of the chain rule leads to the expression
///
/// \f{eqnarray*}{
///   F_i &=& - \sum_j U'_{LJ}(r_{ij})\hat{r}_{ij}\\
///       &=& \sum_j 24 \frac{\epsilon}{r_{ij}} \left\{ 2 \left(\frac{\sigma}{r_{ij}}\right)^{12}
///               - \left(\frac{\sigma}{r_{ij}}\right)^6 \right\} \hat{r}_{ij}
/// \f}
///
/// where \f$\hat{r}_{ij}\f$ is a unit vector in the direction from atom
/// i to atom j.
/// 
///

#include "ljForce.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "constants.h"
#include "mytype.h"
#include "parallel.h"
#include "linkCells.h"
#include "memUtils.h"
#include "CoMDTypes.h"

#if _CD2
#include "cd.h"
//#define CD2_INTERVAL 1000
#endif

#define POT_SHIFT 1.0

// /// Derived struct for a Lennard Jones potential.
// /// Polymorphic with BasePotential.
// /// \see BasePotential
// typedef struct LjPotentialSt
// {
//    real_t cutoff;          //!< potential cutoff distance in Angstroms
//    real_t mass;            //!< mass of atoms in intenal units
//    real_t lat;             //!< lattice spacing (angs) of unit cell
//    char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
//    char  name[3];	   //!< element name
//    int	 atomicNo;	   //!< atomic number  
//    int  (*force)(SimFlat* s); //!< function pointer to force routine
//    void (*print)(FILE* file, BasePotential* pot);
//    void (*destroy)(BasePotential** pot); //!< destruction of the potential
//    real_t sigma;
//    real_t epsilon;
// } LjPotential;

static int ljForce(SimFlat* s);
static void ljPrint(FILE* file, BasePotential* pot);

#if _CD2
unsigned int preserveAtoms(cd_handle_t *cdh, 
                           Atoms *atoms, 
                           int nTotalBoxes,
                           unsigned int is_gid,
                           unsigned int is_r,
                           unsigned int is_p,
                           unsigned int is_f,
                           unsigned int is_U,
                           unsigned int is_iSpecies,
                           unsigned int from,
                           int to,
                           unsigned int is_print, 
                           char* idx
                          );
unsigned int preserveLinkCell(cd_handle_t *cdh, 
    LinkCell *linkcell, 
                              unsigned int is_all,
                              unsigned int is_nAtoms,
                              unsigned int is_nTotalBoxes);
#endif

void ljDestroy(BasePotential** inppot)
{
  if ( ! inppot ) return;
  LjPotential* pot = (LjPotential*)(*inppot);
  if ( ! pot ) return;
  comdFree(pot);
  *inppot = NULL;

  return;
}

/// Initialize an Lennard Jones potential for Copper.
BasePotential* initLjPot(void)
{
  LjPotential *pot = (LjPotential*)comdMalloc(sizeof(LjPotential));
  pot->force = ljForce;
  pot->print = ljPrint;
  pot->destroy = ljDestroy;
  pot->sigma = 2.315;	                  // Angstrom
  pot->epsilon = 0.167;                  // eV
  pot->mass = 63.55 * amuToInternalMass; // Atomic Mass Units (amu)

  pot->lat = 3.615;                      // Equilibrium lattice const in Angs
  strcpy(pot->latticeType, "FCC");       // lattice type, i.e. FCC, BCC, etc.
  pot->cutoff = 2.5*pot->sigma;          // Potential cutoff in Angs

  strcpy(pot->name, "Cu");
  pot->atomicNo = 29;

  return (BasePotential*) pot;
}

void ljPrint(FILE* file, BasePotential* pot)
{
  LjPotential* ljPot = (LjPotential*) pot;
  fprintf(file, "  Potential type   : Lennard-Jones\n");
  fprintf(file, "  Species name     : %s\n", ljPot->name);
  fprintf(file, "  Atomic number    : %d\n", ljPot->atomicNo);
  fprintf(file, "  Mass             : "FMT1" amu\n", ljPot->mass / amuToInternalMass); // print in amu
  fprintf(file, "  Lattice Type     : %s\n", ljPot->latticeType);
  fprintf(file, "  Lattice spacing  : "FMT1" Angstroms\n", ljPot->lat);
  fprintf(file, "  Cutoff           : "FMT1" Angstroms\n", ljPot->cutoff);
  fprintf(file, "  Epsilon          : "FMT1" eV\n", ljPot->epsilon);
  fprintf(file, "  Sigma            : "FMT1" Angstroms\n", ljPot->sigma);
}

// to prevent CD for ljForce from being called during initialization  
#if _CD2
int is_first = 0; 
#endif
int ljForce(SimFlat* s)
{
#if _CD2
  const int CD2_INTERVAL = s->preserveRate;
#endif
  //TODO: not creat sequential CD here since all the below are easy to get 
  LjPotential* pot = (LjPotential *) s->pot;
  real_t sigma = pot->sigma;
  real_t epsilon = pot->epsilon;
  real_t rCut = pot->cutoff;
  real_t rCut2 = rCut*rCut;

  // zero forces and energy
  real_t ePot = 0.0;
  s->ePotential = 0.0;
  int fSize = s->boxes->nTotalBoxes*MAXATOMS;
  for (int ii=0; ii<fSize; ++ii)
  {
    zeroReal3(s->atoms->f[ii]);
    s->atoms->U[ii] = 0.;
  }

  real_t s6 = sigma*sigma*sigma*sigma*sigma*sigma;

  real_t rCut6 = s6 / (rCut2*rCut2*rCut2);
  real_t eShift = POT_SHIFT * rCut6 * (rCut6 - 1.0);

  int nbrBoxes[27];
  // CD handler for level2 CD 
  // TODO: 0xF gets ignored when config.yaml specifies error mask
#if _CD2
  cd_handle_t *lv3_cd = NULL;
  if(is_first) {
   //cd_handle_t *lv3_cd = cd_create(getcurrentcd(), 1, "ljForce", kStrict, 0xF);
   //lv3_cd = cd_create(getleafcd(), 1, "ljForce", kStrict, 0xC);
   //lv3_cd = cd_create(getcurrentcd(), 1, "ljForce", kStrict|kDRAM, 0xC);
   lv3_cd = cd_create(getcurrentcd(), 
                       getNRanks(), 
                       "ljForce", 
                       kStrict|kDRAM,
                       0xC); // detect F8,F4
   //*****************************************************
   //This is important characteristics of CoMD force loop.
   //*****************************************************
   //The positions of entier atoms (s->atoms->r) need to be preserved 
   //no matter how finer/frequently leaf CD (lv3 CD) begins or completes
   //int ljForce_pre_size = preserveAtoms(lv3_cd, s->atoms, 
   //    s->boxes->nTotalBoxes, 
   //    1,  // is_gid
   //    1,  // is_r
   //    0,  // is_p
   //    0,  // is_f
   //    0,  // is_U
   //    0,  // is_iSpecies
   //    MAXATOMS*jBox,          // from 
   //    MAXATOMS*jBox+nJBox-1,  // to 
   //    //0, // from 
   //    //-1, // to
   //    0,
   //    pre_atoms_idx); // is_print
  }
#endif
  //*****************************************************
  //1. finest-grained mapping to wrap iteration for atoms in jBox
  //*****************************************************
  // loop over local boxes (link cells) [#: nLocalBoxes]
  // |-loop over neighbors of iBox k  [#: nNbrBoxes]
  //   |- loop over atoms in iBox [#: nIBox]
  //      -----------------------------------------
  //      ---------------Level 3 CD----------------
  //      |- loop over atoms in jBox [#: nJBox]
  //         |- ljForce computation
  //      ---------------Level 3 CD----------------
  //      -----------------------------------------
  //*****************************************************
  //1. coarser grained mapping
  //*****************************************************
  // loop over local boxes (link cells) [#: nLocalBoxes]
  // |-loop over neighbors of iBox k  [#: nNbrBoxes]
  //   --------------------------------------------
  //   ------------------Level 3 CD----------------
  //   |- loop over atoms in iBox [#: nIBox]
  //      |- loop over atoms in jBox [#: nJBox]
  //         |- ljForce computation
  //   ------------------Level 3 CD----------------
  //   --------------------------------------------
  //*****************************************************
  // beginning of main computation (hot spot)
  //*****************************************************
  // loop over local boxes
  //*****************************************************
  // O{nNbrBoxes, nIBox} <- I{s->boxes, s->boxes->[nLocalBoxes, nAtoms[iBox]], nbrBoxes}
  for (int iBox=0; iBox < s->boxes->nLocalBoxes; iBox++)
  {
#if _CD2
    if(is_first) {
      if(iBox % CD2_INTERVAL == 0) {
        cd_begin(lv3_cd, "ljForce_innermost"); 
        //TODO: cd_preserve
      }
    }
#endif
        //printf("Rank[%d] is processing iBox[%d]\n", getMyRank(), iBox);
    int nIBox = s->boxes->nAtoms[iBox]; // #of atoms in ith box
    if ( nIBox == 0 ) {
#if _CD2
      if(is_first) {
        if(iBox % CD2_INTERVAL == 0) {
          cd_detect(lv3_cd);
          cd_complete(lv3_cd); 
        }
      }
#endif
      continue;
    }
    //Note that neighbors of iBox also include the box itself as 13th element
    int nNbrBoxes = getNeighborBoxes(s->boxes, iBox, nbrBoxes);
    // loop over neighbors of iBox
    // O{atoms->f, atoms->U, ePot} <- I{atoms->gid, epsilon, rCut6, nIBox, ii, iOff}
    for (int jTmp=0; jTmp<nNbrBoxes; jTmp++)
    {
      int jBox = nbrBoxes[jTmp];

      assert(jBox>=0);

      //#of atoms in jth box
      int nJBox = s->boxes->nAtoms[jBox];
      if ( nJBox == 0 ) {
#if _CD2
        if(is_first) {
          if(iBox % CD2_INTERVAL == 0) {
          }
        }
#endif
        continue;
      }
      //*****************************************************
      // loop over atoms in iBox
      //*****************************************************
      // O{iOff, iID} <- I{iOff, iBox, ii, atoms->gid}
      for (int iOff=iBox*MAXATOMS,ii=0; ii<nIBox; ii++,iOff++)
      {
        int iId = s->atoms->gid[iOff];
#if _CD2
/*
        if(is_first) {
          //cd_handle_t *cdh_lv2_inner = getleafcd();
          //This CD has a length of nJBox 
          //Note that cd_complete might fail if there if "continue" between
          //cd_begin and cd_complete, which is the case for innermost loop, 
          //shown below.
          //Also, it is worth mentioning that the number of loop iterations for
          //the innermost loop below changes over time, depending on runtime
          //behavior due to moving atoms.
          if(iOff % CD2_INTERVAL == 0) {
            cd_begin(lv3_cd, "ljForce_innermost"); 
            //TODO: cd_preserve
            //1. loop index for current loop
            //  - iOff, iBox, MAXATOMS, ii, nIBox
            //2. data to be read in the innermost loop below
            //preserve all loop index parameters 
            //From the iBox loop (outmost)
#ifdef DO_PRV
            cd_preserve(lv3_cd, &iBox, sizeof(int), kCopy, 
                "ljForce_innermost_iBox", "ljForce_innermost_iBox");
            //From the neighbors of iBox loop (2nd loop)
            cd_preserve(lv3_cd, &jBox, sizeof(int), kCopy, 
                "ljForce_innermost_jBox", "ljForce_innermost_jBox");
            cd_preserve(lv3_cd, &jTmp, sizeof(int), kCopy, 
                "ljForce_innermost_jTmp", "ljForce_innermost_jTmp");
            //From the atoms of iBox loop (3rd loop, the current loop)
            cd_preserve(lv3_cd, &iOff, sizeof(int), kCopy, 
                "ljForce_innermost_iOff", "ljForce_innermost_iOff");
            cd_preserve(lv3_cd, &ii, sizeof(int), kCopy, 
                "ljForce_innermost_ii", "ljForce_innermost_ii");
            //For the nested loop below
            cd_preserve(lv3_cd, &iId, sizeof(int), kCopy, 
                "ljForce_innermost_iId", "ljForce_innermost_iId");
#endif
            //TODO: the below preserve poistion(r) of all atoms every iteration, 
            //      which is not the optimal case. 
            //      (not all 64 atoms are occupied)
            //      The otptimal preservation preserves poistons of atoms in the
            //      current jBox [#: nJBox = s->atoms->nAtoms[jBox]
            //TODO: need to give different name for each preservation. could be 
            //      associated with the indices, iBox, jTmp, and iOff
            char pre_atoms_idx[256]= "-1";   //FIXME: it this always enough?
            sprintf(pre_atoms_idx, "_%d_%d_%d", iBox, jTmp, iOff);
            // preserve atoms in jBox
            // FIXME: this is correct only when CD2_INTERVAL == 1
            int ljForce_pre_size = preserveAtoms(lv3_cd, s->atoms, 
                                                 s->boxes->nTotalBoxes, 
                                                 1,  // is_gid
                                                 //1,  // is_r
                                                 0,  // is_r
                                                 0,  // is_p
                                                 0,  // is_f
                                                 0,  // is_U
                                                 0,  // is_iSpecies
                                                 MAXATOMS*jBox,          // from 
                                                 //MAXATOMS*jBox+MAXATOMS-1,  // to 
                                                 MAXATOMS*jBox+nJBox-1,  // to 
                                                 //0, // from 
                                                 //-1, // to
                                                 0,
                                                 pre_atoms_idx); // is_print
            ljForce_pre_size += preserveLinkCell(lv3_cd, s->boxes, 
                0,  //all
                0,  //only nAtoms
                1); //nLocalBoxes
          }
        }
*/
#endif
        //*****************************************************
        // loop over atoms in jBox
        //*****************************************************
        // O{atoms->f, atoms->U, ePot} <- I{atoms->{gid, r[][]}, boxes->nLocalBoxes} 
        //                                I{eShift, epsilon, rCut6} 
        //                                I{iOff} ,// array index for s->atoms->r and U
        //                                I{jOff, jBox, nJBox, ij}  //loop param
        for (int jOff=MAXATOMS*jBox,ij=0; ij<nJBox; ij++,jOff++)
        {
          real_t dr[3];
          int jId = s->atoms->gid[jOff];  
          if (jBox < s->boxes->nLocalBoxes && jId <= iId ) {
#if _CD2
            if(is_first) {
              if(iBox % CD2_INTERVAL == 0) {
              }
            }
#endif
            continue; // don't double count local-local pairs.
          }
          real_t r2 = 0.0;
          for (int m=0; m<3; m++)
          {
            //TODO: important pattern
            //with fixed iOff, get the distacne between iOff and all jOff
            dr[m] = s->atoms->r[iOff][m]-s->atoms->r[jOff][m];
            r2+=dr[m]*dr[m];
          }
        
          //If the atom is farther than cutoff, then do not process further.
          if ( r2 > rCut2) {
#if _CD2
            if(is_first) {
              if(iBox % CD2_INTERVAL == 0) {
              }
            }
#endif
            continue;
          }
          // Important note:
          // from this point on r actually refers to 1.0/r
          r2 = 1.0/r2;
          real_t r6 = s6 * (r2*r2*r2);
          real_t eLocal = r6 * (r6 - 1.0) - eShift;
          s->atoms->U[iOff] += 0.5*eLocal;
          s->atoms->U[jOff] += 0.5*eLocal;

          // calculate energy contribution based on whether
          // the neighbor box is local or remote
          if (jBox < s->boxes->nLocalBoxes)
            ePot += eLocal;
          else
            ePot += 0.5 * eLocal;

          // different formulation to avoid sqrt computation
          real_t fr = - 4.0*epsilon*r6*r2*(12.0*r6 - 6.0);
          for (int m=0; m<3; m++)
          {
            s->atoms->f[iOff][m] -= dr[m]*fr;
            s->atoms->f[jOff][m] += dr[m]*fr;
          }

        } // loop over atoms in jBox
#if _CD2
/*
        if(is_first) {
          if(iOff % CD2_INTERVAL == 0) {
            cd_detect(lv3_cd);
            cd_complete(lv3_cd); 
          }
        }
*/
#endif

      } // loop over atoms in iBox
    } // loop over neighbor boxes
#if _CD2
    if(is_first) {
      if(iBox % CD2_INTERVAL == 0) {
        cd_detect(lv3_cd);
        cd_complete(lv3_cd); 
      }
    }
#endif
  } // loop over local boxes in system
  //*****************************************************
  // end of main computation (hot spot)
  //*****************************************************
#if _CD2
  if(is_first) {
    cd_destroy(lv3_cd);
  }
#endif
  ePot = ePot*4.0*epsilon;
  s->ePotential = ePot;
#if _CD2
  is_first = 1;
#endif
  return 0;
}
