#ifndef _CONFIG_OBJ_H
#define _CONFIG_OBJ_H

#include <list>
#include <cstdint>
#include <string>
#include <map>
#include "parser.h"
#include "object.h"
#include "variable.h"
#include "cd_interface.h"

using namespace std;
using namespace cd;
//class Object;
//class Variable;
//class CDElement;

namespace cd {
class CDInterface;

class ConfigBase
{
  string name_;
protected:
  CDInterface *interface_;
public:
	enum ReturnType	{
		Error,
		Normal
	};
	ConfigBase() {}
	ConfigBase(const string& name) 
    : name_(name) {}
	virtual ~ConfigBase()	{}
	virtual int Deserialize(Object *obj, CDInterface *interface)=0;  
	void        SetName(string name)	{ name_ = name;	}
	string      GetName() { return name_; }
};

class ErrorObj : public ConfigBase {
	double  error_rate_;
	double  orig_rate_;	// record the original error rate for future recover
	int64_t num_;
  map<uint32_t, double> task_to_fail_;
  map<uint32_t, double> cd_to_fail_;
public:
	ErrorObj(void) {
    num_ = 0;
    error_rate_= 0.0;
    orig_rate_ = 0.0;
  }
	ErrorObj(const string& name) 
    : ConfigBase(name) {
    num_ = 0;
    error_rate_ = 0.0;
    orig_rate_  = 0.0;
  }
	virtual ~ErrorObj(void)	{}

  void GetNumber(const char *value);
	int Deserialize(Object *obj, CDInterface *interface);
};


class PrvObj : public ConfigBase {
	uint32_t medium_;
public:
	PrvObj(void) { 
    medium_=0; 
  }
	PrvObj(uint32_t medium, const string &name) 
    : ConfigBase(name) {
    medium_ = medium;
  }
	virtual ~PrvObj(void)	{}
	int Deserialize(Object *obj, CDInterface *interface);
};

class CDEntity : public ConfigBase {

};


}
#endif
