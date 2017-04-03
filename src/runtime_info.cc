#include "runtime_info.h"
using namespace std;
using namespace common;

//std::map<std::string, std::string, uint64_t> RuntimeInfo::per_entry_vol;
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
                    "%sexec_cnt :%11lu\n"
                    "%sreex_cnt :%11lu\n"
                    "%stot_time :%11.6lf # [s]\n"
                    "%sreex_time:%11.6lf # [s]\n"
                    "%swait_time:%11.6lf # [s]\n"
                    "%svol_copy :%11lu # [B]\n"
                    "%svol_refer:%11lu # [B]\n"
                    "%scomm_log :%11lu # [B]\n"
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
                    "preserve :%11.6lf # [s]\n"
                    "create   :%11.6lf # [s]\n"
                    "destroy  :%11.6lf # [s]\n"
                    "begin    :%11.6lf # [s]\n"
                    "complete :%11.6lf # [s]\n", 
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
                    "preserve:%11.6lf # [s] (var:%11.6lf)\n"
                    "create  :%11.6lf # [s] (var:%11.6lf)\n"
                    "destroy :%11.6lf # [s] (var:%11.6lf)\n"
                    "begin   :%11.6lf # [s] (var:%11.6lf)\n"
                    "complete:%11.6lf # [s] (var:%11.6lf)\n", 
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
