#include <stdio.h>
#include <mpi.h>
#include "cd.h"

int 
main( int argc, char **argv ){

  int msize=0, mrank=0;
  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &msize );
  MPI_Comm_rank( MPI_COMM_WORLD, &mrank );

  CDHandle *CD0=NULL;
  CD0 = cd::CD_Init( msize, mrank, kHDD );
  
  int i=0;
  int data=0;
  CD_Begin( CD0, true, "CD00" );{

    CD0->Preserve( &data, sizeof(int), kCopy, "data" );

    if( i==0 ) data=1;
    i++;

    printf("Iter=%d   data=%d   %s \n", 
	   i, data, (data==0?"PASS":"FAIL") );
    getchar();
    CD0->CDAssert( data==0 );
  
  }CD_Complete(CD0);

  CD_Finalize();
  MPI_Finalize();
  return 0;
}
