#pragma once
#include "json/json.h"

// typedef Json::Value Param;
class Param : public Json::Value {
public:
  Param();
  Param(const Json::Value &params);
  Param(Json::Value &&params);

  void check_params();
  void load_params(const char *file_name);
  void save_params(const char *file_name);
  void set_param(const Param &param);
};
