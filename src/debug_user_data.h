typedef double real8 ;
typedef int    Index_t ; // array subscript and loop index
typedef real8  Real_t ;  // floating point representation
typedef int    Int_t ;   // integer representation

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
   void Print(const char *str="", FILE *out=stdout) {
     fprintf(out, "[%s] cycle          :%d \n", str, m_cycle             );               
     fprintf(out, "[%s] dtcourant      :%lf\n", str, m_dtcourant         );               
     fprintf(out, "[%s] dthydro        :%lf\n", str, m_dthydro           );               
     fprintf(out, "[%s] numReg         :%d \n", str, m_numReg            );     
     fprintf(out, "[%s] cost           :%d \n", str, m_cost              );      
     fprintf(out, "[%s] regElemSize    :%p \n", str, m_regElemSize       );               
     fprintf(out, "[%s] regNumList     :%p \n", str, m_regNumList        );               
     fprintf(out, "[%s] regElemlist    :%p \n", str, m_regElemlist       );               
     fprintf(out, "[%s] dtfixed        :%lf\n", str, m_dtfixed           );               
     fprintf(out, "[%s] time           :%lf\n", str, m_time              );               
     fprintf(out, "[%s] deltatime      :%lf\n", str, m_deltatime         );               
     fprintf(out, "[%s] deltatimemultlb:%lf\n", str, m_deltatimemultlb   );               
     fprintf(out, "[%s] deltatimemultub:%lf\n", str, m_deltatimemultub   );               
     fprintf(out, "[%s] dtmax          :%lf\n", str, m_dtmax             );               
     fprintf(out, "[%s] stoptime       :%lf\n", str, m_stoptime          );               
     fprintf(out, "[%s] numRanks       :%d \n", str, m_numRanks          );         
     fprintf(out, "[%s] colLoc         :%d \n", str, m_colLoc            );       
     fprintf(out, "[%s] rowLoc         :%d \n", str, m_rowLoc            );       
     fprintf(out, "[%s] planeLoc       :%d \n", str, m_planeLoc          );         
     fprintf(out, "[%s] tp             :%d \n", str, m_tp                );   
     fprintf(out, "[%s] sizeX          :%d \n", str, m_sizeX             );      
     fprintf(out, "[%s] sizeY          :%d \n", str, m_sizeY             );      
     fprintf(out, "[%s] sizeZ          :%d \n", str, m_sizeZ             );      
     fprintf(out, "[%s] numElem        :%d \n", str, m_numElem           );        
     fprintf(out, "[%s] numNode        :%d \n", str, m_numNode           );        
     fprintf(out, "[%s] maxPlaneSize   :%d \n", str, m_maxPlaneSize      );             
     fprintf(out, "[%s] maxEdgeSize    :%d \n", str, m_maxEdgeSize       );            
     fprintf(out, "[%s] nodeElemStart  :%p \n", str, m_nodeElemStart     );              
     fprintf(out, "[%s] nodeElemCorner :%p \n", str, m_nodeElemCornerList);                   
     fprintf(out, "[%s] rowMin         :%d \n", str, m_rowMin            );                
     fprintf(out, "[%s] rowMax         :%d \n", str, m_rowMax            );                
     fprintf(out, "[%s] colMin         :%d \n", str, m_colMin            );                
     fprintf(out, "[%s] colMax         :%d \n", str, m_colMax            );                
     fprintf(out, "[%s] planeMin       :%d \n", str, m_planeMin          );                    
     fprintf(out, "[%s] planeMax       :%d \n", str, m_planeMax          );                    

   }
   void Print(const Internal &that, const char *str="", FILE *out=stdout) {
     fprintf(out, "[%s] cycle          :%d = %d \n", str, m_cycle             , that.m_cycle                );               
     fprintf(out, "[%s] dtcourant      :%lf= %lf\n", str, m_dtcourant         , that.m_dtcourant            );               
     fprintf(out, "[%s] dthydro        :%lf= %lf\n", str, m_dthydro           , that.m_dthydro              );               
     fprintf(out, "[%s] numReg         :%d = %d \n", str, m_numReg            , that.m_numReg               );     
     fprintf(out, "[%s] cost           :%d = %d \n", str, m_cost              , that.m_cost                 );      
     fprintf(out, "[%s] regElemSize    :%p = %p \n", str, m_regElemSize       , that.m_regElemSize          );               
     fprintf(out, "[%s] regNumList     :%p = %p \n", str, m_regNumList        , that.m_regNumList           );               
     fprintf(out, "[%s] regElemlist    :%p = %p \n", str, m_regElemlist       , that.m_regElemlist          );               
     fprintf(out, "[%s] dtfixed        :%lf= %lf\n", str, m_dtfixed           , that.m_dtfixed              );               
     fprintf(out, "[%s] time           :%lf= %lf\n", str, m_time              , that.m_time                 );               
     fprintf(out, "[%s] deltatime      :%lf= %lf\n", str, m_deltatime         , that.m_deltatime            );               
     fprintf(out, "[%s] deltatimemultlb:%lf= %lf\n", str, m_deltatimemultlb   , that.m_deltatimemultlb      );               
     fprintf(out, "[%s] deltatimemultub:%lf= %lf\n", str, m_deltatimemultub   , that.m_deltatimemultub      );               
     fprintf(out, "[%s] dtmax          :%lf= %lf\n", str, m_dtmax             , that.m_dtmax                );               
     fprintf(out, "[%s] stoptime       :%lf= %lf\n", str, m_stoptime          , that.m_stoptime             );               
     fprintf(out, "[%s] numRanks       :%d = %d \n", str, m_numRanks          , that.m_numRanks             );         
     fprintf(out, "[%s] colLoc         :%d = %d \n", str, m_colLoc            , that.m_colLoc               );       
     fprintf(out, "[%s] rowLoc         :%d = %d \n", str, m_rowLoc            , that.m_rowLoc               );       
     fprintf(out, "[%s] planeLoc       :%d = %d \n", str, m_planeLoc          , that.m_planeLoc             );         
     fprintf(out, "[%s] tp             :%d = %d \n", str, m_tp                , that.m_tp                   );   
     fprintf(out, "[%s] sizeX          :%d = %d \n", str, m_sizeX             , that.m_sizeX                );      
     fprintf(out, "[%s] sizeY          :%d = %d \n", str, m_sizeY             , that.m_sizeY                );      
     fprintf(out, "[%s] sizeZ          :%d = %d \n", str, m_sizeZ             , that.m_sizeZ                );      
     fprintf(out, "[%s] numElem        :%d = %d \n", str, m_numElem           , that.m_numElem              );        
     fprintf(out, "[%s] numNode        :%d = %d \n", str, m_numNode           , that.m_numNode              );        
     fprintf(out, "[%s] maxPlaneSize   :%d = %d \n", str, m_maxPlaneSize      , that.m_maxPlaneSize         );             
     fprintf(out, "[%s] maxEdgeSize    :%d = %d \n", str, m_maxEdgeSize       , that.m_maxEdgeSize          );            
     fprintf(out, "[%s] nodeElemStart  :%p = %p \n", str, m_nodeElemStart     , that.m_nodeElemStart        );              
     fprintf(out, "[%s] nodeElemCorner :%p = %p \n", str, m_nodeElemCornerList, that.m_nodeElemCornerList   );                   
     fprintf(out, "[%s] rowMin         :%d = %d \n", str, m_rowMin            , that.m_rowMin               );                
     fprintf(out, "[%s] rowMax         :%d = %d \n", str, m_rowMax            , that.m_rowMax               );                
     fprintf(out, "[%s] colMin         :%d = %d \n", str, m_colMin            , that.m_colMin               );                
     fprintf(out, "[%s] colMax         :%d = %d \n", str, m_colMax            , that.m_colMax               );                
     fprintf(out, "[%s] planeMin       :%d = %d \n", str, m_planeMin          , that.m_planeMin             );                    
     fprintf(out, "[%s] planeMax       :%d = %d \n", str, m_planeMax          , that.m_planeMax             );                    

   }
};
