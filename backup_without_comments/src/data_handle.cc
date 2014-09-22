#include "data_handle.h"
#include <iostream>
using namespace cd;

std::ostream& cd::operator<<(std::ostream& str, const DataHandle& dh)
{
  return str <<"handle type: "<<dh.handle_type()
             <<"\nref_name : "<<dh.ref_name()
             <<"\nvalue : "   <<*(reinterpret_cast<int*>(dh.address_data())) 
             <<"\nlen   : "   <<dh.len()
             <<"\naddress: "  <<dh.address_data()
             <<"\nfilename: " <<dh.file_name();
}
