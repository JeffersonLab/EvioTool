#!/bin/bash
ARCH=`uname -s`-`uname -m`
export DYLD_LIBRARY_PATH=/data/CLAS12/${ARCH}/lib 
export LD_LIBRARY_PATH=/data/CLAS12/${ARCH}/lib 
echo "Starting in ... 1 ..."
sleep 3
echo "now"
/data/CLAS12/${ARCH}/bin/et_start -f /tmp/ETBuffer -p 11111 -d -v


