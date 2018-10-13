
#ifndef _SYN_COMMON_H 
#define _SYN_COMMON_H 


namespace synthesizer {
extern int numRanks;
extern int myRank;
enum CommType {
    kBlockingP2P    = 64 // Recv-Compute-Send
  , kNonBlockingP2P = 128 // Irecv-Compute-Send-Wait
  , kReduction      = 256 // Reduction
  , kSendRecv       = 512 // SendRecv
  , kLocalMemory=4  
  , kRemoteMemory=8 
  , kLocalDisk=16   
  , kGlobalDisk=32  
  , kUndefinedCommType // Undefined
};


}

#define SYN_PRINT_ONE(...) if (synthesizer::myRank == 0) printf(__VA_ARGS__);

//#define SYN_DEBUG
#undef SYN_DEBUG
#ifndef SYN_DEBUG
  #define SYNO(...)
  #define SYN_PRINT(...) 
#else
  #define SYNO(...) __VA_ARGS__
  #define SYN_PRINT(...) __VA_ARGS__
#endif

#endif
