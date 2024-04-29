# Makefile
# $Id$
#MIDASSYS=/home/olchansk/daq/midas/midas

# The MIDASSYS should be defined prior the use of this Makefile
ifndef MIDASSYS
missmidas::
	@echo "...";
	@echo "Missing definition of environment variable 'MIDASSYS' !";
	@echo "...";
endif

#--------------------------------------------------------------------
# The following contains the necessary flags
# get OS type from shell
OSTYPE = $(shell uname)

## for Linux
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif
ifeq ($(OSTYPE),linux)
OS_DIR = linux
OSFLAGS  =
endif

## For Mac OSX
ifeq ($(OSTYPE),Darwin)
OSTYPE = darwin
endif
ifeq ($(OSTYPE),darwin)
OS_DIR = darwin
OSFLAGS = -m32 -DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY -fPIC 
endif

## Shared
CFLAGS   =  -g -O2 -Wall -Wuninitialized -Wno-narrowing
CXXFLAGS = $(CFLAGS)# -DHAVE_ROOT -DUSE_ROOT -I$(ROOTSYS)/include
LIBS = -lm -lrt -lutil -lnsl -lpthread -lz

#-----------------------------------------
# ROOT flags and libs
#
ROOTLIBS := 
ifdef ROOTSYS
ROOTCFLAGS := $(shell  $(ROOTSYS)/bin/root-config --cflags)
ROOTCFLAGS += -DHAVE_ROOT -DUSE_ROOT
ROOTLIBS   := $(shell  $(ROOTSYS)/bin/root-config --libs) -Wl,-rpath,$(ROOTSYS)/lib
ROOTLIBS   += -lThread
endif

DRV_DIR         = $(MIDASSYS)/drivers
INC_DIR         = $(MIDASSYS)/include
LIB_DIR         = $(MIDASSYS)/lib
##LIB_DIR_SBC     = $(MIDASSYS)/linux/lib
VME_DIR         = /usr/include/vme

UFE = fev1730
UFEDPP = fev1730-DPP

# MIDAS library
MIDASLIBS = $(LIB_DIR)/libmidas.a
#MIDASLIBS_SBC = $(LIB_DIR_SBC)/libmidas.a
# compiler
CC = g++
CXX = g++
CFLAGS += -g -I. -I$(INC_DIR) -I$(DRV_DIR) -I$(DRV_DIR)/vmic -I$(VME_DIR)
LDFLAGS += 

#-------------------------------------------------------------------
# List of analyzer modules
#
MODULES   = adccalib.o scaler.o

ifdef ROOTSYS
all: $(UFE) $(UFEDPP)
else
all: $(UFE) $(UFEDPP)
endif

gefvme.o: $(DRV_DIR)/vme/vmic/gefvme.c
	gcc -c -o $@ -O2 -g -Wall -Wuninitialized $< -I$(MIDASSYS)/include

%.o: $(MIDASSYS)/drivers/vme/%.c
	gcc -c -o $@ -O2 -g -Wall -Wuninitialized $< -I$(MIDASSYS)/include

$(UFE): $(MIDASLIBS) $(LIB_DIR)/mfe.o fev1730.o gefvme.o v792.o v1730.o sis3820.o
	$(CXX) -o $@ $(CFLAGS) $(OSFLAGS) $^ $(MIDASLIBS) $(LIBS) $(ROOTLIBS)

$(UFEDPP): $(MIDASLIBS) $(LIB_DIR)/mfe.o gefvme.o v792.o v1730DPP.o sis3820.o fev1730-DPP.o 
	$(CXX) -o $@ $(CFLAGS) $(OSFLAGS) $^ $(MIDASLIBS) $(LIBS) $(ROOTLIBS)

#ifdef ROOTSYS
#analyzer: $(LIB_DIR)/rmana.o analyzer.o $(MODULES) $(MIDASLIBS)
#	g++ -o $@ -O2 -g $^ $(ROOTLIBS) $(LIBS)
#endif

v1730.o: v1730.c
	$(CXX) -c -o $@ $(CFLAGS) $(OSFLAGS) $(ROOTCFLAGS) $< -I$(MIDASSYS)/include

v1730DPP.o: v1730DPP.c
	$(CXX) -c -o $@ $(CFLAGS) $(OSFLAGS) $(ROOTCFLAGS) $< -I$(MIDASSYS)/include

%.o: %.c
	$(CXX) $(CFLAGS) $(OSFLAGS) $(ROOTCFLAGS) -c $<

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $(OSFLAGS) $(ROOTCFLAGS) -c $<

clean::
	-rm -f *.o *.exe $(UFE) $(UFEDPP)

# end
