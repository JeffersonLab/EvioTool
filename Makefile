# Makefile for ClasTool
#
# TOP: Definitions include.
#
# All standard definitions needed for building these libraries 
# are defined here.
#

SRC_CLASS= EvioTool/EvioTool.cc EvioTool/EvioEvent.cc

CREATED_SHLIB=libEvioTool.so

SRC_FILES= et_evio_blaster/et_evio_blaster.cc 
SRC_FILES2= et_evio_test_consumer/et_evio_test_consumer.cc
SRC_FILES3= EvioTool_Test/EvioTool_Test.cc

EXE_LIBS=$(CREATED_SHLIBS)         # This will link to the library
#EXE_LIBS=$(OBJ_SHLIB)                # This will link directly to the objects, so the library does not need to be installed!
CREATED_EXE=et_evio_blaster
CREATED_EXE2=et_evio_test_consumer
CREATED_EXE3=EvioTool_Test

#SRC_FILES3=  gemcFilter.cc
#EXE_LIBS3=$(OBJ_SHLIB)
#CREATED_EXE3= gemcFilter

ROOT_VERION_MAJOR=`root-config --verion | perl -e '<> =~ /([0-9])\.([0-9]+)\/([0-9]+)/; print $1;'`
ROOT_VERION_MINOR=`root-config --verion | perl -e '<> =~ /([0-9])\.([0-9]+)\/([0-9]+)/; print $2;'`
ROOT_VERION_SUBMINOR=`root-config --verion | perl -e '<> =~ /([0-9])\.([0-9]+)\/([0-9]+)/; print $3;'`

OS_NAME=$(shell uname -s)
ARCH=$(shell uname -m)
MACHINE=$(OS_NAME)-$(ARCH)

EVIO= ./evio-4.3.1         # /data/CLAS12/CODA/evio-4.3.1
EVIO_LIB=./evio-4.3.1/li   # /data/CLAS12/$(MACHINE)/lib
ET= ./et-14.0              # /data/CLAS12/CODA/et-14.0
ET_LIB= ./et-14.0/lib      #/data/CLAS12/$(MACHINE)/lib

OTHERLOADLIBS= $(EVIO_LIB)/libevioxx.a $(EVIO_LIB)/libevio.a -L$(ET_LIB) -let -lexpat -lz

#
# "Path" to search for include files 
#
SEARCH_INCLUDES += 
#
# Locate the INCLUDE directory.
#
INCLUDES=-I. -I./EvioTool  -I$(EVIO)/src/libsrc -I$(EVIO)/src/libsrc -I$(EVIO)/src/libsrc++ -I$(ET)/$(MACHINE)/include
#


all: shlib exe exe2 exe3


ifndef ROOTSYS
  $(error "PLEASE setup the ROOT system properly. ROOTSYS is not defined.")
endif

SHELL=/bin/sh

ifeq "$(ROOTSYS)" ""
  all: shlib

  shlib:
	@echo PLEASE SETUP ROOT. Usually, source the thisroot.sh in the root bin directory.
	@echo ALSO read the readme.
endif

#
vpath %.$(DllSuf) .:$(localslib)
vpath %.a         .:$(locallib)
#
ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLDFLAGS   = $(shell root-config --ldflags)
ROOTLIBS      = $(shell root-config --libs) -lEG
ROOTGLIBS     = $(shell root-config --glibs)
ROOTVERSION   = $(shell root-config --version)
ROOTLIBDIR    = $(shell root-config --libdir)
#
# 
# IF you use multiple versions of ROOT, you may
# want to change $(OS_NAME) to $(OS_NAME)_$(ROOTVERSTION)
#
locallib = ./lib/$(MACHINE)
localslib = ./lib/$(MACHINE)
localobjs = ./objs/$(MACHINE)
localdicts= ./dicts/$(MACHINE)
localbin= ./bin/$(MACHINE)
#
ifeq "$(DEBUG)" ""
  CXXFLAGS += -O2 
  LDFLAGS += -O2
else
  CXXFLAGS += -g
  LDFLAGS += -g
endif
#
ifeq ($(OS_NAME),SunOS)
CXX = CC
CXXFLAGS += -KPIC $(ROOTCFLAGS)
LD  = CC
LDFLAGS += $(ROOTLDFLAGS)
SOFLAGS = -G
BOSLIB += -lnsl

ObjSuf        = o
SrcSuf        = cc
ExeSuf        =
DllSuf        = so
OutPutOpt     = -o
endif

ifeq ($(findstring Linux,$(OS_NAME)),Linux)
CXX           = g++
CXXFLAGS      += -Wall -fPIC $(ROOTCFLAGS)
LD            = g++
LDFLAGS       +=  $(ROOTLDFLAGS) -lrt
SOFLAGS       = -Wl,-soname,$(notdir $@) -shared
ObjSuf        = o
SrcSuf        = cc
ExeSuf        =
DllSuf        = so
OutPutOpt     = -o
endif

ifeq ($(findstring Darwin,$(OS_NAME)),Darwin)
CXX           = g++
CXXFLAGS      += -Wall -fPIC $(ROOTCFLAGS)
LD            = g++
LDFLAGS       += $(ROOTLDFLAGS)
# SOFLAGS       = -O2 -dynamiclib -single_module -undefined dynamic_lookup -install_name $@ 
SOFLAGS       = -O2 -dynamiclib -single_module  -install_name $@ 
ObjSuf        = o
SrcSuf        = cc
ExeSuf        =
DllSuf        = dylib
OutPutOpt     = -o
POST_LINK_COMMAND= 
# ln -fs $(localslib)/$(CREATED_SHLIB) $(localslib)//$(subst .dylib,.so,$(CREATED_SHLIB))
endif
#--------------------------------------------------------------------
# If root is compiled with m64 flag the ROOTCFLAG returns
# this flag compile time. And when this flag is not present
# in the Linking stage MAC-OS returns "wrong architecture" warning
# This flag does not affect Linux Linking. 
#--------------------------------------------------------------------
ifeq ($(findstring m64,$(ROOTCFLAGS)),m64)
LDFLAGS += -m64
endif
#
# SRC_FILES contains non-class sources.
#
OBJ_FILES=$(addprefix $(localobjs)/,$(SRC_FILES:.cc=.o))
OBJ_FILES2=$(addprefix $(localobjs)/,$(SRC_FILES2:.cc=.o))
OBJ_FILES3=$(addprefix $(localobjs)/,$(SRC_FILES3:.cc=.o))
#
# SRC_CLASS contains class sources that need dictionaries.
#
OBJ_CLASS = $(addprefix $(localobjs)/,$(SRC_CLASS:.cc=.o))
DICTS_SOURCE = $(addprefix $(localdicts)/,$(SRC_CLASS:.cc=Dict.cc))
DICTS_INCLUDE = $(addprefix $(localdicts)/,$(SRC_CLASS:.cc=Dict.h))
OBJ_DICTS = $(addprefix $(localobjs)/,$(SRC_CLASS:.cc=Dict.o))
OBJ_SHLIB = $(OBJ_CLASS)  $(OBJ_DICTS)

#
# Bottom part of the makefiles.
#
#
help:
	@echo '#############################################'
	@echo '#   Makefile to Create '$(CREATED_SHLIB)'   #'
	@echo '#############################################'
	@echo 'Did you setup ROOT (at jlab: use root )?'
	@echo 'Are you using the correct compiler ?      '
	@echo 'See also the README.'   
	@echo '  type : make shlib - to make shared library'
	@echo '  type : make lib  - to make static library'
	@echo '  type : make docs - to make Html documentation.'
	@echo 'Resulting shared library will be put in '${localslib}
	@echo 'Resulting static library will be put in '${locallib} 
	@echo ''
	@echo 'OS_NAME     :'$(OS_NAME)
	@echo 'MACHINE     :'$(MACHINE)

	@echo 'localdicts  :'$(localdicts)
	@echo 'EXE_NAME     :'$(EXE_NAME)
	@echo 'EXE_NAME2    :'$(EXE_NAME2)
	@echo 'EXE_NAME3    :'$(EXE_NAME3)
	@echo 'OBJ_FILES    :'$(OBJ_FILES)
	@echo 'PROGRAMS     :'$(PROGRAMS)
	@echo 'CREATED_SHLIB:'$(CREATED_SHLIB)
	@echo 'CREATED_LIB  : '$(CREATED_LIB)
	@echo 'SRC_FILES    :'$(SRC_FILES)
	@echo 'SRC_FILES2   :'$(SRC_FILES2)
	@echo 'SRC_FILES3   :'$(SRC_FILES3)
	@echo 'SRC_CLASS    :'$(SRC_CLASS)
	@echo 'DICTS_SOURCE :'$(DICTS_SOURCE)
	@echo 'DICTS_INCLUD :'$(DICTS_INCLUDE)
	@echo 'INC_FILES    :'$(INC_FILES)
	@echo 'OBJ_CLASS    :'$(OBJ_CLASS)
	@echo 'OBJ_DICTS    :'$(OBJ_DICTS)
	@echo 'OBJ_SHLIB    :'$(OBJ_SHLIB)
	@echo 'CXX          :'$(CXX)
	@echo 'CXXFLAGS     :'$(CXXFLAGS)
	@echo 'CXXINCLUDES  :'$(CXXINCLUDES)
	@echo 'INCLUDES     :'$(INCLUDES)
	@echo 'LDFLAGS      :'$(LDFLAGS)
	@echo 'SOFLAGS      :'$(SOFLAGS)

shlib: Makefile_depends $(localslib)/$(CREATED_SHLIB) 

#
# Note: adding $(DICTS_*) here insures that make will not consider the dictionaries
# as intermediate files, and so will not erase them when done.
#
$(localslib)/$(CREATED_SHLIB): $(OBJ_SHLIB) $(DICTS_SOURCE) $(DICTS_INCLUDE)
	@test -d $(localslib) || mkdir -p $(localslib)
	$(LD) $(SOFLAGS) $(LDFLAGS)  $(OBJ_SHLIB) $(ROOTLIBS) -o $(localslib)/$(CREATED_SHLIB) $(OTHERLOADLIBS)
	$(POST_LINK_COMMAND)

lib: Makefile_depends $(localslib)/$(CREATED_LIB)

#$(locallib)/$(CREATED_LIB):  $(OBJ_SHLIB)
#	@test -d $(locallib) || mkdir -p $(locallib)
#	ar r $(locallib)/$(CREATED_LIB) $(OBJ_SHLIB) 

#
# Add the correct path to all the executables made here.
# 

ifdef CREATED_EXE
exe: $(localbin)/$(CREATED_EXE)
else
exe:
	@echo "No executable defined."
endif

ifdef CREATED_EXE2
exe2: $(localbin)/$(CREATED_EXE2)
endif

ifdef CREATED_EXE3
exe3: $(localbin)/$(CREATED_EXE3)
endif

#
$(localbin)/$(CREATED_EXE): $(OBJ_FILES) $(EXE_LIBS) 
	@test -d $(localbin) || mkdir -p $(localbin)
	$(LD)  $(LDFLAGS) -o $@ $(OBJ_FILES) $(EXE_LIBS) $(ROOTLIBS) $(OTHERLOADLIBS)

$(localbin)/$(CREATED_EXE2): $(OBJ_FILES2) $(EXE_LIBS) 
	@test -d $(localbin) || mkdir -p $(localbin)
	$(LD)  $(LDFLAGS) -o $@ $(OBJ_FILES2) $(EXE_LIBS) $(ROOTLIBS) $(OTHERLOADLIBS)

$(localbin)/$(CREATED_EXE3): $(OBJ_FILES3) $(EXE_LIBS) $(localslib)/$(CREATED_SHLIB)
	@test -d $(localbin) || mkdir -p $(localbin)
	$(LD)  $(LDFLAGS) -o $@ $(OBJ_FILES3) $(EXE_LIBS)  $(localslib)/$(CREATED_SHLIB)  $(ROOTLIBS) $(OTHERLOADLIBS)


#
# We try to limit the amount of dependencies that makedepend uses by limiting
# the include search path ( -Y means don't search standard paths.) 
# In addition, the "link" directory for includes will not do us any good, since these files are 

DEP_INCLUDES=$(filter-out -I$(MYSQL_INCLUDE) -I../include,$(INCLUDES))
DEP_CXXFLAGS=$(filter-out -I$(ROOTSYS)/include, $(CXXFLAGS))

dep: Makefile_depends

Makefile_depends: Makefile $(SRC_CLASS) $(SRC_FILES) $(SRC_FILES2) $(INC_FILES)
	@test -r Makefile_depends || echo "### Automatically generated dependencies. Update with 'make dep'" > Makefile_depends 
	@echo "(Re) Making the dependency file "
	@makedepend -f Makefile_depends -p$(localobjs)/ -Y -- $(DEP_CXXFLAGS) $(DEP_INCLUDES) $(CXXINCLUDES) -- $(SRC_CLASS)  $(SRC_FILES) $(SRC_FILES2) >& /dev/null
	@rm -f Makefile_depends.bak

docs: $(SRC_CLASS) $(INC_FILES)
	@root.exe Make_HTML.C

clean:
	@rm -rf $(OBJ_FILES) $(OBJ_FILES2) $(OBJ_FILES3) $(OBJ_SHLIB) $(DICTS_SOURCE) $(DICTS_INCLUDE)

distclean: clean
	@rm -f Makefile_depends 
	@test -z $(CREATED_LIB)   || rm -f $(locallib)/$(CREATED_LIB) 
	@test -z $(CREATED_SHLIB) || rm -f $(localslib)/$(CREATED_SHLIB)
	@test -z $(CREATED_EXE)   || rm -f $(localbin)/$(CREATED_EXE)
	@test -z $(CREATED_EXE2)  || rm -f $(localbin)/$(CREATED_EXE2)
	@test -z $(CREATED_EXE3)  || rm -f $(localbin)/$(CREATED_EXE3)
#
#
#_______________________________________________________

$(localdicts)/%Dict.cc $(localdicts)/%Dict.h: %.h
	@test -d $(dir $@) || mkdir -p $(dir $@)
	$(ROOTSYS)/bin/rootcint -f $@ -c -p $(INCLUDES) $< $(<:.h=_Linkdef.h)

$(localobjs)/%Dict.o: $(localdicts)/%Dict.cc
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo Compiling File $<
	$(CXX) $(CXXFLAGS) -c $(ROOTCFLAGS) $(INCLUDES) $< -o $@

$(localobjs)/%.o:%.cc %.h
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo Compiling Class $<
	$(CXX) $(CXXFLAGS) -c $(ROOTCFLAGS) $(INCLUDES) $< -o $@

$(localobjs)/%.o:%.cc
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo Compiling File $<
	$(CXX) $(CXXFLAGS) -c $(ROOTCFLAGS) $(INCLUDES) $< -o $@

Makefile_depends: Makefile

include Makefile_depends
