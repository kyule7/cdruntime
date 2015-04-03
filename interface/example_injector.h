// This example is based on the error injector written by Brian Austin (LLBL)

/**addtogroup example_error_injection Error Injection Example
 * @{
 */
#include <sys/time.h>
#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <cstring>
#include "error_injector.h"

#define ERROR_INJECTOR_DESC_SIZE 256
#define ERROR_INJECTOR_LIST_SIZE 256
#define ERROR_INJECTOR_MAX_SDATA 1024
#define ERROR_INJECTOR_DEFAULT_CRANGE 256

//gtod: converts gettimeofday to double
double gtod()
{
  struct timeval mytime;
  gettimeofday(&mytime, NULL);
  return static_cast<double>(mytime.tv_usec);
}

typedef struct ErrorRange{
  void    *data; //pointer to some data block
  int64_t ndata; //number of elements in data block
  int64_t sdata; //sizeof(element)
  char    desc[ ERROR_INJECTOR_DESC_SIZE ];
} ErrorRange;

typedef struct InjectedError{
  double  tSinceLast;
  int64_t sizeof_value;
  char    value[ ERROR_INJECTOR_MAX_SDATA ];
  void*   location;
  char*   desc;
} InjectedError;

class ExampleInjector : public MemoryErrorInjector {
  int64_t        ARange; //allocated size of range array
  int64_t        CRange; //allocate Range in chunks of size CRange
  int64_t        NRange; //length of Range array
  ErrorRange *Range; //array of data blocks where errors may be injected
  int64_t    TotalRange; //total data size of all elements in Range

  //These are the soft errors
  double ErrorRate;
  int64_t    ErrorCounter;
  double tLastError;
  InjectedError ErrorList[ ERROR_INJECTOR_LIST_SIZE ];

  //Hard error
  double HardError_Rate;
  int64_t    HardError_Counter;
  double HardError_tLast;
  InjectedError HardError_List[ ERROR_INJECTOR_LIST_SIZE ];

private:
  void SelectError( InjectedError &error ){
    assert( TotalRange>0 );

    if(NRange==0) {
        // no memory ranges to choose from!
        return;
    }

    int64_t k, Nk, iR, Bk;
    k = drand48() * TotalRange;
    assert(0 <= k);
    assert(k < TotalRange);

    // NRange>0: this is redundant, since we check above.
    assert( NRange>0 );
    Nk = 0;
    for( iR=0; iR<NRange; iR++ ){
      Bk = Range[iR].ndata * Range[iR].sdata;
      if( Nk <= k && k < Nk + Bk ) break;
      Nk += Bk;
    }

    // iR >= NRange: should not happen, but can if the search above fails
    assert(iR < NRange);
    // iR < 0: again should never happen, just a bounds check.
    assert(0 <= iR);

    int64_t   p = drand48() * Range[iR].ndata;
    void *v = static_cast<char*>( Range[iR].data ) + p * Range[iR].sdata;
    error.sizeof_value = Range[iR].sdata;
    error.location = v;
    error.desc = Range[iR].desc;

    double d[2] = { 1.e8, 0.0 };
    // paranoia check: don't exceed maximum size of error.value
    assert(error.sizeof_value <= ERROR_INJECTOR_MAX_SDATA);
    memcpy( error.value, d,  error.sizeof_value );
    //flipbit( &(ErrorList[i].value), error.sizeof_value );
    
    return;
  }//SelectError

  void SetSchedule(){

    if( ErrorRate == 0 ) return;
    if( NRange == 0 ) {
        printf("ErrorInjector::SetSchedule:  WARNING:  No memory ranges to inject!\n");
        return;
    }
    
    //for soft errors
    for( int64_t i=0; i<ERROR_INJECTOR_LIST_SIZE; i++ ){
      SelectError( ErrorList[i] );
      ErrorList[i].tSinceLast = -log( 1.0 - drand48() ) / ErrorRate;
    }
    ErrorCounter = 0;
    tLastError = gtod();    
  }//SetSchedule (soft errors)

  void HardError_SetSchedule(){

    if( ErrorRate == 0 ) return;

    for( int64_t i=0; i<ERROR_INJECTOR_LIST_SIZE; i++ ){
      HardError_List[i].location = NULL;
      HardError_List[i].tSinceLast = -log( 1.0 - drand48() ) / HardError_Rate;
      sprintf( HardError_List[i].desc, "HardError" );
    }
    HardError_Counter = 0;
    HardError_tLast = gtod();   
  }//HardError_SetSchedule

  void ExpandRangeArray(){
    ErrorRange *new_range = new ErrorRange[ ARange + CRange ];
    if( ARange > 0 ){
      ErrorRange *old_range = Range;
      memcpy( new_range, old_range, NRange * sizeof( ErrorRange ) );
      delete [] old_range;
    }
    Range = new_range;
    ARange += CRange;
  }//ExpandRangeArray

  void
  flipbit( void *a, int64_t n ){
    //flip a random bit in a bit array
    int64_t *ia = (int64_t*)a;
    int64_t b = drand48() * ( 8 * n + 1 );
    ia[ b/(8*sizeof(int64_t)) ] ^= ( 1<<(b%(8*sizeof(int64_t))) );
    return;
  }//flipbit

public:

  ExampleInjector()
    : MemoryErrorInjector(){

    ARange  = 0;
    CRange  = ERROR_INJECTOR_DEFAULT_CRANGE;
    NRange  = 0;
    Range   = NULL;
    TotalRange = 0;

    ErrorRate = 0.0;
    ErrorCounter = 0;
    tLastError = 0.0;

    HardError_Rate = 0.0;
    HardError_Counter = 0;
    HardError_tLast = 0.0;
    
  }

  int64_t SetRange( int64_t _CRange ) {
    if( _CRange>=0 ) CRange = _CRange;
    return CRange;
  }

  void PushRange( void *_data, uint64_t _ndata, uint64_t _sdata, char *_desc ){

    assert(_sdata <= ERROR_INJECTOR_MAX_SDATA);

    if( NRange == ARange ) 
      ExpandRangeArray();

    Range[ NRange ].data  = _data;
    Range[ NRange ].ndata = _ndata;
    Range[ NRange ].sdata = _sdata;
    strncpy( Range[NRange].desc, _desc, ERROR_INJECTOR_DESC_SIZE );
    NRange++;

    TotalRange += _ndata * _sdata;
  }//PushRange

  void Init( double _SoftError_Rate=0.0, double _HardError_Rate=0.0 ){

    ErrorRate = _SoftError_Rate;
    ErrorCounter = 0;
    tLastError = 0.0;
    if( ErrorRate > 0.0 )
      SetSchedule();

    HardError_Rate = _HardError_Rate;
    HardError_Counter = 0;
    HardError_tLast = 0.0;
    if( HardError_Rate > 0.0 )
      HardError_SetSchedule();

    enabled_    = false;
    logfile_    = stdout;
  }
  

  void Inject(){

    if( enabled_==false ) return;
    double tnow = gtod();
    //std::cout << "Entering Error Injector" << std::endl;

    //soft errors
    if( ErrorRate > 0.0 ){
      /*
  std::cout << "Tnow   " << tnow << std::endl;
  std::cout << "tLast  " << tLastError << std::endl;
  std::cout << "tdiff  " << (tnow - tLastError) << std::endl;
  std::cout << "tnext  " << ErrorList[ ErrorCounter ]. tSinceLast << std::endl;
      */
      if( tnow - tLastError < ErrorList[ ErrorCounter ].tSinceLast ) return;
      //*(ErrorList[ ErrorCounter ].location) = ErrorList[ ErrorCounter ].value;
      memcpy( ErrorList[ErrorCounter].location, 
        ErrorList[ErrorCounter].value, 
        ErrorList[ErrorCounter].sizeof_value );
      fprintf( logfile_, "%s %f %s %s \n", "ErrorInjector:", 
         tnow, "Injected Error", ErrorList[ ErrorCounter ].desc );
      ErrorCounter++;
      if( ErrorCounter==ERROR_INJECTOR_LIST_SIZE ) SetSchedule();
      tLastError = tnow;
    }

    if( HardError_Rate > 0.0 ){
       /*
  std::cout << "Tnow   " << tnow << std::endl;
  std::cout << "tLast  " << HardError_tLast << std::endl;
  std::cout << "tdiff  " << (tnow - HardError_tLast) << std::endl;
  std::cout << "tnext  " << HardError_ErrorList[ ErrorCounter ]. tSinceLast << std::endl;
      */
      if( tnow - HardError_tLast < HardError_List[ HardError_Counter ].tSinceLast ) return;
      fprintf( logfile_, "%s %f %s %s \n", "ErrorInjector:", 
         tnow, "Injected Error", HardError_List[ ErrorCounter ].desc );
      abort();
      HardError_Counter++;
      if( HardError_Counter==ERROR_INJECTOR_LIST_SIZE ) HardError_SetSchedule();
      HardError_tLast = tnow;
   }

    return;
  }
      
};


/** @} */

