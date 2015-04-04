#include "cd_interface.h"
using namespace cd;
using namespace std;

int CDInterface::LoadConfiguration(const char *filename)
{
	Object *root;
	if ( true == parser_.ParseFile(filename) )
	{
		root = parser_.GetRootObject();
		printf("\nParse is done. Configuration File: %s\n", filename);

#ifdef VERBOSE
		deserializer_.Print(root);
#endif						
		deserializer_.Deserialize(root, this);
		return true;
	}

	else return false;
}

void CDInterface::AddPrvObj(PrvObj *preservation)
{	prv_obj_.push_back(preservation); }

void CDInterface::AddErrorObj(ErrorObj *error)
{	error_obj_.push_back(error); }

void CDInterface::SetRoot(CDEntity *root)
{ root_ = root; }

// add find functions for preservation and Error and recovery to connect them with CDs

ErrorObj *CDInterface::FindErrorObj(string name)
{
	list<ErrorObj *>::iterator it;
	for(it = error_obj_.begin(); it!= error_obj_.end(); ++it)	{
		if((*it)->GetName() == name) {
			return (*it);
		}
	}
	return NULL;
}


PrvObj *CDInterface::FindPrvObj(string name)
{
	list<PrvObj *>::iterator it;
	PrvObj *p;
//	printf("preservation size:%d\n",prv_obj_.size());
	for(it = prv_obj_.begin(); it!= prv_obj_.end(); ++it)
	{
		p= (*it);
		if ( p->GetName() == name )
		{
			return (*it);
		}
	}
	return NULL;
}




void Deserializer::PrintIndent(int indent)
{
	for( int i=0; i < indent;  i++ )
	{
		printf("\t");
	}

}

void Deserializer::Print(Object *root) // Print out the whole structure from the root.
{
	static int indent=0;

	list<Variable *>::iterator it;
	printf("\n======== Print Object Start ========\n");
	PrintIndent(indent);
	printf("%s %s{\n",root->GetType().c_str(), root->GetName().c_str());

	for( it = root->_variables.begin() ; it!= root->_variables.end(); ++it )
	{
		PrintIndent(indent);	
		printf("\t%s=%s;\n", (*it)->GetName().c_str(),  (*it)->GetValue().c_str());
	}

	list<Object *>::iterator obj_it;

	for( obj_it = root->_objects.begin() ; obj_it!= root->_objects.end(); ++obj_it )
	{
		indent++;
		Print(*obj_it);
		indent--;
	}
	PrintIndent(indent);
	printf("}\n");
	printf("\n======== Print Object Done ========\n");
}

void Deserializer::Deserialize(Object *root, CDInterface *interface)
{

	list<Variable *>::iterator it;

	for( it = root->_variables.begin() ; it!= root->_variables.end(); ++it ) { 
  // for now we don't have any global variables there are only objects on the root level.
		
	}

	list<Object *>::iterator obj_it;

	for( obj_it = root->_objects.begin() ; obj_it!= root->_objects.end(); ++obj_it ) // now recovery and other objects start here
	{
    Object *obj = *obj_it;
		if(obj->GetType() == "error") {	
      ErrorObj *err;
      if(obj->GetName() != "CHANGED_NAME") {
  			err = new ErrorObj(obj->GetName());
      }
      else {
  			err = new ErrorObj(obj->GetName());
      }
			err->Deserialize(obj, interface);
			interface->AddErrorObj(err);		
		}
		else if ((*obj_it)->GetType()  == "preservation") 
		{		
			PrvObj *pre = new PrvObj;
			pre->Deserialize((*obj_it),interface);
			interface->AddPrvObj(pre);		
		}
	}


}

