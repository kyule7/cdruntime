
namespace synthesizer {

enum CommType {
    kBlockingP2P    = 0 // Recv-Compute-Send
  , kNonBlockingP2P = 1 // Irecv-Compute-Send-Wait
  , kReduction      = 2 // Reduction
  , kSendRecv       = 4 // SendRecv
  , kUndefinedCommType // Undefined
};


}
