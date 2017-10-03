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

#define POT_SHIFT 1.0

#if _CD
#include "cd.h"
#endif

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
   int  (*force)(SimFlat* s); //!< function pointer to force routine
   void (*print)(FILE* file, BasePotential* pot);
   void (*destroy)(BasePotential** pot); //!< destruction of the potential
   real_t sigma;
   real_t epsilon;
} LjPotential;

static int ljForce(SimFlat* s);
static void ljPrint(FILE* file, BasePotential* pot);

void ljcd_destroy(BasePotential** inppot)
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
   pot->destroy = ljcd_destroy;
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

//FIXME: assumed only initLjpot called(used)
void serprvLjPot(LjPotential* pot)
//void serprvLjPot(BasePotential* pot)
{
  cd_handle_t *cdh = getleafcd();
  //FIXME: why these 3 function pointers are 1 byte? it's gcc convention
  cd_preserve(cdh, &(pot->force), sizeof(&ljForce), kCopy, "LjPot_pot_force", NULL);
  cd_preserve(cdh, &(pot->print), sizeof(&ljPrint), kCopy, "LjPot_pot_print", NULL);
  cd_preserve(cdh, &(pot->destroy), sizeof(&ljcd_destroy), kCopy, "LjPot_pot_destroy", NULL);

  cd_preserve(cdh, &(pot->sigma), sizeof(real_t), kCopy, "LjPot_pot_sigma", NULL);
  cd_preserve(cdh, &(pot->epsilon), sizeof(real_t), kCopy, "LjPot_pot_epsilon", NULL);
  cd_preserve(cdh, &(pot->mass), sizeof(real_t), kCopy, "LjPot_pot_mass", NULL);
  cd_preserve(cdh, &(pot->lat), sizeof(real_t), kCopy, "LjPot_pot_lat", NULL);
  cd_preserve(cdh, &(pot->cutoff), sizeof(real_t), kCopy, "LjPot_pot_cutoff", NULL);

  cd_preserve(cdh, pot->latticeType, 8*sizeof(char), kCopy, "LjPot_pot_latticeType", NULL);
  cd_preserve(cdh, pot->name, 3*sizeof(char), kCopy, "LjPot_pot_name", NULL);

  cd_preserve(cdh, &(pot->atomicNo), sizeof(int), kCopy, "LjPot_pot_atomicNo", NULL);

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

int is_first = 0;
int ljForce(SimFlat* s)
{
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

  cd_handle_t *cd_lv3 = NULL;
#if _CD3
  //to prevent being called during initialization
  if(is_first) {
    cd_lv3 = cd_create(getleafcd(), 1, "ljForce_1st_loop", kStrict, 0xC);
  }
#endif
  /////////////////////////////////////////////////////////////////////
  // loop over local boxes (link cells) [#: nLocalBoxes]
  // |-loop over neighbors of iBox k  [#: nNbrBoxes]
  //   |- loop over atoms in iBox [#: nIBox]
  //      |- loop over atoms in jBox [#: nJBox]
  //         |- ljForce computation
  /////////////////////////////////////////////////////////////////////
  // loop over local boxes (link cells)
  //
  // O{nNbrBoxes, nIBox} <- I{s->boxes, s->boxes->[nLocalBoxes, nAtoms[iBox]], nbrBoxes}
  //
  for (int iBox=0; iBox<s->boxes->nLocalBoxes; iBox++)
  {
#if _CD3
    if(is_first) {
      // if(iBox % 16 == 0) {
      cd_begin(cd_lv3, "3_ljForce_loop_over_local_boxes");
      //}
      ///cd_preserve(cd_lv3, &(pot->force), sizeof(&ljForce), kCopy, "LjPot_pot_force", NULL);
    }
#endif

    int nIBox = s->boxes->nAtoms[iBox];
    if ( nIBox == 0 ) continue;
    int nNbrBoxes = getNeighborBoxes(s->boxes, iBox, nbrBoxes);

    cd_handle_t *cd_lv4 = NULL;
#if _CD4
    if(is_first) {
      //if(iBox % 10 == 0) {
      cd_lv4 = cd_create(cd_lv3, 1, "ljForce_2nd_Loop", kStrict, 0x8);
      //}
    }
#endif
    /////////////////////////////////////////////////////////////////////
    // loop over neighbors of iBox
    //
    // O{atoms->f, atoms->U, ePot} <- I{atoms->gid, epsilon, rCut6, nIBox, ii, iOff}
    //
    for (int jTmp=0; jTmp<nNbrBoxes; jTmp++)
    {
#if _CD4
      if(is_first) {
        cd_begin(cd_lv4, "4_ljForce_loop_over_neighbors_of_iBox");
        //cd_lv4->cd_preserve(....);
      }
#endif
      int jBox = nbrBoxes[jTmp];

      assert(jBox>=0);

      int nJBox = s->boxes->nAtoms[jBox];
      if ( nJBox == 0 ) continue;

      cd_handle_t *cd_lv5 = NULL;
#if   _CD5
      if(is_first) {
        //if(iBox % 10 == 0) {
        cd_lv5 = cd_create(cd_lv4, 1, "ljForce_3rd_Loop", kStrict, 0x8);
        //}
      }
#endif
      /////////////////////////////////////////////////////////////////////
      // loop over atoms in iBox
      // O{iOff, iID} <- I{iOff, iBox, ii, atoms->gid}
      for (int iOff=iBox*MAXATOMS,ii=0; ii<nIBox; ii++,iOff++)
      {
#if     _CD5
        if(is_first) {
          cd_begin(cd_lv5, "5_ljForce_loop_over_atoms_in_jBox");
          //cd_lv5->cd_preserve(....);
        }
#endif
        int iId = s->atoms->gid[iOff];
        /////////////////////////////////////////////////////////////////////
        // loop over atoms in jBox
        // O{atoms->f, atoms->U, ePot} <- I{atoms->{gid, r[][]}, boxes->nLocalBoxes} 
        //                                I{eShift, epsilon, rCut6} 
        //                                I{iOff} ,// array index for s->atoms->r and U
        //                                I{jOff, jBox, nJBox, ij}  //loop param
        //
        for (int jOff=MAXATOMS*jBox,ij=0; ij<nJBox; ij++,jOff++)
        {
          real_t dr[3];
          int jId = s->atoms->gid[jOff];  
          if (jBox < s->boxes->nLocalBoxes && jId <= iId )
            continue; // don't double count local-local pairs.
          real_t r2 = 0.0;
          /////////////////////////////////////////////////////////////////////
          //
          for (int m=0; m<3; m++)
          {
            dr[m] = s->atoms->r[iOff][m]-s->atoms->r[jOff][m];
            r2+=dr[m]*dr[m];
          }

          if ( r2 > rCut2) continue;

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
#if     _CD5
        if(is_first) {
          cd_detect(cd_lv5);
          cd_complete(cd_lv5);
        }
#endif
      } // loop over atoms in iBox
#if _CD5
    if(is_first) {
      cd_destroy(cd_lv5);
    }
#endif
#if _CD4
      if(is_first) {
        cd_detect(cd_lv4);
        cd_complete(cd_lv4);
      }
#endif
    } // loop over neighbor boxes
#if _CD4
    if(is_first) {
      cd_destroy(cd_lv4);
    }
#endif
#if _CD3
    if(is_first) {
      //   if((iBox+1) % 16 == 0) {
      cd_detect(cd_lv3);
      cd_complete(cd_lv3);
      //  }
    }
#endif
  } // loop over local boxes in system
  ePot = ePot*4.0*epsilon;
  s->ePotential = ePot;

#if _CD3
  if(is_first) {
    cd_destroy(cd_lv3);
  }
#endif
  is_first = 1;
  return 0;
}
