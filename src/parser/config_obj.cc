#include <cmath>
#include "config_obj.h"
#include "cd_interface.h"
using namespace std;
using namespace cd;

int ErrorObj::Deserialize(Object *obj, CDInterface *interface) 
{
	interface_ = interface;

	list<Object *>::iterator obj_it;
	list<Variable *>::iterator it;	

	for(it = obj->_variables.begin() ; it!= obj->_variables.end(); ++it) {
		string value = (*it)->GetValue();
		string name = (*it)->GetName();

		if( name == "error_rate")	{
		  double val = atof(value.c_str());
			error_rate_ = val;
			orig_rate_ = val;
      cout << "ErrorObj::Deserialize val : " << val << endl;
		}
		else if( name == "task_to_fail")	{

      char *tok = strtok(const_cast<char*>(value.c_str()), ",");
      while(tok != NULL) {
        task_to_fail_[stoi(tok)] = 1.0;
        tok = strtok(NULL, ",");
      }

      for(auto it=task_to_fail_.begin(); it!=task_to_fail_.end(); ++it) {
        cout << it->first << "-"<<it->second << endl;
      }

		}
    else if( name == "cd_to_fail" ) {

      char *tok = strtok(const_cast<char*>(value.c_str()), ",");
      while(tok != NULL) {
        cd_to_fail_[stoi(tok)] = 1.0;
        tok = strtok(NULL, ",");
      }

      for(auto it=cd_to_fail_.begin(); it!=cd_to_fail_.end(); ++it) {
        cout << it->first << "-"<<it->second << endl;
      }

      cout << "ErrorObj::Deserialize cd_to_fail : " << value << endl;
    }
		else {
			cerr << "Unsupported parameter in error class " << obj->GetName() << "::"  << name << endl;
		}

	}

	return true;
}


int PrvObj::Deserialize(Object *obj, CDInterface *interface)
{
  // STUB
	return true;
}


