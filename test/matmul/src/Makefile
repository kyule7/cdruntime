################################################################################
#
# Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
#
# NOTICE TO USER:
#
# This source code is subject to NVIDIA ownership rights under U.S. and
# international Copyright laws.
#
# NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
# CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
# IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
# IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
# OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
# OR PERFORMANCE OF THIS SOURCE CODE.
#
# U.S. Government End Users.  This source code is a "commercial item" as
# that term is defined at 48 C.F.R. 2.101 (OCT 1995), consisting  of
# "commercial computer software" and "commercial computer software
# documentation" as such terms are used in 48 C.F.R. 12.212 (SEPT 1995)
# and is provided to the U.S. Government only as a commercial end item.
# Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
# 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
# source code with only those rights set forth herein.
#
################################################################################
#
# Makefile project only supported on Mac OS X and Linux Platforms)
#
################################################################################

# Location of the CUDA Toolkit
CUDA_PATH ?= "/home/utils/cuda_7.0.28"
CD_ROOT ?= "/home/kyushickl/scratch/work/CDs/overhead"

##############################
# start deprecated interface #
##############################
ifeq ($(x86_64),1)
    $(info WARNING - x86_64 variable has been deprecated)
    $(info WARNING - please use TARGET_ARCH=x86_64 instead)
    TARGET_ARCH ?= x86_64
endif
ifeq ($(ARMv7),1)
    $(info WARNING - ARMv7 variable has been deprecated)
    $(info WARNING - please use TARGET_ARCH=armv7l instead)
    TARGET_ARCH ?= armv7l
endif
ifeq ($(aarch64),1)
    $(info WARNING - aarch64 variable has been deprecated)
    $(info WARNING - please use TARGET_ARCH=aarch64 instead)
    TARGET_ARCH ?= aarch64
endif
ifeq ($(ppc64le),1)
    $(info WARNING - ppc64le variable has been deprecated)
    $(info WARNING - please use TARGET_ARCH=ppc64le instead)
    TARGET_ARCH ?= ppc64le
endif
ifneq ($(GCC),)
    $(info WARNING - GCC variable has been deprecated)
    $(info WARNING - please use HOST_COMPILER=$(GCC) instead)
    HOST_COMPILER ?= $(GCC)
endif
ifneq ($(abi),)
    $(error ERROR - abi variable has been removed)
endif
############################
# end deprecated interface #
############################

# architecture
HOST_ARCH   := $(shell uname -m)
TARGET_ARCH ?= $(HOST_ARCH)
ifneq (,$(filter $(TARGET_ARCH),x86_64 aarch64 ppc64le))
    TARGET_SIZE := 64
else ifeq ($(TARGET_ARCH),armv7l)
    TARGET_SIZE := 32
else
    $(error ERROR - unsupported value $(TARGET_ARCH) for TARGET_ARCH!)
endif
ifneq ($(TARGET_ARCH),$(HOST_ARCH))
    ifeq (,$(filter $(HOST_ARCH)-$(TARGET_ARCH),aarch64-armv7l x86_64-armv7l x86_64-aarch64 x86_64-ppc64le))
        $(error ERROR - cross compiling from $(HOST_ARCH) to $(TARGET_ARCH) is not supported!)
    endif
endif

# operating system
HOST_OS   := $(shell uname -s 2>/dev/null | tr "[:upper:]" "[:lower:]")
TARGET_OS ?= $(HOST_OS)
ifeq (,$(filter $(TARGET_OS),linux darwin qnx android))
    $(error ERROR - unsupported value $(TARGET_OS) for TARGET_OS!)
endif

# host compiler
ifeq ($(TARGET_OS),darwin)
    ifeq ($(shell expr `xcodebuild -version | grep -i xcode | awk '{print $$2}' | cut -d'.' -f1` \>= 5),1)
        HOST_COMPILER ?= clang++
    endif
else ifneq ($(TARGET_ARCH),$(HOST_ARCH))
    ifeq ($(HOST_ARCH)-$(TARGET_ARCH),x86_64-armv7l)
        ifeq ($(TARGET_OS),linux)
            HOST_COMPILER ?= arm-linux-gnueabihf-g++
        else ifeq ($(TARGET_OS),qnx)
            ifeq ($(QNX_HOST),)
                $(error ERROR - QNX_HOST must be passed to the QNX host toolchain)
            endif
            ifeq ($(QNX_TARGET),)
                $(error ERROR - QNX_TARGET must be passed to the QNX target toolchain)
            endif
            export QNX_HOST
            export QNX_TARGET
            HOST_COMPILER ?= $(QNX_HOST)/usr/bin/arm-unknown-nto-qnx6.6.0eabi-g++
        else ifeq ($(TARGET_OS),android)
            HOST_COMPILER ?= arm-linux-androideabi-g++
        endif
    else ifeq ($(TARGET_ARCH),aarch64)
        ifeq ($(TARGET_OS), linux)
            HOST_COMPILER ?= aarch64-linux-gnu-g++
        else ifeq ($(TARGET_OS), android)
            HOST_COMPILER ?= aarch64-linux-android-g++
        endif
    else ifeq ($(TARGET_ARCH),ppc64le)
        HOST_COMPILER ?= powerpc64le-linux-gnu-g++
    endif
endif
HOST_COMPILER ?= mpicxx
NVCC          := $(CUDA_PATH)/bin/nvcc
MULTI_NVCC    := ${NVCC} -ccbin $(HOST_COMPILER)

# internal flags
NVCCFLAGS   := -m${TARGET_SIZE}
CCFLAGS     :=
LDFLAGS     :=

#CCFLAGS += #-std=gnu++0x -I${CD_ROOT}/include
#LDFLAGS += -lmpi #-L${CD_ROOT}/lib -lcd -Wl,-rpath=${CD_ROOT}/lib

# build flags
ifeq ($(TARGET_OS),darwin)
    LDFLAGS += -rpath $(CUDA_PATH)/lib
    CCFLAGS += -arch $(HOST_ARCH)
else ifeq ($(HOST_ARCH)-$(TARGET_ARCH)-$(TARGET_OS),x86_64-armv7l-linux)
    LDFLAGS += --dynamic-linker=/lib/ld-linux-armhf.so.3
    CCFLAGS += -mfloat-abi=hard
else ifeq ($(TARGET_OS),android)
    LDFLAGS += -pie
    CCFLAGS += -fpie -fpic -fexceptions
endif

ifneq ($(TARGET_ARCH),$(HOST_ARCH))
    ifeq ($(TARGET_ARCH)-$(TARGET_OS),armv7l-linux)
        ifneq ($(TARGET_FS),)
            GCCVERSIONLTEQ46 := $(shell expr `$(HOST_COMPILER) -dumpversion` \<= 4.6)
            ifeq ($(GCCVERSIONLTEQ46),1)
                CCFLAGS += --sysroot=$(TARGET_FS)
            endif
            LDFLAGS += --sysroot=$(TARGET_FS)
            LDFLAGS += -rpath-link=$(TARGET_FS)/lib
            LDFLAGS += -rpath-link=$(TARGET_FS)/usr/lib
            LDFLAGS += -rpath-link=$(TARGET_FS)/usr/lib/arm-linux-gnueabihf
        endif
    endif
endif

# Debug build flags
ifeq ($(dbg),1)
      CCFLAGS += -g
      BUILD_TYPE := debug
else
      BUILD_TYPE := release
endif

ALL_CCFLAGS :=
ALL_CCFLAGS += $(NVCCFLAGS)
ALL_CCFLAGS += $(EXTRA_NVCCFLAGS)
ALL_CCFLAGS += $(addprefix -Xcompiler ,$(CCFLAGS))
ALL_CCFLAGS += $(addprefix -Xcompiler ,$(EXTRA_CCFLAGS))

ALL_LDFLAGS :=
ALL_LDFLAGS += $(ALL_CCFLAGS) 
ALL_LDFLAGS += $(addprefix -Xlinker ,$(LDFLAGS))
ALL_LDFLAGS += $(addprefix -Xlinker ,$(EXTRA_LDFLAGS))

# Common includes and paths for CUDA
INCLUDES  := -I./inc
LIBRARIES :=

#INCLUDES += -I${CD_ROOT}/include
#LIBRARIES += -lmpi -L${CD_ROOT}/lib -lcd

################################################################################

UBUNTU = $(shell lsb_release -i -s 2>/dev/null | grep -i ubuntu)

SAMPLE_ENABLED := 1

# This sample is not supported on ARM
ifeq ($(TARGET_ARCH),armv7l)
  $(info >>> WARNING - matmul_cuda_nvrtc is not supported on ARM - waiving sample <<<)
  SAMPLE_ENABLED := 0
endif

# libNVRTC specific libraries
ifeq ($(TARGET_OS),darwin)
 LDFLAGS += -L$(CUDA_PATH)/lib -framework CUDA
else ifeq ($(TARGET_ARCH),x86_64)
 LDFLAGS += -L$(CUDA_PATH)/lib64 -L$(CUDA_PATH)/lib64/stubs
else ifeq ($(TARGET_ARCH),ppc64le)
 LDFLAGS += -L$(CUDA_PATH)/targets/ppc64le-linux/lib -L$(CUDA_PATH)/targets/ppc64le-linux/lib/stubs
endif

ifeq ($(TARGET_OS),darwin)
  ALL_LDFLAGS += -Xlinker -framework -Xlinker CUDA
else
  CUDA_SEARCH_PATH ?=
  ifneq ($(TARGET_ARCH),$(HOST_ARCH))
    ifeq ($(TARGET_ARCH)-$(TARGET_OS),armv7l-linux)
      ifneq ($(TARGET_FS),)
        LIBRARIES += -L$(TARGET_FS)/usr/lib
        CUDA_SEARCH_PATH += $(TARGET_FS)/usr/lib
        CUDA_SEARCH_PATH += $(TARGET_FS)/usr/lib/arm-linux-gnueabihf
      endif
      CUDA_SEARCH_PATH += $(CUDA_PATH)/targets/armv7-linux-gnueabihf/lib/stubs
      CUDA_SEARCH_PATH += /usr/arm-linux-gnueabihf/lib
    else ifeq ($(TARGET_ARCH)-$(TARGET_OS),armv7l-android)
      CUDA_SEARCH_PATH += $(CUDA_PATH)/targets/armv7-linux-androideabi/lib/stubs
    else ifeq ($(TARGET_ARCH)-$(TARGET_OS),aarch64-android)
      CUDA_SEARCH_PATH += $(CUDA_PATH)/targets/aarch64-linux-androideabi/lib/stubs
    else ifeq ($(TARGET_ARCH)-$(TARGET_OS),ppc64le-linux)
      CUDA_SEARCH_PATH += $(CUDA_PATH)/targets/ppc64le-linux/lib/stubs
    endif
  else
    ifneq ($(UBUNTU),)
      CUDA_SEARCH_PATH += /usr/lib
    else
      CUDA_SEARCH_PATH += /usr/lib64
    endif

    ifeq ($(TARGET_ARCH),x86_64)
      CUDA_SEARCH_PATH += $(CUDA_PATH)/lib64/stubs
    endif

    ifeq ($(TARGET_ARCH),armv7l)
      CUDA_SEARCH_PATH += $(CUDA_PATH)/targets/armv7-linux-gnueabihf/lib/stubs
      CUDA_SEARCH_PATH += /usr/lib/arm-linux-gnueabihf
    endif

    ifeq ($(TARGET_ARCH),aarch64)
      CUDA_SEARCH_PATH += /usr/lib
    endif

    ifeq ($(TARGET_ARCH),ppc64le)
      CUDA_SEARCH_PATH += /usr/lib/powerpc64le-linux-gnu
    endif
  endif

  CUDALIB ?= $(shell find -L $(CUDA_SEARCH_PATH) -maxdepth 1 -name libcuda.so)
  ifeq ("$(CUDALIB)","")
    $(info >>> WARNING - libcuda.so not found, CUDA Driver is not installed.  Please re-install the driver. <<<)
    SAMPLE_ENABLED := 0
  endif

  LIBRARIES += -lcuda
endif

INCLUDES += -I$(CUDA_PATH)/include

LIBRARIES += -lnvrtc

ifeq ($(SAMPLE_ENABLED),0)
EXEC ?= @echo "[@]"
endif

################################################################################
CUDA_RT_MM = 1
ENABLE_CD = 1
ENABLE_MPI = 1

# Target rules
all: build

ifeq (${ENABLE_MPI}, 1)
build: test_mm test_cuda matmul_cuda.o
else
build: test_cuda matmul_cuda.o
endif

check.deps:
ifeq ($(SAMPLE_ENABLED),0)
	@echo "Sample will be waived due to the above missing dependencies"
else
	@echo "Sample is ready - all dependencies have been met"
endif

#
#matmul_cuda_nvrtc: matmul_cuda.o


CCFLAGS+= -Xcompiler
LIBRARIES += -lgomp

CCFLAGS+= -std=gnu++0x -O2 -g -D_DEBUG=0 -D_CUDA=1 -I.

ifeq (${ENABLE_CD}, 1)
CCFLAGS += -I${CD_ROOT}/include -D_CD=1
LIBRARIES += -L${CD_ROOT}/lib -lcd -Xlinker -rpath=${CD_ROOT}/lib
endif

ifeq (${ENABLE_MPI}, 1)
CCFLAGS += -D_MPI=1
LIBRARIES += -Xlinker -rpath-link=${MPI_PATH}/lib #-lmpi
else
CCFLAGS += -D_MPI=0
endif


#######################################################################################

matmul_cuda.o: matmul_cuda.cc
	$(EXEC) $(NVCC) $(INCLUDES) $(CCFLAGS) $(EXTRA_CCFLAGS) -o $@ -c $<

ifeq (${ENABLE_CD}, 1)
test_mm: test_mm.cc matmul.h matmul_seq.h matmul_omp.h matmul_mpi.h util.h matmul_cuda.cc matmul_cuda.o
	$(MULTI_NVCC) $(CCFLAGS) $(LDFLAGS) test_mm.cc matmul_cuda.o -o ../bin/test_mm -lm $(LIBRARIES)
endif

ifeq (${CUDA_RT_MM},0)
test_cuda: test_cuda.cc util.h matmul_cuda.cc matmul_cuda.o
	$(NVCC) $(CCFLAGS) -D_CUDA_RT_MM=${CUDA_RT_MM} ${LDFLAGS} test_cuda.cc matmul_cuda.o -o ../bin/test_cuda -lm ${LIBRARIES}
else

matmul_cuda_rt.o: matmul_cuda_rt.cu
	$(NVCC) $(INCLUDES) $(ALL_CCFLAGS) -o matmul_cuda_rt.o -c matmul_cuda_rt.cu

test_cuda: test_cuda.cc util.h matmul_cuda.cc matmul_cuda_rt.o matmul_cuda.o
	$(MULTI_NVCC) $(CCFLAGS) -D_CUDA_RT_MM=${CUDA_RT_MM} ${LDFLAGS} test_cuda.cc matmul_cuda.o matmul_cuda_rt.o -o ../bin/test_cuda -lm ${LIBRARIES}
endif
#test_single: test_single.cc matmul.h matmul_seq.h matmul_omp.h matmul_mpi.h util.h matmul_cuda.cc
#	$(NVCC) $(CCFLAGS) ${LDFLAGS} test_single.cc matmul_cuda.o -o ../bin/test_omp -lm ${LIBRARIES}

#######################################################################################

#	$(EXEC) $(NVCC) $(LDFLAGS) -o $@ $+ $(LIBRARIES)
#	$(EXEC) mkdir -p ../../bin/$(TARGET_ARCH)/$(TARGET_OS)/$(BUILD_TYPE)
#	$(EXEC) cp $@ ../../bin/$(TARGET_ARCH)/$(TARGET_OS)/$(BUILD_TYPE)

run: build
	$(EXEC) ./matmul_cuda_nvrtc

clean:
	rm -f matmul_cuda_nvrtc matmul_cuda.o
	rm *~; rm ../bin/test_mm; rm dbg_*
	rm -rf ../../bin/$(TARGET_ARCH)/$(TARGET_OS)/$(BUILD_TYPE)/matmul_cuda_nvrtc

clobber: clean
