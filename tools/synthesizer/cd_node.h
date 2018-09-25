#include "param.h"
#include "app_comm.h"
#include <random>

namespace synthesizer {

struct MemoryInfo {
  void       *ptr_;
  uint64_t    size_;
  std::string name_;
  MemoryInfo(void *ptr, uint64_t size, const std::string &name)
    : ptr_(ptr)
    , size_(size)
    , name_(name) {}
  ~MemoryInfo(void) { free(ptr_); }    
};

struct CDNodeInfo {
  double exec_time_avg_;
  double exec_time_std_;
  double fault_rate_;
  double prsv_vol_;
  double comm_payload_;
  uint64_t error_vector_;
  std::string prsv_type_;
  std::string comm_type_;
	std::vector<MemoryInfo *> cons_;
	std::vector<MemoryInfo *> prod_;
  static std::map<std::string, CommType> map2id;
  static std::default_random_engine generator;
  std::normal_distribution<double> distribution_;
  CDNodeInfo(const Param &param); 
  ~CDNodeInfo(void);
  double GetExecTime(void);
  void Print(void);
};

CommType commtype2id(const std::string &str);

struct CDNode {
  unsigned iter_;
  std::string label_;
  std::string name_;
  CDNodeInfo info_;
  AppComm comm_;
  CDNode *parent_;
  unsigned level_;
  std::vector<CDNode *> children_;
public:
  
/*******************************************************************
 * Compute body 
 * 
 * Computation time
 * Volume of application state
 * Communication
 * Failure rate (defined in config.yaml?)
 *******************************************************************/
  void ComputePre(void); 
  void ComputePost(void); 
  void CommPre(void); 
  void CommPost(void);
  void operator()(void);
  CDNode(Param &&param, const char *name="noname", CDNode *parent=nullptr, unsigned level=0);
  void Print(void);
};

} // namespace synthesizer ends
