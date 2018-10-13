#include "cd_node.h"
#include <iostream>
#include <unistd.h>
#include <mpi.h>
#include "cd.h"
using namespace cd;
using namespace synthesizer;

std::default_random_engine CDNodeInfo::generator;

std::map<std::string, CommType> CDNodeInfo::map2id = { 
  { "blocking",    kBlockingP2P   },
  { "nonblocking", kNonBlockingP2P},
  { "reduction",   kReduction     },
  { "sendrecv",    kSendRecv      },
  { "GlobakDisk",   kGlobalDisk   },
  { "LocalDisk",    kLocalDisk    },
  { "RemoteMemory", kRemoteMemory },
  { "LocalMemory",  kLocalMemory  },
};

CommType commtype2id(const std::string &str) 
{
  if (str.compare("blocking") == 0) {
    return kBlockingP2P;
  } else if (str.compare("nonblocking") == 0) {
    return kNonBlockingP2P;
  } else if (str.compare("reduction") == 0) {
    return kReduction;
  } else if (str.compare("sendrecv") == 0) {
    return kSendRecv;
  } else {
    return kUndefinedCommType;
  }
};

CDNodeInfo::CDNodeInfo(const Param &param) 
    : exec_time_avg_(param["exec time"].asDouble())
    , exec_time_std_(param["exec time std"].asDouble())
    , fault_rate_(param["fault rate"].asDouble())
    , prsv_vol_(param["preserve vol"].asDouble())
    , comm_payload_(param["comm payload"].asDouble()) 
    , error_vector_(param["error vector"].asUInt64()) 
    , prsv_type_(param["prsv type"].asString())
    , comm_type_(param["comm type"].asString())
    , distribution_(exec_time_avg_, exec_time_std_)
{ 
  SYNO(std::cout << "cons : ";)
  if (param.isMember("cons")) {
    for (auto varname : param["cons"].getMemberNames()) {
      double sz = param["cons"][varname].asUInt64();
      cons_.push_back( new MemoryInfo(malloc(sz), sz, varname) );
      SYNO(std::cout << varname << " : " << sz << std::endl;)
    }
  }
  SYNO(std::cout << "\nprod : ";)
  if (param.isMember("prod")) {
    for (auto varname : param["prod"].getMemberNames()) {
      double sz = param["prod"][varname].asUInt64(); 
      prod_.push_back( new MemoryInfo(malloc(sz), sz, varname) );
      SYNO(std::cout << varname << " : " << sz << std::endl;)
    }
  }
  SYNO(std::cout << "\n";)

  SYN_PRINT("CDNodeInfo created\n");
}

CDNodeInfo::~CDNodeInfo(void)
{
  for (auto &cons : cons_) { 
    //std::cout << cd::myTaskID << ", ptr: " << cons->ptr_ << ", size: " << cons->size_ << ", name:" << cons->name_ << std::endl;
    //std::cout.flush();
    delete cons; 
  }
  for (auto &prod : prod_) { 
    //std::cout << cd::myTaskID <<  ", ptr: " << prod->ptr_ << ", size: " << prod->size_ << ", name:" << prod->name_ << std::endl;
    //std::cout.flush();
    delete prod; 
  }
}

double CDNodeInfo::GetExecTime(void) { 
  const double exec_time = distribution_(generator); 
  if (exec_time > 0) 
    return exec_time;
  else
    return 0.000001;
}

void CDNodeInfo::Print(void) 
{
  SYN_PRINT("*************************************\n"
         "%lf %lf %lf %lf %lf %x %s %s (%lf)\n" 
         "*************************************\n"
         , exec_time_avg_
         , exec_time_std_
         , fault_rate_
         , prsv_vol_
         , comm_payload_
         , error_vector_
         , prsv_type_.c_str()
         , comm_type_.c_str()
         , GetExecTime()
         );

}

void CDNode::ComputePre() 
{
  const unsigned long compute_time = info_.GetExecTime() * 1000000;
  SYNO(std::cout << std::string(level_ << 1, ' ') << "Compute Pre " << compute_time << std::endl;)
  usleep(compute_time);
}

void CDNode::ComputePost() 
{
  SYNO(std::cout << std::string(level_ << 1, ' ') << "Compute Post " << std::endl;)
}

void CDNode::CommPre() 
{
  SYNO(std::cout << std::string(level_ << 1, ' ') << "Comm Pre  ";)
  switch (CDNodeInfo::map2id[info_.comm_type_]) {
    case kBlockingP2P    :
      SYNO(std::cout << "blocking p2p" << std::endl;)
      comm_.Send();
      break;
    case kNonBlockingP2P :
      SYNO(std::cout << "non-blocking p2p" << std::endl;)
      comm_.Irecv();   
      break;
    case kReduction      :
      SYNO(std::cout << "reduction" << std::endl;)
      break;
    case kSendRecv       :
      SYNO(std::cout << "send receive p2p" << std::endl;)
      break;
    default         :
      SYNO(std::cout << "unsupported communication type" << std::endl;)
      break;
  }
}

void CDNode::CommPost() 
{
  SYNO(std::cout << std::string(level_ << 1, ' ') << "Comm Post ";)
  switch (CDNodeInfo::map2id[info_.comm_type_]) {

    case kBlockingP2P    : 
      SYNO(std::cout << "blocking p2p" << std::endl;)
      comm_.Recv();
      break;
    case kNonBlockingP2P :
      SYNO(std::cout << "non-blocking p2p" << std::endl;)
      comm_.Send();   
      comm_.Wait();   
      break;
    case kReduction      :
      SYNO(std::cout << "reduction" << std::endl;)
      comm_.Reduce(); 
      break;
    case kSendRecv       :
      SYNO(std::cout << "send receive p2p" << std::endl;)
      break;
    default         :
      SYNO(std::cout << "unsupported communication type" << std::endl;)
      break;
  }
}

void CDNode::operator()(void) 
{
  Print();
  CDHandle *cdh = GetLeafCD();
  //getchar();
  for (uint32_t i=0; i < iter_; i++) {
    CD_Begin(cdh, label_.c_str());
    uint64_t prv_len = 0;
    for (auto &cons : info_.cons_) {
      //if (cd::myTaskID == 0) std::cout << std::string(level_ << 1, ' ') << label_ << ", ptr: " << cons->ptr_ << ", size: " << cons->size_ << ", name:" << cons->name_ << std::endl;
      prv_len += cdh->Preserve(cons->ptr_, cons->size_, kCopy, cons->name_.c_str());
    }
    CommPre();
    ComputePre();
    if (children_.size() > 0) {
      //if (cd::myTaskID == 0) std::cout << children_[0]->info_.prsv_type_ << "errvec:"<< children_[0]->info_.error_vector_ << std::endl;
      cdh->Create(name_.c_str(), kStrict|CDNodeInfo::map2id[children_[0]->info_.prsv_type_], children_[0]->info_.error_vector_);
    }
    for (auto &c : children_) { (*c)(); }
    if (children_.size() > 0)
      cdh->Destroy();
    ComputePost();
    CommPost();
    SYN_PRINT("%sDone %u-th iteration at %s (comm:%s)\n\n"
        , std::string(level_ << 1, ' ').c_str()
        , i
        , label_.c_str()
        , info_.comm_type_.c_str()
    );
    cdh->Detect();
    cdh->Complete();
  }
}

CDNode::CDNode(Param &&param, const char *name, CDNode *parent, unsigned level)
    : name_(name)
    , info_(param.isMember("profile")? param["profile"] : param)
    , comm_(info_.comm_payload_, CDNodeInfo::map2id[info_.comm_type_], false,
        ((parent != nullptr)? parent->comm_ : AppComm(info_.comm_payload_, 
                                                      param.isMember("comm count")? param["comm count"].asInt() : 1,
                                                      MPI_COMM_WORLD)))
    , parent_(parent)
    , level_(level) 
{
  bool new_format = param.isMember("profile");
  if (new_format) {
    auto &profile = param["profile"];
    iter_ = profile.isMember("iterations") ? profile["iterations"].asUInt64() : 0;
    label_= profile.isMember("label") ? profile["label"].asString() : "no label";
  } else {
    iter_ = param.isMember("iterations") ? param["iterations"].asUInt64() : 0;
    label_= param.isMember("label") ? param["label"].asString() : "no label";
  }
  if (param.isMember("child CDs")) {
    for (auto child_name : param["child CDs"].getMemberNames()) {
      SYN_PRINT("%sParent %s -> %s\n"
          , std::string(level_ << 1, ' ').c_str()
          , name_.c_str()
          , child_name.c_str()
          );
      children_.push_back(new CDNode(Param(param["child CDs"][child_name]), child_name.c_str(), this, level+1));
    }
  } else {
    SYN_PRINT("%sLeaf %s (%s)\n", std::string(level_ << 1, ' ').c_str(), name_.c_str(), label_.c_str());
  }
  //getchar();
}

void CDNode::Print(void) 
{
  SYNO(std::cout << '\n' << std::string(level_ << 1, ' '));
  SYN_PRINT("CDNode:%s (%s), lv:%u, iter:%u\n", name_.c_str(), label_.c_str(), level_, iter_);
  info_.Print();
}
