#ifndef _VARIABLE_H
#define _VARIABLE_H

#include <string>
using namespace std;
namespace cd {


class Variable {
	string _variable_name;
	string _variable_value;
public:
	
	void SetData(string name, string value)
	{
		_variable_name = name;
		_variable_value = value;
	}
	string GetName()
	{
		return _variable_name;
	}

	string GetValue()
	{
		return _variable_value;
	}
	
};

}

#endif

