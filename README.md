EvioTool
========
Author: Maurik Holtrop @ UNH  5/7/14 & 2/15/19

A tool for _fast_ reading of EVIO raw data in C++ from a file or from the ET ring.

This tool was created to avoid the issues with the libevioxx library, the standard library for reading EVIO with C++. The issues are that that implementation is not easy to use with ROOT, and it is very slow.

The current version, though a work in progress, is functional and has shown read speeds of 100kHz on a laptop, about 10x faster than libevioxx. 

For more details on this code, go to the [Wiki](https://github.com/JeffersonLab/EvioTool/wiki/Building-EvioTool)

## Building EvioTool

This tool uses a standard "cmake" build scheme. This includes a build of needed the EVIO and ET libraries.

Steps:

* git clone https://github.com/JeffersonLab/EvioTool.git
* cd EvioTool
* mkdir build # Always use out-of-place build.
* cd build
* cmake -DCMAKE\_INSTALL\_PREFIX=${HOME} # Replace ${HOME} with install dir.
* make 
* make install

The libraries currently created are:

* libevio-4.4.6
* libeviocxx-4.4.6    # Build but  not needed.
* libEvioTool 
* libHPSEvioReader

The executatble currently created are:

* EvioTool_Test
* HPSEvioReader_Test
* evio2xml
* eviocopy

 