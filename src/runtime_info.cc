#include "runtime_info.h"
using namespace std;
using namespace common;
string RuntimeInfo::GetString(void)
{

  return (GetRTInfoStr()
          + std::string("-- CD Overhead -----------\n")
          + GetOverheadStr());
}

string RuntimeInfo::GetRTInfoStr(int cnt)
{
  char stringout[512];
  string indent(cnt<<1, ' ');
  snprintf(stringout, 512, 
                    "%sexec_cnt :%8lu\n"
                    "%sreex_cnt :%8lu\n"
                    "%stot_time :%8.3lf # [s]\n"
                    "%sreex_time:%8.3lf # [s]\n"
                    "%swait_time:%8.3lf # [s]\n"
                    "%svol_copy :%8lu # [B]\n"
                    "%svol_refer:%8lu # [B]\n"
                    "%scomm_log :%8lu # [B]\n"
                    "%serror_vec:0x%lx\n",
                    indent.c_str(), total_exec_,
                    indent.c_str(), reexec_,
                    indent.c_str(), total_time_,
                    indent.c_str(), reexec_time_,
                    indent.c_str(), sync_time_,
                    indent.c_str(), prv_copy_,
                    indent.c_str(), prv_ref_,
                    indent.c_str(), msg_logging_,
                    indent.c_str(), sys_err_vec_);
  return string(stringout);
}

void RuntimeInfo::Print(void)
{
  printf("%s", GetString().c_str());
}

string CDOverhead::GetOverheadStr(void)
{
  char stringout[512];
  snprintf(stringout, 512, 
                    "preserve :%8.3lf # [s]\n"
                    "create   :%8.3lf # [s]\n"
                    "destroy  :%8.3lf # [s]\n"
                    "begin    :%8.3lf # [s]\n"
                    "complete :%8.3lf # [s]\n", 
                    prv_elapsed_time_, 
                    create_elapsed_time_,
                    destroy_elapsed_time_,
                    begin_elapsed_time_,
                    compl_elapsed_time_ 
          );
  return string(stringout);
}

void CDOverhead::Print(void)
{
  printf("%s", GetString().c_str());
}

string CDOverheadVar::GetOverheadVarStr(void)
{
  char stringout[512];
  snprintf(stringout, 512, 
                    "preserve:%8.3lf # [s] (var:%8.3lf)\n"
                    "create  :%8.3lf # [s] (var:%8.3lf)\n"
                    "destroy :%8.3lf # [s] (var:%8.3lf)\n"
                    "begin   :%8.3lf # [s] (var:%8.3lf)\n"
                    "complete:%8.3lf # [s] (var:%8.3lf)\n", 
                    prv_elapsed_time_,     prv_elapsed_time_var_, 
                    create_elapsed_time_,  create_elapsed_time_var_,
                    destroy_elapsed_time_, destroy_elapsed_time_var_,
                    begin_elapsed_time_,   begin_elapsed_time_var_,
                    compl_elapsed_time_,   compl_elapsed_time_var_ 
          );
  return string(stringout);
}

void CDOverheadVar::PrintInfo(void)
{
  printf("%s", GetString().c_str());
}
