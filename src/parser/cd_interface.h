#ifndef _CD_INTERFACE_H
#define _CD_INTERFACE_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#define VERBOSE

#include "config_obj.h"
#include "parser.h"
using namespace std;

namespace cd {

class PrvObj;
class ErrorObj;
class Parser;
class CDEntity;
class CDInterface;

class Deserializer {
private:
	void PrintIndent(int indent);
	
public:
	void Deserialize(Object *root, CDInterface *interface);
	void Print(Object *root); // Print out the whole structure from the root.
};

class CDInterface {
public:
	CDInterface() {}
	~CDInterface() {}
	int LoadConfiguration(const char *filename);
  void AddErrorObj(ErrorObj *error);
  void AddPrvObj(PrvObj *preservation);
  void SetRoot(CDEntity *root);
private:
	ErrorObj *FindErrorObj(string name);
	PrvObj *FindPrvObj(string name);

private:

	list<PrvObj *> prv_obj_;
	list<ErrorObj *> error_obj_;
//  list<ErrorInjector *> error_injector_;

	CDEntity *root_;
	vector<CDEntity *> cd_entity_;
	Parser parser_;
	Deserializer deserializer_;
};

}
#endif
