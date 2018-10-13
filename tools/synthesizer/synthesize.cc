#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <cstdint>
#include "param.h"
#include "cd_node.h"
#include "cd.h"
using namespace cd;
using namespace std;
using namespace synthesizer;
int synthesizer::numRanks = 1;
int synthesizer::myRank = 0;
/*******************************************************************
 * Compute body 
 * 
 * Computation time
 * Volume of application state
 * Communication
 * Failure rate (defined in config.yaml?)
 *******************************************************************/
void ComputeBody() { SYNO(cout << __func__ << endl;) }

template <typename T>
T GetPhaseList(void) { return T(10, ComputeBody); }
//T GetPhaseList(void) { T t; t.push_back(Foo); return t; }

uint32_t GetIter(void) { return 3; }
void NestCDs(void) {
// std::function<void(const Foo&, int)> f
  auto phase_list = GetPhaseList< vector< function<void()> > >();
  uint32_t iteration = GetIter();
  for (uint32_t i=0; i < iteration; i++) {
    for (auto &pl : phase_list) { pl(); }
    SYNO(cout << "Done " << i << "-th iteration" << endl;)
  }

}

struct A {
  void operator()(int n) {
    SYNO(std::cout << n << std::endl;)
  }
};


Param ReadParam(const char *param_file) {
  Param params;
  params.load_params(param_file);
  params.check_params();

  if (params.isMember("CD info")) { // sweep CD level
    SYNO(cout << "CD type: " <<  endl;)
  } else {
    SYNO(cout << "no CD info" << endl;)
  }
  std::string child_name = params["CD info"].getMemberNames()[0];
  return params["CD info"][child_name];
}

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
  std::string inputfile("test.json");
  if (argc > 1) {
    inputfile = argv[1];
  }

  //std::cout << inputfile << std::endl;
    
  MPI_Comm_size(MPI_COMM_WORLD, &synthesizer::numRanks);
  MPI_Comm_rank(MPI_COMM_WORLD, &synthesizer::myRank);
  CD_Init(numRanks, myRank, kPFS);
  A a;
  function<void()> f = bind<void>(a, 1);
  f(); // prints 1

  SYNO(cout << "Nest CD begin" << endl;)
  NestCDs();

  SYNO(cout << "\n\n Start \n\n" << endl;)

//  Param param = ReadParam("test.json");
  CDNode cd_tree(ReadParam(inputfile.c_str()), "root");
  cd_tree();
  CD_Finalize();
  MPI_Finalize();
  return 0;
}

