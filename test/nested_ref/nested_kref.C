#include <stdio.h>
#include <mpi.h>
#include "cd.h"

int 
main( int argc, char **argv ){

  int i=0, data=0;

  int msize=0, mrank=0;
  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &msize );
  MPI_Comm_rank( MPI_COMM_WORLD, &mrank );

  cd::CDErrT cd_err;
  cd::CDHandle *CD0=NULL, *CD1=NULL, *CD2=NULL;

  CD0 = cd::CD_Init( msize, mrank, kDRAM );
  
  printf("Test1: Level-0 preserve-by-copy...\n");
  i=0; data=0;
  CD_Begin( CD0 );{

    CD0->Preserve( &data, sizeof(int), kCopy, "data" );

    if( i==0 ) data=1;
    i++;

    printf("Iter=%d   data=%d   %s \n", 
	   i, data, (data==0?"PASS":"FAIL") );

    CD0->CDAssert( data==0 );

  }CD_Complete(CD0);
  printf("Test1: Complete.\n\n");


  printf("Test2: Level-1 preserve-by-reference...\n");
  i=0; data=0;
  CD_Begin( CD0 );{

    CD0->Preserve( &data, sizeof(int), kCopy, "data" );

    CD1 = CD0->Create( "CD1", kStrict, 0, 0, &cd_err );
    CD_Begin( CD1 );{

      CD1->Preserve( &data, sizeof(int), kRef, "data", "data" );

      if( i==0 ) data=1;
      i++;

      printf("Iter=%d   data=%d   %s \n", 
	     i, data, (data==0?"PASS":"FAIL") );

      CD1->CDAssert( data==0 );

    }CD_Complete( CD1 );
    CD1->Destroy();

  }CD_Complete(CD0);
  printf("Test2: Complete.\n\n");

  printf("Test3: Level-2 preserve-by-reference...\n");
  i=0; data=0;
  CD_Begin( CD0 );{

    CD0->Preserve( &data, sizeof(int), kCopy, "data" );

    CD1 = CD0->Create( "CD1", kStrict, 0, 0, &cd_err );
    CD_Begin( CD1 );{

      CD1->Preserve( &data, sizeof(int), kRef, "data", "data" );

      CD2 = CD1->Create( "CD2", kStrict, 0, 0, &cd_err );
      CD_Begin( CD2 );{

	CD2->Preserve( &data, sizeof(int), kRef, "data", "data" );

	if( i==0 ) data=1;
	i++;
	
	printf("Iter=%d   data=%d   %s \n", 
	       i, data, (data==0?"PASS":"FAIL") );

	CD2->CDAssert( data==0 );

      }CD_Complete( CD2 );
      CD2->Destroy();

    }CD_Complete( CD1 );
    CD1->Destroy();

  }CD_Complete(CD0);
  printf("Test3: Complete.\n\n");


  CD_Finalize();
  MPI_Finalize();
  return 0;
}
