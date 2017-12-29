#include <assert.h>
#include "cd.h"
#include "cd_cosp2.h"
#include "parallel.h" //getNRanks()

unsigned int preserveCommand(cd_handle_t *cdh, Command* cmd) {
  unsigned int size = sizeof(Command);
  cd_preserve(cdh, cmd, size, kCopy, "cmd", "cmd");
  return size;
}

// preserve SparseMatrix structure
// hsize: number of rows
// msize: the max number of non-zeroes per row
unsigned int preserveSparseMatrix(cd_handle_t *cdh,  SparseMatrix* spm, char *name) {
 unsigned int size = sizeof(SparseMatrix);
 //TODO: add matrix name passed from argument
 char spm_with_name[16] = "-1";
 assert(name != NULL);
 sprintf(spm_with_name, "spm_%s", name);
 cd_preserve(cdh, spm, size, kCopy, spm_with_name, spm_with_name);

 //FIXME: it seems to be not necessary to preserve both jja and jjcontig (and val and valcontig) once initialized

 // preserve iia (1D array containing number of non-zero elements per row)
  size += preserveNZs_ELL(cdh, spm->iia, spm->hsize, name);
 // preserve jja (2D array containing column indices of values per row)
  size += preserveIndices_ELL(cdh, spm->jja, spm->hsize, spm->msize, name);
 // perserve val (2D array containing non-zero elements per row)
  size += preserveValues_ELL(cdh, spm->val, spm->hsize, spm->msize, name);
  return size;
}

unsigned int preserveNZs_ELL(cd_handle_t *cdh, int* iia, const int hsize, char* name) {
  unsigned int size = hsize*sizeof(int);
  char iia_with_name[16] = "-1";
  assert(name != NULL);
  sprintf(iia_with_name, "iia_%s", name);
  //TODO: add matrix name passed from argument
  cd_preserve(cdh, iia, size, kCopy, iia_with_name, iia_with_name);
  return size;
}

unsigned int preserveIndices_ELL(cd_handle_t *cdh, int** jja, const int hsize, const int msize, char* name) {
  unsigned int size = 0;
  char jja_with_name[6] = "-1";
  assert(name != NULL);
  sprintf(jja_with_name, "jja_%s", name);

#ifdef CONTIG_MATRIX
  size = hsize*msize*sizeof(int);
  //FIXME: if not CONTIG_MATRIX, need to preserve one row at a time
  //TODO: for now, let's assume CONTIG_MATRIX is always defined
  // Note that jja is 2D array pointer
  cd_preserve(cdh, *jja, size, kCopy, jja_with_name, jja_with_name);
#else
  char jja_with_row_idx[256] = "-1";
  size = hsize*sizeof(int);
  for (int i = 0; i < hsize; i++) {
    sprintf(jja_with_row_idx, "jja_%s_%d", name, i);
    //FIXME
    cd_preserve(cdh, (*jja)+(i*msize*sizeof(int*)), size, kCopy, jja_with_row_idx, jja_with_row_idx);
  }
#endif
  return size;
}

unsigned int preserveValues_ELL(cd_handle_t *cdh, real_t** val, const int hsize, const int msize, char* name) {
  unsigned int size = 0;
  assert(name != NULL);

#ifdef CONTIG_MATRIX
  char val_with_name[6] = "-1";
  sprintf(val_with_name, "val_%s", name);
  size = hsize*msize*sizeof(real_t);
  //FIXME: if not CONTIG_MATRIX, need to preserve one row at a time
  //TODO: for now, let's assume CONTIG_MATRIX is always defined
  // Note that val is 2D array pointer
  cd_preserve(cdh, *val, size, kCopy, val_with_name, val_with_name);
#else
  char val_with_row_idx[256] = "-1";
  size = hsize*sizeof(int);
  for (int i = 0; i < hsize; i++) {
    sprintf(val_with_row_idx, "val_%s_%d", name, i);
    //FIXME
    cd_preserve(cdh, (*val)+(i*msize*sizeof(real_t*)), size, kCopy, val_with_row_idx, val_with_row_idx);
  }
#endif
  return size;
}

unsigned int preserveDomain(cd_handle_t *cdh, Domain* domain) {
  unsigned int size = sizeof(Domain);
  cd_preserve(cdh, domain, size, kCopy, "domain", "domain");
  return size;
}

unsigned int preserveHaloExchange(cd_handle_t *cdh, HaloExchange* haloExchange) {
  unsigned int size = sizeof(HaloExchange);
  cd_preserve(cdh, haloExchange, size, kCopy, "haloExchange", "haloExchange");

  //preserve int* haloProc (array of halo procs to send/recieve)
  unsigned int size_haloProc = (haloExchange->maxHalo)*sizeof(int);
  cd_preserve(cdh, haloExchange->haloProc, size_haloProc, kCopy, "haloExchange_haloProc", "haloExchange_haloProc");
  size += size_haloProc;

  //preserve int* rlist (array of request indices for non-blocking receives)
  //TODO: haloCount varies over time and over ranks
  unsigned int size_rlist = (haloExchange->haloCount)*sizeof(int);
  cd_preserve(cdh, haloExchange->rlist, size_rlist, kCopy, "haloExchange_rlist", "haloExchange_rlist");
  size += size_rlist;

  //preserve char* sendBuf 
  unsigned int size_sendBuf = (haloExchange->bufferSize)*sizeof(char);
  cd_preserve(cdh, haloExchange->sendBuf, size_sendBuf, kCopy, "haloExchange_sendBuf", "haloExchange_sendBuf");
  size += size_sendBuf;

  //preserve char** recvBuf
  unsigned int size_recvBuf_ptr = getNRanks()*sizeof(char*);
  cd_preserve(cdh, haloExchange->recvBuf, size_recvBuf_ptr, kCopy, "haloExchange_recvBuf_ptr", "haloExchange_recvBuf_ptr");
  size += size_recvBuf_ptr;

  unsigned int size_recvBuf = (haloExchange->bufferSize)*sizeof(char);
  char recvBuf_with_idx[6] = "-1";
  for (int i = 0; i < getNRanks(); i++) {
    sprintf(recvBuf_with_idx, "recvBuf_%d", i);
    cd_preserve(cdh, &(haloExchange->recvBuf[i]), size_recvBuf, kCopy, recvBuf_with_idx, recvBuf_with_idx);
    size += size_recvBuf;
  }

  return size;
}
