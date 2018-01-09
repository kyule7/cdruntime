#ifndef _PROF_ENTRY_HPP
#define _PROF_ENTRY_HPP
#include <ostream>
#include <cstdint>
#include <cmath>
//namespace common {

extern int localTaskID;

struct RTInfoInt {
  uint64_t exec_;
  uint64_t reexec_;
  uint64_t prv_copy_;
  uint64_t prv_ref_;
  uint64_t restore_;
  uint64_t msg_logging_;
  RTInfoInt(void) {
    exec_ = 0;
    reexec_ = 0;
    prv_copy_ = 0;
    prv_ref_ = 0;
    restore_ = 0;
    msg_logging_ = 0;
  }
  RTInfoInt(uint64_t exec,
            uint64_t reexec,
            uint64_t prv_copy,
            uint64_t prv_ref,
            uint64_t restore,
            uint64_t msg_logging) {
    exec_          = exec;
    reexec_        = reexec;
    prv_copy_      = prv_copy;
    prv_ref_       = prv_ref;
    restore_       = restore;
    msg_logging_   = msg_logging;
  }
  RTInfoInt(const RTInfoInt &that) 
  {
    Copy(that);
  }
  void Print(const char *str="", FILE *fp=stdout)
  {
    fprintf(fp, "%s"
                "exec    :%lu\n"
                "reexec  :%lu\n"
                "prv copy:%lu\n"
                "prv ref :%lu\n"
                "restore :%lu\n"
                "msg log :%lu\n",
                str,
                exec_,
                reexec_,
                prv_copy_,
                prv_ref_,
                restore_,
                msg_logging_);
  }
  void Copy(const RTInfoInt &that)
  {
    exec_          = that.exec_;
    reexec_        = that.reexec_;
    prv_copy_      = that.prv_copy_;
    prv_ref_       = that.prv_ref_;
    restore_       = that.restore_;
    msg_logging_   = that.msg_logging_;
  }
};

struct RTInfoFloat {
  double total_time_;
  double reexec_time_;
  double sync_time_;
  double prv_elapsed_time_;
  double rst_elapsed_time_;
  double create_elapsed_time_;
  double destroy_elapsed_time_;
  double begin_elapsed_time_;
  double compl_elapsed_time_;
  double advance_elapsed_time_;
  RTInfoFloat(void)
  {
    total_time_ = 0.0;
    reexec_time_ = 0.0;
    sync_time_ = 0.0;
    prv_elapsed_time_ = 0.0;
    rst_elapsed_time_ = 0.0;
    create_elapsed_time_ = 0.0;
    destroy_elapsed_time_ = 0.0;
    begin_elapsed_time_ = 0.0;
    compl_elapsed_time_ = 0.0;
    advance_elapsed_time_ = 0.0;
  }
  RTInfoFloat(const RTInfoFloat &that)
  { 
    Copy(that);
  }
  RTInfoFloat(double total_time,
              double reexec_time,
              double sync_time,
              double prv_elapsed_time,
              double rst_elapsed_time,
              double create_elapsed_time,
              double destroy_elapsed_time,
              double begin_elapsed_time,
              double compl_elapsed_time,
              double advance_elapsed_time) {
    total_time_              = total_time;          
    reexec_time_             = reexec_time;
    sync_time_               = sync_time;
    prv_elapsed_time_        = prv_elapsed_time;
    rst_elapsed_time_        = rst_elapsed_time;
    create_elapsed_time_     = create_elapsed_time;
    destroy_elapsed_time_    = destroy_elapsed_time;
    begin_elapsed_time_      = begin_elapsed_time;
    compl_elapsed_time_      = compl_elapsed_time;
    advance_elapsed_time_    = advance_elapsed_time;
  }
  void Print(const char *str="", FILE *fp=stdout)
  {
    fprintf(fp, "%s"
                "total_time          :%lf\n"
                "reexec_time         :%lf\n"
                "sync_time           :%lf\n"
                "prv_elapsed_time    :%lf\n"
                "rst_elapsed_time    :%lf\n"
                "create_elapsed_time :%lf\n"
                "destroy_elapsed_time:%lf\n"
                "begin_elapsed_time  :%lf\n"
                "compl_elapsed_time  :%lf\n"
                "advance_elapsed_time:%lf\n",
               str,
               total_time_,
               reexec_time_,
               sync_time_,
               prv_elapsed_time_,
               rst_elapsed_time_,
               create_elapsed_time_,
               destroy_elapsed_time_,
               begin_elapsed_time_,
               compl_elapsed_time_,
               advance_elapsed_time_);
  }
  void Copy(const RTInfoFloat &that) 
  {
    total_time_              = that.total_time_;          
    reexec_time_             = that.reexec_time_;
    sync_time_               = that.sync_time_;
    prv_elapsed_time_        = that.prv_elapsed_time_;
    rst_elapsed_time_        = that.rst_elapsed_time_;
    create_elapsed_time_     = that.create_elapsed_time_;
    destroy_elapsed_time_    = that.destroy_elapsed_time_;
    begin_elapsed_time_      = that.begin_elapsed_time_;
    compl_elapsed_time_      = that.compl_elapsed_time_;
    advance_elapsed_time_    = that.advance_elapsed_time_;

  }
};

struct DoubleInt {
  double val_;
  int    rank_;
  DoubleInt(void) : val_(0.0), rank_(0) {}
  DoubleInt(int rank) : val_(0.0), rank_(rank) {}
  DoubleInt(double val) : val_(val), rank_(localTaskID) {}
  DoubleInt(double val, int rank) : val_(val), rank_(rank) {}
  DoubleInt(const DoubleInt& that) : val_(that.val_), rank_(that.rank_) {}
  DoubleInt &operator=(const int &val)         { val_ = (double)val; rank_ = localTaskID; return *this; }
  DoubleInt &operator=(const double &val)      { val_ = val; rank_ = localTaskID; return *this; }
  DoubleInt &operator=(const DoubleInt& that)  { val_ = that.val_; rank_ = that.rank_; return *this; }
  DoubleInt &operator/=(const DoubleInt& that) { val_ /= that.val_; return *this; }
  DoubleInt &operator*=(const DoubleInt& that) { val_ *= that.val_; return *this; }
  DoubleInt &operator+=(const DoubleInt& that) { val_ += that.val_; return *this; }
  DoubleInt &operator-=(const DoubleInt& that) { val_ -= that.val_; return *this; }
};

//double sqrt(const DoubleInt &that) { return sqrt(that.val_); }
//
//std::ostream &operator<<(std::ostream &os, const DoubleInt &that) 
//{ 
//  return os << that.val_ << " (" << that.rank_ << ')'; 
//}

template <typename T>
struct RTInfo {
  // count
  T exec_;
  T reexec_;
  T prv_copy_;
  T prv_ref_;
  T restore_;
  T msg_logging_;
  // time
  T total_time_;
  T reexec_time_;
  T sync_time_;
  T prv_elapsed_time_;
  T rst_elapsed_time_;
  T create_elapsed_time_;
  T destroy_elapsed_time_;
  T begin_elapsed_time_;
  T compl_elapsed_time_;
  T advance_elapsed_time_;

  RTInfo(void)
  {
    exec_ = 0;
    reexec_ = 0;
    prv_copy_ = 0;
    prv_ref_ = 0;
    restore_ = 0;
    msg_logging_ = 0;
    total_time_ = 0;
    reexec_time_ = 0;
    sync_time_ = 0;
    prv_elapsed_time_ = 0;
    rst_elapsed_time_ = 0;
    create_elapsed_time_ = 0;
    destroy_elapsed_time_ = 0;
    begin_elapsed_time_ = 0;
    compl_elapsed_time_ = 0;
    advance_elapsed_time_ = 0;
  }
//  RTInfo(const RTInfo<T> &that)
//  { 
//    Copy(that);
//  }

  template <typename R>
  RTInfo(const RTInfo<R> &that)
  { 
    Copy(that);
  }

  RTInfo(T exec,
         T reexec,
         T prv_copy,
         T prv_ref,
         T restore,
         T msg_logging,
         T total_time,
         T reexec_time,
         T sync_time,
         T prv_elapsed_time,
         T rst_elapsed_time,
         T create_elapsed_time,
         T destroy_elapsed_time,
         T begin_elapsed_time,
         T compl_elapsed_time,
         T advance_elapsed_time) {
    exec_                    = exec;
    reexec_                  = reexec;
    prv_copy_                = prv_copy;
    prv_ref_                 = prv_ref;
    restore_                 = restore;
    msg_logging_             = msg_logging;
    total_time_              = total_time;          
    reexec_time_             = reexec_time;
    sync_time_               = sync_time;
    prv_elapsed_time_        = prv_elapsed_time;
    rst_elapsed_time_        = rst_elapsed_time;
    create_elapsed_time_     = create_elapsed_time;
    destroy_elapsed_time_    = destroy_elapsed_time;
    begin_elapsed_time_      = begin_elapsed_time;
    compl_elapsed_time_      = compl_elapsed_time;
    advance_elapsed_time_    = advance_elapsed_time;
  }
  
  void Print(std::ostream &os, const char *str="")
  {
    os << str
       << *this
//       << "\n - exec : "                 << exec_
//       << "\n - reexec : "               << reexec_
//       << "\n - prv_copy : "             << prv_copy_
//       << "\n - prv_ref : "              << prv_ref_
//       << "\n - restore : "              << restore_
//       << "\n - msg_logging : "          << msg_logging_
//       << "\n - total_time : "           << total_time_
//       << "\n - reexec_time : "          << reexec_time_
//       << "\n - sync_time : "            << sync_time_
//       << "\n - prv_elapsed_time : "     << prv_elapsed_time_
//       << "\n - rst_elapsed_time : "     << rst_elapsed_time_
//       << "\n - create_elapsed_time : "  << create_elapsed_time_
//       << "\n - destroy_elapsed_time : " << destroy_elapsed_time_
//       << "\n - begin_elapsed_time : "   << begin_elapsed_time_
//       << "\n - compl_elapsed_time : "   << compl_elapsed_time_
//       << "\n - advance_elapsed_time : " << advance_elapsed_time_
       << std::endl;
  }

  template <typename R>
  void Copy(const RTInfo<R> &that) 
  {
    exec_                    = that.exec_;
    reexec_                  = that.reexec_;
    prv_copy_                = that.prv_copy_;
    prv_ref_                 = that.prv_ref_;
    restore_                 = that.restore_;
    msg_logging_             = that.msg_logging_;
    total_time_              = that.total_time_;          
    reexec_time_             = that.reexec_time_;
    sync_time_               = that.sync_time_;
    prv_elapsed_time_        = that.prv_elapsed_time_;
    rst_elapsed_time_        = that.rst_elapsed_time_;
    create_elapsed_time_     = that.create_elapsed_time_;
    destroy_elapsed_time_    = that.destroy_elapsed_time_;
    begin_elapsed_time_      = that.begin_elapsed_time_;
    compl_elapsed_time_      = that.compl_elapsed_time_;
    advance_elapsed_time_    = that.advance_elapsed_time_;
  }

  void Divide(double div)
  {
    exec_                   /=  div;
    reexec_                 /=  div;
    prv_copy_               /=  div;
    prv_ref_                /=  div;
    restore_                /=  div;
    msg_logging_            /=  div;
    total_time_             /=  div;
    reexec_time_            /=  div;
    sync_time_              /=  div;
    prv_elapsed_time_       /=  div;
    rst_elapsed_time_       /=  div;
    create_elapsed_time_    /=  div;
    destroy_elapsed_time_   /=  div;
    begin_elapsed_time_     /=  div;
    compl_elapsed_time_     /=  div;
    advance_elapsed_time_   /=  div;
  }

  void DoSq(void)
  {
    exec_                   *=  exec_                ;
    reexec_                 *=  reexec_              ;
    prv_copy_               *=  prv_copy_            ;
    prv_ref_                *=  prv_ref_             ;
    restore_                *=  restore_             ;
    msg_logging_            *=  msg_logging_         ;
    total_time_             *=  total_time_          ;
    reexec_time_            *=  reexec_time_         ;
    sync_time_              *=  sync_time_           ;
    prv_elapsed_time_       *=  prv_elapsed_time_    ;
    rst_elapsed_time_       *=  rst_elapsed_time_    ;
    create_elapsed_time_    *=  create_elapsed_time_ ;
    destroy_elapsed_time_   *=  destroy_elapsed_time_;
    begin_elapsed_time_     *=  begin_elapsed_time_  ;
    compl_elapsed_time_     *=  compl_elapsed_time_  ;
    advance_elapsed_time_   *=  advance_elapsed_time_;
  }

  void DoSqrt(void)
  {
    exec_                 = sqrt(exec_                 );
    reexec_               = sqrt(reexec_               );
    prv_copy_             = sqrt(prv_copy_             );
    prv_ref_              = sqrt(prv_ref_              );
    restore_              = sqrt(restore_              );
    msg_logging_          = sqrt(msg_logging_          );
    total_time_           = sqrt(total_time_           );
    reexec_time_          = sqrt(reexec_time_          );
    sync_time_            = sqrt(sync_time_            );
    prv_elapsed_time_     = sqrt(prv_elapsed_time_     );
    rst_elapsed_time_     = sqrt(rst_elapsed_time_     );
    create_elapsed_time_  = sqrt(create_elapsed_time_  );
    destroy_elapsed_time_ = sqrt(destroy_elapsed_time_ );
    begin_elapsed_time_   = sqrt(begin_elapsed_time_   );
    compl_elapsed_time_   = sqrt(compl_elapsed_time_   );
    advance_elapsed_time_ = sqrt(advance_elapsed_time_ );
  }

  RTInfo<T> &operator-=(const RTInfo<T> &that) 
  {
    exec_                   -= that.exec_;
    reexec_                 -= that.reexec_;
    prv_copy_               -= that.prv_copy_;
    prv_ref_                -= that.prv_ref_;
    restore_                -= that.restore_;
    msg_logging_            -= that.msg_logging_;
    total_time_             -= that.total_time_;          
    reexec_time_            -= that.reexec_time_;
    sync_time_              -= that.sync_time_;
    prv_elapsed_time_       -= that.prv_elapsed_time_;
    rst_elapsed_time_       -= that.rst_elapsed_time_;
    create_elapsed_time_    -= that.create_elapsed_time_;
    destroy_elapsed_time_   -= that.destroy_elapsed_time_;
    begin_elapsed_time_     -= that.begin_elapsed_time_;
    compl_elapsed_time_     -= that.compl_elapsed_time_;
    advance_elapsed_time_   -= that.advance_elapsed_time_;
    return *this;
  }

  RTInfo<T> &operator+=(const RTInfo<T> &that) 
  {
    exec_                   += that.exec_;
    reexec_                 += that.reexec_;
    prv_copy_               += that.prv_copy_;
    prv_ref_                += that.prv_ref_;
    restore_                += that.restore_;
    msg_logging_            += that.msg_logging_;
    total_time_             += that.total_time_;          
    reexec_time_            += that.reexec_time_;
    sync_time_              += that.sync_time_;
    prv_elapsed_time_       += that.prv_elapsed_time_;
    rst_elapsed_time_       += that.rst_elapsed_time_;
    create_elapsed_time_    += that.create_elapsed_time_;
    destroy_elapsed_time_   += that.destroy_elapsed_time_;
    begin_elapsed_time_     += that.begin_elapsed_time_;
    compl_elapsed_time_     += that.compl_elapsed_time_;
    advance_elapsed_time_   += that.advance_elapsed_time_;
    return *this;
  }

  size_t Length(void) { return sizeof(RTInfo<T>) / sizeof(T); }
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const RTInfo<T> &info)
{
  os << "\n - exec : "                 << info.exec_
     << "\n - reexec : "               << info.reexec_
     << "\n - prv_copy : "             << info.prv_copy_
     << "\n - prv_ref : "              << info.prv_ref_
     << "\n - restore : "              << info.restore_
     << "\n - msg_logging : "          << info.msg_logging_
     << "\n - total_time : "           << info.total_time_
     << "\n - reexec_time : "          << info.reexec_time_
     << "\n - sync_time : "            << info.sync_time_
     << "\n - prv_elapsed_time : "     << info.prv_elapsed_time_
     << "\n - rst_elapsed_time : "     << info.rst_elapsed_time_
     << "\n - create_elapsed_time : "  << info.create_elapsed_time_
     << "\n - destroy_elapsed_time : " << info.destroy_elapsed_time_
     << "\n - begin_elapsed_time : "   << info.begin_elapsed_time_
     << "\n - compl_elapsed_time : "   << info.compl_elapsed_time_
     << "\n - advance_elapsed_time : " << info.advance_elapsed_time_
     << std::endl;
  return os;
}

//} // namespace common ends

double sqrt(const DoubleInt &that);
std::ostream &operator<<(std::ostream &os, const DoubleInt &that);

#endif
