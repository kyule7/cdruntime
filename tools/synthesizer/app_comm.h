#include "cd_syn_common.h"
#include <cassert>
#include <mpi.h>

#define check_flag(X,Y)         (((X) & (Y)) == (Y))
namespace synthesizer {

struct CommStat {
  int *send_buf_;
  int *recv_buf_;
  int rank_;
  MPI_Status stat_;
  MPI_Request mreq_;
  CommStat(void)
    : send_buf_(nullptr)  
    , recv_buf_(nullptr)  
    , rank_(0) 
  { printf("CommStat Default Constructed\n"); }
  CommStat(const CommStat &that)
    : send_buf_(that.send_buf_)  
    , recv_buf_(that.recv_buf_)  
    , rank_(that.rank_) 
  { printf("CommStat Copy Constructed\n"); }
  CommStat(int rank, int count)
    : rank_(rank)
    , send_buf_(new int[count]()) 
    , recv_buf_(new int[count]())
  {
    printf("[%d] create %p %p\n", rank_, send_buf_, recv_buf_);
  }

  ~CommStat(void) 
  { 
    printf("[%d] delete %p %p\n", rank_, send_buf_, recv_buf_);
    delete [] send_buf_;
    delete [] recv_buf_;
  }
};

struct AppComm {
  int  count_;
  std::vector<CommStat *> src_;
  std::vector<CommStat *> dst_;
  int tag_;
  int rank_;
  int size_;
  CommType comm_type_;
  MPI_Datatype datatype_;
  MPI_Comm comm_;
  void Print(const char *func) {
    printf(">> Comm: %s %d %d %zu %zu (Type) %x %x (MPI_INT)\n", func, rank_, size_, src_.size(), dst_.size(), datatype_, MPI_INT);
  }
  void Init(void) {
    // set up src and dst for each rank
    // default: comm b/w i and i+i
    //
    //assert(size_ % 2 == 0);
    int target = (rank_ % 2 == 0) ? rank_ + 1 : rank_ - 1;
    src_.push_back(new CommStat(target, count_));
    dst_.push_back(new CommStat(target, count_));
    printf("AppComm %s\n", __func__); 
  }
  AppComm(void)
    : count_(0)
    , tag_(0)
    , datatype_(MPI_INT)
    , comm_(MPI_COMM_WORLD) 
  { assert(0); }

  AppComm(const AppComm &that) 
  {
    count_ =     that.count_;
    tag_   =     that.tag_;
    rank_  =     that.rank_;
    size_  =     that.size_;
    comm_type_ = that.comm_type_;
    datatype_  = that.datatype_;
    comm_  =     that.comm_;
    printf("datatype:%d\n", that.datatype_);
    that.Print("that"); Print("Copy"); 
    //getchar();
  }
  
  AppComm(double payload, const MPI_Comm &comm) 
    : count_(static_cast<int>(payload / sizeof(int)))
    , tag_(0)
    , datatype_(MPI_INT)
    , comm_(comm) 
  {
    MPI_Comm_rank(comm_, &rank_);
    MPI_Comm_size(comm_, &size_);
    Init();
  }

  AppComm(double payload, CommType comm_type, bool split, const AppComm &parent)
    : count_(static_cast<int>(payload / sizeof(int)))
    , comm_type_(comm_type)
  {
    if (split) {
    //if (check_flag(comm_type, kNewComm)) {
//      MPI_Comm_split(parent.comm_, group_id, id_in_group, &comm_);
//      MPI_Comm_rank(comm_, &rank_);
//      MPI_Comm_size(comm_, &size_);
      
    } else {
      comm_ = parent.comm_;
      rank_ = parent.rank_;
      size_ = parent.size_;
    }
    tag_   =     parent.tag_;
    comm_type_ = parent.comm_type_;
    datatype_  = parent.datatype_;
    Init();
    Print("Split"); 
    //getchar();
  }

  int Send(void)   
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (auto &dst : dst_) { ret = MPI_Send(dst->send_buf_, count_, datatype_, dst->rank_, tag_, comm_); }
    } else {
      printf("count is zero\n"); assert(0);
    }
    return ret;
  }

  int Recv(void)   
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (auto &src : src_) { ret = MPI_Recv(src->recv_buf_, count_, datatype_, src->rank_, tag_, comm_, &src->stat_); }
    }
    return ret;
  }

  int Isend(void)  
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (auto &dst : dst_) { ret = MPI_Isend(dst->send_buf_, count_, datatype_, dst->rank_, tag_, comm_, &dst->mreq_); }
    }
    return ret;
  }

  int Irecv(void)  
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (auto &src : src_) { ret = MPI_Irecv(src->recv_buf_, count_, datatype_, src->rank_, tag_, comm_, &src->mreq_); }
    }
    return ret;
  }

  int Reduce(void) 
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      ret = MPI_Reduce(dst_[0]->send_buf_, src_[0]->recv_buf_, count_, datatype_, MPI_SUM, 0, comm_); 
    }
    return ret;
  }

  int Wait(bool for_recv=true)   
  {
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      if (for_recv) {
        for (auto &src : src_) { ret = MPI_Wait(&src->mreq_, &src->stat_); }
      } else {
        for (auto &dst : dst_) { ret = MPI_Wait(&dst->mreq_, &dst->stat_); }
      }
    }
    return ret;
  }

};

}
