EvioTool
========
Author: Maurik Holtrop @ UNH  5/7/14 & 2/15/19

A tool for _fast_ reading of EVIO raw data in C++ from a file or from the ET ring.

This tool was created to avoid the issues with the libevioxx library, the standard library for reading EVIO with C++. The issues are that that implementation is not easy to use with ROOT, and it is very slow.

The current version, though a work in progress, is functional and has shown read speeds of 100kHz on a laptop, about 10x faster than libevioxx. 

For more details on this code, go to the [Wiki](https://github.com/JeffersonLab/EvioTool/wiki)

## Building EvioTool

This tool uses a standard "cmake" build scheme. This includes a build of needed the EVIO and ET libraries.

Requirements:

* An installation of [ROOT](http://root.cern.ch) 
    * Tested with ROOT versions up to 6.20

Steps:

* git clone https://github.com/JeffersonLab/EvioTool.git
* cd EvioTool
* mkdir build # Always use out-of-place build.
* cd build
* cmake -DCMAKE\_INSTALL\_PREFIX=${HOME} .. # Replace ${HOME} with install dir.
* make 
* make install

The libraries currently created are:

* libevio-5.1
    * The libevioxx is skipped by default.
* libEvioTool 
* libHPSEvioReader

The executatble currently created include:

* EvioTool_Test
* HPSEvioReader_Test
* HPS_Trigger_Filter
* evio2xml
* eviocopy

## Using the HPS_Trigger_Filter

Make sure that the libraries created in the build step are found by your operating system. 
On Linux, this means setting LD_LIBRARY_PATH to include the directory where you installed
libraries (on MacOS replace LD_LIBRARY_PATH with DYLD_LIBRARY_PATH):
* export LD_LIBRARY_PATH=${HOME}/lib:${LD_LIBRARY_PATH}

The trigger filter can now be run as:
 
* HPS_Trigger_Filter -o filtered_file.evio  -trigger FEE  inputfile1.evio [inputfile2.evio etc]

Details about the options are provided with the -h option, currently:

    HPS_Trigger_Filter <options>  EVIO_file 
    
     Options: 
      -q                 Quiet 
      -a  -analyze       Analyze triggers in addition to fileter.
      -x  -nooutput      Do not write an output file, analyze only.
      -d  -debug         Debug 
      -o  -output  name  Output file. (If not provided, output is <infile>_FEE.evio)
      -T  -trigger name -bits bitpat  Filter on trigger name (default: FEE) or bit pattern.
                         Bit pattern is an integer ie: 16 or 0x10 or 0b010000000 
      -E  -exclusive     Use exclusive filtering = only the exact bit pattern passes.
      -et                Use ET ring 
      -f  -et_name name  Attach ET to process with file <name>
      -H  -host    host  Attach ET to host
      -p  -et_port port  Attach ET to port 
    
    
    Trigger filter arguments to the -T (-trigger) option: 
     'FEE'      - FEE either top or bottom 
     'FEE_Top'  - FEE top only. 
     'FEE_Bot'  - FEE bottom only
     'muon'     - Pair3 mu+mu- trigger
     '2gamma'   - Multiplicity-0 or 2 photon trigger. 
     '3gamma'   - Multiplicity-1 or 3 photon trigger. 
     'pulser'   - Pulser trigger bit. 
     'fcup'     - Faraday Cup trigger + pulser trigger.
     '######'   - Where ##### is an integer value (int, hex, bin) whose bits represent the trigger you want.
    
    NOTES: 
    You cannot use both the -T and -B switches to set the bit pattern. -B will override -T.
    For the -analyze switch, this code will count the triggers of each type and print
    the result to the screen, including the total number of events processed.
    This can be used with the -x switch if you want only to analyze. 
    NOTE: The trigger analyzer will count one event with two trigger bits set once for each bit,
    so the number of triggers counted will be larger than the number of events.

