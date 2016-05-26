#ifndef _UTIL_H
#define _UTIL_H

#include <math.h>
#include <stdio.h>
#include <map>
#include <utility>
#include "define.h"
#define DEFAULT_SIZE    128
#define DEFAULT_THREADS 8
#define MAX_VAL 3

int count[64];

enum {
  I_MAT =0,
  INDEX =1,
  ONES  =2,
  ZEROS =3,
  RANDOM=4,
  FUNC1 =5,
  FUNC2 =6
};

static inline
void InitArray(FLOAT *A, int coldim, int rowdim, int ops) {
  switch(ops) {
    case I_MAT: {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          if(j == i) {
            A[stride + j] = 1;
          }
          else
            A[stride + j] = 0;
        }
      }
      break;
    }
    case INDEX: {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = stride + j;
        }
      }
      break;
    }
    case ONES: {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = 1;
        }
      }
      break;
    }
    case ZEROS : {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = 0;
        }
      }
      break;
    }
    case RANDOM : {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = rand();
        }
      }
      break;
    }
    default : {
      for(int i=0; i<coldim; i++) {
        int stride = i*rowdim;
        for(int j=0; j<rowdim; j++) {
          A[stride + j] = 0;
        }
      }
      break;
    }
  }
}

FLOAT GenElem(
		       int ops,
		       int x,
		       int y,
           int stride) {
  switch(ops) {
  case ZEROS:  
    return 0.0;
  case ONES: 
    return 1.0;
  case INDEX:
    return (y*stride + x);
  case I_MAT:
    if (x == y) return 1.0;
    else return 0.0;
  case RANDOM: 
    return rand() % RAND_MAX_VAL;
  case FUNC1: 
    return ((FLOAT)(x + 1)/(y + 1));
  case FUNC2: 
    return ((FLOAT)(x)/(y + 1));
  default:
    printf("error, don't know ops = %d\n", ops);
    exit(1);
  }
}


FLOAT *InitSubMatrix(
		       int ops, 
		       FLOAT *result, // a pointer to storage for the flattened array of the requested sub matrix
		       int x_lo,       // the starting x value of the requested sub matrix
		       int x_hi,       // the ending x value of the requested sub matrix
		       int x_stride,   // the stride in the x dimension
		       int y_lo,       // the starting y value of the requested sub matrix
		       int y_hi,       // the ending y value of the requested sub matrix
		       int y_stride,   // the stride in the y dimension
		       int is_row_major)// whether the returned sub matrix is in row_major or column_major order
{
  int i = 0, y = 0, x = 0;
  // bounds check
  if ((xdim_max - 1) < x_hi) {
    printf("error: x_hi is too large\n");
    exit(1);
  }

  if ((ydim_max - 1) < y_hi) {
    printf("error: y_hi is too large\n");
    exit(1);
  }

  if (is_row_major) {
    for (y = y_lo; y <= y_hi; y++) {
      for (x = x_lo; x <= x_hi; x++, i++) {
      	result[i] = GenElem(ops, x, y, x_stride);
      }
    }
  } else { // column major
    for (x = x_lo; x <= x_hi; x++) {
      for (y = y_lo; y <= y_hi; y++, i++) {
      	result[i] = GenElem(ops, x, y, y_stride);
      }
    }
  }

  return result;
}

long int balancer[2] = {0,0};
long int bal_flip = 0;

bool GetExponentPair(long int denominator, long int &numerator, std::pair<long int,long int> &result_out)
{
  bool valid_first = false,
       valid_second = false;
  long int flip = 0;
  // Let's say numerator = denominator^(a+b) * rest_factors,
  // it finds a and b. 
  long int tmp = 0;
  // Print the number of denominators that divide numerator
  while (numerator%denominator == 0) {
    PRINT("%ld ", denominator);
    numerator = numerator/denominator;
    if(flip%2 == 0) 
      valid_first = true;
    else 
      valid_second = true;

    tmp++; // update tmp
    balancer[bal_flip%2]++; // update balancer
    bal_flip++;
    flip++; // update flip 
  }

  if(tmp == 0) 
    return false;
  else {
    if(tmp%2 == 0) {
      tmp /= 2;
      result_out.first  = tmp;
      result_out.second  = tmp;
    }
    else {
      tmp /= 2;
      if(bal_flip%2 == 0) {
        result_out.first  = tmp+1;
        result_out.second  = tmp;
      }
      else {
        result_out.first  = tmp;
        result_out.second  = tmp+1;
      }
    }
    return true;
  }
}

int GetTwoFactors(long int n, int *res_a, int *res_b)
{
  long int orig_n = n;
  std::map<long int, std::pair<long int,long int> > factor_list;
  long int i=0;
  long int sqrt_n = 0;
 
  std::pair<long int,long int> result(0,0);
  if(GetExponentPair(2, n, result)) {
    factor_list[2] = result;
  } 


  if(n != 1) {
    // n must be odd at this point.  So we can skip one element (Note i = i + 2)
    sqrt_n = sqrt((double)n);
    for (i = 3; i <= sqrt_n; i = i+2) {
      std::pair<long int,long int> result(0,0);
      if(GetExponentPair(i, n, result)) {
        factor_list[i] = result;
      } 
    }
  }

  std::pair<int,int> two_factor(1,1);
  int lcm = 1;
  for(auto it=factor_list.begin(); it!=factor_list.end(); ++it) {
//    PRINT("%ld : %ld %ld\n", it->first, it->second.first, it->second.second);
    two_factor.first  *= (int)pow(it->first, it->second.first);
    two_factor.second *= (int)pow(it->first, it->second.second);
    lcm *= (int)pow(it->first, 
                   (it->second.first > it->second.second)? it->second.first : 
                                                           it->second.second
            );
  }
//  PRINT("Result = %d x %d = %ld = %ld\n", two_factor.first, two_factor.second, 
//      (long int)two_factor.first * (long int)two_factor.second, orig_n);
  *res_a = two_factor.first;
  *res_b = two_factor.second;
  return lcm;
}


#endif
