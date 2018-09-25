#include "param.h"
#include "app_comm.h"
#include <random>

namespace synthesizer {

struct CDNodeInfo {
  double exec_time_avg_;
  double exec_time_std_;
  double fault_rate_;
  double prsv_vol_;
  double comm_payload_;
  std::string comm_type_;
  static std::map<std::string, CommType> map2id;
  static std::default_random_engine generator;
  std::normal_distribution<double> distribution_;
  CDNodeInfo(const Param &param); 
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
