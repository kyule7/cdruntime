#include "data_handle.h"
#include <iostream>
using namespace cd;

//std::ostream& cd::operator<<(std::ostream& str, const DataHandle& dh)
//{
//  return str <<"handle type: "<<dh.handle_type()
//             <<"\nref_name : "<<dh.ref_name()
//             <<"\nvalue : "   <<*(reinterpret_cast<int*>(dh.address_data())) 
//             <<"\nlen   : "   <<dh.len()
//             <<"\naddress: "  <<dh.address_data()
//             <<"\nfilename: " <<dh.file_name();
//}

std::ostream& cd::operator<<(std::ostream& str, const DataHandle& dh)
{
  return str << "\n== Data Handle Information ======="
             << "\nhandle T:\t" << dh.handle_type_  
             << "\nNode ID :\t" << dh.node_id_  
             << "\nAddress :\t" << dh.address_data_
             << "\nlength  :\t" << dh.len_
             << "\nfilename:\t" << dh.file_name_
             << "\nref_name:\t" << dh.ref_name_
             << "\nref_offset:\t"<<dh.ref_offset_
             << "\n==================================\n";
}
