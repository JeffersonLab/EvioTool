#!/bin/bash
ARCH=`uname -s`-`uname -m`
echo "Staring ET for ${ARCH}"
if [ ${ARCH} = "Linux-x86_64" ]; then
    SCRIPTS=/home/maurik/Documents/codes/EvioTool/scripts
else
    SCRIPTS=/Users/maurik/Documents/codes/EvioTool/scripts
fi
cd $SCRIPTS
export DYLD_LIBRARY_PATH=/data/CLAS12/${ARCH}/lib 
export LD_LIBRARY_PATH=/data/CLAS12/${ARCH}/lib 
echo "Xterm 1"
xterm -bg '#300000' -fg white -e ${SCRIPTS}/Et_Start.sh &
sleep 2
echo "Xterm 2"
xterm -bg '#002020' -fg white -e /data/CLAS12/${ARCH}/bin/et_monitor  -f /tmp/ETBuffer -p 11111 &
echo "Started"


