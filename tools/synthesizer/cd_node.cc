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
  { "sendrecv",    kSendRecv      }
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
    , comm_type_(param["comm type"].asString())
    , distribution_(exec_time_avg_, exec_time_std_)
{ 
  printf("CDNodeInfo created\n");
}

double CDNodeInfo::GetExecTime(void) { return distribution_(generator); }

void CDNodeInfo::Print(void) 
{
  printf("*************************************\n"
         "%lf %lf %lf %lf %lf %s (%lf)\n" 
         "*************************************\n"
         , exec_time_avg_
         , exec_time_std_
         , fault_rate_
         , prsv_vol_
         , comm_payload_
         , comm_type_.c_str()
         , GetExecTime()
         );

}

void CDNode::ComputePre() 
{
  const unsigned long compute_time = info_.GetExecTime() * 1000000;
  std::cout << std::string(level_ << 1, ' ') << "Compute Pre " << compute_time << std::endl;
  usleep(compute_time);
}

void CDNode::ComputePost() 
{
  std::cout << std::string(level_ << 1, ' ') << "Compute Post " << std::endl;
}

void CDNode::CommPre() 
{
  std::cout << std::string(level_ << 1, ' ') << "Comm Pre  ";
  switch (CDNodeInfo::map2id[info_.comm_type_]) {
    case kBlockingP2P    :
      std::cout << "blocking p2p" << std::endl;
      comm_.Send();
      break;
    case kNonBlockingP2P :
      std::cout << "non-blocking p2p" << std::endl;
      break;
    case kReduction      :
      std::cout << "reduction" << std::endl;
      break;
    case kSendRecv       :
      std::cout << "send receive p2p" << std::endl;
      break;
    default         :
      std::cout << "unsupported communication type" << std::endl;
      break;
  }
}

void CDNode::CommPost() 
{
  std::cout << std::string(level_ << 1, ' ') << "Comm Post ";
  switch (CDNodeInfo::map2id[info_.comm_type_]) {

    case kBlockingP2P    : 
      std::cout << "blocking p2p" << std::endl;
      comm_.Recv();
      break;
    case kNonBlockingP2P :
      std::cout << "non-blocking p2p" << std::endl;
      break;
    case kReduction      :
      std::cout << "reduction" << std::endl;
      break;
    case kSendRecv       :
      std::cout << "send receive p2p" << std::endl;
      break;
    default         :
      std::cout << "unsupported communication type" << std::endl;
      break;
  }
}

void CDNode::operator()(void) 
{
  Print();
  CDHandle *cdh = GetLeafCD();
  CD_Begin(cdh, label_.c_str());
  //getchar();
  for (uint32_t i=0; i < iter_; i++) {
    CommPre();
    ComputePre();
    cdh->Create(name_.c_str(), kStrict|kPFS, 0xF);
    for (auto &c : children_) { (*c)(); }
    cdh->Destroy();
    ComputePost();
    CommPost();
    printf("%sDone %u-th iteration at %s (comm:%s)\n\n"
        , std::string(level_ << 1, ' ').c_str()
        , i
        , label_.c_str()
        , info_.comm_type_.c_str()
    );
  }
  cdh->Detect();
  cdh->Complete();
}

CDNode::CDNode(Param &&param, const char *name, CDNode *parent, unsigned level)
    : iter_((param.isMember("iterations")) ? param["iterations"].asUInt64() : 0)
    , label_((param.isMember("label")) ? param["label"].asString() : "no label")
    , name_(name)
    , info_(param)
    , comm_(info_.comm_payload_, CDNodeInfo::map2id[info_.comm_type_], false,
        ((parent != nullptr)? parent->comm_ : AppComm(info_.comm_payload_, MPI_COMM_WORLD)))
    , parent_(parent)
    , level_(level) {
  if (param.isMember("child CDs")) {
    for (auto child_name : param["child CDs"].getMemberNames()) {
      printf("%sParent %s -> %s\n"
          , std::string(level_ << 1, ' ').c_str()
          , name_.c_str()
          , child_name.c_str()
          );
      children_.push_back(new CDNode(Param(param["child CDs"][child_name]), child_name.c_str(), this, level+1));
    }
  } else {
    printf("%sLeaf %s (%s)\n", std::string(level_ << 1, ' ').c_str(), name_.c_str(), label_.c_str());
  }
  //getchar();
}

void CDNode::Print(void) 
{
  std::cout << '\n' << std::string(level_ << 1, ' ');
  printf("%s (%s)\n", name_.c_str(), label_.c_str());
  info_.Print();
}
