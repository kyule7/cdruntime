#ifndef _PARSER_H
#define _PARSER_H

#include <string.h>
#include <string>
#include <list>
#include <iostream>
#include <stdlib.h>
#include "object.h"
#include "variable.h"

using namespace std;
namespace cd {

class Parser {
	Object root_;
public:
	Parser() {}
	~Parser()	{}

private:
	char *ReadFile(const char *filename, int *size)
	{
		FILE *fp;
		char *buffer;
		fp = fopen(filename,"r");
		if( fp != NULL ) {

  		fseek(fp, 0L, SEEK_END);
  		int sz = ftell(fp);
  		fseek(fp, 0L, SEEK_SET);
  
  		buffer = (char *)malloc(sizeof(char)*(sz + 1));
  
  		if( buffer == NULL ) {
  			printf("Memory error\n");
  		}
  
  		int result = fread (buffer, 1,sz,fp);
  		buffer[sz] = '\0';
  
  		if( result != sz) 
        printf("Reading Error\n");
  
  		*size = sz+1;
  		return buffer;
		}
		else {
			*size = 0;
			printf("File not found: %s\n", filename);
			return NULL;
		}

	}


public:
	int ParseFile(const char *filename)
	{
		int size=0;
		char *buffer;
		buffer = ReadFile(filename, &size);
		if( size != 0 )
		{
			char *dest = new char[size];
			RemoveComment(buffer,size,dest);
			int ret= Parse(dest, strlen(dest), &root_);
			delete [] buffer;
			delete [] dest;
			return ret;
		}
		else return false;
	}

	Object *GetRootObject()
	{
		return &root_;
	}


	void RemoveComment(char *chunk, int length, char *dest)
	{
		int comment_mode = 0;
		for( int i=0; i < length ; i++ ) {
			if( i < length -1 ) {

				if( comment_mode != 2) 	{
					if( *chunk == '/') {
						if( *(chunk +1 ) == '/' ) 	{
							comment_mode =1;
						}
					}
				}

				if( *chunk == '/')	{
					if( *(chunk +1 ) == '*' )		{
						comment_mode =2;
					}
				}
				else if( *chunk == '*')	{
					if( *(chunk +1 ) == '/' )	{
						comment_mode =0;
					}
				}

				if( *chunk == '\n' && comment_mode == 1)	{
					comment_mode = 0;
				}

			}

			if( comment_mode == 0 ) {
				*dest = *chunk;
				dest++;
			}

			chunk++;

		} // for ends
	}

	int Parse(string chunk, Object *parent)	{
		return Parse(chunk.c_str(), chunk.size(), parent);
	}
	
	char *GetCopy(string str)	{
		char * writable = new char[str.size() + 1];
		std::copy(str.begin(), str.end(), writable);
		writable[str.size()] = '\0';
		return writable;
	}

	int Parse(const char *chunk, int length, Object *parent)	{
		string unit;
		
		int bracket=0;
		int object=0;
		int objectfound=0;
		int equal=0;

		if( chunk == NULL ){
			cerr << "Parse error: parse input data is null" << endl;	
			return false;
		}
		// basic element is a chunk which will end with ";" 
		// if any ";" occurs within {  } bracket, they are ignored
#ifdef VERBOSE
printf("Parse::======================\n");
		printf("%d\n",length);
		printf("[%s]\n",chunk);
#endif

		for( int i=0; i < length ; i++ ) { 
			if( *chunk == '{') {

#ifdef VERBOSE
				printf("{");
#endif
				bracket++;
				object=1;
			}
			else if( *chunk == '}') {

#ifdef VERBOSE
				printf("}");
#endif
				bracket--;

				if ( bracket == 0 )	{
					objectfound = 1;  // find out if person missed the ';' at the end.
				}
			}
			else if ( *chunk == '=' && bracket == 0)	{
				equal++;
			}

      // if it is an object that includes {}
			if( bracket == 0  && objectfound == 1 && object ==1 && *chunk!='}' ) { 
			
        // here is the end of a chunk
				if( equal >= 1 ){
					printf("Missing ';' for a parameter\n");
					return false;
				}

				unit.push_back(' '); // give a space for this algorithm to work properly
				ParseObject(unit, parent);
				object=0;
				objectfound =0;
				unit = "";
				chunk++;
				continue;

			}
			else if( bracket == 0 &&  *chunk == ';' && object ==0 && equal ==1 )  {
        // if it's just an parameter

				if( equal >= 2 ) {
					printf("Missing ';' for a parameter\n");
					return false;
				}
#ifdef VERBOSE
				printf("Parameter Detected = '%c', '%s'\n", *chunk, unit.c_str());
#endif
				ParseParameter(unit, parent);
				equal =0;
				unit = "";
				chunk++;
				continue;
			}


			unit.push_back(*chunk);

			chunk++;
		} // for ends

		if( equal >=1 )	{
				 printf("Missing ';' for a parameter\n");
				 return false;
		}

		if( bracket != 0) {
			printf("Error bracket pair does not match\n");
			return false;
		}
		return true;

	}

private:
	
	void ParseObject(string raw_obj, Object *target) {

#ifdef VERBOSE
		printf("ParseObject::%s\n",raw_obj.c_str());
#endif

		char *pobj= GetCopy(raw_obj);
		char *pch;
		Object *new_object;
		pch = strtok (pobj," \t\n\r");
		string type;
		string name;

#ifdef VERBOSE
					printf("################## object '%s', [[%s]]\n",name.c_str(), raw_obj.c_str());
#endif
		if (pch != NULL)
	  {
			new_object = new Object();
			type = pch;
			pch = strtok (NULL," \t\n\r");
			if( pch != NULL )
			{
				name = pch;
#ifdef VERBOSE				
				printf("Object detected:'%s'::'%s'\n",type.c_str(), name.c_str());
#endif 
				if( name == "{") {
					
					printf("Error object name has not defined for '%s' type\n",type.c_str());
				}
				else	{
					size_t nearby =  name.find("{");
					if( nearby != string::npos ) {
#ifdef VERBOSE
						printf ("attatched!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif
					   name = name.substr(0, nearby);	
					}
				}

			
				new_object->SetObject(type,name);
				size_t startpos = raw_obj.find("{");
				size_t endpos = raw_obj.rfind("}");

				if( startpos != string::npos && endpos != string::npos ) {
					string content = raw_obj.substr(startpos+1 , endpos-startpos -2);
					content.push_back(' ');
					target->AddObject(new_object); 
					Parse	( content  , new_object);
				}
				else if( startpos == string::npos) {
					printf("Parse Error: Start bracket not found in object '%s', [[%s]]\n",name.c_str(), raw_obj.c_str());
//					delete new_object;
				}
				else if( endpos == string::npos) 
				{
					printf("Parse Error: End bracket not found in object '%s'\n",name.c_str());
				} 
			}

		}
	}

	void ParseParameter(string parameter, Object *parent)	{

		char *p= GetCopy(parameter);

#ifdef VERBOSE
		printf("ParseParameter::[%s]\n",p);
#endif
		char *pch = strtok (p," =\t\n\r");
	
		Variable *var;
		string name;
		string value;
		if( pch != NULL )	{		
			name = pch ;

			pch = strtok (NULL," =\t");
			if( pch != NULL )	{
				value = pch;
#ifdef VERBOSE
				printf("%s::%s=%s\n",(parent->GetName()).c_str(),name.c_str(),value.c_str());
#endif
				var = new Variable;
				var->SetData(name,value);

				parent->AddVariable(var);

			} 
			else {
				printf("Error could not found the value of parameter '%s'",name.c_str());
			}
		}

	}
};



}
#endif
