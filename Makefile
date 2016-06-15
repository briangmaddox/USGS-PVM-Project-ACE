# $Id: Makefile,v 1.1.1.1 2001-03-16 16:31:19 geouser Exp $
# Makefile for the UGM program
# Last modified by $Author: geouser $ on $Date: 2001-03-16 16:31:19 $

prefix       = /home/geouser
host_os      = linux
srcdir       = .
top_srcdir   = .
enable_debug = no

# Set up the include paths
INCPATHS = -I$(prefix)/include -I$(prefix)/include/tiff -I$(prefix)/include/geotiff -I/usr/share/pvm3/include -I/data1/ACE_wrappers
LIBDIRS  = -L$(prefix)/lib -L/data1/ACE_wrappers/ace -L/usr/share/pvm3/lib/LINUX

# Libraries we need to link in
LIBS =  -lProjectionMesh -lMathLib -lProjectionIO -lImageLib  -lgeotiff -ltiff -lProjection  -lgeolib -lFrodo -lACE -lpthread -lpvm3 

#SlaveLibs
SLIBS = -lProjectionMesh -lMathLib -lProjectionIO -lImageLib  -lgeotiff -ltiff -lProjection  -lgeolib -lFrodo -lpvm3

# Linker flags
LDFLAGS   = $(LIBDIRS)
LOADLIBES = $(LIBS)

# Set our compiler options
ifeq ($(enable_debug),yes)
DEBUG = -g -Wall -DTNT_NO_BOUNDS_CHECK
else
DEBUG = -O3 -DTNT_NO_BOUNDS_CHECK
#-march=pentiumpro -mcpu=pentiumpro -fomit-frame-pointer -mieee-fp -fschedule-insns2 -finline-functions -frerun-loop-opt -fstrength-reduce -ffast-math -funroll-loops -fexpensive-optimizations -fthread-jumps
endif

# Compiler and other defs
CC   = gcc
CXX  = g++ 
CXXFLAGS = $(DEBUG) $(INCPATHS)

# Suffix rules
.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

# Dependencies for the master program
OBJS = Projector.o ProjectionParams.o mastermain.o ProjectorException.o \
       ProjectorStitcher.o PvmProjector.o

SOBJ = Projector.o ProjectionParams.o slavemain.o ProjectorException.o \
       PvmProjectorSlave.o

all: master slave

master : $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o master $(LIBDIRS) $(LIBS)
slave : $(SOBJ)
	$(CXX) $(CXXFLAGS) $(SOBJ) -o slave $(LIBDIRS) $(SLIBS)


clean:
	rm -f $(OBJS) $(SOBJ) *~ master slave











