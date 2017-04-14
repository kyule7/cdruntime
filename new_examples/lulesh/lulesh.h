#if !defined(USE_MPI)
# error "You should specify USE_MPI=0 or USE_MPI=1 on the compile line"
#endif

#if _CD
#define _DEBUG_LULESH_0402
#define SERDES_ENABLED 
#include "cd_packer.hpp"
#include "cd.h"
using namespace std;
using namespace packer;
#include <map>
#include <string>

/////////////////////////////////////////////////////////////////////
#define  M__NO_SERDES          0x0000000000000000  // 0
/* COORDINATES */
#define  M__X                  0x0000000000000001  // 1
#define  M__Y                  0x0000000000000002  // 2                      
#define  M__Z                  0x0000000000000004  // 3                      
/* VELOCITIES */
#define  M__XD                 0x0000000000000008  // 4                      
#define  M__YD                 0x0000000000000010  // 5                      
#define  M__ZD                 0x0000000000000020  // 6                      
/* ACCELERATIONS */
#define  M__XDD                0x0000000000000040  // 7                      
#define  M__YDD                0x0000000000000080  // 8                      
#define  M__ZDD                0x0000000000000100  // 9                      
/* FORCES */
#define  M__FX                 0x0000000000000200  // 10                     
#define  M__FY                 0x0000000000000400  // 11                     
#define  M__FZ                 0x0000000000000800  // 12                     
/* MASS */
#define  M__NODALMASS          0x0000000000001000  // 13                     
/* SYMMETRY PLANE NODESETS */
#define  M__SYMMX              0x0000000000002000  // 14                     
#define  M__SYMMY              0x0000000000004000  // 15                     
#define  M__SYMMZ              0x0000000000008000  // 16                     
/* MATERIAL INDEXSET */
#define  M__MATELEMLIST        0x0000000000010000  // 17                     
/* ELEMTONODE CONNECTIVITY */
#define  M__NODELIST           0x0000000000020000  // 18                     
/* ELEMENT CONNECTIVITY ACROSS EACH FACE */
#define  M__LXIM               0x0000000000040000  // 19                     
#define  M__LXIP               0x0000000000080000  // 20                     
#define  M__LETAM              0x0000000000100000  // 21                     
#define  M__LETAP              0x0000000000200000  // 22                     
#define  M__LZETAM             0x0000000000400000  // 23                     
#define  M__LZETAP             0x0000000000800000  // 24                     
/* SYMMETRY/FREE-SURFACE FLAGS FOR EACH ELEM FACE */
#define  M__ELEMBC             0x0000000001000000  // 25                     
/* PRINCIPAL STRAINS -- TEMPORARY */
#define  M__DXX                0x0000000002000000  // 26                     
#define  M__DYY                0x0000000004000000  // 27                     
#define  M__DZZ                0x0000000008000000  // 28                     
/* VELOCITY GRADIENT -- TEMPORARY */
#define  M__DELV_XI            0x0000000010000000  // 29                     
#define  M__DELV_ETA           0x0000000020000000  // 30                     
#define  M__DELV_ZETA          0x0000000040000000  // 31                     
/* COORDINATE GRADIENT -- TEMPORARY */
#define  M__DELX_XI            0x0000000080000000  // 32                     
#define  M__DELX_ETA           0x0000000100000000  // 33                     
#define  M__DELX_ZETA          0x0000000200000000  // 34                     
/* ENERGY */
#define  M__E                  0x0000000400000000  // 35                     
/* PRESSURE */
#define  M__P                  0x0000000800000000  // 36                     
/* PRESSURE */
/* Q */
#define  M__Q                  0x0000001000000000  // 37                     
/* LINEAR TERM FOR Q */
#define  M__QL                 0x0000002000000000  // 38                     
/* QUADRATIC TERM FOR Q */
#define  M__QQ                 0x0000004000000000  // 39                     
/* RELATIVE VOLUME */
#define  M__V                  0x0000008000000000  // 40                     
/* REFERENCE VOLUME */
#define  M__VOLO               0x0000010000000000  // 41                     
/* NEW RELATIVE VOLUME -- TEMPORARY */
#define  M__VNEW               0x0000020000000000  // 42                     
/* M__VNEW - M__V */
#define  M__DELV               0x0000040000000000  // 43                     
/* VOLUME DERIVATIVE OVER VOLUME */
#define  M__VDOV               0x0000080000000000  // 44                     
   /* CHARACTERISTIC LENGTH OF AN ELEMENT */
#define  M__AREALG             0x0000100000000000  // 45                     
/* "SOUND SPEED" */
#define  M__SS                 0x0000200000000000  // 46                     
/* MASS */
#define  M__ELEMMASS           0x0000400000000000  // 47         

/* REGION */
#define  M__REGELEMSIZE        0x0000800000000000  // 48                     
#define  M__REGELEMLIST        0x0001000000000000  // 49                     
#define  M__REGELEMLIST_INNER  0x0002000000000000  // 50                     
#define  M__REG_NUMLIST        0x0004000000000000  // 51                     
#define  M__SERDES_ALL         0x0007FFFFFFFFFFFF  // 52
/////////////////////////////////////////////////////////////////////
#define  TOTAL_ELEMENT_COUNT   53
/////////////////////////////////////////////////////////////////////
#define  ID__NO_SERDES         0  
#define  ID__X                 1
#define  ID__Y                 2 
#define  ID__Z                 3 
#define  ID__XD                4 
#define  ID__YD                5 
#define  ID__ZD                6 
#define  ID__XDD               7 
#define  ID__YDD               8 
#define  ID__ZDD               9 
#define  ID__FX                10
#define  ID__FY                11
#define  ID__FZ                12
#define  ID__NODALMASS         13
#define  ID__SYMMX             14
#define  ID__SYMMY             15
#define  ID__SYMMZ             16
#define  ID__MATELEMLIST       17
#define  ID__NODELIST          18
#define  ID__LXIM              19
#define  ID__LXIP              20
#define  ID__LETAM             21
#define  ID__LETAP             22
#define  ID__LZETAM            23
#define  ID__LZETAP            24
#define  ID__ELEMBC            25
#define  ID__DXX               26
#define  ID__DYY               27
#define  ID__DZZ               28
#define  ID__DELV_XI           29
#define  ID__DELV_ETA          30
#define  ID__DELV_ZETA         31
#define  ID__DELX_XI           32
#define  ID__DELX_ETA          33
#define  ID__DELX_ZETA         34
#define  ID__E                 35
#define  ID__P                 36
#define  ID__Q                 37
#define  ID__QL                38
#define  ID__QQ                39
#define  ID__V                 40
#define  ID__VOLO              41
#define  ID__VNEW              42
#define  ID__DELV              43
#define  ID__VDOV              44
#define  ID__AREALG            45
#define  ID__SS                46
#define  ID__ELEMMASS          47
#define  ID__REGELEMSIZE       48
#define  ID__REGELEMLIST       49
#define  ID__REGELEMLIST_INNER 50
#define  ID__REG_NUMLIST       51
#define  ID__SERDES_ALL        52
/////////////////////////////////////////////////////////////////////
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
#include <vector>

//**************************************************
// Allow flexibility for arithmetic representations 
//**************************************************

#define MAX(a, b) ( ((a) > (b)) ? (a) : (b))


// Precision specification
typedef float        real4 ;
typedef double       real8 ;
typedef long double  real10 ;  // 10 bytes on x86

typedef int    Index_t ; // array subscript and loop index
typedef real8  Real_t ;  // floating point representation
typedef int    Int_t ;   // integer representation

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
extern   Int_t myRank ;
extern   Int_t numRanks ;
#if _CD
extern unsigned vec2id(unsigned long long n);
extern MagicStore magic __attribute__((aligned(0x1000)));
#endif
 struct Internal {
   // Region information
   Int_t    m_numReg ;
   Int_t    m_cost; //imbalance cost
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

   // Used in setup
   Index_t m_rowMin, m_rowMax;
   Index_t m_colMin, m_colMax;
   Index_t m_planeMin, m_planeMax ;

};

class Domain  : public Internal {
#ifdef SERDES_ENABLED
 friend class DomainSerdes;
#endif
   public:

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

   // Nodal coordinates
   Real_t& x(Index_t idx)    { return m_x[idx] ; }
   Real_t& y(Index_t idx)    { return m_y[idx] ; }
   Real_t& z(Index_t idx)    { return m_z[idx] ; }

   // Nodal velocities
   Real_t& xd(Index_t idx)   { return m_xd[idx] ; }
   Real_t& yd(Index_t idx)   { return m_yd[idx] ; }
   Real_t& zd(Index_t idx)   { return m_zd[idx] ; }

   // Nodal accelerations
   Real_t& xdd(Index_t idx)  { return m_xdd[idx] ; }
   Real_t& ydd(Index_t idx)  { return m_ydd[idx] ; }
   Real_t& zdd(Index_t idx)  { return m_zdd[idx] ; }

   // Nodal forces
   Real_t& fx(Index_t idx)   { return m_fx[idx] ; }
   Real_t& fy(Index_t idx)   { return m_fy[idx] ; }
   Real_t& fz(Index_t idx)   { return m_fz[idx] ; }

   // Nodal mass
   Real_t& nodalMass(Index_t idx) { return m_nodalMass[idx] ; }

   // Nodes on symmertry planes
   Index_t symmX(Index_t idx) { return m_symmX[idx] ; }
   Index_t symmY(Index_t idx) { return m_symmY[idx] ; }
   Index_t symmZ(Index_t idx) { return m_symmZ[idx] ; }
   bool symmXempty()          { return m_symmX.empty(); }
   bool symmYempty()          { return m_symmY.empty(); }
   bool symmZempty()          { return m_symmZ.empty(); }

   //
   // Element-centered
   //
   Index_t&  regElemSize(Index_t idx) { return m_regElemSize[idx] ; }
   Index_t&  regNumList(Index_t idx) { return m_regNumList[idx] ; }
   Index_t*  regNumList()            { return &m_regNumList[0] ; }
   Index_t*  regElemlist(Int_t r)    { return m_regElemlist[r] ; }
   Index_t&  regElemlist(Int_t r, Index_t idx) { return m_regElemlist[r][idx] ; }

   Index_t*  nodelist(Index_t idx)    { return &m_nodelist[Index_t(8)*idx] ; }

   // elem connectivities through face
   Index_t&  lxim(Index_t idx) { return m_lxim[idx] ; }
   Index_t&  lxip(Index_t idx) { return m_lxip[idx] ; }
   Index_t&  letam(Index_t idx) { return m_letam[idx] ; }
   Index_t&  letap(Index_t idx) { return m_letap[idx] ; }
   Index_t&  lzetam(Index_t idx) { return m_lzetam[idx] ; }
   Index_t&  lzetap(Index_t idx) { return m_lzetap[idx] ; }

   // elem face symm/free-surface flag
   Int_t&  elemBC(Index_t idx) { return m_elemBC[idx] ; }

   // Principal strains - temporary
   Real_t& dxx(Index_t idx)  { return m_dxx[idx] ; }
   Real_t& dyy(Index_t idx)  { return m_dyy[idx] ; }
   Real_t& dzz(Index_t idx)  { return m_dzz[idx] ; }

   // Velocity gradient - temporary
   Real_t& delv_xi(Index_t idx)    { return m_delv_xi[idx] ; }
   Real_t& delv_eta(Index_t idx)   { return m_delv_eta[idx] ; }
   Real_t& delv_zeta(Index_t idx)  { return m_delv_zeta[idx] ; }

   // Position gradient - temporary
   Real_t& delx_xi(Index_t idx)    { return m_delx_xi[idx] ; }
   Real_t& delx_eta(Index_t idx)   { return m_delx_eta[idx] ; }
   Real_t& delx_zeta(Index_t idx)  { return m_delx_zeta[idx] ; }

   // Energy
   Real_t& e(Index_t idx)          { return m_e[idx] ; }

   // Pressure
   Real_t& p(Index_t idx)          { return m_p[idx] ; }

   // Artificial viscosity
   Real_t& q(Index_t idx)          { return m_q[idx] ; }

   // Linear term for q
   Real_t& ql(Index_t idx)         { return m_ql[idx] ; }
   // Quadratic term for q
   Real_t& qq(Index_t idx)         { return m_qq[idx] ; }

   // Relative volume
   Real_t& v(Index_t idx)          { return m_v[idx] ; }
   Real_t& delv(Index_t idx)       { return m_delv[idx] ; }

   // Reference volume
   Real_t& volo(Index_t idx)       { return m_volo[idx] ; }

   // volume derivative over volume
   Real_t& vdov(Index_t idx)       { return m_vdov[idx] ; }

   // Element characteristic length
   Real_t& arealg(Index_t idx)     { return m_arealg[idx] ; }

   // Sound speed
   Real_t& ss(Index_t idx)         { return m_ss[idx] ; }

   // Element mass
   Real_t& elemMass(Index_t idx)  { return m_elemMass[idx] ; }

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
   Index_t&  numRanks()           { return m_numRanks ; }

   Index_t&  colLoc()             { return m_colLoc ; }
   Index_t&  rowLoc()             { return m_rowLoc ; }
   Index_t&  planeLoc()           { return m_planeLoc ; }
   Index_t&  tp()                 { return m_tp ; }

   Index_t&  sizeX()              { return m_sizeX ; }
   Index_t&  sizeY()              { return m_sizeY ; }
   Index_t&  sizeZ()              { return m_sizeZ ; }
   Index_t&  numReg()             { return m_numReg ; }
   Int_t&  cost()             { return m_cost ; }
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
#ifdef SERDES_ENABLED
class DomainSerdes : public cd::PackerSerializable {
    uint64_t serdes_vec;
    Domain *dom;
    struct SerdesInfo {
        enum { 
          NoVec=0,
          Real=1,
          Index=2
        };
        char *src;
        uint64_t size;
        uint32_t elem_type;
        bool fixedsize;
        char name[256];
        char *chunk;
       
        public:
        SerdesInfo(void) {}
        SerdesInfo(char *_src, uint64_t _size, uint32_t type, const char *str, bool fixed=false)
          : src(_src), size(_size), elem_type(type) { strcpy(name, str); chunk = NULL; fixedsize = fixed;}
        SerdesInfo(char *_src, uint32_t type, const char *str, bool fixed=false)
          : src(_src), size(0), elem_type(type) { strcpy(name, str); chunk = NULL; fixedsize = fixed;}
        SerdesInfo(char *_src, uint64_t _size, uint32_t type)
          : src(_src), size(_size), elem_type(type) {}
//        SerdesInfo(void *_vector, uint32_t _elem_type)
//          : src(NULL), size(0), vector(_vector), elem_type(_elem_type) {}
        SerdesInfo &operator=(const SerdesInfo &that) {
          size = that.size;
          src = that.src;
          //vector = that.vector;
          elem_type = that.elem_type;
          strcpy(name, that.name);
          return *this;
        }
        uint64_t len(void) {
          uint64_t ret = reinterpret_cast<std::vector<Real_t> *>(src)->size();
          if(elem_type == Real) {
            if(fixedsize) {
              if(size != 0) { 
                return size;
              } else {
                size = ret;
                return ret;
              }
            } else {
              return ret;
            }
          } else if(elem_type == Index) {
            uint64_t ret = reinterpret_cast<std::vector<Index_t> *>(src)->size();
            if(fixedsize) {
              if(size != 0) { 
                return size;
              } else {
                size = ret;
                return ret;
              }
            } else {
              return ret;
            }
          } else if(elem_type == NoVec) {
            return size;
          } else {
            assert(0); return 0;
          }
        }
        char *ptr(void) {
          if(elem_type == Real) {
            char *ret = (char *)reinterpret_cast<std::vector<Real_t> *>(src)->data();
            if(fixedsize) {
              if(chunk != NULL) { 
                return chunk;
              } else {
                chunk = ret;
                return ret;
              }
            } else {
              return ret;
            }
          } else if(elem_type == Index) {
            char *ret = (char *)reinterpret_cast<std::vector<Index_t> *>(src)->data();
            if(fixedsize) {
              if(chunk != NULL) { 
                return chunk;
              } else {
                chunk = ret;
                return ret;
              }
            } else {
              return ret;
            }
          } else if(elem_type == NoVec) {
            return src;
          } else {
            assert(0); return 0;
          }
        }
    };
    static SerdesInfo serdes_table[TOTAL_ELEMENT_COUNT];
    static bool serdes_table_init; 
  public:
    Index_t prv_numRegSize; 
    Index_t prv_numElemSize;
    DomainSerdes(const Domain *domain=NULL) {
      serdes_vec = M__NO_SERDES;
      dom = const_cast<Domain *>(domain);
//      if(serdes_table_init == false) {
//        InitSerdesTable();
//        serdes_table_init = true;
//      }
      prv_numRegSize=0; 
      prv_numElemSize=0;
    }

//    unsigned vec2id(unsigned long long n) {
//      unsigned cnt=0;
//      while(n != 0) {
//        n >>= 1;
//        cnt++;
//      }
//      return cnt;
//    }

    static SerdesInfo *serdesRegElem;

    void InitDynElem(void) {
      //serdesRegElem = NULL;
      Index_t &numRegSize  = dom->numReg();
      Index_t &numElemSize = dom->numElem();
      if(numRegSize == prv_numRegSize && numElemSize == prv_numElemSize) 
        return;
      else {
        prv_numRegSize == numRegSize;
        prv_numElemSize == numElemSize;
        serdes_table[ID__REGELEMSIZE] = 
          SerdesInfo((char *)(dom->m_regElemSize), sizeof(Index_t) * numRegSize, SerdesInfo::NoVec, "regElemSize");
        serdes_table[ID__REGELEMLIST] = 
          SerdesInfo((char *)(dom->m_regElemlist), sizeof(Index_t*)* numRegSize, SerdesInfo::NoVec, "regElemList");
  //      SerdesInfo *serdesRegElem = NULL;
        //if(serdes_table[ID__REGELEMLIST_INNER].ptr() == NULL) {
        if(serdesRegElem == NULL) {
          serdesRegElem = new SerdesInfo[numRegSize];
          //printf("%s alloc %p\n", __func__, serdesRegElem);
          serdes_table[ID__REGELEMLIST_INNER] = 
            SerdesInfo((char *)serdesRegElem, sizeof(SerdesInfo) * numRegSize, SerdesInfo::NoVec, "regElemList_inner");
          char tmp_str_buf[16];
          for(int i=0; i<numRegSize; ++i) {
            sprintf(tmp_str_buf, "regElemList_%d", i);
            serdesRegElem[i] = 
              SerdesInfo((char *)((dom->m_regElemlist)[i]), (dom->m_regElemSize)[i], SerdesInfo::NoVec, tmp_str_buf);
          }
//        } else if((Index_t)serdes_table[ID__REGELEMLIST_INNER].len() != numRegSize) {
        } else { 
          //printf("%s delete  %p\n", __func__, serdesRegElem);
          delete [] serdesRegElem;
          //delete serdes_table[ID__REGELEMLIST_INNER].ptr();
          serdesRegElem = new SerdesInfo[numRegSize];
          //printf("%s realloc %p\n", __func__, serdesRegElem);
          serdes_table[ID__REGELEMLIST_INNER] = 
            SerdesInfo((char *)serdesRegElem, sizeof(SerdesInfo) * numRegSize, SerdesInfo::NoVec, "regElemList_inner");
          char tmp_str_buf[16];
          for(int i=0; i<numRegSize; ++i) {
            sprintf(tmp_str_buf, "regElemList_%d", i);
            serdesRegElem[i] = 
              SerdesInfo((char *)((dom->m_regElemlist)[i]), (dom->m_regElemSize)[i], SerdesInfo::NoVec, tmp_str_buf);
          }
        }
        serdes_table[ID__REG_NUMLIST]       = 
          SerdesInfo((char *)(dom->m_regNumList), sizeof(Index_t) * numElemSize, SerdesInfo::NoVec, "regNumList");
      }
    }
    ~DomainSerdes(void) {
      //printf("%s delete %p\n", __func__, serdesRegElem);
      if(serdesRegElem != NULL) delete [] serdesRegElem;
    }
    void InitSerdesTable(void) {
      serdes_table[ID__X]           = SerdesInfo((char *)&(dom->m_x),           SerdesInfo::Real , "x", true);
      serdes_table[ID__Y]           = SerdesInfo((char *)&(dom->m_y),           SerdesInfo::Real , "y", true);
      serdes_table[ID__Z]           = SerdesInfo((char *)&(dom->m_z),           SerdesInfo::Real , "z", true);
      serdes_table[ID__XD]          = SerdesInfo((char *)&(dom->m_xd),          SerdesInfo::Real , "xd", true);
      serdes_table[ID__YD]          = SerdesInfo((char *)&(dom->m_yd),          SerdesInfo::Real , "yd", true);
      serdes_table[ID__ZD]          = SerdesInfo((char *)&(dom->m_zd),          SerdesInfo::Real , "zd", true);
      serdes_table[ID__XDD]         = SerdesInfo((char *)&(dom->m_xdd),         SerdesInfo::Real , "xdd", true);
      serdes_table[ID__YDD]         = SerdesInfo((char *)&(dom->m_ydd),         SerdesInfo::Real , "ydd", true);
      serdes_table[ID__ZDD]         = SerdesInfo((char *)&(dom->m_zdd),         SerdesInfo::Real , "zdd", true);
      serdes_table[ID__FX]          = SerdesInfo((char *)&(dom->m_fx),          SerdesInfo::Real , "fx", true);
      serdes_table[ID__FY]          = SerdesInfo((char *)&(dom->m_fy),          SerdesInfo::Real , "fy", true);
      serdes_table[ID__FZ]          = SerdesInfo((char *)&(dom->m_fz),          SerdesInfo::Real , "fz", true);
      serdes_table[ID__NODALMASS]   = SerdesInfo((char *)&(dom->m_nodalMass),   SerdesInfo::Real , "nodalMass");
      serdes_table[ID__SYMMX]       = SerdesInfo((char *)&(dom->m_symmX),       SerdesInfo::Index, "symmX");
      serdes_table[ID__SYMMY]       = SerdesInfo((char *)&(dom->m_symmY),       SerdesInfo::Index, "symmY");
      serdes_table[ID__SYMMZ]       = SerdesInfo((char *)&(dom->m_symmZ),       SerdesInfo::Index, "symmZ");
//      serdes_table[ID__MATELEMLIST] = SerdesInfo((char *)&(dom->m_matElemlist), SerdesInfo::Index);
      serdes_table[ID__NODELIST]    = SerdesInfo((char *)&(dom->m_nodelist),    SerdesInfo::Index, "nodelist");
      serdes_table[ID__LXIM]        = SerdesInfo((char *)&(dom->m_lxim),        SerdesInfo::Index, "lxim");
      serdes_table[ID__LXIP]        = SerdesInfo((char *)&(dom->m_lxip),        SerdesInfo::Index, "lxip");
      serdes_table[ID__LETAM]       = SerdesInfo((char *)&(dom->m_letam),       SerdesInfo::Index, "letam");
      serdes_table[ID__LETAP]       = SerdesInfo((char *)&(dom->m_letap),       SerdesInfo::Index, "letap");
      serdes_table[ID__LZETAM]      = SerdesInfo((char *)&(dom->m_lzetam),      SerdesInfo::Index, "lzetam");
      serdes_table[ID__LZETAP]      = SerdesInfo((char *)&(dom->m_lzetap),      SerdesInfo::Index, "lzetap");
      serdes_table[ID__ELEMBC]      = SerdesInfo((char *)&(dom->m_elemBC),      SerdesInfo::Index, "elemBC");
      serdes_table[ID__DXX]         = SerdesInfo((char *)&(dom->m_dxx),         SerdesInfo::Real , "dxx");
      serdes_table[ID__DYY]         = SerdesInfo((char *)&(dom->m_dyy),         SerdesInfo::Real , "dyy");
      serdes_table[ID__DZZ]         = SerdesInfo((char *)&(dom->m_dzz),         SerdesInfo::Real , "dzz");
      serdes_table[ID__DELV_XI]     = SerdesInfo((char *)&(dom->m_delv_xi),     SerdesInfo::Real , "delv_xi");
      serdes_table[ID__DELV_ETA]    = SerdesInfo((char *)&(dom->m_delv_eta),    SerdesInfo::Real , "delv_eta");
      serdes_table[ID__DELV_ZETA]   = SerdesInfo((char *)&(dom->m_delv_zeta),   SerdesInfo::Real , "delv_zeta");
      serdes_table[ID__DELX_XI]     = SerdesInfo((char *)&(dom->m_delx_xi),     SerdesInfo::Real , "delx_xi");
      serdes_table[ID__DELX_ETA]    = SerdesInfo((char *)&(dom->m_delx_eta),    SerdesInfo::Real , "delta_eta");
      serdes_table[ID__DELX_ZETA]   = SerdesInfo((char *)&(dom->m_delx_zeta),   SerdesInfo::Real , "delx_zeta");
      serdes_table[ID__E]           = SerdesInfo((char *)&(dom->m_e),           SerdesInfo::Real , "e");
      serdes_table[ID__P]           = SerdesInfo((char *)&(dom->m_p),           SerdesInfo::Real , "p");
      serdes_table[ID__Q]           = SerdesInfo((char *)&(dom->m_q),           SerdesInfo::Real , "q");
      serdes_table[ID__QL]          = SerdesInfo((char *)&(dom->m_ql),          SerdesInfo::Real , "ql");
      serdes_table[ID__QQ]          = SerdesInfo((char *)&(dom->m_qq),          SerdesInfo::Real , "qq");
      serdes_table[ID__V]           = SerdesInfo((char *)&(dom->m_v),           SerdesInfo::Real , "v");
      serdes_table[ID__VOLO]        = SerdesInfo((char *)&(dom->m_volo),        SerdesInfo::Real , "volo");
      serdes_table[ID__VNEW]        = SerdesInfo((char *)&(dom->m_vnew),        SerdesInfo::Real , "vnew");
      serdes_table[ID__DELV]        = SerdesInfo((char *)&(dom->m_delv),        SerdesInfo::Real , "delv");
      serdes_table[ID__VDOV]        = SerdesInfo((char *)&(dom->m_vdov),        SerdesInfo::Real , "vdov");
      serdes_table[ID__AREALG]      = SerdesInfo((char *)&(dom->m_arealg),      SerdesInfo::Real , "arealg");
      serdes_table[ID__SS]          = SerdesInfo((char *)&(dom->m_ss),          SerdesInfo::Real , "ss");
      serdes_table[ID__ELEMMASS]    = SerdesInfo((char *)&(dom->m_elemMass),    SerdesInfo::Real , "elemmass");
      InitDynElem();                                                                             
    }                                                                                            
                                                                                                 
    DomainSerdes &SetOp (const uint64_t &vec) {                                                  
      serdes_vec = vec;                                                                          
//      printf("serdes vec : %lx\n", serdes_vec);                                                
      return *this;                                                                              
    }        
#if 0
static inline
uint64_t Preserve(cd::CDHandle *cdh, int id, const char *entry_str) {
  uint64_t len = 0;
  char tmpbuf[256];
  switch(id) {
    case ID__X         : sprintf(tmpbuf, "%s_%s", entry_str, "x"        ); cdh->Preserve(dom.m_x.data(),         dom.m_x.size(),          prvType, tmpbuf ); len = dom.m_x.size()       ; break;
    case ID__Y         : sprintf(tmpbuf, "%s_%s", entry_str, "y"        ); cdh->Preserve(dom.m_y.data(),         dom.m_y.size(),          prvType, tmpbuf ); len = dom.m_y.size()       ; break;
    case ID__Z         : sprintf(tmpbuf, "%s_%s", entry_str, "z"        ); cdh->Preserve(dom.m_z.data(),         dom.m_z.size(),          prvType, tmpbuf ); len = dom.m_z.size()       ; break;
    case ID__XD        : sprintf(tmpbuf, "%s_%s", entry_str, "xd"       ); cdh->Preserve(dom.m_xd.data(),        dom.m_xd.size(),         prvType, tmpbuf ); len = dom.m_xd.size()      ; break;
    case ID__YD        : sprintf(tmpbuf, "%s_%s", entry_str, "yd"       ); cdh->Preserve(dom.m_yd.data(),        dom.m_yd.size(),         prvType, tmpbuf ); len = dom.m_yd.size()      ; break;
    case ID__ZD        : sprintf(tmpbuf, "%s_%s", entry_str, "zd"       ); cdh->Preserve(dom.m_zd.data(),        dom.m_zd.size(),         prvType, tmpbuf ); len = dom.m_zd.size()      ; break;
    case ID__XDD       : sprintf(tmpbuf, "%s_%s", entry_str, "xdd"      ); cdh->Preserve(dom.m_xdd.data(),       dom.m_xdd.size(),        prvType, tmpbuf ); len = dom.m_xdd.size()     ; break;
    case ID__YDD       : sprintf(tmpbuf, "%s_%s", entry_str, "ydd"      ); cdh->Preserve(dom.m_ydd.data(),       dom.m_ydd.size(),        prvType, tmpbuf ); len = dom.m_ydd.size()     ; break;
    case ID__ZDD       : sprintf(tmpbuf, "%s_%s", entry_str, "zdd"      ); cdh->Preserve(dom.m_zdd.data(),       dom.m_zdd.size(),        prvType, tmpbuf ); len = dom.m_zdd.size()     ; break;
    case ID__FX        : sprintf(tmpbuf, "%s_%s", entry_str, "fx"       ); cdh->Preserve(dom.m_fx.data(),        dom.m_fx.size(),         prvType, tmpbuf ); len = dom.m_fx.size()      ; break;
    case ID__FY        : sprintf(tmpbuf, "%s_%s", entry_str, "fy"       ); cdh->Preserve(dom.m_fy.data(),        dom.m_fy.size(),         prvType, tmpbuf ); len = dom.m_fy.size()      ; break;
    case ID__FZ        : sprintf(tmpbuf, "%s_%s", entry_str, "fz"       ); cdh->Preserve(dom.m_fz.data(),        dom.m_fz.size(),         prvType, tmpbuf ); len = dom.m_fz.size()      ; break;
    case ID__NODALMASS : sprintf(tmpbuf, "%s_%s", entry_str, "nodalMass"); cdh->Preserve(dom.m_nodalMass.data(), dom.m_nodalMass.size(),  prvType, tmpbuf ); len = dom.m_nodalMass.size(); break;
    case ID__SYMMX     : sprintf(tmpbuf, "%s_%s", entry_str, "symmX"    ); cdh->Preserve(dom.m_symmX.data(),     dom.m_symmX.size(),      prvType, tmpbuf ); len = dom.m_symmX.size()   ; break;
    case ID__SYMMY     : sprintf(tmpbuf, "%s_%s", entry_str, "symmY"    ); cdh->Preserve(dom.m_symmY.data(),     dom.m_symmY.size(),      prvType, tmpbuf ); len = dom.m_symmY.size()   ; break;
    case ID__SYMMZ     : sprintf(tmpbuf, "%s_%s", entry_str, "symmZ"    ); cdh->Preserve(dom.m_symmZ.data(),     dom.m_symmZ.size(),      prvType, tmpbuf ); len = dom.m_symmZ.size()   ; break;  
    case ID__NODELIST  : sprintf(tmpbuf, "%s_%s", entry_str, "nodelist" ); cdh->Preserve(dom.m_nodelist.data(),  dom.m_nodelist.size(),   prvType, tmpbuf ); len = dom.m_nodelist.size(); break;
    case ID__LXIM      : sprintf(tmpbuf, "%s_%s", entry_str, "lxim"     ); cdh->Preserve(dom.m_lxim.data(),      dom.m_lxim.size(),       prvType, tmpbuf ); len = dom.m_lxim.size()    ; break;
    case ID__LXIP      : sprintf(tmpbuf, "%s_%s", entry_str, "lxip"     ); cdh->Preserve(dom.m_lxip.data(),      dom.m_lxip.size(),       prvType, tmpbuf ); len = dom.m_lxip.size()    ; break;
    case ID__LETAM     : sprintf(tmpbuf, "%s_%s", entry_str, "letam"    ); cdh->Preserve(dom.m_letam.data(),     dom.m_letam.size(),      prvType, tmpbuf ); len = dom.m_letam.size()   ; break;
    case ID__LETAP     : sprintf(tmpbuf, "%s_%s", entry_str, "letap"    ); cdh->Preserve(dom.m_letap.data(),     dom.m_letap.size(),      prvType, tmpbuf ); len = dom.m_letap.size()   ; break;
    case ID__LZETAM    : sprintf(tmpbuf, "%s_%s", entry_str, "lzetam"   ); cdh->Preserve(dom.m_lzetam.data(),    dom.m_lzetam.size(),     prvType, tmpbuf ); len = dom.m_lzetam.size()  ; break;
    case ID__LZETAP    : sprintf(tmpbuf, "%s_%s", entry_str, "lzetap"   ); cdh->Preserve(dom.m_lzetap.data(),    dom.m_lzetap.size(),     prvType, tmpbuf ); len = dom.m_lzetap.size()  ; break;
    case ID__ELEMBC    : sprintf(tmpbuf, "%s_%s", entry_str, "elemBC"   ); cdh->Preserve(dom.m_elemBC.data(),    dom.m_elemBC.size(),     prvType, tmpbuf ); len = dom.m_elemBC.size()  ; break;
    case ID__DXX       : sprintf(tmpbuf, "%s_%s", entry_str, "dxx"      ); cdh->Preserve(dom.m_dxx.data(),       dom.m_dxx.size(),        prvType, tmpbuf ); len = dom.m_dxx.size()     ; break;
    case ID__DYY       : sprintf(tmpbuf, "%s_%s", entry_str, "dyy"      ); cdh->Preserve(dom.m_dyy.data(),       dom.m_dyy.size(),        prvType, tmpbuf ); len = dom.m_dyy.size()     ; break;
    case ID__DZZ       : sprintf(tmpbuf, "%s_%s", entry_str, "dzz"      ); cdh->Preserve(dom.m_dzz.data(),       dom.m_dzz.size(),        prvType, tmpbuf ); len = dom.m_dzz.size()     ; break;
    case ID__DELV_XI   : sprintf(tmpbuf, "%s_%s", entry_str, "delv_xi"  ); cdh->Preserve(dom.m_delv_xi.data(),   dom.m_delv_xi.size(),    prvType, tmpbuf ); len = dom.m_delv_xi.size() ; break;
    case ID__DELV_ETA  : sprintf(tmpbuf, "%s_%s", entry_str, "delv_eta" ); cdh->Preserve(dom.m_delv_eta.data(),  dom.m_delv_eta(),        prvType, tmpbuf ); len = dom.m_delv_eta()     ; break;
    case ID__DELV_ZETA : sprintf(tmpbuf, "%s_%s", entry_str, "delv_zeta"); cdh->Preserve(dom.m_delv_zeta.data(), dom.m_delv_zeta(),       prvType, tmpbuf ); len = dom.m_delv_zeta()    ; break;
    case ID__DELX_XI   : sprintf(tmpbuf, "%s_%s", entry_str, "delx_xi"  ); cdh->Preserve(dom.m_delx_xi.data(),   dom.m_delx_xi.size(),    prvType, tmpbuf ); len = dom.m_delx_xi.size() ; break;
    case ID__DELX_ETA  : sprintf(tmpbuf, "%s_%s", entry_str, "delta_eta"); cdh->Preserve(dom.m_delx_eta.data(),  dom.m_delx_eta.size(),   prvType, tmpbuf ); len = dom.m_delx_eta.size(); break;
    case ID__DELX_ZETA : sprintf(tmpbuf, "%s_%s", entry_str, "delx_zeta"); cdh->Preserve(dom.m_delx_zeta.data(), dom.m_delx_zeta.size(),  prvType, tmpbuf ); len = dom.m_delx_zeta.size(); break;
    case ID__E         : sprintf(tmpbuf, "%s_%s", entry_str, "e"        ); cdh->Preserve(dom.m_e.data(),         dom.m_e.size(),          prvType, tmpbuf ); len = dom.m_e.size()       ; break;
    case ID__P         : sprintf(tmpbuf, "%s_%s", entry_str, "p"        ); cdh->Preserve(dom.m_p.data(),         dom.m_p.size(),          prvType, tmpbuf ); len = dom.m_p.size()       ; break;
    case ID__Q         : sprintf(tmpbuf, "%s_%s", entry_str, "q"        ); cdh->Preserve(dom.m_q.data(),         dom.m_q.size(),          prvType, tmpbuf ); len = dom.m_q.size()       ; break;
    case ID__QL        : sprintf(tmpbuf, "%s_%s", entry_str, "ql"       ); cdh->Preserve(dom.m_ql.data(),        dom.m_ql.size(),         prvType, tmpbuf ); len = dom.m_ql.size()      ; break;
    case ID__QQ        : sprintf(tmpbuf, "%s_%s", entry_str, "qq"       ); cdh->Preserve(dom.m_qq.data(),        dom.m_qq.size(),         prvType, tmpbuf ); len = dom.m_qq.size()      ; break;
    case ID__V         : sprintf(tmpbuf, "%s_%s", entry_str, "v"        ); cdh->Preserve(dom.m_v.data(),         dom.m_v.size(),          prvType, tmpbuf ); len = dom.m_v.size()       ; break;
    case ID__VOLO      : sprintf(tmpbuf, "%s_%s", entry_str, "volo"     ); cdh->Preserve(dom.m_volo.data(),      dom.m_volo.size(),       prvType, tmpbuf ); len = dom.m_volo.size()    ; break;
    case ID__VNEW      : sprintf(tmpbuf, "%s_%s", entry_str, "vnew"     ); cdh->Preserve(dom.m_vnew.data(),      dom.m_vnew.size(),       prvType, tmpbuf ); len = dom.m_vnew.size()    ; break;
    case ID__DELV      : sprintf(tmpbuf, "%s_%s", entry_str, "delv"     ); cdh->Preserve(dom.m_delv.data(),      dom.m_delv.size(),       prvType, tmpbuf ); len = dom.m_delv.size()    ; break;
    case ID__VDOV      : sprintf(tmpbuf, "%s_%s", entry_str, "vdov"     ); cdh->Preserve(dom.m_vdov.data(),      dom.m_vdov.size(),       prvType, tmpbuf ); len = dom.m_vdov.size()    ; break;
    case ID__AREALG    : sprintf(tmpbuf, "%s_%s", entry_str, "arealg"   ); cdh->Preserve(dom.m_arealg.data(),    dom.m_arealg.size(),     prvType, tmpbuf ); len = dom.m_arealg.size()  ; break;
    case ID__SS        : sprintf(tmpbuf, "%s_%s", entry_str, "ss"       ); cdh->Preserve(dom.m_ss.data(),        dom.m_ss.size(),         prvType, tmpbuf ); len = dom.m_ss.size()      ; break;
    case ID__ELEMMASS  : sprintf(tmpbuf, "%s_%s", entry_str, "elemmass" ); cdh->Preserve(dom.m_elemMass.data(),  dom.m_elemMasss.size(),  prvType, tmpbuf ); len = dom.m_elemMasss.size(); break;
    default:
  }
  return len;
}

    uint64_t Preserve(cd::CDHandle *cdh, Domain &dom, uint64_t vec, uint32_t prvType, const char *entry_str) {                          
      uint64_t target_vec = 1;
      uint64_t tot_len = 0;
      int count = 0;
      while(vec  != 0) {
        uint64_t id = vec2id(target_vec);
        if(vec & 0x1) {
          if(id == ID__MATELEMLIST) { vec >>= 1; target_vec <<= 1; continue; }
          tot_len += Preserve(cdh, id, entry_str);
          count++;
        }
        target_vec <<= 1;
        vec >>= 1;
      } // while ends
#ifdef _DEBUG_LULESH_0402
      if(myRank == 0) { 
        printf("## %s total:%lx\n", entry_str, tot_len); 
      }
#endif

    }
#endif
    uint64_t Preserve(cd::CDHandle *cdh, uint64_t vec, uint32_t prvType, const char *entry_str) {                          
                                                                                                 
      static bool init = false;                                                                  
      if(init == false) {                                                                        
        InitSerdesTable();                                                                       
        init = true;                                                                             
      }
//
      uint64_t target_vec = 1;
      uint64_t tot_len = 0;
      int count = 0;
      char tmp_name_buf[256];
      while(vec  != 0) {
        uint64_t id = vec2id(target_vec);
        if(myRank == 1)
          printf("%d SERDES: %lx %ld %p\n", count, target_vec, id, serdes_table[id].ptr());
        if(vec & 0x1) {
          if(id == ID__MATELEMLIST) { vec >>= 1; target_vec <<= 1; continue; }
          if(id != ID__REGELEMLIST_INNER) {
            if(myRank == 1)
              printf("%d SERDES: %p %lx %ld\n", count, serdes_table[id].ptr(),  target_vec, id);
            char *ptr = serdes_table[id].ptr();
#ifdef _DEBUG_LULESH_0402
            if(myRank == 0 && id == ID__DXX) {
              printf("SERDES: ID_DXX: %p, %lu\n", ptr, serdes_table[id].len() );
            }
#endif
            //if(ptr != NULL) 
            {
              sprintf(tmp_name_buf, "%s_%s", entry_str, serdes_table[id].name);
//              printf("id:%s name:%s\n", tmp_name_buf, serdes_table[id].name);
              uint64_t entry_id = GetCDEntryID(tmp_name_buf);
#ifdef _DEBUG_LULESH_0402
              if(myRank == 0) { 
                printf("%lx  %p %lx (%s %lx)\n", id, ptr, serdes_table[id].len(), tmp_name_buf, entry_id);
              }
#endif
              cdh->Preserve(ptr, serdes_table[id].len(), prvType, tmp_name_buf, "", target_vec);
              tot_len += serdes_table[id].len();
              count++;
            }
          } else {
            Index_t &numRegSize = dom->numReg();
            for(uint64_t j=0; j<numRegSize; ++j) {
              char *ptr = ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].ptr();
//              printf("check:%u:%p\n", id, ptr);
              //if(ptr != NULL) 
              {
                sprintf(tmp_name_buf, "%s_%s", entry_str, serdes_table[id].name);
                uint64_t entry_id = GetCDEntryID(tmp_name_buf);
#ifdef _DEBUG_LULESH_0402
                if(myRank == 0) { 
                  printf("%lx  %p %lx (%s %lx)\n", 
                        j + ID__SERDES_ALL, 
                        ptr, 
                        ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len(),
                        tmp_name_buf, entry_id);
                }
#endif
                cdh->Preserve(ptr, 
                              ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len(),
                              prvType, tmp_name_buf, "", target_vec);
                tot_len += serdes_table[id].len();
                count++;
              }
            }
          }
        }
        target_vec <<= 1;
        vec >>= 1;
      } // while ends

#ifdef _DEBUG_LULESH_0402
      if(myRank == 0) { 
        printf("## %s total:%lx\n", entry_str, tot_len); 
      }
#endif
      


      return tot_len;


    }
                                                                                                 
    // With this interface, user can switch to a different serializer with a method flag.        
    uint64_t PreserveObject(CDPacker &packer, const std::string &entry_name) {                          
                                                                                                 
      InitDynElem();                                                                             
      static bool init = false;                                                                  
      if(init == false) {                                                                        
        InitSerdesTable();                                                                       
        init = true;                                                                             
      }
      uint64_t orig_data_size = packer.data_->used();
      uint64_t orig_table_size = packer.table_->usedsize();
      uint64_t vec = serdes_vec;
      uint64_t target_vec = 1;
      uint64_t tot_len = 0;
      int count = 0;
      char tmp_name_buf[256];
      while(vec  != 0) {
        uint64_t id = vec2id(target_vec);
        if(vec & 0x1) {
          if(id == ID__MATELEMLIST) { vec >>= 1; target_vec <<= 1; continue; }
          if(id != ID__REGELEMLIST_INNER) {
            char *ptr = serdes_table[id].ptr();
#ifdef _DEBUG_LULESH_0402
            if(myRank == 0 && id == ID__DXX) {
              printf("SERDES: ID_DXX: %p, %lu\n", ptr, serdes_table[id].len() );
            }
#endif
            if(ptr != NULL) {
              sprintf(tmp_name_buf, "%s_%s", entry_name.c_str(), serdes_table[id].name);
//              printf("id:%s name:%s\n", tmp_name_buf, serdes_table[id].name);
              uint64_t entry_id = GetCDEntryID(tmp_name_buf);
#ifdef _DEBUG_LULESH_0402
              if(myRank == 0) { 
                printf("%lx  %p %lx (%s %lx)\n", id, ptr, serdes_table[id].len(), tmp_name_buf, entry_id);
              }
#endif
              packer.Add(ptr, CDEntry(entry_id, serdes_table[id].len(), 0, ptr));
              tot_len += serdes_table[id].len();
              count++;
            }
          } else {
            Index_t &numRegSize = dom->numReg();
            for(uint64_t j=0; j<numRegSize; ++j) {
              char *ptr = ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].ptr();
//              printf("check:%u:%p\n", id, ptr);
              if(ptr != NULL) {
                sprintf(tmp_name_buf, "%s_%s", entry_name.c_str(), serdes_table[id].name);
                uint64_t entry_id = GetCDEntryID(tmp_name_buf);
#ifdef _DEBUG_LULESH_0402
                if(myRank == 0) { 
                  printf("%lx  %p %lx (%s %lx)\n", 
                        j + ID__SERDES_ALL, 
                        ptr, 
                        ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len(),
                        tmp_name_buf, entry_id);
                }
#endif
                packer.Add( ptr,
                              CDEntry(entry_id, 
                                     ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len(),
                                     0,
                                     ptr)
                            );
                tot_len += serdes_table[id].len();
                count++;
              }
            }
          }
        }
        target_vec <<= 1;
        vec >>= 1;
      } // while ends

      total_size_   = packer.data_->used() - orig_data_size;
      table_offset_ = packer.table_->usedsize() - orig_table_size;// + packer.data_->offset());
      table_type_   = kCDEntry;

#ifdef _DEBUG_LULESH_0402
      if(myRank == 0) { 
        printf("## total:%lx, table_pos:%lx, entry_cnt:%d, table_size:%lx (==%lx), type:%lu\n", 
          total_size_, table_offset_, count, 
          total_size_ - table_offset_, 
          packer.table_->usedsize(), table_type_);
      }
#endif
      


      return tot_len;


    }


    uint64_t PreserveObject(DataStore* pPacker) {
      InitDynElem();
      static bool init = false;
      if(init == false) {
        InitSerdesTable();
        init = true;
      }
      // Use the same DataStore for in-place preservation,
      // but use the separate table. This table is different from entry_directory_,
      // and will be just appended in its DataStore, not in its TableStore.
      CDPacker packer(NULL, pPacker);
      CD_DEBUG("LULESH %s, serdes vec: %lx\n", __func__, serdes_vec);
//      uint64_t orig_size = packer.data_->used(); // return

//      packer.data_->SetFileType(kVolatile);
      uint64_t magic_offset = packer.data_->PadZeros(sizeof(MagicStore));
      uint64_t orig_size = magic_offset;// + packer.data_->offset();//packer.data_->used(); // return

      uint64_t vec = serdes_vec;
      uint64_t target_vec = 1;
      int count = 0;
      while(vec  != 0) {
        uint64_t id = vec2id(target_vec);
        if(vec & 0x1) {
//          printf("preserve id:%u\n", id);
//          if(id == ID__VNEW || id == ID__DELX_ZETA || id == ID__DELX_ETA || id == ID__DELX_XI) { vec >>=1; continue; }
          if(id == ID__MATELEMLIST) { vec >>= 1; target_vec <<= 1; continue; }
          if(id != ID__REGELEMLIST_INNER) {
            char *ptr = serdes_table[id].ptr();
//            printf("check:%u:%p\n", id, ptr);
#ifdef _DEBUG_LULESH_0402
            if(myRank == 0 && id == ID__DXX) {
              printf("SERDES: ID_DXX: %p, %lu\n", ptr, serdes_table[id].len() );
            }
#endif
            if(ptr != NULL) {
#ifdef _DEBUG_LULESH_0402
              if(myRank == 0) { 
                printf("%lx  %p %lx\n", id, ptr, serdes_table[id].len());
              }
#endif
              packer.Add(ptr, CDEntry(id, serdes_table[id].len(), 0, ptr));
              count++;
            }
          } else {
            Index_t &numRegSize = dom->numReg();
            for(uint64_t j=0; j<numRegSize; ++j) {
              char *ptr = ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].ptr();
//              printf("check:%u:%p\n", id, ptr);
              if(ptr != NULL) {
#ifdef _DEBUG_LULESH_0402
                if(myRank == 0) { 
                  printf("%lx  %p %lx\n", 
                        j + ID__SERDES_ALL, ptr, ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len());
                }
#endif
                packer.Add( ptr,
                              CDEntry(j + ID__SERDES_ALL, 
                                     ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len(),
                                     0,
                                     ptr)
                            );
                count++;
              }
            }
          }
        }
        target_vec <<= 1;
        vec >>= 1;
      } // while ends

      packer.data_->PadZeros(0); 
      uint64_t table_offset = packer.AppendTable();
//      packer.data_->PadZeros(0); 
      total_size_   = packer.data_->used() - orig_size;
      table_offset_ = table_offset - (magic_offset);// + packer.data_->offset());
      table_type_   = kCDEntry;
      magic.total_size_   = total_size_, 
      magic.table_offset_ = table_offset_;
      magic.entry_type_   = table_type_;
      magic.reserved2_    = 0x12345678;
      magic.reserved_     = 0x01234567;
      uint64_t magic_offset_in_buf = magic_offset - packer.data_->offset();
      packer.data_->Write((char *)&magic, sizeof(MagicStore), magic_offset_in_buf);
//      packer.data_->Flush();

#ifdef _DEBUG_LULESH_0402
      if(myRank == 0) { 
        printf("## total:%lx, table_pos:%lx, entry_cnt:%d, table_size:%lx (==%lx), type:%lu, offset:%lx\n", 
          total_size_, table_offset_, count, 
          total_size_ - table_offset_, 
          packer.table_->usedsize(), table_type_, table_offset);
      }
#endif
      


      return table_offset;
    }

    uint64_t GetTotalSize(void) {
      uint64_t table_size = 0;
      uint64_t vec = serdes_vec;
      while(vec != 0) {
        uint32_t id = vec2id(vec);
        if(vec & 0x1) {
          char *ptr = serdes_table[id].ptr();
//          if(id == ID__VNEW || id == ID__DELX_ZETA || id == ID__DELX_ETA) { vec >>=1; continue; }
          if(id != ID__REGELEMLIST_INNER) {
            if(ptr != NULL) {
              table_size += serdes_table[id].len();
            }
          } else {
            Index_t &numRegSize = dom->numReg();
            for(int j=0; j<numRegSize; ++j) {
              table_size += ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len();
            }
          }
        }
        vec >>= 1;
      } // while ends

      return table_size;
    }
    void *Serialize(uint64_t &len_in_bytes) {
      // User define whatever they want.
      CD_DEBUG("LULESH %s, serdes vec: %lx\n", __func__, serdes_vec);
//      printf("LULESH %s, serdes vec: %lx\n", __func__, serdes_vec);
      CDPacker packer;
      PreserveObject(packer.data_);
      return packer.GetTotalData(len_in_bytes);
    }
    
    void Deserialize(void *obj){}
    uint64_t Deserialize(CDPacker &packer, const std::string &entry_name) {
      uint64_t vec = serdes_vec;
      uint64_t target_vec = 1;
      uint64_t tot_len = 0;
      int count = 0;

      char tmp_name_buf[256];
      while(vec  != 0) {
        uint64_t id = vec2id(target_vec);
        if(vec & 0x1) {
          char *ptr = serdes_table[id].ptr();
          if(id == ID__MATELEMLIST) { vec >>= 1; target_vec <<= 1; continue; }
          if(id != ID__REGELEMLIST_INNER) {
            if(ptr != NULL) {
              sprintf(tmp_name_buf, "%s_%s", entry_name.c_str(), serdes_table[id].name);
              uint64_t entry_id = GetCDEntryID(tmp_name_buf);
#ifdef _DEBUG_LULESH_0402
              if(myRank == 0) { 
                printf("id:%s name:%s\n", tmp_name_buf, serdes_table[id].name);
                printf("%lx  %p %lx\n", id, ptr, serdes_table[id].len());
              }
#endif
              packer.Restore(entry_id);
              tot_len += serdes_table[id].len();
              count++;
            }
          } else {
            Index_t &numRegSize = dom->numReg();
            for(uint64_t j=0; j<numRegSize; ++j) {
              char *ptr = ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].ptr();
//              printf("check:%u:%p\n", id, ptr);
              if(ptr != NULL) {
                sprintf(tmp_name_buf, "%s_%s", entry_name.c_str(), serdes_table[id].name);
                uint64_t entry_id = GetCDEntryID(tmp_name_buf);
#ifdef _DEBUG_LULESH_0402
                if(myRank == 0) { 
                  printf("id:%s name:%s\n", tmp_name_buf, serdes_table[id].name);
                  printf("%lx  %p %lx\n", 
                        j + ID__SERDES_ALL, ptr, ((SerdesInfo *)(serdes_table[ID__REGELEMLIST_INNER].src))[j].len());
                }
#endif
                packer.Restore(entry_id);
                tot_len += serdes_table[id].len();
                count++;
              }
            }
          }
        }
        target_vec <<= 1;
        vec >>= 1;
      } // while ends
      return tot_len;
    }

};
public:
   DomainSerdes serdes;
#endif

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

   // Region information
   Index_t *m_regElemSize ;   // Size of region sets
   Index_t *m_regNumList ;    // Region number per domain element
   Index_t **m_regElemlist ;  // region indexset 

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

//   struct Internal {
//   // Region information
//   Int_t    m_numReg ;
//   Int_t    m_cost; //imbalance cost
//   // Variables to keep track of timestep, simulation time, and cycle
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
//
//
//   Int_t   m_numRanks ;
//
//   Index_t m_colLoc ;
//   Index_t m_rowLoc ;
//   Index_t m_planeLoc ;
//   Index_t m_tp ;
//
//   Index_t m_sizeX ;
//   Index_t m_sizeY ;
//   Index_t m_sizeZ ;
//   Index_t m_numElem ;
//   Index_t m_numNode ;
//
//   Index_t m_maxPlaneSize ;
//   Index_t m_maxEdgeSize ;
//
   // OMP hack 
   Index_t *m_nodeElemStart ;
   Index_t *m_nodeElemCornerList ;
//
//   // Used in setup
//   Index_t m_rowMin, m_rowMax;
//   Index_t m_colMin, m_colMax;
//   Index_t m_planeMin, m_planeMax ;
//
//   };
//  Internal internal;
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

#if _CD
static inline
uint64_t SelectPreserve(cd::CDHandle *cdh, Domain &dom, int id, uint32_t prvType, const char *entry_str) 
{
  uint64_t len = 0;
  char tmpbuf[256];
  switch(id) {
    case ID__X         : sprintf(tmpbuf, "%s_%s", entry_str, "x"        ); cdh->Preserve(dom.m_x.data(),         sizeof(Real_t ) * dom.m_x.size(),          prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_x.size()       ; break;
    case ID__Y         : sprintf(tmpbuf, "%s_%s", entry_str, "y"        ); cdh->Preserve(dom.m_y.data(),         sizeof(Real_t ) * dom.m_y.size(),          prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_y.size()       ; break;
    case ID__Z         : sprintf(tmpbuf, "%s_%s", entry_str, "z"        ); cdh->Preserve(dom.m_z.data(),         sizeof(Real_t ) * dom.m_z.size(),          prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_z.size()       ; break;
    case ID__XD        : sprintf(tmpbuf, "%s_%s", entry_str, "xd"       ); cdh->Preserve(dom.m_xd.data(),        sizeof(Real_t ) * dom.m_xd.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_xd.size()      ; break;
    case ID__YD        : sprintf(tmpbuf, "%s_%s", entry_str, "yd"       ); cdh->Preserve(dom.m_yd.data(),        sizeof(Real_t ) * dom.m_yd.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_yd.size()      ; break;
    case ID__ZD        : sprintf(tmpbuf, "%s_%s", entry_str, "zd"       ); cdh->Preserve(dom.m_zd.data(),        sizeof(Real_t ) * dom.m_zd.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_zd.size()      ; break;
    case ID__XDD       : sprintf(tmpbuf, "%s_%s", entry_str, "xdd"      ); cdh->Preserve(dom.m_xdd.data(),       sizeof(Real_t ) * dom.m_xdd.size(),        prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_xdd.size()     ; break;
    case ID__YDD       : sprintf(tmpbuf, "%s_%s", entry_str, "ydd"      ); cdh->Preserve(dom.m_ydd.data(),       sizeof(Real_t ) * dom.m_ydd.size(),        prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_ydd.size()     ; break;
    case ID__ZDD       : sprintf(tmpbuf, "%s_%s", entry_str, "zdd"      ); cdh->Preserve(dom.m_zdd.data(),       sizeof(Real_t ) * dom.m_zdd.size(),        prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_zdd.size()     ; break;
    case ID__FX        : sprintf(tmpbuf, "%s_%s", entry_str, "fx"       ); cdh->Preserve(dom.m_fx.data(),        sizeof(Real_t ) * dom.m_fx.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_fx.size()      ; break;
    case ID__FY        : sprintf(tmpbuf, "%s_%s", entry_str, "fy"       ); cdh->Preserve(dom.m_fy.data(),        sizeof(Real_t ) * dom.m_fy.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_fy.size()      ; break;
    case ID__FZ        : sprintf(tmpbuf, "%s_%s", entry_str, "fz"       ); cdh->Preserve(dom.m_fz.data(),        sizeof(Real_t ) * dom.m_fz.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_fz.size()      ; break;
    case ID__NODALMASS : sprintf(tmpbuf, "%s_%s", entry_str, "nodalMass"); cdh->Preserve(dom.m_nodalMass.data(), sizeof(Real_t ) * dom.m_nodalMass.size(),  prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_nodalMass.size(); break;
    case ID__SYMMX     : sprintf(tmpbuf, "%s_%s", entry_str, "symmX"    ); cdh->Preserve(dom.m_symmX.data(),     sizeof(Index_t) * dom.m_symmX.size(),      prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_symmX.size()   ; break;
    case ID__SYMMY     : sprintf(tmpbuf, "%s_%s", entry_str, "symmY"    ); cdh->Preserve(dom.m_symmY.data(),     sizeof(Index_t) * dom.m_symmY.size(),      prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_symmY.size()   ; break;
    case ID__SYMMZ     : sprintf(tmpbuf, "%s_%s", entry_str, "symmZ"    ); cdh->Preserve(dom.m_symmZ.data(),     sizeof(Index_t) * dom.m_symmZ.size(),      prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_symmZ.size()   ; break;  
    case ID__NODELIST  : sprintf(tmpbuf, "%s_%s", entry_str, "nodelist" ); cdh->Preserve(dom.m_nodelist.data(),  sizeof(Index_t) * dom.m_nodelist.size(),   prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_nodelist.size(); break;
    case ID__LXIM      : sprintf(tmpbuf, "%s_%s", entry_str, "lxim"     ); cdh->Preserve(dom.m_lxim.data(),      sizeof(Index_t) * dom.m_lxim.size(),       prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_lxim.size()    ; break;
    case ID__LXIP      : sprintf(tmpbuf, "%s_%s", entry_str, "lxip"     ); cdh->Preserve(dom.m_lxip.data(),      sizeof(Index_t) * dom.m_lxip.size(),       prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_lxip.size()    ; break;
    case ID__LETAM     : sprintf(tmpbuf, "%s_%s", entry_str, "letam"    ); cdh->Preserve(dom.m_letam.data(),     sizeof(Index_t) * dom.m_letam.size(),      prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_letam.size()   ; break;
    case ID__LETAP     : sprintf(tmpbuf, "%s_%s", entry_str, "letap"    ); cdh->Preserve(dom.m_letap.data(),     sizeof(Index_t) * dom.m_letap.size(),      prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_letap.size()   ; break;
    case ID__LZETAM    : sprintf(tmpbuf, "%s_%s", entry_str, "lzetam"   ); cdh->Preserve(dom.m_lzetam.data(),    sizeof(Index_t) * dom.m_lzetam.size(),     prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_lzetam.size()  ; break;
    case ID__LZETAP    : sprintf(tmpbuf, "%s_%s", entry_str, "lzetap"   ); cdh->Preserve(dom.m_lzetap.data(),    sizeof(Index_t) * dom.m_lzetap.size(),     prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_lzetap.size()  ; break;
    case ID__ELEMBC    : sprintf(tmpbuf, "%s_%s", entry_str, "elemBC"   ); cdh->Preserve(dom.m_elemBC.data(),    sizeof(Index_t) * dom.m_elemBC.size(),     prvType, tmpbuf ); len = sizeof(Index_t) * dom.m_elemBC.size()  ; break;
    case ID__DXX       : sprintf(tmpbuf, "%s_%s", entry_str, "dxx"      ); cdh->Preserve(dom.m_dxx.data(),       sizeof(Real_t ) * dom.m_dxx.size(),        prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_dxx.size()     ; break;
    case ID__DYY       : sprintf(tmpbuf, "%s_%s", entry_str, "dyy"      ); cdh->Preserve(dom.m_dyy.data(),       sizeof(Real_t ) * dom.m_dyy.size(),        prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_dyy.size()     ; break;
    case ID__DZZ       : sprintf(tmpbuf, "%s_%s", entry_str, "dzz"      ); cdh->Preserve(dom.m_dzz.data(),       sizeof(Real_t ) * dom.m_dzz.size(),        prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_dzz.size()     ; break;
    case ID__DELV_XI   : sprintf(tmpbuf, "%s_%s", entry_str, "delv_xi"  ); cdh->Preserve(dom.m_delv_xi.data(),   sizeof(Real_t ) * dom.m_delv_xi.size(),    prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_delv_xi.size() ; break;
    case ID__DELV_ETA  : sprintf(tmpbuf, "%s_%s", entry_str, "delv_eta" ); cdh->Preserve(dom.m_delv_eta.data(),  sizeof(Real_t ) * dom.m_delv_eta.size(),   prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_delv_eta.size(); break;
    case ID__DELV_ZETA : sprintf(tmpbuf, "%s_%s", entry_str, "delv_zeta"); cdh->Preserve(dom.m_delv_zeta.data(), sizeof(Real_t ) * dom.m_delv_zeta.size(),  prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_delv_zeta.size(); break;
    case ID__DELX_XI   : sprintf(tmpbuf, "%s_%s", entry_str, "delx_xi"  ); cdh->Preserve(dom.m_delx_xi.data(),   sizeof(Real_t ) * dom.m_delx_xi.size(),    prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_delx_xi.size() ; break;
    case ID__DELX_ETA  : sprintf(tmpbuf, "%s_%s", entry_str, "delta_eta"); cdh->Preserve(dom.m_delx_eta.data(),  sizeof(Real_t ) * dom.m_delx_eta.size(),   prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_delx_eta.size(); break;
    case ID__DELX_ZETA : sprintf(tmpbuf, "%s_%s", entry_str, "delx_zeta"); cdh->Preserve(dom.m_delx_zeta.data(), sizeof(Real_t ) * dom.m_delx_zeta.size(),  prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_delx_zeta.size(); break;
    case ID__E         : sprintf(tmpbuf, "%s_%s", entry_str, "e"        ); cdh->Preserve(dom.m_e.data(),         sizeof(Real_t ) * dom.m_e.size(),          prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_e.size()       ; break;
    case ID__P         : sprintf(tmpbuf, "%s_%s", entry_str, "p"        ); cdh->Preserve(dom.m_p.data(),         sizeof(Real_t ) * dom.m_p.size(),          prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_p.size()       ; break;
    case ID__Q         : sprintf(tmpbuf, "%s_%s", entry_str, "q"        ); cdh->Preserve(dom.m_q.data(),         sizeof(Real_t ) * dom.m_q.size(),          prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_q.size()       ; break;
    case ID__QL        : sprintf(tmpbuf, "%s_%s", entry_str, "ql"       ); cdh->Preserve(dom.m_ql.data(),        sizeof(Real_t ) * dom.m_ql.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_ql.size()      ; break;
    case ID__QQ        : sprintf(tmpbuf, "%s_%s", entry_str, "qq"       ); cdh->Preserve(dom.m_qq.data(),        sizeof(Real_t ) * dom.m_qq.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_qq.size()      ; break;
    case ID__V         : sprintf(tmpbuf, "%s_%s", entry_str, "v"        ); cdh->Preserve(dom.m_v.data(),         sizeof(Real_t ) * dom.m_v.size(),          prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_v.size()       ; break;
    case ID__VOLO      : sprintf(tmpbuf, "%s_%s", entry_str, "volo"     ); cdh->Preserve(dom.m_volo.data(),      sizeof(Real_t ) * dom.m_volo.size(),       prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_volo.size()    ; break;
    case ID__VNEW      : sprintf(tmpbuf, "%s_%s", entry_str, "vnew"     ); cdh->Preserve(dom.m_vnew.data(),      sizeof(Real_t ) * dom.m_vnew.size(),       prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_vnew.size()    ; break;
    case ID__DELV      : sprintf(tmpbuf, "%s_%s", entry_str, "delv"     ); cdh->Preserve(dom.m_delv.data(),      sizeof(Real_t ) * dom.m_delv.size(),       prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_delv.size()    ; break;
    case ID__VDOV      : sprintf(tmpbuf, "%s_%s", entry_str, "vdov"     ); cdh->Preserve(dom.m_vdov.data(),      sizeof(Real_t ) * dom.m_vdov.size(),       prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_vdov.size()    ; break;
    case ID__AREALG    : sprintf(tmpbuf, "%s_%s", entry_str, "arealg"   ); cdh->Preserve(dom.m_arealg.data(),    sizeof(Real_t ) * dom.m_arealg.size(),     prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_arealg.size()  ; break;
    case ID__SS        : sprintf(tmpbuf, "%s_%s", entry_str, "ss"       ); cdh->Preserve(dom.m_ss.data(),        sizeof(Real_t ) * dom.m_ss.size(),         prvType, tmpbuf ); len = sizeof(Real_t ) * dom.m_ss.size()      ; break;
    case ID__ELEMMASS  : sprintf(tmpbuf, "%s_%s", entry_str, "elemmass" ); cdh->Preserve(dom.m_elemMass.data(),  sizeof(Real_t ) * dom.m_elemMass.size(),  prvType, tmpbuf ); len =  sizeof(Real_t ) * dom.m_elemMass.size(); break;
    default: break;
  }
  return len;
}

extern uint64_t Preserve(cd::CDHandle *cdh, Domain &dom, uint64_t vec, uint32_t prvType, const char *entry_str); 
#endif
