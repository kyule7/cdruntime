#include "cd_syn_common.h"
#include <cassert>
#include <mpi.h>

#define check_flag(X,Y)         (((X) & (Y)) == (Y))
namespace synthesizer {

struct CommStat {
  int *send_buf_;
  int *recv_buf_;
  int rank_;
  MPI_Status *stat_;
  MPI_Request *mreq_;
  CommStat(void)
    : send_buf_(nullptr)  
    , recv_buf_(nullptr)  
    , rank_(0) 
  { SYN_PRINT("CommStat Default Constructed\n"); }
  CommStat(const CommStat &that)
    : send_buf_(that.send_buf_)  
    , recv_buf_(that.recv_buf_)  
    , rank_(that.rank_) 
    , stat_(that.stat_)  
    , mreq_(that.mreq_) 
  { SYN_PRINT("CommStat Copy Constructed\n"); }
  CommStat(int rank, int count, int tags)
    : rank_(rank)
    , send_buf_(new int[count]()) 
    , recv_buf_(new int[count]())
    , stat_(new MPI_Status[tags])  
    , mreq_(new MPI_Request[tags]) 
  {
    SYN_PRINT("[%d] create %p %p\n", rank_, send_buf_, recv_buf_);
  }

  ~CommStat(void) 
  { 
    SYN_PRINT("[%d] delete %p %p\n", rank_, send_buf_, recv_buf_);
    delete [] send_buf_;
    delete [] recv_buf_;
    delete [] stat_;
    delete [] mreq_;
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
    SYN_PRINT_ONE(">> Comm: %s cnt:%d rank:%d size:%d src:%zu dst:%zu (Type) %x %x (MPI_INT)\n", func, count_, rank_, size_, src_.size(), dst_.size(), datatype_, MPI_INT);
  }
  void Init(void) {
    // set up src and dst for each rank
    // default: comm b/w i and i+i
    //
    //assert(size_ % 2 == 0);
    int target = (rank_ % 2 == 0) ? rank_ + 1 : rank_ - 1;
    src_.push_back(new CommStat(target, count_, tag_));
    dst_.push_back(new CommStat(target, count_, tag_));
    SYN_PRINT("AppComm %s\n", __func__); 
  }

  ~AppComm(void) {
    for (auto &src : src_) delete src;
    for (auto &dst : dst_) delete dst;
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
    SYN_PRINT("datatype:%d\n", that.datatype_);
    that.Print("that"); Print("Copy"); 
    //getchar();
  }
  
  AppComm(double payload, int tags, const MPI_Comm &comm) 
    : count_(static_cast<int>(payload / sizeof(int)))
    , tag_(tags)
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
      for (auto &dst : dst_) { 
        for (int i=0; i<tag_; i++) {
          ret = MPI_Send(dst->send_buf_, count_, datatype_, dst->rank_, i, comm_); } }
    } else {
      SYN_PRINT("count is zero\n"); assert(0);
    }
    return ret;
  }

  int Recv(void)   
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (auto &src : src_) { 
        for (int i=0; i<tag_; i++) {
          ret = MPI_Recv(src->recv_buf_, count_, datatype_, src->rank_, i, comm_, &(src->stat_[i])); } }
    }
    return ret;
  }

  int Isend(void)  
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (auto &dst : dst_) { 
        for (int i=0; i<tag_; i++) {
          ret = MPI_Isend(dst->send_buf_, count_, datatype_, dst->rank_, i, comm_, &(dst->mreq_[i])); } }
    }
    return ret;
  }

  int Irecv(void)  
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (auto &src : src_) { 
        for (int i=0; i<tag_; i++) {
          if (myRank == 0) printf("MPI_Irecv(%p, %d, %d, %d, %d, %lx, %d)\n", 
              src->recv_buf_, count_, datatype_, src->rank_, i, comm_, &(src->mreq_[i])); 

          ret = MPI_Irecv(src->recv_buf_, count_, datatype_, src->rank_, i, comm_, &(src->mreq_[i])); } }
    }
    return ret;
  }

  int Reduce(void) 
  { 
    Print(__func__);
    int ret = 0;
    if (count_ > 0) { 
      for (int i=0; i<tag_; i++) {
        ret = MPI_Reduce(dst_[0]->send_buf_, src_[0]->recv_buf_, count_, datatype_, MPI_SUM, 0, comm_); 
      }
    }
    return ret;
  }

  int Wait(bool for_recv=true)   
  {
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      if (for_recv) {
        for (auto &src : src_) { 
          for (int i=0; i<tag_; i++) {
            ret = MPI_Wait(&(src->mreq_[i]), &(src->stat_[i])); } }
      } else {
        for (auto &dst : dst_) { 
          for (int i=0; i<tag_; i++) {
            ret = MPI_Wait(&(dst->mreq_[i]), &(dst->stat_[i])); } }
      }
    }
    return ret;
  }
  
  int SendRecv(void) 
  {
    Print(__func__);
    int ret = 0;
    if (count_ > 0) {
      for (int i=0; i<src_.size(); i++) {
        ret = MPI_Sendrecv(dst_[0]->send_buf_, count_, datatype_, dst_[0]->rank_, tag_,
                           src_[0]->recv_buf_, count_, datatype_, src_[0]->rank_, tag_, 
                           comm_, &(src_[0]->stat_[i]));
      }
    }
  }
};

}
