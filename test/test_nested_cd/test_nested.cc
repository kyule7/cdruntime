#include <cstdio>
#include <cd.h>
#include <cd_path.h>
#include <iostream>
using namespace cd;

#ifndef KMASK
//#define KMASK kRef
#define KMASK kCopy
#endif

#define MAX_RETRY 10

int main(){

  CDErrT cd_err;
  int i, j, retry_count;
 
  double *x = new double[100];
  for( i=0; i<100; i++ ) x[i] = i;
  double *y = new double[100];
  for( i=0; i<100; i++ ) y[i] = i;

  CDHandle *root_cd = CD_Init( 0, 0 );
  CD_Begin( root_cd );
  root_cd->Preserve( x, 100 * sizeof(double), kCopy, "x", "x" );
  root_cd->Preserve( y, 100 * sizeof(double), kCopy, "y", "y" );
  std::cout << "test "<< root_cd->ptr_cd()->GetCDName() << std::endl; getchar();
  CDHandle* child1_cd = root_cd->Create("Child1", kStrict, 0, 0, &cd_err );
  {
    CD_Begin( child1_cd );
  std::cout << "test "<< child1_cd->ptr_cd()->GetCDName() << std::endl; getchar();

    CDHandle* child2_cd = child1_cd->Create("Child2", kStrict, 0, 0, &cd_err );
    {
      CD_Begin( child2_cd );
  std::cout << "test "<< child2_cd->ptr_cd()->GetCDName() << std::endl; getchar();

      CDHandle* leaf_cd = child2_cd->Create("Leaf", kStrict, 0, 0, &cd_err );
      {
  
        retry_count = 0;
        CD_Begin( leaf_cd );
  std::cout << "test "<< leaf_cd->ptr_cd()->GetCDName() << std::endl; getchar();
        leaf_cd->Preserve( x, 100 * sizeof(double), KMASK, "x", "x" );
        if( retry_count==0 ) x[0]=1;
        printf("Within child1_cd. retry_count=%d    x=%f \n", retry_count, x[0] );
        retry_count++;
        assert( retry_count < MAX_RETRY );
        leaf_cd->CDAssert( x[0]==0 );
        CD_Complete( leaf_cd );
        
        std::cout <<   "\n\nCDPath check\n\n"<< std::endl;
        std::cout << "CDPath size: "<< CDPath::GetCDPath()->size() << std::endl;
        std::cout <<   "\n\nCDPath check end\n\n"<< std::endl;

 
        retry_count = 0;
        CD_Begin( leaf_cd );
        leaf_cd->Preserve( y, 100 * sizeof(double), KMASK, "y", "y" );
        if( retry_count==0 ) y[0]=1;
        printf("Within child2_cd. retry_count=%d    y=%f \n", retry_count, y[0] );
        retry_count++;
        assert( retry_count < MAX_RETRY ); //ASSERTION FAILS
        leaf_cd->CDAssert( y[0]==0 );
        CD_Complete( leaf_cd );

        leaf_cd->Destroy();
      }
      CD_Complete( child2_cd );
      child2_cd->Destroy();
    }
    CD_Complete( child1_cd );
    child1_cd->Destroy();
  }
  CD_Complete( root_cd );
//  root_cd->Destroy();
  CD_Finalize();
  delete [] x;
  delete [] y;

  return 0;
}//main
