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
/// where \f$\epsilon\f$ and \f$\sigma\f$ are the material parameters in the
/// potential.
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
///       &=& \sum_j 24 \frac{\epsilon}{r_{ij}} \left\{ 2
///       \left(\frac{\sigma}{r_{ij}}\right)^{12}
///               - \left(\frac{\sigma}{r_{ij}}\right)^6 \right\} \hat{r}_{ij}
/// \f}
///
/// where \f$\hat{r}_{ij}\f$ is a unit vector in the direction from atom
/// i to atom j.
///
///

#include "ljForce.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "CoMDTypes.h"
#include "constants.h"
#include "linkCells.h"
#include "memUtils.h"
#include "mytype.h"
#include "parallel.h"
#include "performanceTimers.h"

#if _CD3
#include "cd.h"
#include "cd_comd.h"
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

static int ljForce(SimFlat *s);
static void ljPrint(FILE *file, BasePotential *pot);

void ljDestroy(BasePotential **inppot) {
  if (!inppot)
    return;
  LjPotential *pot = (LjPotential *)(*inppot);
  if (!pot)
    return;
  comdFree(pot);
  *inppot = NULL;

  return;
}

/// Initialize an Lennard Jones potential for Copper.
BasePotential *initLjPot(void) {
  LjPotential *pot = (LjPotential *)comdMalloc(sizeof(LjPotential));
  pot->force = ljForce;
  pot->print = ljPrint;
  pot->destroy = ljDestroy;
  pot->sigma = 2.315;                    // Angstrom
  pot->epsilon = 0.167;                  // eV
  pot->mass = 63.55 * amuToInternalMass; // Atomic Mass Units (amu)

  pot->lat = 3.615;                // Equilibrium lattice const in Angs
  strcpy(pot->latticeType, "FCC"); // lattice type, i.e. FCC, BCC, etc.
  pot->cutoff = 2.5 * pot->sigma;  // Potential cutoff in Angs

  strcpy(pot->name, "Cu");
  pot->atomicNo = 29;

  return (BasePotential *)pot;
}

void ljPrint(FILE *file, BasePotential *pot) {
  LjPotential *ljPot = (LjPotential *)pot;
  fprintf(file, "  Potential type   : Lennard-Jones\n");
  fprintf(file, "  Species name     : %s\n", ljPot->name);
  fprintf(file, "  Atomic number    : %d\n", ljPot->atomicNo);
  fprintf(file, "  Mass             : " FMT1 " amu\n",
          ljPot->mass / amuToInternalMass); // print in amu
  fprintf(file, "  Lattice Type     : %s\n", ljPot->latticeType);
  fprintf(file, "  Lattice spacing  : " FMT1 " Angstroms\n", ljPot->lat);
  fprintf(file, "  Cutoff           : " FMT1 " Angstroms\n", ljPot->cutoff);
  fprintf(file, "  Epsilon          : " FMT1 " eV\n", ljPot->epsilon);
  fprintf(file, "  Sigma            : " FMT1 " Angstroms\n", ljPot->sigma);
}

// To prevent lv3_cd from being called during initialization
#if _CD3
int is_not_first = 0;
#endif
int ljForce(SimFlat *s) {
#if _CD3
  cd_handle_t *lv3_cd = NULL;
  // CD3_INTERVAL(default) = 5763 (10 iterations for 1024K atoms per rank)
  const int CD3_INTERVAL = s->preserveRateLevel3;
  unsigned int is_lv3_completed = 0;
  if (is_not_first) {
    lv3_cd = getleafcd();
  }
#endif

  LjPotential *pot = (LjPotential *)s->pot;
  real_t sigma = pot->sigma;
  real_t epsilon = pot->epsilon;
  real_t rCut = pot->cutoff;
  real_t rCut2 = rCut * rCut;

  // zero forces and energy
  real_t ePot = 0.0;
  s->ePotential = 0.0;
  int fSize = s->boxes->nTotalBoxes * MAXATOMS;
  for (int ii = 0; ii < fSize; ++ii) {
    zeroReal3(s->atoms->f[ii]);
    s->atoms->U[ii] = 0.;
  }

  real_t s6 = sigma * sigma * sigma * sigma * sigma * sigma;

  real_t rCut6 = s6 / (rCut2 * rCut2 * rCut2);
  real_t eShift = POT_SHIFT * rCut6 * (rCut6 - 1.0);

  int nbrBoxes[27];

  // loop over local boxes in system
  // Note that, for 1024K atoms per rank (nLocalBoxes = 57624), the default
  // CD3_INTERVAL (5763)  begins and completes 10 times.
  for (int iBox = 0; iBox < s->boxes->nLocalBoxes; iBox++) {
#if _CD3
    if (is_not_first) {
      if (iBox % CD3_INTERVAL == 0) {
        cd_begin(lv3_cd, "ljForce_loop");
        is_lv3_completed = 0;

        // Preserve: atoms->r and atoms-> gid via kRef

        // atoms->r and atoms->gid is going to read throughout the iterations.
        // Note that r and gid for entire atoms are preserved at the parents
        // at the leve 2 via copy and they are  not going to be updated here
        // while being read Therefore, let's preserve r and gid via reference
        // first.
        int computeForce_loop_pre_lv3_size =
            preserveAtoms(lv3_cd, kRef, s->atoms,
                          1,                     // is_gid
                          1,                     // is_r
                          0,                     // is_p
                          0,                     // is_f
                          0,                     // is_U
                          0,                     // is_iSpecies
                          0,                     // from
                          s->boxes->nLocalBoxes, // to
                          0, "Local");
#ifdef DO_PRV
        // Preserve: loop index (iBox)
        cd_preserve(lv3_cd, &iBox, sizeof(int), kCopy, "iBox", "iBox");
#endif
        computeForce_loop_pre_lv3_size += sizeof(int);

        // Preserve: atoms->U and atoms->f via kCopy
        // atoms->U and atoms->f are not only to be read but also to be written.
        // Therefore, they need to be preserved carefully.
        // By carefull, I mean the box (iBox) and its neighboring boxes (jBoxes)

        // Let's preserve atoms->U and atoms->F in iBox first
        // int to = iBox + CD3_INTERVAL;
        // if (to >= s->boxes->nLocalBoxes) to = s->boxes->nLocalBoxes -1;
        // FIXME: Now, this does preserve all boxes for some challnegs of
        //       preserving neighboring boxes for all iBoxes in the given
        //       interval.
        // FIXME: This may be acceptable for coarse enough interval but may add
        //       significant overhead for very fine interval (CD3_INTERVAL)
        computeForce_loop_pre_lv3_size =
            preserveAtoms(lv3_cd, kCopy, s->atoms,
                          0,                     // is_gid
                          0,                     // is_r
                          0,                     // is_p
                          1,                     // is_f
                          1,                     // is_U
                          0,                     // is_iSpecies
                          0,                     // from
                          s->boxes->nLocalBoxes, // to
                          0, "Local");
        // TODO: then preserve atoms-U and atoms->F in jBox (neighboring box)
      } // CD3_INTERVAL
    }
#endif
    // Added local Timer
    startTimer(ljForceTimer);

    int nIBox = s->boxes->nAtoms[iBox]; // #of atoms in ith box
    if (nIBox == 0) {
#if _CD3
      if (is_not_first) {
        if (iBox % CD3_INTERVAL == (CD3_INTERVAL - 1)) {
          is_lv3_completed = 1;
          cd_detect(lv3_cd);
          cd_complete(lv3_cd);
        }
      }
#endif
      continue;
    }
    // Note that neighbors of iBox also include the box itself as 13th element
    int nNbrBoxes = getNeighborBoxes(s->boxes, iBox, nbrBoxes);

    // TODO:  Too fine to optimize futher nested for-loop below fron here on
    //       However, this depends on input problem size as well and thus
    //       may need to revist the following for-loops in a certain setting
    //*****************************************************
    // loop over neighbors of iBox
    //*****************************************************
    for (int jTmp = 0; jTmp < nNbrBoxes; jTmp++) {
      int jBox = nbrBoxes[jTmp];

      assert(jBox >= 0);

      //#of atoms in jth box
      int nJBox = s->boxes->nAtoms[jBox];
      if (nJBox == 0) {
        continue;
      }
      //*****************************************************
      // loop over atoms in iBox
      //*****************************************************
      // O{iOff, iID} <- I{iOff, iBox, ii, atoms->gid}
      for (int iOff = iBox * MAXATOMS, ii = 0; ii < nIBox; ii++, iOff++) {
        int iId = s->atoms->gid[iOff];

        //*****************************************************
        // loop over atoms in jBox
        //*****************************************************
        for (int jOff = MAXATOMS * jBox, ij = 0; ij < nJBox; ij++, jOff++) {
          real_t dr[3];
          int jId = s->atoms->gid[jOff];
          if (jBox < s->boxes->nLocalBoxes && jId <= iId) {
            continue; // don't double count local-local pairs.
          }
          real_t r2 = 0.0;
          for (int m = 0; m < 3; m++) {
            // TODO: important pattern
            // with fixed iOff, get the distacne between iOff and all jOff
            dr[m] = s->atoms->r[iOff][m] - s->atoms->r[jOff][m];
            r2 += dr[m] * dr[m];
          }

          // If the atom is farther than cutoff, then do not process further.
          if (r2 > rCut2) {
            continue;
          }
          // Important note:
          // from this point on r actually refers to 1.0/r
          r2 = 1.0 / r2;
          real_t r6 = s6 * (r2 * r2 * r2);
          real_t eLocal = r6 * (r6 - 1.0) - eShift;
          s->atoms->U[iOff] += 0.5 * eLocal;
          s->atoms->U[jOff] += 0.5 * eLocal;

          // calculate energy contribution based on whether
          // the neighbor box is local or remote
          if (jBox < s->boxes->nLocalBoxes)
            ePot += eLocal;
          else
            ePot += 0.5 * eLocal;

          // different formulation to avoid sqrt computation
          real_t fr = -4.0 * epsilon * r6 * r2 * (12.0 * r6 - 6.0);
          for (int m = 0; m < 3; m++) {
            s->atoms->f[iOff][m] -= dr[m] * fr;
            s->atoms->f[jOff][m] += dr[m] * fr;
          }

        } // loop over atoms in jBox
      }   // loop over atoms in iBox
    }     // loop over neighbor boxes
    stopTimer(ljForceTimer);
#if _CD3
    if (is_not_first) {
      if (iBox % CD3_INTERVAL == (CD3_INTERVAL - 1)) {
        is_lv3_completed = 1;
        cd_detect(lv3_cd);
        cd_complete(lv3_cd);
      } // CD3_INTERVAL
    }
#endif
  } // loop over local boxes in system

  //*****************************************************
  // end of main computation (hot spot)
  //*****************************************************
#if _CD3
  if (is_not_first) {
    if (is_lv3_completed == 0) {
      cd_detect(lv3_cd);
      cd_complete(lv3_cd);
    }
  }
#endif
  ePot = ePot * 4.0 * epsilon;
  s->ePotential = ePot;
#if _CD3
  is_not_first = 1;
#endif
  return 0;
}
