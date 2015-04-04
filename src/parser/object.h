#ifndef _OBJECT_H
#define _OBJECT_H
#include <list>
#include "variable.h"

using namespace std;
namespace cd {


class Object {
	string _object_type;
	string _object_name;
public:

	void AddObject(Object *object)
	{	_objects.push_back(object);	}

	void AddVariable(Variable *variable)
	{	_variables.push_back(variable);	}

	void SetObject(string type, string name)
	{ _object_type = type; _object_name = name;	}

	void SetName(string name)	{	_object_name = name; }
	
	string GetType() { return _object_type;	}
	string GetName() { return _object_name;	}

	list <Object *> _objects;
	list <Variable *> _variables;

};

}

#endif
