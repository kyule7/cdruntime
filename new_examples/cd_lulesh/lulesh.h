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
#include "cd_containers.hpp"
#include "cd_def.h"
#include "cd.h"
using namespace cd;

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
};


class Domain : public Internal, public PackerSerializable {

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
  public:
   /* Node-centered */
   CDVector<Real_t> m_x ;  /* coordinates */
   CDVector<Real_t> m_y ;
   CDVector<Real_t> m_z ;

   CDVector<Real_t> m_xd ; /* velocities */
   CDVector<Real_t> m_yd ;
   CDVector<Real_t> m_zd ;

   CDVector<Real_t> m_xdd ; /* accelerations */
   CDVector<Real_t> m_ydd ;
   CDVector<Real_t> m_zdd ;

   CDVector<Real_t> m_fx ;  /* forces */
   CDVector<Real_t> m_fy ;
   CDVector<Real_t> m_fz ;

   CDVector<Real_t> m_nodalMass ;  /* mass */

   CDVector<Index_t> m_symmX ;  /* symmetry plane nodesets */
   CDVector<Index_t> m_symmY ;
   CDVector<Index_t> m_symmZ ;

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
   CDVector<Index_t>  m_nodelist ;     /* elemToNode connectivity */

   CDVector<Index_t>  m_lxim ;  /* element connectivity across each face */
   CDVector<Index_t>  m_lxip ;
   CDVector<Index_t>  m_letam ;
   CDVector<Index_t>  m_letap ;
   CDVector<Index_t>  m_lzetam ;
   CDVector<Index_t>  m_lzetap ;

   CDVector<Int_t>    m_elemBC ;  /* symmetry/free-surface flags for each elem face */

   CDVector<Real_t> m_dxx ;  /* principal strains -- temporary */
   CDVector<Real_t> m_dyy ;
   CDVector<Real_t> m_dzz ;

   CDVector<Real_t> m_delv_xi ;    /* velocity gradient -- temporary */
   CDVector<Real_t> m_delv_eta ;
   CDVector<Real_t> m_delv_zeta ;

   CDVector<Real_t> m_delx_xi ;    /* coordinate gradient -- temporary */
   CDVector<Real_t> m_delx_eta ;
   CDVector<Real_t> m_delx_zeta ;
   
   CDVector<Real_t> m_e ;   /* energy */

   CDVector<Real_t> m_p ;   /* pressure */
   CDVector<Real_t> m_q ;   /* q */
   CDVector<Real_t> m_ql ;  /* linear term for q */
   CDVector<Real_t> m_qq ;  /* quadratic term for q */

   CDVector<Real_t> m_v ;     /* relative volume */
   CDVector<Real_t> m_volo ;  /* reference volume */
   CDVector<Real_t> m_vnew ;  /* new relative volume -- temporary */
   CDVector<Real_t> m_delv ;  /* m_vnew - m_v */
   CDVector<Real_t> m_vdov ;  /* volume derivative over volume */

   CDVector<Real_t> m_arealg ;  /* characteristic length of an element */
   
   CDVector<Real_t> m_ss ;      /* "sound speed" */

   CDVector<Real_t> m_elemMass ;  /* mass */

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

// Kyushick
   uint64_t serdes_vec;
   Domain &SetOp(const uint64_t &vec) {                                                  
     serdes_vec = vec;                                                                          
     return *this;                                                                              
   }        
   void *Serialize(uint64_t &len_in_bytes) { return NULL; }
   void Deserialize(void *object) {}
   uint64_t PreserveObject(packer::DataStore *packer) { return 0; } 
   uint64_t PreserveObject(packer::CDPacker &packer, const std::string &entry_name) {
      Internal *base = dynamic_cast<Internal *>(this);
      packer.Add((char *)base, packer::CDEntry(GetCDEntryID("BaseObj"), sizeof(Internal), 0, (char *)base));
      uint64_t target_vec = 1;
      while(serdes_vec  != 0) {
         uint64_t id = vec2id(target_vec);
         if(serdes_vec & 0x1) {
            printf("target: %32s (%32lx)\n", id2str[id], target_vec);
            SelectPreserve(id, packer);
         }
         target_vec <<= 1;
         serdes_vec >>= 1;
      } // while ends
      printf("------------ Done -----------\n");
      return 0; 
   }
   uint64_t Deserialize(packer::CDPacker &packer, const std::string &entry_name) { 
      packer.Restore(GetCDEntryID("BaseObj"));
      uint64_t target_vec = 1;
      while(serdes_vec  != 0) {
         uint64_t id = vec2id(target_vec);
         if(serdes_vec & 0x1) {
            printf("target: %32s (%32lx)\n", id2str[id], target_vec);
            SelectRestore(id, packer);
         }
         target_vec <<= 1;
         serdes_vec >>= 1;
      } // while ends
      printf("------------ Done -----------\n");
      return 0;
   }

   void SelectPreserve(uint64_t id, packer::CDPacker &packer) {
      switch(id) {
         case ID__X         : m_x.PreserveObject(packer,        "X"        ); break;
         case ID__Y         : m_y.PreserveObject(packer,        "Y"        ); break;
         case ID__Z         : m_z.PreserveObject(packer,        "Z"        ); break;
         case ID__XD        : m_xd.PreserveObject(packer,       "XD"       ); break; 
         case ID__YD        : m_yd.PreserveObject(packer,       "YD"       ); break;
         case ID__ZD        : m_zd.PreserveObject(packer,       "ZD"       ); break;
         case ID__XDD       : m_xdd.PreserveObject(packer,      "XDD"      ); break;
         case ID__YDD       : m_ydd.PreserveObject(packer,      "YDD"      ); break;
         case ID__ZDD       : m_zdd.PreserveObject(packer,      "ZDD"      ); break;
         case ID__FX        : m_fx.PreserveObject(packer,       "FX"       ); break; 
         case ID__FY        : m_fy.PreserveObject(packer,       "FY"       ); break;
         case ID__FZ        : m_fz.PreserveObject(packer,       "FZ"       ); break;
         case ID__NODALMASS : m_nodalMass.PreserveObject(packer,"NODALMASS"); break; 
         case ID__SYMMX     : m_symmX.PreserveObject(packer,    "SYMMX"    ); break;  
         case ID__SYMMY     : m_symmY.PreserveObject(packer,    "SYMMY"    ); break;
         case ID__SYMMZ     : m_symmZ.PreserveObject(packer,    "SYMMZ"    ); break;
         case ID__NODELIST  : m_nodelist.PreserveObject(packer, "NODELIST" ); break;
         case ID__LXIM      : m_lxim.PreserveObject(packer,     "LXIM"     ); break;  
         case ID__LXIP      : m_lxip.PreserveObject(packer,     "LXIP"     ); break;
         case ID__LETAM     : m_letam.PreserveObject(packer,    "LETAM"    ); break;
         case ID__LETAP     : m_letap.PreserveObject(packer,    "LETAP"    ); break;
         case ID__LZETAM    : m_lzetam.PreserveObject(packer,   "LZETAM"   ); break;
         case ID__LZETAP    : m_lzetap.PreserveObject(packer,   "LZETAP"   ); break;
         case ID__ELEMBC    : m_elemBC.PreserveObject(packer,   "ELEMBC"   ); break;
         case ID__DXX       : m_dxx.PreserveObject(packer,      "DXX"      ); break;  
         case ID__DYY       : m_dyy.PreserveObject(packer,      "DYY"      ); break;
         case ID__DZZ       : m_dzz.PreserveObject(packer,      "DZZ"      ); break;
         case ID__DELV_XI   : m_delv_xi.PreserveObject(packer,  "DELV_XI"  ); break; 
         case ID__DELV_ETA  : m_delv_eta.PreserveObject(packer, "DELV_ETA" ); break;
         case ID__DELV_ZETA : m_delv_zeta.PreserveObject(packer,"DELV_ZETA"); break;  
         case ID__DELX_XI   : m_delx_xi.PreserveObject(packer,  "DELX_XI"  ); break; 
         case ID__DELX_ETA  : m_delx_eta.PreserveObject(packer, "DELX_ETA" ); break;
         case ID__DELX_ZETA : m_delx_zeta.PreserveObject(packer,"DELX_ZETA"); break; 
         case ID__E         : m_e.PreserveObject(packer,        "E"        ); break; 
         case ID__P         : m_p.PreserveObject(packer,        "P"        ); break; 
         case ID__Q         : m_q.PreserveObject(packer,        "Q"        ); break; 
         case ID__QL        : m_ql.PreserveObject(packer,       "QL"       ); break;
         case ID__QQ        : m_qq.PreserveObject(packer,       "QQ"       ); break;
         case ID__V         : m_v.PreserveObject(packer,        "V"        ); break; 
         case ID__VOLO      : m_volo.PreserveObject(packer,     "VOLO"     ); break;
         case ID__VNEW      : m_vnew.PreserveObject(packer,     "VNEW"     ); break;
         case ID__DELV      : m_delv.PreserveObject(packer,     "DELV"     ); break;
         case ID__VDOV      : m_vdov.PreserveObject(packer,     "VDOV"     ); break;
         case ID__AREALG    : m_arealg.PreserveObject(packer,   "AREALG"   ); break;  
         case ID__SS        : m_ss.PreserveObject(packer,       "SS"       ); break;      
         case ID__ELEMMASS  : m_elemMass.PreserveObject(packer, "ELEMMASS" ); break;
         case ID__REGELEMSIZE : 
                     packer.Add((char *)m_regElemSize, packer::CDEntry(GetCDEntryID("REGELEMSIZE"), m_numReg, 0, (char *)m_regElemSize));
                     break; 
         case ID__REGNUMLIST  : 
                     packer.Add((char *)m_regNumList, packer::CDEntry(GetCDEntryID("REGNUMLIST"), m_numElem, 0, (char *)m_regNumList)); 
                     break;
         case ID__REGELEMLIST : 
                     packer.Add((char *)m_regElemlist, packer::CDEntry(GetCDEntryID("REGELEMLIST"), m_numReg, 0, (char *)m_regElemlist));
                     for(int i=0; i<m_numReg; i++) {
                        char elemID[32];
                        sprintf(elemID, "REGELEMLIST_%d", i);
                        packer.Add((char *)(m_regElemlist[i]), packer::CDEntry(GetCDEntryID(elemID), regElemSize(i), 0, (char *)(m_regElemlist[i])));
                     }
                     break;
         default: assert(0);
      }
   }

   void SelectRestore(uint64_t id, packer::CDPacker &packer) {
      switch(id) {
         case ID__X         : m_x.Deserialize(packer,        "X"        ); break;
         case ID__Y         : m_y.Deserialize(packer,        "Y"        ); break;
         case ID__Z         : m_z.Deserialize(packer,        "Z"        ); break;
         case ID__XD        : m_xd.Deserialize(packer,       "XD"       ); break; 
         case ID__YD        : m_yd.Deserialize(packer,       "YD"       ); break;
         case ID__ZD        : m_zd.Deserialize(packer,       "ZD"       ); break;
         case ID__XDD       : m_xdd.Deserialize(packer,      "XDD"      ); break;
         case ID__YDD       : m_ydd.Deserialize(packer,      "YDD"      ); break;
         case ID__ZDD       : m_zdd.Deserialize(packer,      "ZDD"      ); break;
         case ID__FX        : m_fx.Deserialize(packer,       "FX"       ); break; 
         case ID__FY        : m_fy.Deserialize(packer,       "FY"       ); break;
         case ID__FZ        : m_fz.Deserialize(packer,       "FZ"       ); break;
         case ID__NODALMASS : m_nodalMass.Deserialize(packer,"NODALMASS"); break; 
         case ID__SYMMX     : m_symmX.Deserialize(packer,    "SYMMX"    ); break;  
         case ID__SYMMY     : m_symmY.Deserialize(packer,    "SYMMY"    ); break;
         case ID__SYMMZ     : m_symmZ.Deserialize(packer,    "SYMMZ"    ); break;
         case ID__NODELIST  : m_nodelist.Deserialize(packer, "NODELIST" ); break;
         case ID__LXIM      : m_lxim.Deserialize(packer,     "LXIM"     ); break;  
         case ID__LXIP      : m_lxip.Deserialize(packer,     "LXIP"     ); break;
         case ID__LETAM     : m_letam.Deserialize(packer,    "LETAM"    ); break;
         case ID__LETAP     : m_letap.Deserialize(packer,    "LETAP"    ); break;
         case ID__LZETAM    : m_lzetam.Deserialize(packer,   "LZETAM"   ); break;
         case ID__LZETAP    : m_lzetap.Deserialize(packer,   "LZETAP"   ); break;
         case ID__ELEMBC    : m_elemBC.Deserialize(packer,   "ELEMBC"   ); break;
         case ID__DXX       : m_dxx.Deserialize(packer,      "DXX"      ); break;  
         case ID__DYY       : m_dyy.Deserialize(packer,      "DYY"      ); break;
         case ID__DZZ       : m_dzz.Deserialize(packer,      "DZZ"      ); break;
         case ID__DELV_XI   : m_delv_xi.Deserialize(packer,  "DELV_XI"  ); break; 
         case ID__DELV_ETA  : m_delv_eta.Deserialize(packer, "DELV_ETA" ); break;
         case ID__DELV_ZETA : m_delv_zeta.Deserialize(packer,"DELV_ZETA"); break;  
         case ID__DELX_XI   : m_delx_xi.Deserialize(packer,  "DELX_XI"  ); break; 
         case ID__DELX_ETA  : m_delx_eta.Deserialize(packer, "DELX_ETA" ); break;
         case ID__DELX_ZETA : m_delx_zeta.Deserialize(packer,"DELX_ZETA"); break; 
         case ID__E         : m_e.Deserialize(packer,        "E"        ); break; 
         case ID__P         : m_p.Deserialize(packer,        "P"        ); break; 
         case ID__Q         : m_q.Deserialize(packer,        "Q"        ); break; 
         case ID__QL        : m_ql.Deserialize(packer,       "QL"       ); break;
         case ID__QQ        : m_qq.Deserialize(packer,       "QQ"       ); break;
         case ID__V         : m_v.Deserialize(packer,        "V"        ); break; 
         case ID__VOLO      : m_volo.Deserialize(packer,     "VOLO"     ); break;
         case ID__VNEW      : m_vnew.Deserialize(packer,     "VNEW"     ); break;
         case ID__DELV      : m_delv.Deserialize(packer,     "DELV"     ); break;
         case ID__VDOV      : m_vdov.Deserialize(packer,     "VDOV"     ); break;
         case ID__AREALG    : m_arealg.Deserialize(packer,   "AREALG"   ); break;  
         case ID__SS        : m_ss.Deserialize(packer,       "SS"       ); break;      
         case ID__ELEMMASS  : m_elemMass.Deserialize(packer, "ELEMMASS" ); break;
         case ID__REGELEMSIZE : 
                     packer.Restore(GetCDEntryID("REGELEMSIZE"));
                     break; 
         case ID__REGNUMLIST  : 
                     packer.Restore(GetCDEntryID("REGNUMLIST"));
                     break;
         case ID__REGELEMLIST : 
                     packer.Restore(GetCDEntryID("REGELEMLIST"));
                     for(int i=0; i<m_numReg; i++) {
                        char elemID[32];
                        sprintf(elemID, "REGELEMLIST_%d", i);
                        packer.Restore(GetCDEntryID(elemID));
                     }
                     break;
         default: assert(0);
      }
   }

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
