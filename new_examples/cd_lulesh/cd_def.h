
#define MAX_ID_STR_LEN 32
#define M__INTERNAL    0x0000000000000001
#define M__X           0x0000000000000002
#define M__Y           0x0000000000000004
#define M__Z           0x0000000000000008
#define M__XD          0x0000000000000010
#define M__YD          0x0000000000000020
#define M__ZD          0x0000000000000040
#define M__XDD         0x0000000000000080
#define M__YDD         0x0000000000000100
#define M__ZDD         0x0000000000000200
#define M__FX          0x0000000000000400
#define M__FY          0x0000000000000800
#define M__FZ          0x0000000000001000
#define M__NODALMASS   0x0000000000002000
#define M__SYMMX       0x0000000000004000
#define M__SYMMY       0x0000000000008000
#define M__SYMMZ       0x0000000000010000
#define M__NODELIST    0x0000000000020000
#define M__LXIM        0x0000000000040000
#define M__LXIP        0x0000000000080000
#define M__LETAM       0x0000000000100000
#define M__LETAP       0x0000000000200000
#define M__LZETAM      0x0000000000400000
#define M__LZETAP      0x0000000000800000
#define M__ELEMBC      0x0000000001000000
#define M__DXX         0x0000000002000000
#define M__DYY         0x0000000004000000
#define M__DZZ         0x0000000008000000
#define M__DELV_XI     0x0000000010000000
#define M__DELV_ETA    0x0000000020000000
#define M__DELV_ZETA   0x0000000040000000
#define M__DELX_XI     0x0000000080000000
#define M__DELX_ETA    0x0000000100000000
#define M__DELX_ZETA   0x0000000200000000
#define M__E           0x0000000400000000
#define M__P           0x0000000800000000
#define M__Q           0x0000001000000000
#define M__QL          0x0000002000000000
#define M__QQ          0x0000004000000000
#define M__V           0x0000008000000000
#define M__VOLO        0x0000010000000000
#define M__VNEW        0x0000020000000000
#define M__DELV        0x0000040000000000
#define M__VDOV        0x0000080000000000
#define M__AREALG      0x0000100000000000
#define M__SS          0x0000200000000000
#define M__ELEMMASS    0x0000400000000000
#define M__REGELEMSIZE 0x0000800000000000
#define M__REGNUMLIST  0x0001000000000000
#define M__REGELEMLIST 0x0002000000000000
#define M__COMMBUFSEND 0x0004000000000000
#define M__COMMBUFRECV 0x0008000000000000
#define M__SERDES_ALL  0x000FFFFFFFFFFFFE
#define PRVEC_MASK     0x0003FFFFFFFFFFFE
enum { ID__INVALID  = 0
     , ID__INTERNAL = 1
     , ID__X        = 2
     , ID__Y        
     , ID__Z        
     , ID__XD       
     , ID__YD       
     , ID__ZD       
     , ID__XDD      
     , ID__YDD      
     , ID__ZDD      
     , ID__FX       
     , ID__FY       
     , ID__FZ       
     , ID__NODALMASS
     , ID__SYMMX    
     , ID__SYMMY    
     , ID__SYMMZ    
     , ID__NODELIST 
     , ID__LXIM     
     , ID__LXIP     
     , ID__LETAM    
     , ID__LETAP    
     , ID__LZETAM   
     , ID__LZETAP   
     , ID__ELEMBC   
     , ID__DXX      
     , ID__DYY      
     , ID__DZZ      
     , ID__DELV_XI  
     , ID__DELV_ETA 
     , ID__DELV_ZETA
     , ID__DELX_XI  
     , ID__DELX_ETA 
     , ID__DELX_ZETA
     , ID__E        
     , ID__P        
     , ID__Q        
     , ID__QL       
     , ID__QQ       
     , ID__V        
     , ID__VOLO     
     , ID__VNEW     
     , ID__DELV     
     , ID__VDOV     
     , ID__AREALG   
     , ID__SS       
     , ID__ELEMMASS
     , ID__REGELEMSIZE
     , ID__REGNUMLIST
     , ID__REGELEMLIST
     , ID__COMMBUFSEND
     , ID__COMMBUFRECV
     , TOTAL_IDS 
};


extern char id2str[TOTAL_IDS][MAX_ID_STR_LEN];
//= {
//      "INVALID" 
//    , "INTERNAL"
//    , "X"
//    , "Y"
//    , "Z"
//    , "XD"
//    , "YD"
//    , "ZD"
//    , "XDD"
//    , "YDD"
//    , "ZDD"
//    , "FX"
//    , "FY"
//    , "FZ"
//    , "NODALMASS"
//    , "SYMMX"
//    , "SYMMY"
//    , "SYMMZ"
//    , "NODELIST"
//    , "LXIM"
//    , "LXIP"
//    , "LETAM"
//    , "LETAP"
//    , "LZETAM"
//    , "LZETAP"
//    , "ELEMBC"
//    , "DXX"
//    , "DYY"
//    , "DZZ"
//    , "DELV_XI"
//    , "DELV_ETA"
//    , "DELV_ZETA"
//    , "DELX_XI"
//    , "DELX_ETA"
//    , "DELX_ZETA"
//    , "E"
//    , "P"
//    , "Q"
//    , "QL"
//    , "QQ"
//    , "V"
//    , "VOLO"
//    , "VNEW"
//    , "DELV"
//    , "VDOV"
//    , "AREALG"
//    , "SS"
//    , "ELEMMASS"
//    , "REGELEMSIZE"
//    , "REGNUMLIST"
//    , "REGELEMLIST"
//};

static inline 
unsigned vec2id(unsigned long long n) {
  unsigned cnt=0;
  while(n != 0) {
    n >>= 1;
    cnt++;
  }
  return cnt;
}
