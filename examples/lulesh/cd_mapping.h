// SWITCH_X_Y_Z 
// X : function depth
// Y : function ID at the function depth level
// Z : phase ID in the same function scope
//
// LOOP_X_Y_Z
// This decides Create() or not.
//
// Rule 1. Before calling function, complete the current CD.
// Because CD should begin and complete at the same scope.
// This rule implicitly means that the beginning of function,
// the leaf CD state is inactive (idle).
// To enable this, CD runtime allows Create CDs at inactive state.
//
// Basic mapping strategy is that
//   1. Create a CD level per function call. (Create/Begin/Complete/Destry)
//   2. No benefit, Not create but Begin/Complete reusing the same CD object.
//   3. No benefit, do not map the scope as a CD. (Merged)
// 
// For mapping a CD for a function,
//   1. BEG_CDMAPPING_FUNC at the beginning of function
//   2. END_CDMAPPING_FUNC at the end of function
//
//   Foo(...) {
//     CDBegin()
//     ...
//     ...
//     CreateAndBegin();
//     CompleteBegin();
//     {Nothing}
//     Boo(...);
//     ...
//     CDComplete();
//   }
//
//   Boo(...) {
//     CDBegin()                  // Complete and Begin or nothing or Create and Begin
//
//
//     CDComplete();
//   }
//
//
// For mapping a CD for a loop,
//   1. BEG_CDMAPPING_LOOP before the loop
//   2. END_CDMAPPING_LOOP(i) after the loop
//   3. BEG_CDMAPPING_LOOP(i) at the beginning of loop body
//   4. END_CDMAPPING_LOOP at the end of loop body
// User can control Begin/Complete interval by i
//
// BEG_CDMAPPING_LOOP                  // Create&Begin or Complete/Begin or nothing
// while() { 
//   ...
//   ... Loop Body
//   ...
//   SPLIT_CDMAPPING()
//                      // Complete
//                      // Begin
// }
// END_CDMAPPING_LOOP                  // Complete&Destroy or Complete or nothing
//
// For mapping a CD for different phases,
//   1. BEG_CDMAPPING_TCH_1_0                  // LagrangeLeapFrog
//
//
//
// A(...) {
//   CD_BEGIN
//   ...
//   ...
//
//   Complete
//                    // CD_Create
//   B(...)
//                    // CD_Destroy
//   Begin
//
//   ...
//   ...
//   CD_END
// }
//
// B(...) {
//   CD_Begin                  // Means Complete or Complete+Create
//
//
//
//   CD_Complete
// }

//#define CDFLAG_SIZE 6
//#define CDFLAG_MASK 0x3F

//#define CDMAPPING_BEGIN_NESTED(SWITCH, CDH, FUNC_NAME, SERDES_OPS, CD_TYPE) \
//  CDH = GetCurrentCD()->Create(SWITCH >> CDFLAG_SIZE, \
//                                  (string(FUNC_NAME)+GetCurrentCD()->node_id()->GetStringID()).c_str(), \
//                                   SWITCH & CDFLAG_MASK); \
//  CD_Begin(CDH, false, FUNC_NAME); \
//  CDH->Preserve(&domain, sizeof(domain), CD_TYPE, (string("domain_")+string(FUNC_NAME)).c_str()); \
//  CDH->Preserve(domain.serdes.SetOp(SERDES_OPS), CD_TYPE, (string("AllMembers_")+string(FUNC_NAME)).c_str()); 
//
//#define CDMAPPING_BEGIN_SEQUENTIAL(SWITCH, CDH, FUNC_NAME, SERDES_OPS, CD_TYPE) \
//  CD_Complete(CDH); \
//  CD_Begin(CDH, false, FUNC_NAME); \
//  CDH->Preserve(&domain, sizeof(domain), CD_TYPE, (string("domain_")+string(FUNC_NAME)).c_str()); \
//  CDH->Preserve(domain.serdes.SetOp(SERDES_OPS), CD_TYPE, (string("AllMembers_")+string(FUNC_NAME)).c_str()); 

//#define CDMAPPING_BEGIN_NESTED(SWITCH, CDH, FUNC_NAME, SERDES_OPS, CD_TYPE) \
//  CDH = GetCurrentCD()->Create(SWITCH >> CDFLAG_SIZE, \
//                                  (string(FUNC_NAME)+GetCurrentCD()->node_id()->GetStringID()).c_str(), \
//                                   SWITCH & CDFLAG_MASK); \
//  CD_Begin(CDH, false, FUNC_NAME); \
//  CDH->Preserve(&domain, sizeof(domain), domain.serdes.SetOp(SERDES_OPS), CD_TYPE, (string("AllMembers_")+string(FUNC_NAME)).c_str()); 
//
//#define CDMAPPING_BEGIN_SEQUENTIAL(SWITCH, CDH, FUNC_NAME, SERDES_OPS, CD_TYPE) \
//  CD_Complete(CDH); \
//  CD_Begin(CDH, false, FUNC_NAME); \
//  CDH->Preserve(&domain, sizeof(domain), domain.serdes.SetOp(SERDES_OPS), CD_TYPE, (string("AllMembers_")+string(FUNC_NAME)).c_str()); 
//
//#define CDMAPPING_END_NESTED(SWITCH, CDH) \
//   CDH->Detect(); \
//   CD_Complete(CDH); \
//   CDH->Destroy(); 

#if _CD
// Comm at main
#define SEQUENTIAL_CD  1
#define NESTED_CD_EX   2
#define NESTED_CD      3
#define SWITCH_PRESERVE_INIT 1

#define ERROR_VEC_0 0x0F00
#define ERROR_VEC_1 0x0E00
#define ERROR_VEC_2 0x0C00
#define ERROR_VEC_3 0x0800
//#define ERROR_VEC_4 0x0F00
//#define ERROR_VEC_5 0x0700
//#define ERROR_VEC_6 0x0300
//#define ERROR_VEC_7 0x0100

//#define SWITCH_0_0_0   (8<<CDFLAG_SIZE) | kStrict  | kDRAM              // Main Loop
#define SWITCH_0_0_0  3                                                   // Main Loop
#define CD_MAP_0_0_0  ((1<<CDFLAG_SIZE) | kStrict  | kHDD  | ERROR_VEC_0) // 
#define SWITCH_1_0_0  3                                                   // TimeIncrement
#define CD_MAP_1_0_0  ((1<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //
#define SWITCH_1_2_0  0                                                   // LagrangeLeapFrog
#define CD_MAP_1_2_0  ((1<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //
#define SWITCH_2_0_0  3                                                   //       LagrangeNodal
#define CD_MAP_2_0_0  ((1<<CDFLAG_SIZE) | kStrict  | kHDD  | ERROR_VEC_0) //   
//                    *************************************************** CommRecvSBN ***************************************************** 
#define SWITCH_3_0_0  3                                                   //           CalcForceForNodes 
#define CD_MAP_3_0_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_1) //
#define SWITCH_4_0_0  0                                                   //               CalcVolumeForceForElems ** MAP FINEGRAINED
#define CD_MAP_4_0_0  ((1<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_2) //                 # O{fx,fy,xz}, I{nodelist,x,y,z}  
#define SWITCH_5_0_0  0                                                   //                   InitStressTermsForElems
#define CD_MAP_5_0_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{sigxx,yy,zz}, I{nodelist,x,y,z}
#define SWITCH_5_1_0  0                                                   //                   IntegrateStressForElems
#define CD_MAP_5_1_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{fx,fy,fz}, I{sigxx,yy,zz,nodelist}
#define SWITCH_5_1_1  0                                                   //                       Loop_IntegrateStressForElems
#define CD_MAP_5_1_1  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{}, I{B}
#define SWITCH_6_0_0  0                                                   //                       CollectDomainNodesToElemNodes ()
#define CD_MAP_6_0_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{x_loc,y_loc,z_loc}, I{x,y,z}
#define SWITCH_6_1_0  0                                                   //                       CalcElemShapeFunctionDerivatives (loop)
#define CD_MAP_6_1_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{B,determ}, I{}
#define SWITCH_6_2_0  0                                                   //                       CalcElemNodeNormals (loop)
#define CD_MAP_6_2_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{B}, I{x_loc,..}
#define SWITCH_6_3_0  0                                                   //                       SumElemStressesToNodeForces (loop)
#define CD_MAP_6_3_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{fx,fy,fz}, I{}
#define SWITCH_5_2_0  0                                                   //                   CalcHourglassControlForElems 
#define CD_MAP_5_2_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{determ,hgcoef}, I{x_loc,y_loc,z_loc} 
#define SWITCH_6_4_0  0                                                   //                       Loop_CalcHourglassControlForElems
#define CD_MAP_6_4_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //             
#define SWITCH_6_5_0  0                                                   //                       CollectDomainNodesToElemNodes
#define CD_MAP_6_5_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{x1,y1,z1}, I{x,y,z} 
#define SWITCH_6_6_0  0                                                   //                       CalcElemVolumeDerivative
#define CD_MAP_6_6_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{dvdx,..,x8n,..,determ}, I{x1,y1,z1}
#define SWITCH_6_7_0  0                                                   //                       CalcFBHourglassForceForElems
#define CD_MAP_6_7_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{hourgram}, I{dvdx,..,x8n,..,determ} 
#define SWITCH_7_0_0  0                                                   //                           Loop_CalcFBHourglassForceForElems
#define CD_MAP_7_0_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //              
#define SWITCH_7_1_0  0                                                   //                           CalcElemFBHourglassForce
#define CD_MAP_7_1_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                             # O{fx,fy,fz}, I{hourgram} 
//                    *************************************************** CommSendSBN && CommSBN && CommRecvSYNC **************************  
#define SWITCH_3_1_0  3                                                   //           CalcAccelerationForNodes ** MAP FINEGRAINED 
#define CD_MAP_3_1_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_1) //             # O{xdd,ydd,zdd}, I{fx,fy,fz}
#define SWITCH_3_2_0  0                                                   //           ApplyAccelerationBoundaryConditionsForNodes 
#define CD_MAP_3_2_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //             # O{xdd,ydd,zdd}, I{symmX,symmY,symmZ}
#define SWITCH_3_3_0  0                                                   //           CalcVelocityForNodes 
#define CD_MAP_3_3_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //             # O{xd,yd,zd}, I{xd,yd,zd,xdd,ydd,zdd,deltaT}
#define SWITCH_3_4_0  0                                                   //           CalcPositionForNodes 
#define CD_MAP_3_4_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //             # O{x,y,z}, I{xd,yd,zd,deltaT}
//                    *************************************************** CommSendSYNC && CommSyncPosVel **********************************
#define SWITCH_2_1_0  3                                                   //       LagrangeElements ** MAP FINEGRAINED
#define CD_MAP_2_1_0  ((1<<CDFLAG_SIZE) | kStrict  | kHDD  | ERROR_VEC_0) //# O{vnew,delv,arealg,vdov,dxx,...},I{deltaT,volo,v,x,y,z,xd,dxx,.} 
#define SWITCH_3_5_0  3                                                   //           CalcLagrangeElements
#define CD_MAP_3_5_0  ((1<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_2) //# O{vnew,delv,arealg,vdov,dxx,...},I{deltaT,volo,v,x,y,z,xd,dxx,.}
#define SWITCH_4_1_0  0                                                   //               CalcKinematicsForElems
#define CD_MAP_4_1_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                 # O{dxx,dyy,dzz,delv,vnew}, I{deltaT,volo,v,x,y,z}
#define SWITCH_5_4_0  0                                                   //                   CollectDomainNodesToElemNodes
#define CD_MAP_5_4_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{x_loc,y_loc,z_loc}, I{x,y,z} 
#define SWITCH_5_5_0  0                                                   //                   CalcElemVolume
#define CD_MAP_5_5_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{delv,vnew}, I{v,volo,xloc,..}
#define SWITCH_5_6_0  0                                                   //                   CalcElemCharacteristicLength
#define CD_MAP_5_6_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{arealg}, I{x_loc,y_loc,z_loc}
#define SWITCH_5_7_0  0                                                   //                   InnerLoopInCalcKinematicsForElems
#define CD_MAP_5_7_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{x_loc,y_loc,z_loc}, I{xd,yd,zd,deltaT} 
#define SWITCH_5_8_0  0                                                   //                   CalcElemShapeFunctionDerivatives
#define CD_MAP_5_8_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{B,detJ}, I{x_loc,y_loc,z_loc}
#define SWITCH_5_9_0  0                                                   //                   CalcElemVelocityGradient
#define CD_MAP_5_9_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{D}, I{xd,yd,zd,B,detJ}
#define SWITCH_4_2_0  0                                                   //               LoopInCalcLagrangeElements
#define CD_MAP_4_2_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                 # O{vdov,dxx,dyy,dzz}, I{dxx,dyy,dzz,D}
#define SWITCH_3_6_0  0                                                   //           CalcQForElems 
#define CD_MAP_3_6_0  ((1<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_1) //             # O{delx_zeta,delv_xi,delx_eta,delv_eta}, I{vnew}
//                    *************************************************** CommRecvMONOQ ************************************************* 
#define SWITCH_4_3_0  0                                                   //               CalcMonotonicQGradientsForElems
#define CD_MAP_4_3_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //  # O{delx_zeta,delv_xi,delx_eta,delv_eta}, I{nodelist,x,xd,volo}
//                    *************************************************** CommSendMONOQ && CommMonoQ ************************************
#define SWITCH_4_4_0  0                                                   //               CalcMonotonicQForElems (loop in it)
#define CD_MAP_4_4_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                 # O{qq,ql}, I{numReg,dom,zeta,xi,x_eta,v_eta...}} 
#define SWITCH_5_10_0 0                                                   //                   CalcMonotonicQRegionForElems (loop in it)
#define CD_MAP_5_10_0 ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{qq,ql}, I{dom,zeta,xi,x_eta,v_eta,elemMass,volo,vdov,letam,letap,regElemlist,elemBC,regElemsize,vnew}
#define SWITCH_3_7_0  0                                                   //           ApplyMaterialPropertiesForElems
#define CD_MAP_3_7_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //            # O{vnew,p,e,q,ss}, I{v,numreg,p,e,q,qq,ql,delv}
#define SWITCH_4_5_0  0                                                   //               EvalEOSForElems FIXME
#define CD_MAP_4_5_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                 # O{p,e,q,ss}, I{p,e,q,qq,ql,delv}    
#define SWITCH_5_11_0 0                                                   //                   CalcEnergyForElems (loop)
#define CD_MAP_5_11_0 ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{p_new,e_new,q_new,bvc,pbvc}, I{}
#define SWITCH_6_8_0  0                                                   //                       CalcPressureForElems1
#define CD_MAP_6_8_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{}, I{}  
#define SWITCH_6_9_0  0                                                   //                       CalcPressureForElems2
#define CD_MAP_6_9_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{}, I{}     
#define SWITCH_6_10_0 0                                                   //                       CalcPressureForElems3
#define CD_MAP_6_10_0 ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                         # O{}, I{}   
#define SWITCH_5_12_0 0                                                   //                   CalcSoundSpeedForElems
#define CD_MAP_5_12_0 ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //                     # O{ss}, I{p_new,e_new,bvc,pbvc} 
#define SWITCH_3_8_0  0                                                   //           UpdateVolumesForElems
#define CD_MAP_3_8_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //             # O{v}, I{vnew,v_cut}
//                    *************************************************** CommRecvSYNC && CommSendSYNC ***********************
#define SWITCH_2_2_0  3                                                   //       CalcTimeConstraintsForElems ** MAP FINEGRAINED
#define CD_MAP_2_2_0  ((1<<CDFLAG_SIZE) | kStrict  | kHDD  | ERROR_VEC_0) // # O{dtcourant,dthydro}, I{regElemSize,regElemlist,qqc,ss,vdov,arealg,dvovmax}
#define SWITCH_3_9_0  0                                                   //           CalcCourantConstraintForElems (dtcourant)
#define CD_MAP_3_9_0  ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //              # O{dtcourant}, I{qqc,ss,vdov,arealg}
#define SWITCH_3_10_0 0                                                   //           CalcHydroConstraintForElems (dthydro)
#define CD_MAP_3_10_0 ((8<<CDFLAG_SIZE) | kStrict  | kDRAM | ERROR_VEC_0) //              # O{dthydro}, I{dvovmax,vdov}
//                    *************************************************** CommSyncPosVel *************************************
                                                         
#else
                                                         
#define SWITCH_0_0_0  0
#define SWITCH_1_1_0  0
#define SWITCH_1_2_0  0
#define SWITCH_2_0_0  0
#define SWITCH_3_0_0  0
#define SWITCH_4_0_0  0
#define SWITCH_5_0_0  0
#define SWITCH_5_1_0  0
#define SWITCH_5_1_1  0
#define SWITCH_6_0_0  0
#define SWITCH_6_1_0  0
#define SWITCH_6_2_0  0
#define SWITCH_6_3_0  0
#define SWITCH_5_2_0  0
#define SWITCH_6_4_0  0
#define SWITCH_6_5_0  0
#define SWITCH_6_6_0  0
#define SWITCH_6_7_0  0
#define SWITCH_7_0_0  0
#define SWITCH_7_1_0  0
#define SWITCH_3_1_0  0
#define SWITCH_3_2_0  0
#define SWITCH_3_3_0  0
#define SWITCH_3_4_0  0
#define SWITCH_2_1_0  0
#define SWITCH_3_5_0  0
#define SWITCH_4_1_0  0
#define SWITCH_5_4_0  0
#define SWITCH_5_5_0  0
#define SWITCH_5_6_0  0
#define SWITCH_5_7_0  0
#define SWITCH_5_8_0  0
#define SWITCH_5_9_0  0
#define SWITCH_4_2_0  0
#define SWITCH_3_6_0  0
#define SWITCH_4_3_0  0
#define SWITCH_4_4_0  0
#define SWITCH_5_10_0 0
#define SWITCH_3_7_0  0
#define SWITCH_4_5_0  0
#define SWITCH_5_11_0 0
#define SWITCH_6_8_0  0
#define SWITCH_6_9_0  0
#define SWITCH_6_10_0 0
#define SWITCH_5_12_0 0
#define SWITCH_3_8_0  0
#define SWITCH_2_2_0  0
#define SWITCH_3_9_0  0
#define SWITCH_3_10_0 0

#define CD_MAP_0_0_0  0
#define CD_MAP_1_1_0  0
#define CD_MAP_1_2_0  0
#define CD_MAP_2_0_0  0
#define CD_MAP_3_0_0  0
#define CD_MAP_4_0_0  0
#define CD_MAP_5_0_0  0
#define CD_MAP_5_1_0  0
#define CD_MAP_5_1_1  0
#define CD_MAP_6_0_0  0
#define CD_MAP_6_1_0  0
#define CD_MAP_6_2_0  0
#define CD_MAP_6_3_0  0
#define CD_MAP_5_2_0  0
#define CD_MAP_6_4_0  0
#define CD_MAP_6_5_0  0
#define CD_MAP_6_6_0  0
#define CD_MAP_6_7_0  0
#define CD_MAP_7_0_0  0
#define CD_MAP_7_1_0  0
#define CD_MAP_3_1_0  0
#define CD_MAP_3_2_0  0
#define CD_MAP_3_3_0  0
#define CD_MAP_3_4_0  0
#define CD_MAP_2_1_0  0
#define CD_MAP_3_5_0  0
#define CD_MAP_4_1_0  0
#define CD_MAP_5_4_0  0
#define CD_MAP_5_5_0  0
#define CD_MAP_5_6_0  0
#define CD_MAP_5_7_0  0
#define CD_MAP_5_8_0  0
#define CD_MAP_5_9_0  0
#define CD_MAP_4_2_0  0
#define CD_MAP_3_6_0  0
#define CD_MAP_4_3_0  0
#define CD_MAP_4_4_0  0
#define CD_MAP_5_10_0 0
#define CD_MAP_3_7_0  0
#define CD_MAP_4_5_0  0
#define CD_MAP_5_11_0 0
#define CD_MAP_6_8_0  0
#define CD_MAP_6_9_0  0
#define CD_MAP_6_10_0 0
#define CD_MAP_5_12_0 0
#define CD_MAP_3_8_0  0
#define CD_MAP_2_2_0  0
#define CD_MAP_3_9_0  0
#define CD_MAP_3_10_0 0

#endif
                                                         
                                                         
                                                          
                                                          
                                                          
                                                          
                                                          
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
