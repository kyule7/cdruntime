#include <cstdio>
#include <cd.h>

using namespace cd;

#ifndef KMASK
#define KMASK kRef
//#define KMASK kCopy
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

  CDHandle* child_cd = root_cd->Create("Child", kStrict, 0, 0, &cd_err );
  {

    retry_count = 0;
    CD_Begin( child_cd );
    child_cd->Preserve( x, 100 * sizeof(double), KMASK, "x", "x" );
    if( retry_count==0 ) x[0]=1;
    printf("Within child1_cd. retry_count=%d    x=%f \n", retry_count, x[0] );
    retry_count++;
    assert( retry_count < MAX_RETRY );
    child_cd->CDAssert( x[0]==0 );
    CD_Complete( child_cd );

    retry_count = 0;
    CD_Begin( child_cd );
    child_cd->Preserve( y, 100 * sizeof(double), KMASK, "y", "y" );
    if( retry_count==0 ) y[0]=1;
    printf("Within child2_cd. retry_count=%d    y=%f \n", retry_count, y[0] );
    retry_count++;
    assert( retry_count < MAX_RETRY ); //ASSERTION FAILS
    child_cd->CDAssert( y[0]==0 );
    CD_Complete( child_cd );

  }
  child_cd->Destroy();

 
  CD_Complete( root_cd );
  root_cd->Destroy();

  delete [] x;
  delete [] y;

  return 0;
}//main
