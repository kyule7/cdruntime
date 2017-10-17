#mpicxx -g -std=gnu++0x -fmax-errors=1 -I${CD_PATH}${CD_PATH}src/persistence -I${CD_PATH}${CD_PATH}src -I${CD_PATH}${CD_PATH}include test_vector.cc -o test_vector -L${CD_PATH}${CD_PATH}src/persistence -lpacker -Wl,-rpath=${CD_PATH}${CD_PATH}src/persistence
#CD_PATH=/home/kyushick/Work/CDs/pfsFix2/pfsFix 
CD_PATH=$(pwd)/../..
#echo $CD_PATH
mpicxx -g -std=gnu++0x -fmax-errors=1 -I${CD_PATH}/src/persistence -I${CD_PATH}/plugin/wrapper -I${CD_PATH}/src -I${CD_PATH}/include test_vector.cc -o test_vector \
  -L${CD_PATH}/lib -lcd -Wl,-rpath=${CD_PATH}/lib \
  -L${CD_PATH}/src/persistence -lpacker -Wl,-rpath=${CD_PATH}/src/persistence \
  -lmpi -lmpicxx -lc -L${CD_PATH}/plugin/wrapper -lwrapLibc -Wl,-rpath=${CD_PATH}/plugin/wrapper
#mpicxx -g -std=gnu++0x -fmax-errors=1 -I${CD_PATH}/src/persistence -I${CD_PATH}/plugin/wrapper -I${CD_PATH}/src -I${CD_PATH}/include test_vector.cc -o test_vector \
#  -L${CD_PATH}/plugin/wrapper -lwrapLibc -Wl,-rpath=${CD_PATH}/plugin/wrapper -lc \
#  -L${CD_PATH}/lib -lcd -Wl,-rpath=${CD_PATH}/lib \
#  -L${CD_PATH}/src/persistence -lpacker -Wl,-rpath=${CD_PATH}/src/persistence
