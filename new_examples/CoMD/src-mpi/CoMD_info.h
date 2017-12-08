#ifndef CoMD_info_hpp
#define CoMD_info_hpp

#define CoMD_VARIANT "CoMD-mpi"
#define CoMD_HOSTNAME "login1.stampede.tacc.utexas.edu"
#define CoMD_KERNEL_NAME "'Linux'"
#define CoMD_KERNEL_RELEASE "'2.6.32-431.17.1.el6.x86_64'"
#define CoMD_PROCESSOR "'x86_64'"

#define CoMD_COMPILER "'/opt/apps/gcc4_9/mvapich2/2.1/bin/mpicc'"
#define CoMD_COMPILER_VERSION "'gcc (GCC) 4.9.1'"
#define CoMD_CFLAGS "'-std=gnu11 -I../../..//include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1   -gdwarf-2 -DDOUBLE -DDO_MPI -g -O5   '"
#define CoMD_LDFLAGS "' -lm -L../../..//lib -lcd -Wl,-rpath=../../..//lib -L../../..//src/persistence -lpacker -Wl,-rpath=../../..//src/persistence '"

#endif
