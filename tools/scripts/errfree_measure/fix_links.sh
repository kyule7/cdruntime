#!/bin/bash
targetaddr=/scratch/03341/kyushick/lulesh/0312
cd run_8; 
rm lulesh_bare
rm lulesh_incr
rm lulesh_full
rm lulesh_cdrt
rm lulesh_fgcd
ln -s $targetaddr/lulesh_bare
ln -s $targetaddr/lulesh_incr
ln -s $targetaddr/lulesh_full
ln -s $targetaddr/lulesh_cdrt
ln -s $targetaddr/lulesh_fgcd
cd ..
cd run_64; 
rm lulesh_bare
rm lulesh_incr
rm lulesh_full
rm lulesh_cdrt
rm lulesh_fgcd
ln -s $targetaddr/lulesh_bare
ln -s $targetaddr/lulesh_incr
ln -s $targetaddr/lulesh_full
ln -s $targetaddr/lulesh_cdrt
ln -s $targetaddr/lulesh_fgcd
cd ..
cd run_216; 
rm lulesh_bare
rm lulesh_incr
rm lulesh_full
rm lulesh_cdrt
rm lulesh_fgcd
ln -s $targetaddr/lulesh_bare
ln -s $targetaddr/lulesh_incr
ln -s $targetaddr/lulesh_full
ln -s $targetaddr/lulesh_cdrt
ln -s $targetaddr/lulesh_fgcd
cd ..
cd run_512; 
rm lulesh_bare
rm lulesh_incr
rm lulesh_full
rm lulesh_cdrt
rm lulesh_fgcd
ln -s $targetaddr/lulesh_bare
ln -s $targetaddr/lulesh_incr
ln -s $targetaddr/lulesh_full
ln -s $targetaddr/lulesh_cdrt
ln -s $targetaddr/lulesh_fgcd
cd ..
cd run_1000; 
rm lulesh_bare
rm lulesh_incr
rm lulesh_full
rm lulesh_cdrt
rm lulesh_fgcd
ln -s $targetaddr/lulesh_bare
ln -s $targetaddr/lulesh_incr
ln -s $targetaddr/lulesh_full
ln -s $targetaddr/lulesh_cdrt
ln -s $targetaddr/lulesh_fgcd
cd ..

