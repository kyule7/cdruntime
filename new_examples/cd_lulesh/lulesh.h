#if !defined(USE_MPI)
# error "You should specify USE_MPI=0 or USE_MPI=1 on the compile line"
#endif


// OpenMP will be compiled in if this flag is set to 1 AND the compiler beging
// used supports it (i.e. the _OPENMP symbol is defined)
#define USE_OMP 1

#if USE_MPI
#include <mpi.h>

/*
   define one of these three symbols:

   SEDOV_SYNC_POS_VEL_NONE
   SEDOV_SYNC_POS_VEL_EARLY
   SEDOV_SYNC_POS_VEL_LATE
*/

#define SEDOV_SYNC_POS_VEL_EARLY 1
#endif

#include <math.h>
// Kyushick
//include <vector>
#define DO_CHECK 0
#define DOMAIN_INIT_NAME ""
#define LULESH_PRINT(...)
#include "cd_def.h"
#if _CD
#include "cd_containers.hpp"
#include "cd.h"
using namespace cd;
#define PRINT_COMPARE(...) printf(__VA_ARGS__)
#else
#include <assert.h>
#include <vector>
#endif
//**************************************************
// Allow flexibility for arithmetic representations 
//**************************************************

#define MAX(a, b) ( ((a) > (b)) ? (a) : (b))
#define CHECK_PRV_TYPE(X,Y) (((X) & (Y)) == (Y))

// Precision specification
typedef float        real4 ;
typedef double       real8 ;
typedef long double  real10 ;  // 10 bytes on x86

typedef int    Index_t ; // array subscript and loop index
typedef real8  Real_t ;  // floating point representation
typedef int    Int_t ;   // integer representation

// kyushick
extern   Int_t myRank ;
extern   Int_t numRanks ;
extern Index_t comBufSize;

enum { VolumeError = -1, QStopError = -2 } ;

inline real4  SQRT(real4  arg) { return sqrtf(arg) ; }
inline real8  SQRT(real8  arg) { return sqrt(arg) ; }
inline real10 SQRT(real10 arg) { return sqrtl(arg) ; }

inline real4  CBRT(real4  arg) { return cbrtf(arg) ; }
inline real8  CBRT(real8  arg) { return cbrt(arg) ; }
inline real10 CBRT(real10 arg) { return cbrtl(arg) ; }

inline real4  FABS(real4  arg) { return fabsf(arg) ; }
inline real8  FABS(real8  arg) { return fabs(arg) ; }
inline real10 FABS(real10 arg) { return fabsl(arg) ; }


// Stuff needed for boundary conditions
// 2 BCs on each of 6 hexahedral faces (12 bits)
#define XI_M        0x00007
#define XI_M_SYMM   0x00001
#define XI_M_FREE   0x00002
#define XI_M_COMM   0x00004

#define XI_P        0x00038
#define XI_P_SYMM   0x00008
#define XI_P_FREE   0x00010
#define XI_P_COMM   0x00020

#define ETA_M       0x001c0
#define ETA_M_SYMM  0x00040
#define ETA_M_FREE  0x00080
#define ETA_M_COMM  0x00100

#define ETA_P       0x00e00
#define ETA_P_SYMM  0x00200
#define ETA_P_FREE  0x00400
#define ETA_P_COMM  0x00800

#define ZETA_M      0x07000
#define ZETA_M_SYMM 0x01000
#define ZETA_M_FREE 0x02000
#define ZETA_M_COMM 0x04000

#define ZETA_P      0x38000
#define ZETA_P_SYMM 0x08000
#define ZETA_P_FREE 0x10000
#define ZETA_P_COMM 0x20000

// MPI Message Tags
#define MSG_COMM_SBN      1024
#define MSG_SYNC_POS_VEL  2048
#define MSG_MONOQ         3072

#define MAX_FIELDS_PER_MPI_COMM 6

// Assume 128 byte coherence
// Assume Real_t is an "integral power of 2" bytes wide
#define CACHE_COHERENCE_PAD_REAL (128 / sizeof(Real_t))

#define CACHE_ALIGN_REAL(n) \
   (((n) + (CACHE_COHERENCE_PAD_REAL - 1)) & ~(CACHE_COHERENCE_PAD_REAL-1))

//////////////////////////////////////////////////////
// Primary data structure
//////////////////////////////////////////////////////

/*
 * The implementation of the data abstraction used for lulesh
 * resides entirely in the Domain class below.  You can change
 * grouping and interleaving of fields here to maximize data layout
 * efficiency for your underlying architecture or compiler.
 *
 * For example, fields can be implemented as STL objects or
 * raw array pointers.  As another example, individual fields
 * m_x, m_y, m_z could be budled into
 *
 *    struct { Real_t x, y, z ; } *m_coord ;
 *
 * allowing accessor functions such as
 *
 *  "Real_t &x(Index_t idx) { return m_coord[idx].x ; }"
 *  "Real_t &y(Index_t idx) { return m_coord[idx].y ; }"
 *  "Real_t &z(Index_t idx) { return m_coord[idx].z ; }"
 */
extern bool r_regElemSize;  
extern bool r_regNumList ;   
extern bool r_regElemlist;  

struct Internal {
   // Region information
   Int_t    m_numReg ;
   Int_t    m_cost; //imbalance cost
   // Kyushick: This are just restoring pointers
   Index_t *m_regElemSize ;   // Size of region sets, list size of numReg
   Index_t *m_regNumList ;    // Region number per domain element
                              // list size of numElem
   Index_t **m_regElemlist ;  // region indexset, list size of numReg
                              // each elem is a pointer to an array of which
                              // size is m_regElemSize[i]

   // Variables to keep track of timestep, simulation time, and cycle
   Real_t  m_dtcourant ;         // courant constraint 
   Real_t  m_dthydro ;           // volume change constraint 
   Int_t   m_cycle ;             // iteration count for simulation 
   Real_t  m_dtfixed ;           // fixed time increment 
   Real_t  m_time ;              // current time 
   Real_t  m_deltatime ;         // variable time increment 
   Real_t  m_deltatimemultlb ;
   Real_t  m_deltatimemultub ;
   Real_t  m_dtmax ;             // maximum allowable time increment 
   Real_t  m_stoptime ;          // end time for simulation 

   Int_t   m_numRanks ;

   Index_t m_colLoc ;
   Index_t m_rowLoc ;
   Index_t m_planeLoc ;
   Index_t m_tp ;

   Index_t m_sizeX ;
   Index_t m_sizeY ;
   Index_t m_sizeZ ;
   Index_t m_numElem ;
   Index_t m_numNode ;

   Index_t m_maxPlaneSize ;
   Index_t m_maxEdgeSize ;

   // Kyushick: This are just restoring pointers
   // OMP hack 
   Index_t *m_nodeElemStart ;
   Index_t *m_nodeElemCornerList ;

   // Used in setup
   Index_t m_rowMin, m_rowMax;
   Index_t m_colMin, m_colMax;
   Index_t m_planeMin, m_planeMax;

   Internal(void) :
      m_numReg            (-1),
      m_cost              (-1),
      m_regElemSize       (NULL),
      m_regNumList        (NULL),
      m_regElemlist       (NULL),
      m_dtcourant         (-1),
      m_dthydro           (-1),
      m_cycle             (-1),
      m_dtfixed           (-1),
      m_time              (-1),
      m_deltatime         (-1),
      m_deltatimemultlb   (-1),
      m_deltatimemultub   (-1),
      m_dtmax             (-1),
      m_stoptime          (-1),
      m_numRanks          (-1),
      m_colLoc            (-1),
      m_rowLoc            (-1),
      m_planeLoc          (-1),
      m_tp                (-1),
      m_sizeX             (-1),
      m_sizeY             (-1),
      m_sizeZ             (-1),
      m_numElem           (-1),
      m_numNode           (-1),
      m_maxPlaneSize      (-1),
      m_maxEdgeSize       (-1),
      m_nodeElemStart     (NULL),
      m_nodeElemCornerList(NULL),
      m_rowMin            (-1),
      m_rowMax            (-1),
      m_colMin            (-1),
      m_colMax            (-1),
      m_planeMin          (-1),
      m_planeMax          (-1) {}
   Internal(const Internal &that) { copy(that); }
   Internal &operator=(const Internal &that) { copy(that); return *this; }
   void copy(const Internal &that) {
      m_numReg             = that.m_numReg;
      m_cost               = that.m_cost;
      m_regElemSize        = that.m_regElemSize;
      m_regNumList         = that.m_regNumList;
      m_regElemlist        = that.m_regElemlist;
      m_dtcourant          = that.m_dtcourant;
      m_dthydro            = that.m_dthydro;
      m_cycle              = that.m_cycle;
      m_dtfixed            = that.m_dtfixed;
      m_time               = that.m_time;
      m_deltatime          = that.m_deltatime;
      m_deltatimemultlb    = that.m_deltatimemultlb;
      m_deltatimemultub    = that.m_deltatimemultub;
      m_dtmax              = that.m_dtmax;
      m_stoptime           = that.m_stoptime;
      m_numRanks           = that.m_numRanks;
      m_colLoc             = that.m_colLoc;
      m_rowLoc             = that.m_rowLoc;
      m_planeLoc           = that.m_planeLoc;
      m_tp                 = that.m_tp;
      m_sizeX              = that.m_sizeX;
      m_sizeY              = that.m_sizeY;
      m_sizeZ              = that.m_sizeZ;
      m_numElem            = that.m_numElem;
      m_numNode            = that.m_numNode;
      m_maxPlaneSize       = that.m_maxPlaneSize;
      m_maxEdgeSize        = that.m_maxEdgeSize;
      m_nodeElemStart      = that.m_nodeElemStart;
      m_nodeElemCornerList = that.m_nodeElemCornerList;
      m_rowMin             = that.m_rowMin;
      m_rowMax             = that.m_rowMax;
      m_colMin             = that.m_colMin;
      m_colMax             = that.m_colMax;
      m_planeMin           = that.m_planeMin;
      m_planeMax           = that.m_planeMax;
   }
   ~Internal() {}

   bool CheckInternal(const Internal &that, const char *str="", bool checkall=false) {
      bool any_changed = false;
#if _CD 
      bool numReg_changed = false;
      if(m_numReg          != that.m_numReg) {
        PRINT_COMPARE("numReg ");
        m_numReg            = that.m_numReg;
        numReg_changed = true;
        any_changed |= true;
      }
      if(m_cost            != that.m_cost) {
        PRINT_COMPARE("cost ");
        any_changed |= true;
        m_cost              = that.m_cost;
      }
      if(m_dtcourant       != that.m_dtcourant) {
        PRINT_COMPARE("dtcourant ");
        any_changed |= true;
        m_dtcourant         = that.m_dtcourant;
      }
      if(m_dthydro         != that.m_dthydro) {
        PRINT_COMPARE("dthydro ");
        any_changed |= true;
        m_dthydro           = that.m_dthydro;
      }
      if(m_cycle           != that.m_cycle) {
        PRINT_COMPARE("cycle ");
        any_changed |= true;
        m_cycle             = that.m_cycle;
      }
      if(m_dtfixed         != that.m_dtfixed) {
        PRINT_COMPARE("dtfixed ");
        any_changed |= true;
        m_dtfixed           = that.m_dtfixed;
      }
      if(m_time            != that.m_time) {
        PRINT_COMPARE("time ");
        any_changed |= true;
        m_time              = that.m_time;
      }
      if(m_deltatime       != that.m_deltatime) {
        PRINT_COMPARE("deltatime ");
        any_changed |= true;
        m_deltatime         = that.m_deltatime;
      }
      if(m_deltatimemultlb != that.m_deltatimemultlb) {
        PRINT_COMPARE("deltatimemultlb ");
        any_changed |= true;
        m_deltatimemultlb   = that.m_deltatimemultlb;
      }
      if(m_deltatimemultub != that.m_deltatimemultub) {
        PRINT_COMPARE("deltatimemultub ");
        any_changed |= true;
        m_deltatimemultub   = that.m_deltatimemultub;
      }
      if(m_dtmax           != that.m_dtmax) {
        PRINT_COMPARE("dtmax ");
        any_changed |= true;
        m_dtmax             = that.m_dtmax;
      }
      if(m_stoptime        != that.m_stoptime) {
        PRINT_COMPARE("stoptime ");
        any_changed |= true;
        m_stoptime          = that.m_stoptime;
      }
      if(m_numRanks        != that.m_numRanks) {
        PRINT_COMPARE("numRanks ");
        any_changed |= true;
        m_numRanks          = that.m_numRanks;
      }
      if(m_colLoc          != that.m_colLoc) {
        PRINT_COMPARE("colLoc ");
        any_changed |= true;
        m_colLoc            = that.m_colLoc;
      }
      if(m_rowLoc          != that.m_rowLoc) {
        PRINT_COMPARE("rowLoc ");
        any_changed |= true;
        m_rowLoc            = that.m_rowLoc;
      }
      if(m_planeLoc        != that.m_planeLoc) {
        PRINT_COMPARE("planeLoc ");
        any_changed |= true;
        m_planeLoc          = that.m_planeLoc;
      }
      if(m_tp              != that.m_tp) {
        PRINT_COMPARE("tp ");
        any_changed |= true;
        m_tp                = that.m_tp;
      }
      if(m_sizeX           != that.m_sizeX) {
        PRINT_COMPARE("sizeX ");
        any_changed |= true;
        m_sizeX             = that.m_sizeX;
      }
      if(m_sizeY           != that.m_sizeY) {
        PRINT_COMPARE("sizeY ");
        any_changed |= true;
        m_sizeY             = that.m_sizeY;
      }
      if(m_sizeZ           != that.m_sizeZ) {
        PRINT_COMPARE("sizeZ ");
        any_changed |= true;
        m_sizeZ             = that.m_sizeZ;
      }
      if(m_numElem         != that.m_numElem) {
        PRINT_COMPARE("numElem ");
        any_changed |= true;
        m_numElem           = that.m_numElem;
      }
      if(m_numNode         != that.m_numNode) {
        PRINT_COMPARE("numNode ");
        any_changed |= true;
        m_numNode           = that.m_numNode;
      }
      if(m_maxPlaneSize    != that.m_maxPlaneSize) {
        PRINT_COMPARE("maxPlaneSize ");
        any_changed |= true;
        m_maxPlaneSize      = that.m_maxPlaneSize;
      }
      if(m_maxEdgeSize     != that.m_maxEdgeSize) {
        PRINT_COMPARE("maxEdgeSize ");
        any_changed |= true;
        m_maxEdgeSize       = that.m_maxEdgeSize;
      }
      if(m_rowMin          != that.m_rowMin) {
        PRINT_COMPARE("rowMin ");
        any_changed |= true;
        m_rowMin            = that.m_rowMin;
      }
      if(m_rowMax          != that.m_rowMax) {
        PRINT_COMPARE("rowMax ");
        any_changed |= true;
        m_rowMax            = that.m_rowMax;
      }
      if(m_colMin          != that.m_colMin) {
        PRINT_COMPARE("colMin ");
        any_changed |= true;
        m_colMin            = that.m_colMin;
      }
      if(m_colMax          != that.m_colMax) {
        PRINT_COMPARE("colMax ");
        any_changed |= true;
        m_colMax            = that.m_colMax;
      }
      if(m_planeMin        != that.m_planeMin) {
        PRINT_COMPARE("planeMin ");
        any_changed |= true;
        m_planeMin          = that.m_planeMin;
      }
      if(m_planeMax        != that.m_planeMax) {
        PRINT_COMPARE("planeMax ");
        any_changed |= true;
        m_planeMax          = that.m_planeMax;
      }
      if(checkall) {
        if(myRank == 0 && (r_regElemSize || r_regNumList|| r_regElemlist)) {
          printf("Read %s %s %s %s\n", str, 
                (r_regElemSize)? "regElemSize": "", 
                (r_regNumList )? "regNumList" : "", 
                (r_regElemlist)? "regElemlist" : "" );
        }
        float diff = cd::Compare(     m_regElemSize,      m_numReg,
                that.m_regElemSize, that.m_numReg, sizeof(Index_t),
                str, "regElemSize"
                );
        bool regElemSize_changed = (diff > 0.0000)? true : false;
        diff = cd::Compare(     m_regNumList,      m_numElem,
                that.m_regNumList, that.m_numElem, sizeof(Index_t),
                str, "regNumList"
                );
        bool regNumList_changed  = (diff > 0.0000)? true : false;
        bool regElemList_changed = false;
        if(numReg_changed) {
          if(m_regElemlist != NULL) 
            m_regElemlist = (Index_t **)realloc(m_regElemlist, m_numReg * sizeof(Index_t **));
          else 
            m_regElemlist = (Index_t **)malloc(m_numReg * sizeof(Index_t **));
        }
        for(int i=0; i<m_numReg; i++) {
          char elemID[32];
          sprintf(elemID, "regElemlist_%d", i);
          float diff = cd::Compare(     m_regElemlist[i],      m_regElemSize[i],
                  that.m_regElemlist[i], that.m_regElemSize[i], sizeof(Index_t),
                  str, elemID
                  );
          regElemList_changed  = (diff > 0.0000)? true : false;
  
        }
        r_regElemSize = false;  
        r_regNumList  = false;   
        r_regElemlist = false;  
        any_changed |= (regElemSize_changed | regNumList_changed | regElemList_changed);
      }
//      m_nodeElemStart      = that.m_nodeElemStart;
//      m_nodeElemCornerList = that.m_nodeElemCornerList;
//      Add((char *)commDataSend, (("COMMBUFSEND"), comBufSize * sizeof(Real_t), 0, (char *)commDataSend)); 
//      Add((char *)commDataRecv, (("COMMBUFRECV"), comBufSize * sizeof(Real_t), 0, (char *)commDataRecv)); 
#endif // _CD ends
      return (any_changed);
   }
   void PrintInternal(void) {
     if(myRank != 1) return;
//   Int_t    m_numReg ;
//   Int_t    m_cost; //imbalance cost
//   Index_t *m_regElemSize ;   // Size of region sets, list size of numReg
//   Index_t *m_regNumList ;    // Region number per domain element
//   Index_t **m_regElemlist ;  // region indexset, list size of numReg
//   Real_t  m_dtcourant ;         // courant constraint 
//   Real_t  m_dthydro ;           // volume change constraint 
//   Int_t   m_cycle ;             // iteration count for simulation 
//   Real_t  m_dtfixed ;           // fixed time increment 
//   Real_t  m_time ;              // current time 
//   Real_t  m_deltatime ;         // variable time increment 
//   Real_t  m_deltatimemultlb ;
//   Real_t  m_deltatimemultub ;
//   Real_t  m_dtmax ;             // maximum allowable time increment 
//   Real_t  m_stoptime ;          // end time for simulation 
//   Int_t   m_numRanks ;
//   Index_t m_colLoc ;
//   Index_t m_rowLoc ;
//   Index_t m_planeLoc ;
//   Index_t m_tp ;
//   Index_t m_sizeX ;
//   Index_t m_sizeY ;
//   Index_t m_sizeZ ;
//   Index_t m_numElem ;
//   Index_t m_numNode ;
//   Index_t m_maxPlaneSize ;
//   Index_t m_maxEdgeSize ;
//   Index_t m_rowMin, m_rowMax;
//   Index_t m_colMin, m_colMax;
//   Index_t m_planeMin, m_planeMax;
   printf("cycle                :%d\n", m_cycle               );
   printf("numRanks             :%d\n", m_numRanks            );
   printf("numReg               :%d\n", m_numReg              );
   printf("numElem              :%d\n", m_numElem             );
   printf("numNode              :%d\n", m_numNode             );
   printf("cost                 :%d\n", m_cost                );
   printf("dtfixed              :%f\n", m_dtfixed             );
   printf("time                 :%f\n", m_time                );
   printf("deltatime            :%f\n", m_deltatime           );
   printf("deltatimemultlb      :%f\n", m_deltatimemultlb     );
   printf("deltatimemultub      :%f\n", m_deltatimemultub     );
   printf("dtcourant            :%e\n", m_dtcourant           );
   printf("dthydro              :%e\n", m_dthydro             );
   printf("dtmax                :%f\n", m_dtmax               );
   printf("stoptime             :%f\n", m_stoptime            );
   printf("Loc(col,row,plane,tp):(%d,%d,%d,%d)\n", m_colLoc, m_rowLoc, m_planeLoc, m_tp);
   printf("size(X,Y,Z)          :(%d,%d,%d)\n", m_sizeX, m_sizeY, m_sizeZ);
   printf("max (Plane,Edge)     :(%d,%d)\n", m_maxPlaneSize, m_maxEdgeSize);
   printf("row Min %d~%d Max\n",       m_rowMin, m_rowMax    );
   printf("col Min %d~%d Max\n",       m_colMin, m_colMax    );
   printf("planeMin %d~%d m_planeMax\n",m_planeMin, m_planeMax);
   printf("regElemSize        :%p\n", m_regElemSize);
   printf("regNumList         :%p\n", m_regNumList);
   printf("regElemlist        :%p\n", m_regElemlist);
   if(m_regElemSize != NULL)
     printf("regElemSize        :%d %d %d %d\n", m_regElemSize[0], m_regElemSize[4], m_regElemSize[8], m_regElemSize[12]);
   if(m_regNumList != NULL)
     printf("regNumList         :%d %d %d %d\n",  m_regNumList[0],  m_regNumList[4],  m_regNumList[8],  m_regNumList[12]);
   if(m_regElemlist != NULL) 
     printf("regElemlist        :%d %d %d %d\n", m_regElemlist[0][0], m_regElemlist[2][0],  m_regElemlist[4][0], m_regElemlist[8][0]);

   }
};

#if _CD
class Domain : public Internal, public cd::PackerSerializable {
#else
class Domain : public Internal {
#endif
   public:
#if _CD
   Internal *preserved_;
   std::string name_;
   void PrintDomain(void) {
     //if(myRank != 1) 
       return;
     if(preserved_ == NULL) 
       PrintInternal();
     else {
       printf("cycle                :%d==%d\n", m_cycle          , preserved_->m_cycle              );
       printf("numRanks             :%d==%d\n", m_numRanks       , preserved_->m_numRanks           );
       printf("numReg               :%d==%d\n", m_numReg         , preserved_->m_numReg             );
       printf("numElem              :%d==%d\n", m_numElem        , preserved_->m_numElem            );
       printf("numNode              :%d==%d\n", m_numNode        , preserved_->m_numNode            );
       printf("cost                 :%d==%d\n", m_cost           , preserved_->m_cost               );
       printf("dtfixed              :%f==%f\n", m_dtfixed        , preserved_->m_dtfixed            );
       printf("time                 :%f==%f\n", m_time           , preserved_->m_time               );
       printf("deltatime            :%f==%f\n", m_deltatime      , preserved_->m_deltatime          );
       printf("deltatimemultlb      :%f==%f\n", m_deltatimemultlb, preserved_->m_deltatimemultlb    );
       printf("deltatimemultub      :%f==%f\n", m_deltatimemultub, preserved_->m_deltatimemultub    );
       printf("dtcourant            :%e==%e\n", m_dtcourant      , preserved_->m_dtcourant          );
       printf("dthydro              :%e==%e\n", m_dthydro        , preserved_->m_dthydro            );
       printf("dtmax                :%f==%f\n", m_dtmax          , preserved_->m_dtmax              );
       printf("stoptime             :%f==%f\n", m_stoptime       , preserved_->m_stoptime           );
       printf("Loc(col,row,plane,tp):(%d,%d,%d,%d)==(%d,%d,%d,%d)\n", m_colLoc, m_rowLoc, m_planeLoc, m_tp, 
           preserved_->m_colLoc, preserved_->m_rowLoc, preserved_->m_planeLoc, preserved_->m_tp);
       printf("size(X,Y,Z)          :(%d,%d,%d)==(%d,%d,%d)\n",    m_sizeX, m_sizeY, m_sizeZ,
                                                      preserved_->m_sizeX, preserved_->m_sizeY, preserved_->m_sizeZ);
       printf("max (Plane,Edge)     :(%d,%d)==(%d,%d)\n", m_maxPlaneSize, m_maxEdgeSize, 
           preserved_->m_maxPlaneSize, preserved_->m_maxEdgeSize);
       printf("row Min %d~%d Max ==(%d~%d)\n",        m_rowMin, m_rowMax    , preserved_->m_rowMin,   preserved_->m_rowMax   );
       printf("col Min %d~%d Max ==(%d~%d)\n",        m_colMin, m_colMax    , preserved_->m_colMin,   preserved_->m_colMax   );
       printf("planeMin %d~%d m_planeMax == (%d~%d)\n",m_planeMin, m_planeMax, preserved_->m_planeMin, preserved_->m_planeMax );
       printf("regElemSize        :%p==%p\n",   m_regElemSize, preserved_->m_regElemSize);
       printf("regNumList         :%p==%p\n",   m_regNumList,  preserved_->m_regNumList);
       printf("regElemlist        :%p==%p\n",   m_regElemlist, preserved_->m_regElemlist);
//       printf("numReg                :%d==%d\n", m_numReg            , preserved_->m_numReg          );
//       printf("cost                  :%d==%d\n", m_cost              , preserved_->m_cost            );
//       printf("m_dtcourant           :%f==%f\n", m_dtcourant         , preserved_->m_dtcourant       );
//       printf("m_dthydro             :%f==%f\n", m_dthydro           , preserved_->m_dthydro         );
//       printf("m_cycle               :%d==%d\n", m_cycle             , preserved_->m_cycle           );
//       printf("m_dtfixed             :%f==%f\n", m_dtfixed           , preserved_->m_dtfixed         );
//       printf("m_time                :%f==%f\n", m_time              , preserved_->m_time            );
//       printf("m_deltatime           :%f==%f\n", m_deltatime         , preserved_->m_deltatime       );
//       printf("m_deltatimemultlb     :%f==%f\n", m_deltatimemultlb   , preserved_->m_deltatimemultlb );
//       printf("m_deltatimemultub     :%f==%f\n", m_deltatimemultub   , preserved_->m_deltatimemultub );
//       printf("m_dtmax               :%f==%f\n", m_dtmax             , preserved_->m_dtmax           );
//       printf("m_stoptime            :%f==%f\n", m_stoptime          , preserved_->m_stoptime        );
//       printf("m_numRanks            :%d==%d\n", m_numRanks          , preserved_->m_numRanks        );
//       printf("m_colLoc              :%d==%d\n", m_colLoc            , preserved_->m_colLoc          );
//       printf("m_rowLoc              :%d==%d\n", m_rowLoc            , preserved_->m_rowLoc          );
//       printf("m_planeLoc            :%d==%d\n", m_planeLoc          , preserved_->m_planeLoc        );
//       printf("m_tp                  :%d==%d\n", m_tp                , preserved_->m_tp              );
//       printf("m_sizeX               :%d==%d\n", m_sizeX             , preserved_->m_sizeX           );
//       printf("m_sizeY               :%d==%d\n", m_sizeY             , preserved_->m_sizeY           );
//       printf("m_sizeZ               :%d==%d\n", m_sizeZ             , preserved_->m_sizeZ           );
//       printf("m_numElem             :%d==%d\n", m_numElem           , preserved_->m_numElem         );
//       printf("m_numNode             :%d==%d\n", m_numNode           , preserved_->m_numNode         );
//       printf("m_maxPlaneSize        :%d==%d\n", m_maxPlaneSize      , preserved_->m_maxPlaneSize    );
//       printf("m_maxEdgeSize         :%d==%d\n", m_maxEdgeSize       , preserved_->m_maxEdgeSize     );
//       printf("m_rowMin %d~%d m_rowMax\n",m_rowMin, m_rowMax    );
//       printf("m_colMin %d~%d m_colMax\n",m_colMin, m_colMax    );
//       printf("m_planeMin %d~%d m_planeMax\n",m_planeMin, m_planeMax);
//       printf("regElemSize        :%p\n", m_regElemSize);
//       printf("regNumList         :%p\n", m_regNumList);
//       printf("regElemlist        :%p\n", m_regElemlist);
       if(m_regElemSize != NULL)
         printf("regElemSize        :%d %d %d %d\n", m_regElemSize[0], m_regElemSize[4], m_regElemSize[8], m_regElemSize[12]);
       if(m_regNumList != NULL)
         printf("regNumList         :%d %d %d %d\n",  m_regNumList[0],  m_regNumList[4],  m_regNumList[8],  m_regNumList[12]);
       if(m_regElemlist != NULL) 
         printf("regElemlist        :%d %d %d %d\n", m_regElemlist[0][0], m_regElemlist[2][0],  m_regElemlist[4][0], m_regElemlist[8][0]);
     }
     assert(m_cycle);
   }
#endif
   // Constructor
   Domain(Int_t numRanks, Index_t colLoc,
          Index_t rowLoc, Index_t planeLoc,
          Index_t nx, Int_t tp, Int_t nr, Int_t balance, Int_t cost);

   //
   // ALLOCATION
   //

   void AllocateNodePersistent(Int_t numNode) // Node-centered
   {
      m_x.resize(numNode);  // coordinates
      m_y.resize(numNode);
      m_z.resize(numNode);

      m_xd.resize(numNode); // velocities
      m_yd.resize(numNode);
      m_zd.resize(numNode);

      m_xdd.resize(numNode); // accelerations
      m_ydd.resize(numNode);
      m_zdd.resize(numNode);

      m_fx.resize(numNode);  // forces
      m_fy.resize(numNode);
      m_fz.resize(numNode);

      m_nodalMass.resize(numNode);  // mass
   }

   void AllocateElemPersistent(Int_t numElem) // Elem-centered
   {
      m_nodelist.resize(8*numElem);

      // elem connectivities through face
      m_lxim.resize(numElem);
      m_lxip.resize(numElem);
      m_letam.resize(numElem);
      m_letap.resize(numElem);
      m_lzetam.resize(numElem);
      m_lzetap.resize(numElem);

      m_elemBC.resize(numElem);

      m_e.resize(numElem);
      m_p.resize(numElem);

      m_q.resize(numElem);
      m_ql.resize(numElem);
      m_qq.resize(numElem);

      m_v.resize(numElem);

      m_volo.resize(numElem);
      m_delv.resize(numElem);
      m_vdov.resize(numElem);

      m_arealg.resize(numElem);

      m_ss.resize(numElem);

      m_elemMass.resize(numElem);
   }

   void AllocateGradients(Int_t numElem, Int_t allElem)
   {
      // Position gradients
      m_delx_xi.resize(numElem) ;
      m_delx_eta.resize(numElem) ;
      m_delx_zeta.resize(numElem) ;

      // Velocity gradients
      m_delv_xi.resize(allElem) ;
      m_delv_eta.resize(allElem);
      m_delv_zeta.resize(allElem) ;
   }

   void DeallocateGradients()
   {
      m_delx_zeta.clear() ;
      m_delx_eta.clear() ;
      m_delx_xi.clear() ;

      m_delv_zeta.clear() ;
      m_delv_eta.clear() ;
      m_delv_xi.clear() ;
   }

   void AllocateStrains(Int_t numElem)
   {
      m_dxx.resize(numElem) ;
      m_dyy.resize(numElem) ;
      m_dzz.resize(numElem) ;
   }

   void DeallocateStrains()
   {
      m_dzz.clear() ;
      m_dyy.clear() ;
      m_dxx.clear() ;
   }
   
   //
   // ACCESSORS
   //

   // Node-centered
#if _CD
   // Nodal coordinates
   Real_t& x(Index_t idx)    { m_x.SetRead(); return m_x[idx] ; }
   Real_t& y(Index_t idx)    { m_y.SetRead(); return m_y[idx] ; }
   Real_t& z(Index_t idx)    { m_z.SetRead(); return m_z[idx] ; }

   // Nodal velocities
   Real_t& xd(Index_t idx)   { m_xd.SetRead(); return m_xd[idx] ; }
   Real_t& yd(Index_t idx)   { m_yd.SetRead(); return m_yd[idx] ; }
   Real_t& zd(Index_t idx)   { m_zd.SetRead(); return m_zd[idx] ; }

   // Nodal accelerations
   Real_t& xdd(Index_t idx)  { m_xdd.SetRead(); return m_xdd[idx] ; }
   Real_t& ydd(Index_t idx)  { m_ydd.SetRead(); return m_ydd[idx] ; }
   Real_t& zdd(Index_t idx)  { m_zdd.SetRead(); return m_zdd[idx] ; }

   // Nodal forces
   Real_t& fx(Index_t idx)   { m_fx.SetRead();  return m_fx[idx] ; }
   Real_t& fy(Index_t idx)   { m_fy.SetRead();  return m_fy[idx] ; }
   Real_t& fz(Index_t idx)   { m_fz.SetRead();  return m_fz[idx] ; }

   // Nodal mass
   Real_t& nodalMass(Index_t idx) { return m_nodalMass[idx] ; }

   // Nodes on symmertry planes
   Index_t symmX(Index_t idx) { m_symmX.SetRead(); return m_symmX[idx] ; }
   Index_t symmY(Index_t idx) { m_symmY.SetRead(); return m_symmY[idx] ; }
   Index_t symmZ(Index_t idx) { m_symmZ.SetRead(); return m_symmZ[idx] ; }
   bool symmXempty()          { m_symmX.SetRead(); return m_symmX.empty(); }
   bool symmYempty()          { m_symmY.SetRead(); return m_symmY.empty(); }
   bool symmZempty()          { m_symmZ.SetRead(); return m_symmZ.empty(); }
#else
   Real_t& x(Index_t idx)    { return m_x[idx] ; }
   Real_t& y(Index_t idx)    { return m_y[idx] ; }
   Real_t& z(Index_t idx)    { return m_z[idx] ; }

   // Nodal velocities
   Real_t& xd(Index_t idx)   {  return m_xd[idx] ; }
   Real_t& yd(Index_t idx)   {  return m_yd[idx] ; }
   Real_t& zd(Index_t idx)   {  return m_zd[idx] ; }

   // Nodal accelerations
   Real_t& xdd(Index_t idx)  {  return m_xdd[idx] ; }
   Real_t& ydd(Index_t idx)  {  return m_ydd[idx] ; }
   Real_t& zdd(Index_t idx)  {  return m_zdd[idx] ; }

   // Nodal forces
   Real_t& fx(Index_t idx)   {   return m_fx[idx] ; }
   Real_t& fy(Index_t idx)   {   return m_fy[idx] ; }
   Real_t& fz(Index_t idx)   {   return m_fz[idx] ; }

   // Nodal mass
   Real_t& nodalMass(Index_t idx) { return m_nodalMass[idx] ; }

   // Nodes on symmertry planes
   Index_t symmX(Index_t idx) { return m_symmX[idx] ; }
   Index_t symmY(Index_t idx) { return m_symmY[idx] ; }
   Index_t symmZ(Index_t idx) { return m_symmZ[idx] ; }
   bool symmXempty()          { return m_symmX.empty(); }
   bool symmYempty()          { return m_symmY.empty(); }
   bool symmZempty()          { return m_symmZ.empty(); }
#endif
   //
   // Element-centered
   //
   Index_t&  regElemSize(Index_t idx) { r_regElemSize = true;  return m_regElemSize[idx] ; }
   Index_t&  regNumList(Index_t idx)  { r_regNumList = true;   return m_regNumList[idx] ; }
   Index_t*  regNumList()             { r_regNumList = true;   return &m_regNumList[0] ; }
   Index_t*  regElemlist(Int_t r)     { r_regElemlist = true;  return m_regElemlist[r] ; }
   Index_t&  regElemlist(Int_t r, Index_t idx) { r_regElemlist = true; return m_regElemlist[r][idx] ; }

   Index_t prv_idx;
#if _CD
   Index_t*  nodelist(Index_t idx)    { 
     m_nodelist.SetRead();
     static uint64_t count = 0;
     prv_idx = idx;
     if(count++ % 100000 == 0) {
#if 0 
     const Index_t base = Index_t(8)*idx;
     LULESH_PRINT("[%d] nodelist size:%zu/%zu (%d), %d %d %d %d %d %d %d %d\n", myRank, m_nodelist.size(), m_nodelist.capacity(), numElem(),
                                                            m_nodelist[base],
                                                            m_nodelist[base+1],
                                                            m_nodelist[base+2],
                                                            m_nodelist[base+3],
                                                            m_nodelist[base+4],
                                                            m_nodelist[base+5],
                                                            m_nodelist[base+6],
                                                            m_nodelist[base+7]
                                                            ); 
#endif
     }
     return &m_nodelist[Index_t(8)*idx] ; 
   }
#else
   Index_t*  nodelist(Index_t idx)    { 
     return &m_nodelist[Index_t(8)*idx] ; 
   }
#endif
   Index_t*  Mynodelist()    { 
#if _CD
      m_nodelist.SetRead();
#endif
      return &m_nodelist[Index_t(8)*prv_idx] ; 
   }
#if _CD
   // elem connectivities through face
   Index_t&  lxim(Index_t idx) { m_lxim.SetRead();   return m_lxim[idx] ; }
   Index_t&  lxip(Index_t idx) { m_lxip.SetRead();   return m_lxip[idx] ; }
   Index_t&  letam(Index_t idx) { m_letam.SetRead(); return m_letam[idx] ; }
   Index_t&  letap(Index_t idx) { m_letap.SetRead(); return m_letap[idx] ; }
   Index_t&  lzetam(Index_t idx) { m_lzetam.SetRead(); return m_lzetam[idx] ; }
   Index_t&  lzetap(Index_t idx) { m_lzetap.SetRead(); return m_lzetap[idx] ; }

   // elem face symm/free-surface flag
   Int_t&  elemBC(Index_t idx) { m_elemBC.SetRead(); return m_elemBC[idx] ; }
                                         
   // Principal strains - tempora        ry
   Real_t& dxx(Index_t idx)    { m_dxx.SetRead(); return m_dxx[idx] ; }
   Real_t& dyy(Index_t idx)    { m_dyy.SetRead(); return m_dyy[idx] ; }
   Real_t& dzz(Index_t idx)    { m_dzz.SetRead(); return m_dzz[idx] ; }

   // Velocity gradient - temporary
   Real_t& delv_xi(Index_t idx)    { m_delv_xi.SetRead();   return m_delv_xi[idx] ; }
   Real_t& delv_eta(Index_t idx)   { m_delv_eta.SetRead();  return m_delv_eta[idx] ; }
   Real_t& delv_zeta(Index_t idx)  { m_delv_zeta.SetRead(); return m_delv_zeta[idx] ; }

   // Position gradient - temporary
   Real_t& delx_xi(Index_t idx)    { m_delx_xi.SetRead();   return m_delx_xi[idx] ; }
   Real_t& delx_eta(Index_t idx)   { m_delx_eta.SetRead();  return m_delx_eta[idx] ; }
   Real_t& delx_zeta(Index_t idx)  { m_delx_zeta.SetRead(); return m_delx_zeta[idx] ; }

   // Energy
   Real_t& e(Index_t idx)          { m_e.SetRead();  return m_e[idx] ; }
                                         
   // Pressure                           
   Real_t& p(Index_t idx)          { m_p.SetRead();  return m_p[idx] ; }
                                         
   // Artificial viscosity               
   Real_t& q(Index_t idx)          { m_q.SetRead();  return m_q[idx] ; }
                                         
   // Linear term for q                  
   Real_t& ql(Index_t idx)         { m_ql.SetRead(); return m_ql[idx] ; }
   // Quadratic term for q               
   Real_t& qq(Index_t idx)         { m_qq.SetRead(); return m_qq[idx] ; }
                                         
   // Relative volume                    
   Real_t& v(Index_t idx)          { m_v.SetRead();  return m_v[idx] ; }
   Real_t& delv(Index_t idx)       { m_delv.SetRead();  return m_delv[idx] ; }
                                           
   // Reference volume                     
   Real_t& volo(Index_t idx)       { m_volo.SetRead();  return m_volo[idx] ; }
                                           
   // volume derivative over volume        
   Real_t& vdov(Index_t idx)       { m_vdov.SetRead();  return m_vdov[idx] ; }
                                           
   // Element characteristic length        
   Real_t& arealg(Index_t idx)     { m_arealg.SetRead();return m_arealg[idx] ; }

   // Sound speed
   Real_t& ss(Index_t idx)         { m_ss.SetRead(); return m_ss[idx] ; }

   // Element mass
   Real_t& elemMass(Index_t idx)  { m_elemMass.SetRead(); return m_elemMass[idx] ; }
#else
   // elem connectivities through face
   Index_t&  lxim(Index_t idx)   {  return m_lxim[idx] ; }
   Index_t&  lxip(Index_t idx)   {  return m_lxip[idx] ; }
   Index_t&  letam(Index_t idx)  { return m_letam[idx] ; }
   Index_t&  letap(Index_t idx)  { return m_letap[idx] ; }
   Index_t&  lzetam(Index_t idx) {  return m_lzetam[idx] ; }
   Index_t&  lzetap(Index_t idx) {  return m_lzetap[idx] ; }

   // elem face symm/free-surface flag
   Int_t&  elemBC(Index_t idx)   { return m_elemBC[idx] ; }
                                         
   // Principal strains - tempora        ry
   Real_t& dxx(Index_t idx)    { return m_dxx[idx] ; }
   Real_t& dyy(Index_t idx)    { return m_dyy[idx] ; }
   Real_t& dzz(Index_t idx)    { return m_dzz[idx] ; }

   // Velocity gradient - temporary
   Real_t& delv_xi(Index_t idx)    { return m_delv_xi[idx] ; }
   Real_t& delv_eta(Index_t idx)   { return m_delv_eta[idx] ; }
   Real_t& delv_zeta(Index_t idx)  { return m_delv_zeta[idx] ; }

   // Position gradient - temporary
   Real_t& delx_xi(Index_t idx)    { return m_delx_xi[idx] ; }
   Real_t& delx_eta(Index_t idx)   { return m_delx_eta[idx] ; }
   Real_t& delx_zeta(Index_t idx)  { return m_delx_zeta[idx] ; }

   // Energy
   Real_t& e(Index_t idx)          {  return m_e[idx] ; }
                                         
   // Pressure                           
   Real_t& p(Index_t idx)          {  return m_p[idx] ; }
                                     
   // Artificial viscosity           
   Real_t& q(Index_t idx)          {  return m_q[idx] ; }
                                     
   // Linear term for q              
   Real_t& ql(Index_t idx)         {  return m_ql[idx] ; }
   // Quadratic term for q           
   Real_t& qq(Index_t idx)         {  return m_qq[idx] ; }
                                     
   // Relative volume                
   Real_t& v(Index_t idx)          {  return m_v[idx] ; }
   Real_t& delv(Index_t idx)       {   return m_delv[idx] ; }
                                     
   // Reference volume               
   Real_t& volo(Index_t idx)       {   return m_volo[idx] ; }
                                     
   // volume derivative over volume  
   Real_t& vdov(Index_t idx)       {   return m_vdov[idx] ; }
                                     
   // Element characteristic length  
   Real_t& arealg(Index_t idx)     { return m_arealg[idx] ; }

   // Sound speed
   Real_t& ss(Index_t idx)         {  return m_ss[idx] ; }

   // Element mass
   Real_t& elemMass(Index_t idx)  {  return m_elemMass[idx] ; }

#endif
   Index_t nodeElemCount(Index_t idx)
   { return m_nodeElemStart[idx+1] - m_nodeElemStart[idx] ; }

   Index_t *nodeElemCornerList(Index_t idx)
   { return &m_nodeElemCornerList[m_nodeElemStart[idx]] ; }

   // Parameters 

   // Cutoffs
   Real_t u_cut() const               { return m_u_cut ; }
   Real_t e_cut() const               { return m_e_cut ; }
   Real_t p_cut() const               { return m_p_cut ; }
   Real_t q_cut() const               { return m_q_cut ; }
   Real_t v_cut() const               { return m_v_cut ; }

   // Other constants (usually are settable via input file in real codes)
   Real_t hgcoef() const              { return m_hgcoef ; }
   Real_t qstop() const               { return m_qstop ; }
   Real_t monoq_max_slope() const     { return m_monoq_max_slope ; }
   Real_t monoq_limiter_mult() const  { return m_monoq_limiter_mult ; }
   Real_t ss4o3() const               { return m_ss4o3 ; }
   Real_t qlc_monoq() const           { return m_qlc_monoq ; }
   Real_t qqc_monoq() const           { return m_qqc_monoq ; }
   Real_t qqc() const                 { return m_qqc ; }

   Real_t eosvmax() const             { return m_eosvmax ; }
   Real_t eosvmin() const             { return m_eosvmin ; }
   Real_t pmin() const                { return m_pmin ; }
   Real_t emin() const                { return m_emin ; }
   Real_t dvovmax() const             { return m_dvovmax ; }
   Real_t refdens() const             { return m_refdens ; }

   // Timestep controls, etc...
   Real_t& time()                 { return m_time ; }
   Real_t& deltatime()            { return m_deltatime ; }
   Real_t& deltatimemultlb()      { return m_deltatimemultlb ; }
   Real_t& deltatimemultub()      { return m_deltatimemultub ; }
   Real_t& stoptime()             { return m_stoptime ; }
   Real_t& dtcourant()            { return m_dtcourant ; }
   Real_t& dthydro()              { return m_dthydro ; }
   Real_t& dtmax()                { return m_dtmax ; }
   Real_t& dtfixed()              { return m_dtfixed ; }

   Int_t&  cycle()                { return m_cycle ; }
#if _CD
   bool    check_begin(int intvl) { if(intvl == 1) return true;
                                    else {
                                    const Int_t cycle = m_cycle - 1; 
                                    if(cycle < 0) {
                                      PrintDomain();
                                      assert(0);
                                    } 
                                    return (cycle % intvl == 0); 
                                    }
   }
   bool    check_end(int intvl)   { if(intvl == 1) return true;
                                    else {
                                    const Int_t cycle = m_cycle - 1; 
                                    if(cycle < 0) {
                                      PrintDomain();
                                      assert(0);
                                    } 
                                    return (cycle % intvl == intvl - 1); }
   }
#endif
   Index_t&  numRanks()           { return m_numRanks ; }

   Index_t&  colLoc()             { return m_colLoc ; }
   Index_t&  rowLoc()             { return m_rowLoc ; }
   Index_t&  planeLoc()           { return m_planeLoc ; }
   Index_t&  tp()                 { return m_tp ; }

   Index_t&  sizeX()              { return m_sizeX ; }
   Index_t&  sizeY()              { return m_sizeY ; }
   Index_t&  sizeZ()              { return m_sizeZ ; }
   Index_t&  numReg()             { return m_numReg ; }
   Int_t&  cost()                 { return m_cost ; }
   Index_t&  numElem()            { return m_numElem ; }
   Index_t&  numNode()            { return m_numNode ; }
   
   Index_t&  maxPlaneSize()       { return m_maxPlaneSize ; }
   Index_t&  maxEdgeSize()        { return m_maxEdgeSize ; }
   
   //
   // MPI-Related additional data
   //

#if USE_MPI   
   // Communication Work space 
   Real_t *commDataSend ;
   Real_t *commDataRecv ;
   
   // Maximum number of block neighbors 
   MPI_Request recvRequest[26] ; // 6 faces + 12 edges + 8 corners 
   MPI_Request sendRequest[26] ; // 6 faces + 12 edges + 8 corners 
#endif

  private:

   void BuildMesh(Int_t nx, Int_t edgeNodes, Int_t edgeElems);
   void SetupThreadSupportStructures();
   void CreateRegionIndexSets(Int_t nreg, Int_t balance);
   void SetupCommBuffers(Int_t edgeNodes);
   void SetupSymmetryPlanes(Int_t edgeNodes);
   void SetupElementConnectivities(Int_t edgeElems);
   void SetupBoundaryConditions(Int_t edgeElems);

   //
   // IMPLEMENTATION
   //
  public:
#if _CD
   /* Node-centered */
   cd::CDVector<Real_t> m_x ;  /* coordinates */
   cd::CDVector<Real_t> m_y ;
   cd::CDVector<Real_t> m_z ;

   cd::CDVector<Real_t> m_xd ; /* velocities */
   cd::CDVector<Real_t> m_yd ;
   cd::CDVector<Real_t> m_zd ;

   cd::CDVector<Real_t> m_xdd ; /* accelerations */
   cd::CDVector<Real_t> m_ydd ;
   cd::CDVector<Real_t> m_zdd ;

   cd::CDVector<Real_t> m_fx ;  /* forces */
   cd::CDVector<Real_t> m_fy ;
   cd::CDVector<Real_t> m_fz ;

   cd::CDVector<Real_t> m_nodalMass ;  /* mass */

   cd::CDVector<Index_t> m_symmX ;  /* symmetry plane nodesets */
   cd::CDVector<Index_t> m_symmY ;
   cd::CDVector<Index_t> m_symmZ ;

   // Element-centered

/**
 * Kyushick: defined in base class 
   // Region information
   Int_t    m_numReg ;
   Int_t    m_cost; //imbalance cost
   Index_t *m_regElemSize ;   // Size of region sets
   Index_t *m_regNumList ;    // Region number per domain element
   Index_t **m_regElemlist ;  // region indexset 
*/
   cd::CDVector<Index_t>  m_nodelist ;     /* elemToNode connectivity */

   cd::CDVector<Index_t>  m_lxim ;  /* element connectivity across each face */
   cd::CDVector<Index_t>  m_lxip ;
   cd::CDVector<Index_t>  m_letam ;
   cd::CDVector<Index_t>  m_letap ;
   cd::CDVector<Index_t>  m_lzetam ;
   cd::CDVector<Index_t>  m_lzetap ;

   cd::CDVector<Int_t>    m_elemBC ;  /* symmetry/free-surface flags for each elem face */

   cd::CDVector<Real_t> m_dxx ;  /* principal strains -- temporary */
   cd::CDVector<Real_t> m_dyy ;
   cd::CDVector<Real_t> m_dzz ;

   cd::CDVector<Real_t> m_delv_xi ;    /* velocity gradient -- temporary */
   cd::CDVector<Real_t> m_delv_eta ;
   cd::CDVector<Real_t> m_delv_zeta ;

   cd::CDVector<Real_t> m_delx_xi ;    /* coordinate gradient -- temporary */
   cd::CDVector<Real_t> m_delx_eta ;
   cd::CDVector<Real_t> m_delx_zeta ;
   
   cd::CDVector<Real_t> m_e ;   /* energy */

   cd::CDVector<Real_t> m_p ;   /* pressure */
   cd::CDVector<Real_t> m_q ;   /* q */
   cd::CDVector<Real_t> m_ql ;  /* linear term for q */
   cd::CDVector<Real_t> m_qq ;  /* quadratic term for q */

   cd::CDVector<Real_t> m_v ;     /* relative volume */
   cd::CDVector<Real_t> m_volo ;  /* reference volume */
   cd::CDVector<Real_t> m_vnew ;  /* new relative volume -- temporary */
   cd::CDVector<Real_t> m_delv ;  /* m_vnew - m_v */
   cd::CDVector<Real_t> m_vdov ;  /* volume derivative over volume */

   cd::CDVector<Real_t> m_arealg ;  /* characteristic length of an element */
   
   cd::CDVector<Real_t> m_ss ;      /* "sound speed" */

   cd::CDVector<Real_t> m_elemMass ;  /* mass */
#else
   /* Node-centered */
   std::vector<Real_t> m_x ;  /* coordinates */
   std::vector<Real_t> m_y ;
   std::vector<Real_t> m_z ;

   std::vector<Real_t> m_xd ; /* velocities */
   std::vector<Real_t> m_yd ;
   std::vector<Real_t> m_zd ;

   std::vector<Real_t> m_xdd ; /* accelerations */
   std::vector<Real_t> m_ydd ;
   std::vector<Real_t> m_zdd ;

   std::vector<Real_t> m_fx ;  /* forces */
   std::vector<Real_t> m_fy ;
   std::vector<Real_t> m_fz ;

   std::vector<Real_t> m_nodalMass ;  /* mass */

   std::vector<Index_t> m_symmX ;  /* symmetry plane nodesets */
   std::vector<Index_t> m_symmY ;
   std::vector<Index_t> m_symmZ ;

   // Element-centered

/**
 * Kyushick: defined in base class 
   // Region information
   Int_t    m_numReg ;
   Int_t    m_cost; //imbalance cost
   Index_t *m_regElemSize ;   // Size of region sets
   Index_t *m_regNumList ;    // Region number per domain element
   Index_t **m_regElemlist ;  // region indexset 
*/
   std::vector<Index_t>  m_nodelist ;     /* elemToNode connectivity */

   std::vector<Index_t>  m_lxim ;  /* element connectivity across each face */
   std::vector<Index_t>  m_lxip ;
   std::vector<Index_t>  m_letam ;
   std::vector<Index_t>  m_letap ;
   std::vector<Index_t>  m_lzetam ;
   std::vector<Index_t>  m_lzetap ;

   std::vector<Int_t>    m_elemBC ;  /* symmetry/free-surface flags for each elem face */

   std::vector<Real_t> m_dxx ;  /* principal strains -- temporary */
   std::vector<Real_t> m_dyy ;
   std::vector<Real_t> m_dzz ;

   std::vector<Real_t> m_delv_xi ;    /* velocity gradient -- temporary */
   std::vector<Real_t> m_delv_eta ;
   std::vector<Real_t> m_delv_zeta ;

   std::vector<Real_t> m_delx_xi ;    /* coordinate gradient -- temporary */
   std::vector<Real_t> m_delx_eta ;
   std::vector<Real_t> m_delx_zeta ;
   
   std::vector<Real_t> m_e ;   /* energy */

   std::vector<Real_t> m_p ;   /* pressure */
   std::vector<Real_t> m_q ;   /* q */
   std::vector<Real_t> m_ql ;  /* linear term for q */
   std::vector<Real_t> m_qq ;  /* quadratic term for q */

   std::vector<Real_t> m_v ;     /* relative volume */
   std::vector<Real_t> m_volo ;  /* reference volume */
   std::vector<Real_t> m_vnew ;  /* new relative volume -- temporary */
   std::vector<Real_t> m_delv ;  /* m_vnew - m_v */
   std::vector<Real_t> m_vdov ;  /* volume derivative over volume */

   std::vector<Real_t> m_arealg ;  /* characteristic length of an element */
   
   std::vector<Real_t> m_ss ;      /* "sound speed" */

   std::vector<Real_t> m_elemMass ;  /* mass */

#endif
   // Cutoffs (treat as constants)
   const Real_t  m_e_cut ;             // energy tolerance 
   const Real_t  m_p_cut ;             // pressure tolerance 
   const Real_t  m_q_cut ;             // q tolerance 
   const Real_t  m_v_cut ;             // relative volume tolerance 
   const Real_t  m_u_cut ;             // velocity tolerance 

   // Other constants (usually setable, but hardcoded in this proxy app)

   const Real_t  m_hgcoef ;            // hourglass control 
   const Real_t  m_ss4o3 ;
   const Real_t  m_qstop ;             // excessive q indicator 
   const Real_t  m_monoq_max_slope ;
   const Real_t  m_monoq_limiter_mult ;
   const Real_t  m_qlc_monoq ;         // linear term coef for q 
   const Real_t  m_qqc_monoq ;         // quadratic term coef for q 
   const Real_t  m_qqc ;
   const Real_t  m_eosvmax ;
   const Real_t  m_eosvmin ;
   const Real_t  m_pmin ;              // pressure floor 
   const Real_t  m_emin ;              // energy floor 
   const Real_t  m_dvovmax ;           // maximum allowable volume change 
   const Real_t  m_refdens ;           // reference density 

/**
 *  Kyushick: In base class
   // Variables to keep track of timestep, simulation time, and cycle
   Real_t  m_dtcourant ;         // courant constraint 
   Real_t  m_dthydro ;           // volume change constraint 
   Int_t   m_cycle ;             // iteration count for simulation 
   Real_t  m_dtfixed ;           // fixed time increment 
   Real_t  m_time ;              // current time 
   Real_t  m_deltatime ;         // variable time increment 
   Real_t  m_deltatimemultlb ;
   Real_t  m_deltatimemultub ;
   Real_t  m_dtmax ;             // maximum allowable time increment 
   Real_t  m_stoptime ;          // end time for simulation 


   Int_t   m_numRanks ;

   Index_t m_colLoc ;
   Index_t m_rowLoc ;
   Index_t m_planeLoc ;
   Index_t m_tp ;

   Index_t m_sizeX ;
   Index_t m_sizeY ;
   Index_t m_sizeZ ;
   Index_t m_numElem ;
   Index_t m_numNode ;

   Index_t m_maxPlaneSize ;
   Index_t m_maxEdgeSize ;

   // Kyushick: Never updated during execution.
   //           Updated only during init phase.
   //           Just preserving pointers
   // OMP hack 
   Index_t *m_nodeElemStart ;
   Index_t *m_nodeElemCornerList ;

   // Used in setup
   Index_t m_rowMin, m_rowMax;
   Index_t m_colMin, m_colMax;
   Index_t m_planeMin, m_planeMax ;
*/
#if _CD
// Kyushick
   uint64_t serdes_vec;
//   Internal preserved;
   Domain &SetOp(const uint64_t &vec) {                                                  
     serdes_vec = vec;                                                                          
     return *this;                                                                              
   }    
   Index_t prv_prv_idx;   
   void PrintDebugDetailInternal(int tg_idx) {
     //const Index_t base = Index_t(8)*tg_idx; 
     LULESH_PRINT("prv idx:%d == %d\n", prv_idx, tg_idx);
     LULESH_PRINT("[Rank:%d] PrintDetail nodelist size:%zu/%zu (numElem:%d, base:%d)\n"
            "%6x %6x %6x %6x %6x %6x %6x %6x\n", 
             myRank, m_nodelist.size(), m_nodelist.capacity(), numElem(), base,
             m_nodelist[base],
             m_nodelist[base+1],
             m_nodelist[base+2],
             m_nodelist[base+3],
             m_nodelist[base+4],
             m_nodelist[base+5],
             m_nodelist[base+6],
             m_nodelist[base+7]
             ); 
#if 0
     Index_t nd0i = m_nodelist[base];
     Index_t nd1i = m_nodelist[base+1];
     Index_t nd2i = m_nodelist[base+2];
     Index_t nd3i = m_nodelist[base+3];
     Index_t nd4i = m_nodelist[base+4];
     Index_t nd5i = m_nodelist[base+5];
     Index_t nd6i = m_nodelist[base+6];
     Index_t nd7i = m_nodelist[base+7];
#endif
     LULESH_PRINT("x: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n"
            "y: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n"
            "z: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n"
           , x(nd0i)
           , x(nd1i)
           , x(nd2i)
           , x(nd3i)
           , x(nd4i)
           , x(nd5i)
           , x(nd6i)
           , x(nd7i)
           , y(nd0i)
           , y(nd1i)
           , y(nd2i)
           , y(nd3i)
           , y(nd4i)
           , y(nd5i)
           , y(nd6i)
           , y(nd7i)
           , z(nd0i)
           , z(nd1i)
           , z(nd2i)
           , z(nd3i)
           , z(nd4i)
           , z(nd5i)
           , z(nd6i)
           , z(nd7i)
           );
     for(int stride = 0; stride <= 16; stride += 8) {
       LULESH_PRINT("-- stride %d -------------------------------------\n", stride);
       LULESH_PRINT("x: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n"
              "y: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n"
              "z: %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f %5.3f\n"
             , x(0 + stride)
             , x(1 + stride)
             , x(2 + stride)
             , x(3 + stride)
             , x(4 + stride)
             , x(5 + stride)
             , x(6 + stride)
             , x(7 + stride)
             , y(0 + stride)
             , y(1 + stride)
             , y(2 + stride)
             , y(3 + stride)
             , y(4 + stride)
             , y(5 + stride)
             , y(6 + stride)
             , y(7 + stride)
             , z(0 + stride)
             , z(1 + stride)
             , z(2 + stride)
             , z(3 + stride)
             , z(4 + stride)
             , z(5 + stride)
             , z(6 + stride)
             , z(7 + stride)
             );
     }




   }
   void PrintDebugDetail(bool do_prv=false) {
     //static int counter = 0;
     if(myRank == 0) {
       LULESH_PRINT("======== [%s] Check %d %d ===================\n", (do_prv)? "Preserve":"Restore", m_cycle, counter++);
       if(do_prv) {
         prv_prv_idx = prv_idx;
         PrintDebugDetailInternal(prv_idx);
       } else {
         if(prv_prv_idx != prv_idx)  {
           LULESH_PRINT("Same when PreserveObject\n");
           PrintDebugDetailInternal(prv_prv_idx);
           PrintDebugDetailInternal(prv_idx);   
         } else {
           LULESH_PRINT("Same when PreserveObject\n");
           PrintDebugDetailInternal(prv_idx);   
         }

       }
     } 
     LULESH_PRINT("=====================================\n\n");
   }
   void *Serialize(uint64_t &len_in_bytes) { return NULL; }
   void Deserialize(void *object) {}
   void CheckUpdate(const char *str) 
   {
#if DO_CHECK      
      dynamic_cast<Internal *>(this)->CheckInternal(dynamic_cast<Internal &>(*preserved_), str);
      m_x.CompareVector(        str /*"X"        */);
      m_y.CompareVector(        str /*"Y"        */);
      m_z.CompareVector(        str /*"Z"        */);
      m_xd.CompareVector(       str /*"XD"       */); 
      m_yd.CompareVector(       str /*"YD"       */);
      m_zd.CompareVector(       str /*"ZD"       */);
      m_xdd.CompareVector(      str /*"XDD"      */);
      m_ydd.CompareVector(      str /*"YDD"      */);
      m_zdd.CompareVector(      str /*"ZDD"      */);
      m_fx.CompareVector(       str /*"FX"       */); 
      m_fy.CompareVector(       str /*"FY"       */);
      m_fz.CompareVector(       str /*"FZ"       */);
      m_nodalMass.CompareVector(str /*"NODALMASS"*/); 
      m_symmX.CompareVector(    str /*"SYMMX"    */);  
      m_symmY.CompareVector(    str /*"SYMMY"    */);
      m_symmZ.CompareVector(    str /*"SYMMZ"    */);
      m_nodelist.CompareVector( str /*"NODELIST" */);
      m_lxim.CompareVector(     str /*"LXIM"     */);  
      m_lxip.CompareVector(     str /*"LXIP"     */);
      m_letam.CompareVector(    str /*"LETAM"    */);
      m_letap.CompareVector(    str /*"LETAP"    */);
      m_lzetam.CompareVector(   str /*"LZETAM"   */);
      m_lzetap.CompareVector(   str /*"LZETAP"   */);
      m_elemBC.CompareVector(   str /*"ELEMBC"   */);
      m_dxx.CompareVector(      str /*"DXX"      */);  
      m_dyy.CompareVector(      str /*"DYY"      */);
      m_dzz.CompareVector(      str /*"DZZ"      */);
      m_delv_xi.CompareVector(  str /*"DELV_XI"  */); 
      m_delv_eta.CompareVector( str /*"DELV_ETA" */);
      m_delv_zeta.CompareVector(str /*"DELV_ZETA"*/);  
      m_delx_xi.CompareVector(  str /*"DELX_XI"  */); 
      m_delx_eta.CompareVector( str /*"DELX_ETA" */);
      m_delx_zeta.CompareVector(str /*"DELX_ZETA"*/); 
      m_e.CompareVector(        str /*"E"        */); 
      m_p.CompareVector(        str /*"P"        */); 
      m_q.CompareVector(        str /*"Q"        */); 
      m_ql.CompareVector(       str /*"QL"       */);
      m_qq.CompareVector(       str /*"QQ"       */);
      m_v.CompareVector(        str /*"V"        */); 
      m_volo.CompareVector(     str /*"VOLO"     */);
      m_vnew.CompareVector(     str /*"VNEW"     */);
      m_delv.CompareVector(     str /*"DELV"     */);
      m_vdov.CompareVector(     str /*"VDOV"     */);
      m_arealg.CompareVector(   str /*"AREALG"   */);  
      m_ss.CompareVector(       str /*"SS"       */);      
      m_elemMass.CompareVector( str /*"ELEMMASS" */);
#endif
   }

   const char *GetID(void) { return name_.c_str(); }
   uint64_t PreserveObject(packer::DataStore *packer) { return 0; }
   uint64_t PreserveObject(packer::CDPacker &packer, CDPrvType prv_type, const char *entry_str) {
      std::string entry_name(entry_str);
      uint64_t init_data_size  = packer.data_->used();
      uint64_t init_tabl_size  = packer.table_->buf_used();
      if(entry_str != NULL && name_ == DOMAIN_INIT_NAME) {
        name_ += entry_str;
      }
      PrintDebugDetail(true);

      // Copied by first preserve call.
//      if(preserved_ == nullptr) {
//        preserved_ = new Internal(*dynamic_cast<Internal *>(this));
//      }
      
      //packer.Add((char *)base, packer::CDEntry(cd::GetCDEntryID("BaseObj"), sizeof(Internal), 0, (char *)base));
      uint64_t target_vec = 1;
      uint64_t prv_size = 0;
      while(serdes_vec  != 0) {
         uint64_t id = vec2id(target_vec);
         if(serdes_vec & 0x1) {
//            LULESH_PRINT("target: %32s (%32lx)\n", id2str[id], target_vec);
            prv_size += SelectPreserve(id, packer, prv_type);
         }
         target_vec <<= 1;
         serdes_vec >>= 1;
      } // while ends
      uint64_t prv_data_size  = packer.data_->used()      - init_data_size;
      uint64_t prv_tabl_size  = packer.table_->buf_used() - init_tabl_size;

      total_size_   = prv_data_size + prv_tabl_size; 
      table_offset_ = prv_data_size;
      table_type_   = 0x12345678;
      id_           = 0x56789;
//      LULESH_PRINT("------------ Done -----------\n");
     
//      // Update Magic 
//      uint64_t preserved_size_only   = packer.data_->used();
//      packer.data_->PadZeros(0); 
//      uint64_t preserved_size   = packer.data_->used();
//      uint64_t table_offset = packer.AppendTable();
//      uint64_t total_size_   = packer.data_->used();
//      uint64_t table_offset_ = table_offset;
//      uint64_t table_type_   = packer::kCDEntry;
//      packer.data_->Flush();
//      packer::MagicStore &magic = packer.data_->GetMagicStore();
//      magic.total_size_   = total_size_, 
//      magic.table_offset_ = table_offset_;
//      magic.entry_type_   = table_type_;
//      magic.reserved2_    = 0x12345678;
//      magic.reserved_     = 0x01234567;
//      packer.data_->FlushMagic(&magic);

      LULESH_PRINT("====================\npreservation size: %lu == %lu ~= %lu, (w/ table: %lu)\n======================\n", prv_size, preserved_size_only, preserved_size, total_size_);
      PrintDebugDetail(true);


      return prv_size; 
   }

   uint64_t Deserialize(packer::CDPacker &packer, const char *entry_str) { 
     std::string entry_name(entry_str);
     // Dealloced and checked by first restore call
     // This means domain obj must be first restored,
     // and succeeding Deserialize() will check the restored domain object.
//     if(preserved_ != nullptr) {
//       bool different = CheckInternal(*preserved_, "DomainObj Check:");
//       assert(different);
////       delete preserved_;
//       preserved_ = nullptr;
//     }
     //printf("Deserialize %s\n", entry_str);
//      packer.Restore(cd::GetCDEntryID("BaseObj"));
//      copy(preserved);
      uint64_t target_vec = 1;
      uint64_t rst_size = 0;
      while(serdes_vec  != 0) {
         uint64_t id = vec2id(target_vec);
         if(serdes_vec & 0x1) {
//            LULESH_PRINT("target: %32s (%32lx)\n", id2str[id], target_vec);
            rst_size += SelectRestore(id, packer);
         }
         target_vec <<= 1;
         serdes_vec >>= 1;
      } // while ends
//      LULESH_PRINT("------------ Done -----------\n");
      PrintDebugDetail(false);
      return rst_size;
   }




   static bool restarted;
   uint64_t SelectPreserve(uint64_t id, packer::CDPacker &packer, CDPrvType prv_type) {
      restarted = false;
      if(myRank == 0) 
         LULESH_PRINT("%s %s\n", __func__, id2str[id]);
      uint64_t prv_size = 0;
      bool is_ref = ((prv_type & kRef) == kRef);
      switch(id) {
         case ID__X         : prv_size = m_x.PreserveObject(packer,         prv_type, "X"        ); break;
         case ID__Y         : prv_size = m_y.PreserveObject(packer,         prv_type, "Y"        ); break;
         case ID__Z         : prv_size = m_z.PreserveObject(packer,         prv_type, "Z"        ); break;
         case ID__XD        : prv_size = m_xd.PreserveObject(packer,        prv_type, "XD"       ); break; 
         case ID__YD        : prv_size = m_yd.PreserveObject(packer,        prv_type, "YD"       ); break;
         case ID__ZD        : prv_size = m_zd.PreserveObject(packer,        prv_type, "ZD"       ); break;
         case ID__XDD       : prv_size = m_xdd.PreserveObject(packer,       prv_type, "XDD"      ); break;
         case ID__YDD       : prv_size = m_ydd.PreserveObject(packer,       prv_type, "YDD"      ); break;
         case ID__ZDD       : prv_size = m_zdd.PreserveObject(packer,       prv_type, "ZDD"      ); break;
         case ID__FX        : prv_size = m_fx.PreserveObject(packer,        prv_type, "FX"       ); break; 
         case ID__FY        : prv_size = m_fy.PreserveObject(packer,        prv_type, "FY"       ); break;
         case ID__FZ        : prv_size = m_fz.PreserveObject(packer,        prv_type, "FZ"       ); break;
         case ID__NODALMASS : prv_size = m_nodalMass.PreserveObject(packer, prv_type, "NODALMASS"); break; 
         case ID__SYMMX     : prv_size = m_symmX.PreserveObject(packer,     prv_type, "SYMMX"    ); break;  
         case ID__SYMMY     : prv_size = m_symmY.PreserveObject(packer,     prv_type, "SYMMY"    ); break;
         case ID__SYMMZ     : prv_size = m_symmZ.PreserveObject(packer,     prv_type, "SYMMZ"    ); break;
         case ID__NODELIST  : prv_size = m_nodelist.PreserveObject(packer,  prv_type, "NODELIST" ); LULESH_PRINT("[%d] Nodelist preserved %zu/%zu\n", myRank, m_nodelist.size(), m_nodelist.capacity()); break;
         case ID__LXIM      : prv_size = m_lxim.PreserveObject(packer,      prv_type, "LXIM"     ); break;  
         case ID__LXIP      : prv_size = m_lxip.PreserveObject(packer,      prv_type, "LXIP"     ); break;
         case ID__LETAM     : prv_size = m_letam.PreserveObject(packer,     prv_type, "LETAM"    ); break;
         case ID__LETAP     : prv_size = m_letap.PreserveObject(packer,     prv_type, "LETAP"    ); break;
         case ID__LZETAM    : prv_size = m_lzetam.PreserveObject(packer,    prv_type, "LZETAM"   ); break;
         case ID__LZETAP    : prv_size = m_lzetap.PreserveObject(packer,    prv_type, "LZETAP"   ); break;
         case ID__ELEMBC    : prv_size = m_elemBC.PreserveObject(packer,    prv_type, "ELEMBC"   ); break;
         case ID__DXX       : prv_size = m_dxx.PreserveObject(packer,       prv_type, "DXX"      ); break;  
         case ID__DYY       : prv_size = m_dyy.PreserveObject(packer,       prv_type, "DYY"      ); break;
         case ID__DZZ       : prv_size = m_dzz.PreserveObject(packer,       prv_type, "DZZ"      ); break;
         case ID__DELV_XI   : prv_size = m_delv_xi.PreserveObject(packer,   prv_type, "DELV_XI"  ); break; 
         case ID__DELV_ETA  : prv_size = m_delv_eta.PreserveObject(packer,  prv_type, "DELV_ETA" ); break;
         case ID__DELV_ZETA : prv_size = m_delv_zeta.PreserveObject(packer, prv_type, "DELV_ZETA"); break;  
         case ID__DELX_XI   : prv_size = m_delx_xi.PreserveObject(packer,   prv_type, "DELX_XI"  ); break; 
         case ID__DELX_ETA  : prv_size = m_delx_eta.PreserveObject(packer,  prv_type, "DELX_ETA" ); break;
         case ID__DELX_ZETA : prv_size = m_delx_zeta.PreserveObject(packer, prv_type, "DELX_ZETA"); break; 
         case ID__E         : prv_size = m_e.PreserveObject(packer,         prv_type, "E"        ); break; 
         case ID__P         : prv_size = m_p.PreserveObject(packer,         prv_type, "P"        ); break; 
         case ID__Q         : prv_size = m_q.PreserveObject(packer,         prv_type, "Q"        ); break; 
         case ID__QL        : prv_size = m_ql.PreserveObject(packer,        prv_type, "QL"       ); break;
         case ID__QQ        : prv_size = m_qq.PreserveObject(packer,        prv_type, "QQ"       ); break;
         case ID__V         : prv_size = m_v.PreserveObject(packer,         prv_type, "V"        ); break; 
         case ID__VOLO      : prv_size = m_volo.PreserveObject(packer,      prv_type, "VOLO"     ); break;
         case ID__VNEW      : prv_size = m_vnew.PreserveObject(packer,      prv_type, "VNEW"     ); break;
         case ID__DELV      : prv_size = m_delv.PreserveObject(packer,      prv_type, "DELV"     ); break;
         case ID__VDOV      : prv_size = m_vdov.PreserveObject(packer,      prv_type, "VDOV"     ); break;
         case ID__AREALG    : prv_size = m_arealg.PreserveObject(packer,    prv_type, "AREALG"   ); break;  
         case ID__SS        : prv_size = m_ss.PreserveObject(packer,        prv_type, "SS"       ); break;      
         case ID__ELEMMASS  : prv_size = m_elemMass.PreserveObject(packer,  prv_type, "ELEMMASS" ); break;
         case ID__REGELEMSIZE : 
                     packer.Add((char *)m_regElemSize, 
                                packer::CDEntry(cd::GetCDEntryID("REGELEMSIZE"), 
                                                (is_ref)? packer::Attr::krefer:0, 
                                                m_numReg * sizeof(Index_t), 
                                                0, 
                                                (char *)m_regElemSize)); 
                     prv_size = m_numReg* sizeof(Index_t);
                     break; 
         case ID__REGNUMLIST  : 
                     packer.Add((char *)m_regNumList, packer::CDEntry(cd::GetCDEntryID("REGNUMLIST"), CHECK_PRV_TYPE(prv_type,kRef)? packer::Attr::krefer:0, m_numElem * sizeof(Index_t), 0, (char *)m_regNumList)); prv_size = m_numElem* sizeof(Index_t);
                     break;
         case ID__REGELEMLIST : 
                     packer.Add((char *)m_regElemlist, packer::CDEntry(cd::GetCDEntryID("REGELEMLIST"), CHECK_PRV_TYPE(prv_type,kRef)? packer::Attr::krefer:0, m_numReg * sizeof(Index_t *), 0, (char *)m_regElemlist)); prv_size = m_numReg* sizeof(Index_t *);
                     for(int i=0; i<m_numReg; i++) {
                        char elemID[32];
                        sprintf(elemID, "REGELEMLIST_%d", i);
                        packer.Add((char *)(m_regElemlist[i]), packer::CDEntry(cd::GetCDEntryID(elemID), CHECK_PRV_TYPE(prv_type,kRef)? packer::Attr::krefer:0, regElemSize(i) * sizeof(Index_t), 0, (char *)(m_regElemlist[i]))); prv_size = regElemSize(i) * sizeof(Index_t);
                     }
                     break;
         case ID__COMMBUFSEND : 
                     packer.Add((char *)commDataSend, packer::CDEntry(cd::GetCDEntryID("COMMBUFSEND"), CHECK_PRV_TYPE(prv_type,kRef)? packer::Attr::krefer:0, comBufSize * sizeof(Real_t), 0, (char *)commDataSend)); prv_size = comBufSize * sizeof(Real_t);
                     break;
         case ID__COMMBUFRECV : 
                     packer.Add((char *)commDataRecv, packer::CDEntry(cd::GetCDEntryID("COMMBUFRECV"), CHECK_PRV_TYPE(prv_type,kRef)? packer::Attr::krefer:0, comBufSize * sizeof(Real_t), 0, (char *)commDataRecv)); prv_size = comBufSize * sizeof(Real_t);
                     break;
         default: LULESH_PRINT("Error: Unsupported ID:%lu\n", id);
                     assert(0);
      }
      return prv_size;
   }

   uint64_t SelectRestore(uint64_t id, packer::CDPacker &packer) {
      restarted = true;
      if(myRank == 0) 
         LULESH_PRINT("%s %s\n", __func__, id2str[id]);
      uint64_t rst_size = 0;
      switch(id) {
         case ID__X         : rst_size = m_x.Deserialize(packer,        "X"        ); break;
         case ID__Y         : rst_size = m_y.Deserialize(packer,        "Y"        ); break;
         case ID__Z         : rst_size = m_z.Deserialize(packer,        "Z"        ); break;
         case ID__XD        : rst_size = m_xd.Deserialize(packer,       "XD"       ); break; 
         case ID__YD        : rst_size = m_yd.Deserialize(packer,       "YD"       ); break;
         case ID__ZD        : rst_size = m_zd.Deserialize(packer,       "ZD"       ); break;
         case ID__XDD       : rst_size = m_xdd.Deserialize(packer,      "XDD"      ); break;
         case ID__YDD       : rst_size = m_ydd.Deserialize(packer,      "YDD"      ); break;
         case ID__ZDD       : rst_size = m_zdd.Deserialize(packer,      "ZDD"      ); break;
         case ID__FX        : rst_size = m_fx.Deserialize(packer,       "FX"       ); break; 
         case ID__FY        : rst_size = m_fy.Deserialize(packer,       "FY"       ); break;
         case ID__FZ        : rst_size = m_fz.Deserialize(packer,       "FZ"       ); break;
         case ID__NODALMASS : rst_size = m_nodalMass.Deserialize(packer,"NODALMASS"); break; 
         case ID__SYMMX     : rst_size = m_symmX.Deserialize(packer,    "SYMMX"    ); break;  
         case ID__SYMMY     : rst_size = m_symmY.Deserialize(packer,    "SYMMY"    ); break;
         case ID__SYMMZ     : rst_size = m_symmZ.Deserialize(packer,    "SYMMZ"    ); break;
         case ID__NODELIST  : rst_size = m_nodelist.Deserialize(packer, "NODELIST" ); 
                              if(myRank == 0) {
                                LULESH_PRINT("[%d] Nodelist restored, %zu/%zu\n", myRank, m_nodelist.size(), m_nodelist.capacity());
                              }
                              break;
         case ID__LXIM      : rst_size = m_lxim.Deserialize(packer,     "LXIM"     ); break;  
         case ID__LXIP      : rst_size = m_lxip.Deserialize(packer,     "LXIP"     ); break;
         case ID__LETAM     : rst_size = m_letam.Deserialize(packer,    "LETAM"    ); break;
         case ID__LETAP     : rst_size = m_letap.Deserialize(packer,    "LETAP"    ); break;
         case ID__LZETAM    : rst_size = m_lzetam.Deserialize(packer,   "LZETAM"   ); break;
         case ID__LZETAP    : rst_size = m_lzetap.Deserialize(packer,   "LZETAP"   ); break;
         case ID__ELEMBC    : rst_size = m_elemBC.Deserialize(packer,   "ELEMBC"   ); break;
         case ID__DXX       : rst_size = m_dxx.Deserialize(packer,      "DXX"      ); break;  
         case ID__DYY       : rst_size = m_dyy.Deserialize(packer,      "DYY"      ); break;
         case ID__DZZ       : rst_size = m_dzz.Deserialize(packer,      "DZZ"      ); break;
         case ID__DELV_XI   : rst_size = m_delv_xi.Deserialize(packer,  "DELV_XI"  ); break; 
         case ID__DELV_ETA  : rst_size = m_delv_eta.Deserialize(packer, "DELV_ETA" ); break;
         case ID__DELV_ZETA : rst_size = m_delv_zeta.Deserialize(packer,"DELV_ZETA"); break;  
         case ID__DELX_XI   : rst_size = m_delx_xi.Deserialize(packer,  "DELX_XI"  ); break; 
         case ID__DELX_ETA  : rst_size = m_delx_eta.Deserialize(packer, "DELX_ETA" ); break;
         case ID__DELX_ZETA : rst_size = m_delx_zeta.Deserialize(packer,"DELX_ZETA"); break; 
         case ID__E         : rst_size = m_e.Deserialize(packer,        "E"        ); break; 
         case ID__P         : rst_size = m_p.Deserialize(packer,        "P"        ); break; 
         case ID__Q         : rst_size = m_q.Deserialize(packer,        "Q"        ); break; 
         case ID__QL        : rst_size = m_ql.Deserialize(packer,       "QL"       ); break;
         case ID__QQ        : rst_size = m_qq.Deserialize(packer,       "QQ"       ); break;
         case ID__V         : rst_size = m_v.Deserialize(packer,        "V"        ); break; 
         case ID__VOLO      : rst_size = m_volo.Deserialize(packer,     "VOLO"     ); break;
         case ID__VNEW      : rst_size = m_vnew.Deserialize(packer,     "VNEW"     ); break;
         case ID__DELV      : rst_size = m_delv.Deserialize(packer,     "DELV"     ); break;
         case ID__VDOV      : rst_size = m_vdov.Deserialize(packer,     "VDOV"     ); break;
         case ID__AREALG    : rst_size = m_arealg.Deserialize(packer,   "AREALG"   ); break;  
         case ID__SS        : rst_size = m_ss.Deserialize(packer,       "SS"       ); break;      
         case ID__ELEMMASS  : rst_size = m_elemMass.Deserialize(packer, "ELEMMASS" ); break;
         case ID__REGELEMSIZE : 
                     rst_size = packer.Restore(cd::GetCDEntryID("REGELEMSIZE"))->size();

                     break; 
         case ID__REGNUMLIST  : 
                     rst_size = packer.Restore(cd::GetCDEntryID("REGNUMLIST"))->size();
                     break;
         case ID__REGELEMLIST : 
                     rst_size = packer.Restore(cd::GetCDEntryID("REGELEMLIST"))->size();
                     for(int i=0; i<m_numReg; i++) {
                        char elemID[32];
                        sprintf(elemID, "REGELEMLIST_%d", i);
                        rst_size += packer.Restore(cd::GetCDEntryID(elemID))->size();
                     }
                     break;
         case ID__COMMBUFSEND : 
                     rst_size += packer.Restore(cd::GetCDEntryID("COMMBUFSEND"))->size();
                     break;
         case ID__COMMBUFRECV : 
                     rst_size += packer.Restore(cd::GetCDEntryID("COMMBUFRECV"))->size();
                     break;
         default: LULESH_PRINT("Error: Unsupported ID:%lu\n", id);
                  assert(0);
      }
      return rst_size;
   }
#else
   void CheckUpdate(const char *str) {}
#endif // _CD ends
} ;

typedef Real_t &(Domain::* Domain_member )(Index_t) ;

struct cmdLineOpts {
   Int_t its; // -i 
   Int_t nx;  // -s 
   Int_t numReg; // -r 
   Int_t numFiles; // -f
   Int_t showProg; // -p
   Int_t quiet; // -q
   Int_t viz; // -v 
   Int_t cost; // -c
   Int_t balance; // -b
};



// Function Prototypes

// lulesh-par
Real_t CalcElemVolume( const Real_t x[8],
                       const Real_t y[8],
                       const Real_t z[8]);

// lulesh-util
void ParseCommandLineOptions(int argc, char *argv[],
                             Int_t myRank, struct cmdLineOpts *opts);
void VerifyAndWriteFinalOutput(Real_t elapsed_time,
                               Domain& locDom,
                               Int_t nx,
                               Int_t numRanks);

// lulesh-viz
void DumpToVisit(Domain& domain, int numFiles, int myRank, int numRanks);

// lulesh-comm
void CommRecv(Domain& domain, Int_t msgType, Index_t xferFields,
              Index_t dx, Index_t dy, Index_t dz,
              bool doRecv, bool planeOnly);
void CommSend(Domain& domain, Int_t msgType,
              Index_t xferFields, Domain_member *fieldData,
              Index_t dx, Index_t dy, Index_t dz,
              bool doSend, bool planeOnly);
void CommSBN(Domain& domain, Int_t xferFields, Domain_member *fieldData);
void CommSyncPosVel(Domain& domain);
void CommMonoQ(Domain& domain);

// lulesh-init
void InitMeshDecomp(Int_t numRanks, Int_t myRank,
                    Int_t *col, Int_t *row, Int_t *plane, Int_t *side);

