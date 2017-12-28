#ifndef __CD_COSP2_H
#define __CD_COSP2_H

#include "mycommand.h"
#include "decomposition.h"
#include "sparseMatrix.h"
#include "haloExchange.h"
#include "cd.h"

//preserve Command struct
unsigned int preserveCommand(cd_handle_t *cdh, Command* cmd);

//preserve SparseMatrix struct(ELL format: NZs, Column Indices, Values)
unsigned int preserveSparseMatrix(cd_handle_t *cdh,  SparseMatrix* spm, char* name);
unsigned int preserveNZs_ELL(cd_handle_t *cdh, int* iia, const int hsize, char* name);
unsigned int preserveIndices_ELL(cd_handle_t *cdh,  int** jja, const int hsize, const int msize, char* name);
unsigned int preserveValues_ELL( cd_handle_t *cdh,  real_t** val, const int hsize, const int msize, char* name);

//preserve Domain struct
unsigned int preserveDomain(cd_handle_t *cdh, Domain* domain);

//preserve HaloExchange struct
unsigned int preserveHaloExchange(cd_handle_t *cdh, HaloExchange* haloExchange);
#endif
