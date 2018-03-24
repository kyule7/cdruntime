#!/bin/bash
mkdir -p m4
autoreconf --install
./configure --enable-parallel=mpi --enable-logging CXX=mpicxx CXXFLAGS="-std=gnu++0x -Wall -g -O -fPIC" --enable-tuning --enable-runtime --enable-error-injection --enable-profiler --enable-keep-preservation-files --enable-cd-test CC=icc
cd src/persistence/
make -j
cd ../../plugin/wrapper/
make -j
cd ../../src/
make install -j16
