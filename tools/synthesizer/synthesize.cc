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
    Param cdparam = params["CD info"]["root CD"];

    std::string type_str = cdparam["type"].asString();
//    cdparam["global param"] = params["global param"];
    SYNO(cout << "CD type: " << type_str << endl;)
  } else {
    SYNO(cout << "no CD info" << endl;)
  }
  return params["CD info"]["root CD"];
}

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
  int numRanks = 1;
  int myRank = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  CD_Init(numRanks, myRank, kPFS);
  A a;
  function<void()> f = bind<void>(a, 1);
  f(); // prints 1

  SYNO(cout << "Nest CD begin" << endl;)
  NestCDs();

  SYNO(cout << "\n\n Start \n\n" << endl;)

//  Param param = ReadParam("test.json");
  CDNode cd_tree(ReadParam("test.json"), "root");
  cd_tree();
  CD_Finalize();
  MPI_Finalize();
  return 0;
}

