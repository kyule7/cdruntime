#!/bin/bash 

# Build BLCR in your machine

#---------------------------------------------------------------
# a few global variables
#---------------------------------------------------------------
PKG_DIR=~/debug_kernel_image
POOL="http://ddebs.ubuntu.com/pool/main/l/linux/" 
WGET=/usr/bin/wget
SUDO=/usr/bin/sudo
DPKG=/usr/bin/dpkg
MKDIR=/bin/mkdir
RM=/bin/rm 

#---------------------------------------------------------------
# check my architecture
#---------------------------------------------------------------
HW=`uname -m`
case "${HW}" in 
   i386)
      ARCH="i386"
      ;;
   i686) 
      ARCH="i386"
      ;;
   x86_64)
      ARCH="amd64"
      ;;
   *)
      echo "-> Unknow architecture ${HW}"
      exit 1
esac
echo "Architecture for kernel: ${ARCH}" 

#--------------------------------------------------------------
# check if image is already installed
#--------------------------------------------------------------
KERNEL="linux-image-$(uname -r)-dbgsym"
${DPKG} -l "${KERNEL}" > /dev/null
[ $? -eq 0 ] && echo "${KERNEL} is already installed. Quitting." && exit 0

#---------------------------------------------------------------
# find matching kernel with debug symbols in pool
#---------------------------------------------------------------
DBG_KERNEL_LINK=`${WGET} ${POOL} -O - 2> /dev/null | grep "${KERNEL}" | grep ${ARCH} | sed 's/^.*href=\"//g' | sed 's/\".*$//g' | tail -n 1`

#--------------------------------------------------------------
# Create ${PKG_DIR} 
#--------------------------------------------------------------
${MKDIR} ${PKG_DIR} 

#--------------------------------------------------------------
# load kernel with debug symbols from POOL
#--------------------------------------------------------------
${RM} ${DBG_KERNEL_LINK}
${WGET} ${POOL}/${DBG_KERNEL_LINK} 

#--------------------------------------------------------------
# install the kernel 
#--------------------------------------------------------------
${SUDO} ${DPKG} -i ${DBG_KERNEL_LINK} 

tar zxvf ./blcr-0.8.5.tar.gz
cd blcr-0.8.5
mkdir builddir
cd ./builddir
${SUDO} ../configure
${SUDO} make
${SUDO} make install
${SUDO} make insmod check

#--------------------------------------------------------------
# Patch for memory exclusion functionality
#--------------------------------------------------------------
${SUDO} patch < ../../mem_excl.patch00

#--------------------------------------------------------------
# Load BLCR kernel module 
#--------------------------------------------------------------
${SUDO} /sbin/insmod /usr/local/lib/blcr/$(uname -r)/blcr_imports.ko
${SUDO} /sbin/insmod /usr/local/lib/blcr/$(uname -r)/blcr.ko


