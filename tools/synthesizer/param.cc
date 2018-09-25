#include "param.h"
#include <fstream>
#include <iostream>
#include <sstream>

Param::Param() : Json::Value() {}

Param::Param(const Json::Value &params) : Json::Value(params) {}

Param::Param(Json::Value &&params) : Json::Value(params) {}

void Param::check_params() {
  std::vector<const char *> reqrd_params = {"CD info", "global param"};

  bool flag = false;
  for (auto it = reqrd_params.begin(); it != reqrd_params.end(); ++it) {
    if (!(*this).isMember(*it)) {
      std::cout << "[Parsing Error] Missing \"" << *it << "\"" << std::endl;
      flag = true;
    }
  }

  if (flag)
    exit(1);
}

void Param::load_params(const char *file_name) {
  std::ifstream in(file_name);
  if (!in) {
    std::cout << "[Error] File " << file_name << " does not exist" << std::endl;
    exit(1);
  }
  in >> (*this);
  in.close();
}

void Param::save_params(const char *file_name) {
  std::ofstream out(file_name, std::ofstream::out);
  out << (*this);
  out.close();
}

void Param::set_param(const Param &param) {
  for (auto it : param.getMemberNames()) {
    std::stringstream ss(it);
    std::string tmp_str;
    std::vector<std::string> tmp_vec;

    while (std::getline(ss, tmp_str, '.')) {
      tmp_vec.push_back(tmp_str);
    }

    Json::Value *tmp_param = this;
    for (auto it1 = tmp_vec.begin(); it1 != --tmp_vec.end(); ++it1) {
      tmp_param = &((*tmp_param)[*it1]);
    }

    (*tmp_param)[*(--(tmp_vec.end()))] = param[it];
  }
}
