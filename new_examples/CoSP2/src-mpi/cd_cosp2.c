#include <assert.h>
#include "parallel.h" //getNRanks()
#ifdef _ROOTCD
#include "cd.h"
#include "cd_cosp2.h"

unsigned int preserveCommand(cd_handle_t *cdh, Command* cmd) {
  unsigned int size = sizeof(Command);
  cd_preserve(cdh, cmd, size, kCopy, "cmd", "cmd");
  return size;
}

// preserve SparseMatrix structure
// hsize: number of rows
// msize: the max number of non-zeroes per row
unsigned int preserveSparseMatrix(cd_handle_t *cdh,  SparseMatrix* spm, char *name) {
  printf("inside %s\n", __func__);
  unsigned int size = sizeof(SparseMatrix);
  // add matrix name passed from argument
  char spm_with_name[16] = "-1";
  assert(name != NULL);
  sprintf(spm_with_name, "spm_%s", name);
  printf("before preservation: spm=%p\n", spm);
  cd_preserve(cdh, spm, size, kCopy, spm_with_name, spm_with_name);
  printf("after preservation: spm=%p\n", spm);

  // preserve iia (1D array containing number of non-zero elements per row)
  size += preserveNZs_ELL(cdh, spm->iia, spm->hsize, name);
  // preserve jja (2D array containing column indices of values per row)
  size += preserveIndices_ELL(cdh, spm->jja, spm->hsize, spm->msize, name);
  // perserve val (2D array containing non-zero elements per row)
  size += preserveValues_ELL(cdh, spm->val, spm->hsize, spm->msize, name);

  printf("before return: spm=%p\n", spm);
  printf("inside %s: before return (size=%u)\n", __func__, size);
  return size;
}

unsigned int preserveNZs_ELL(cd_handle_t *cdh, int* iia, const int hsize, char* name) {
  printf("inside %s\n", __func__);
  unsigned int size = hsize*sizeof(int);
  char iia_with_name[16] = "-1";
  // add matrix name passed from argument
  assert(name != NULL);
  sprintf(iia_with_name, "iia_%s", name);
  cd_preserve(cdh, iia, size, kCopy, iia_with_name, iia_with_name);
  return size;
}

unsigned int preserveIndices_ELL(cd_handle_t *cdh, int** jja, const int hsize, const int msize, char* name) {
  printf("inside %s\n", __func__);
  unsigned int size = 0;
  char jja_with_name[6] = "-1";
  assert(name != NULL);
  sprintf(jja_with_name, "jja_%s", name);

#ifdef CONTIG_MATRIX
  // If matrix is contiguous, preserve all at once
  size = hsize*msize*sizeof(int);
  cd_preserve(cdh, jja[0], size, kCopy, jja_with_name, jja_with_name);
#else
  // If not CONTIG_MATRIX, need to preserve one row at a time
  char jja_with_row_idx[256] = "-1";
  size = msize*sizeof(int);
  for (int i = 0; i < hsize; i++) {
    sprintf(jja_with_row_idx, "jja_%s_%d", name, i);
    cd_preserve(cdh, jja[i], size, kCopy, jja_with_row_idx, jja_with_row_idx);
  }
#endif
  return hsize*msize*sizeof(int);
}

unsigned int preserveValues_ELL(cd_handle_t *cdh, real_t** val, const int hsize, const int msize, char* name) {
  printf("inside %s\n", __func__);
  unsigned int size = 0;
  assert(name != NULL);

#ifdef CONTIG_MATRIX
  // If matrix is contiguous, preserve all at once
  char val_with_name[6] = "-1";
  sprintf(val_with_name, "val_%s", name);
  size = hsize*msize*sizeof(real_t);
  cd_preserve(cdh, val[0], size, kCopy, val_with_name, val_with_name);
#else
  // If not CONTIG_MATRIX, need to preserve one row at a time
  char val_with_row_idx[256] = "-1";
  size = msize*sizeof(int);
  for (int i = 0; i < hsize; i++) {
    sprintf(val_with_row_idx, "val_%s_%d", name, i);
    cd_preserve(cdh, val[i], size, kCopy, val_with_row_idx, val_with_row_idx);
  }
#endif
  return hsize*msize*sizeof(real_t);
}

unsigned int preserveDomain(cd_handle_t *cdh, Domain* domain, char* name) {
  printf("inside %s\n", __func__);
  unsigned int size = sizeof(Domain);
  char prv_name[30];
  sprintf(prv_name, "domain_%s", name);
  cd_preserve(cdh, domain, size, kCopy, prv_name, prv_name);
  return size;
}

unsigned int preserveHaloExchange(cd_handle_t *cdh, HaloExchange* haloExchange, char* name) {
  printf("inside %s\n", __func__);
  unsigned int size = sizeof(HaloExchange);
  cd_preserve(cdh, haloExchange, size, kCopy, "haloExchange", "haloExchange");

  //preserve int* haloProc (array of halo procs to send/recieve)
  //NOTE: only preserve valid elements (guarded by haloExchange->haloCount)
  unsigned int size_haloProc = (haloExchange->haloCount)*sizeof(int);
  cd_preserve(cdh, haloExchange->haloProc, size_haloProc, kCopy, "haloExchange_haloProc", "haloExchange_haloProc");
  size += size_haloProc;

  //preserve int* rlist (array of request indices for non-blocking receives)
  //NOTE: haloCount varies over time and over ranks
  unsigned int size_rlist = (haloExchange->haloCount)*sizeof(int);
  cd_preserve(cdh, haloExchange->rlist, size_rlist, kCopy, "haloExchange_rlist", "haloExchange_rlist");
  size += size_rlist;

  //SZNOTE: not need to preserve sendBuf and recvBuf, but need to preserve address information
  //        sendBuf's address information has been preserved with preservation of struct HaloExchange
  //preserve char** recvBuf
  unsigned int size_recvBuf_ptr = getNRanks()*sizeof(char*);
  cd_preserve(cdh, haloExchange->recvBuf, size_recvBuf_ptr, kCopy, "haloExchange_recvBuf_ptr", "haloExchange_recvBuf_ptr");
  size += size_recvBuf_ptr;

  return size;
}
#endif
