#!/bin/sh
#
# Quick and dumb way to build the EVIO libraries.
# Sorry, but the standard Makefile or SConstruct are simply overkill, and make installation more difficult.
#
# CLEANUP first.
#
rm -rf bin lib obj
mkdir bin
mkdir lib
mkdir obj
#
# Account for minor differences between machines.
#
ARCH=`uname -s`
MACH=`uname -m`

echo "Arch  = $ARCH "

if [ $ARCH = "Darwin" ]; then
    FLAGS=" -arch i386 -arch x86_64 -O3"
    LIBS=" -L/usr/lib -L/usr/local/lib "
    INSTALL_ARCH="Darwin-x86_64"
fi
if [ $ARCH = "Linux" ]; then
    FLAGS="-O3 "
    if [ $MACH = "i386" ]; then
	LIBS=" -L/usr/lib -L/usr/local/lib "
    else
	LIBS=" -L/usr/lib64 -L/usr/local/lib64 -L/usr/local/lib -lpthread"
    fi
    INSTALL_ARCH="Linux-x86_64"
fi

INCL=" -I/usr/include -I/usr/local/include -Isrc/libsrc -Isrc/libsrc++ "

gcc -o obj/evio.o  -c $FLAGS -fPIC $INCL src/libsrc/evio.c
gcc -o obj/evioswap.o -c $FLAGS -fPIC $INCL src/libsrc/evioswap.c
gcc -o obj/eviofmt.o  -c $FLAGS -fPIC $INCL src/libsrc/eviofmt.c
gcc -o obj/eviofmtswap.o -c $FLAGS -fPIC $INCL src/libsrc/eviofmtswap.c
gcc $FLAGS -o lib/libevio.so -shared obj/evio.o obj/evioswap.o obj/eviofmt.o obj/eviofmtswap.o $LIBS
ar r   lib/libevio.a  obj/evio.o obj/evioswap.o obj/eviofmt.o obj/eviofmtswap.o
ranlib lib/libevio.a
g++ -o obj/evioBankIndex.o     -c $FLAGS -fPIC $INCL src/libsrc++/evioBankIndex.cc
g++ -o obj/evioBufferChannel.o -c $FLAGS -fPIC $INCL src/libsrc++/evioBufferChannel.cc
g++ -o obj/evioDictionary.o -c $FLAGS -fPIC $INCL src/libsrc++/evioDictionary.cc
g++ -o obj/evioException.o     -c $FLAGS -fPIC $INCL src/libsrc++/evioException.cc
g++ -o obj/evioFileChannel.o   -c $FLAGS -fPIC $INCL src/libsrc++/evioFileChannel.cc
g++ -o obj/evioSocketChannel.o -c $FLAGS -fPIC $INCL src/libsrc++/evioSocketChannel.cc
g++ -o obj/evioUtil.o          -c $FLAGS -fPIC $INCL src/libsrc++/evioUtil.cc
g++ $FLAGS -o lib/libevioxx.so -shared obj/evioBankIndex.o obj/evioException.o obj/evioDictionary.o obj/evioFileChannel.o obj/evioBufferChannel.o obj/evioSocketChannel.o obj/evioUtil.o -Llib -levio $LIBS -lexpat -lz
ar r lib/libevioxx.a  obj/evioBankIndex.o obj/evioException.o obj/evioDictionary.o obj/evioFileChannel.o obj/evioBufferChannel.o obj/evioSocketChannel.o obj/evioUtil.o 
ranlib lib/libevioxx.a
g++ -o obj/evio2xml.o -c $FLAGS $INCL src/execsrc/evio2xml.cc
g++ -o bin/evio2xml obj/evio2xml.o -Llib $LIBS -levio -levioxx -lexpat -lz
gcc -o obj/eviocopy.o -c $FLAGS $INCL src/execsrc/eviocopy.c
gcc -o bin/eviocopy obj/eviocopy.o -Llib $LIBS -levio -lexpat -lz
gcc -o obj/xml2evio.o -c $FLAGS $INCL src/execsrc/xml2evio.c
gcc -o bin/xml2evio obj/xml2evio.o -Llib $LIBS -levio -lexpat -lz

echo "Installing to /data/CLAS12/${INSTALL_ARCH}/lib and bin"
mkdir -p  /data/CLAS12/${INSTALL_ARCH}/lib
mkdir -p  /data/CLAS12/${INSTALL_ARCH}/bin
cp lib/libevio*.so /data/CLAS12/${INSTALL_ARCH}/lib
cp lib/libevio*.a /data/CLAS12/${INSTALL_ARCH}/lib
cp bin/evio2xml   /data/CLAS12/${INSTALL_ARCH}/bin

if [ ${ARCH} = "Darwin" ]; then
install_name_tool -id  /data/CLAS12/${INSTALL_ARCH}/lib/libevio.so /data/CLAS12/${INSTALL_ARCH}/lib/libevio.so
install_name_tool -id  /data/CLAS12/${INSTALL_ARCH}/lib/libevioxx.so /data/CLAS12/${INSTALL_ARCH}/lib/libevioxx.so
install_name_tool -change lib/libevio.so /data/CLAS12/${INSTALL_ARCH}/lib/libevio.so /data/CLAS12/${INSTALL_ARCH}/lib/libevioxx.so
install_name_tool -change lib/libevio.so /data/CLAS12/${INSTALL_ARCH}/lib/libevio.so /data/CLAS12/${INSTALL_ARCH}/bin/evio2xml
install_name_tool -change lib/libevioxx.so /data/CLAS12/${INSTALL_ARCH}/lib/libevioxx.so /data/CLAS12/${INSTALL_ARCH}/bin/evio2xml
fi
