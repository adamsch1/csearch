.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h .cc

DEBUG=1
# replace the CXX variable with a path to a C++11 compatible compiler.
# to build an aligned version, add -DUSE_ALIGNED=1
ifeq ($(INTEL), 1)
# if you wish to use the Intel compiler, please do "make INTEL=1".
    CXX ?= /opt/intel/bin/icpc
    CC ?= /opt/intel/bin/icpc
ifeq ($(DEBUG),1)
    CXXFLAGS = -fpic -std=c++11 -O3 -Wall -ansi -xAVX -DDEBUG=1 -D_GLIBCXX_DEBUG   -ggdb
    CCFLAGS = -fpic -std=c99 -O3 -Wall -ansi -xAVX -DDEBUG=1 -D_GLIBCXX_DEBUG   -ggdb
else
    CXXFLAGS = -fpic -std=c++11 -O2 -Wall -ansi -xAVX -DNDEBUG=1  -ggdb
    CCFLAGS = -fpic -std=c99 -O2 -Wall -ansi -xAVX -DNDEBUG=1  -ggdb
endif # debug
else #intel
    CXX ?= g++-4.7
ifeq ($(DEBUG),1)
    CXXFLAGS = -fpic -mavx -std=c++11  -Weffc++ -pedantic -ggdb -DDEBUG=1 -D_GLIBCXX_DEBUG -Wall -Wextra
    CCFLAGS = -fpic -mavx -std=c99  -pedantic -ggdb -DDEBUG=1 -D_GLIBCXX_DEBUG -Wall -Wextra
else
    CXXFLAGS = -fpic -mavx -std=c++11  -Weffc++ -pedantic -O3 -Wall -Wextra
    CCFLAGS = -fpic -mavx -std=c99 -pedantic -O3 -Wall -Wextra
endif #debug
endif #intel





HEADERS= $(shell ls SIMDCompressionAndIntersection/include/*h)

all: tool
	echo "please run unit tests by running the unit executable"


tool:  $(HEADERS) tool.c  $(OBJECTS)
	$(CC) $(CCFLAGS)  -o tool tool.c $(OBJECTS) -L simdcomp/ -l simdcomp

toolc:  $(HEADERS) tool.c  $(OBJECTS)
	$(CXX) $(CXXFLAGS)  -o tool tool.c  $(OBJECTS) -ISIMDCompressionAndIntersection/include/ SIMDCompressionAndIntersection/libSIMDCompressionAndIntersection.a



testintegration:  bitpacking.o simdbitpacking.o usimdbitpacking.o integratedbitpacking.o     simdintegratedbitpacking.o src/testintegration.cpp  $(HEADERS)
	$(CXX) $(CXXFLAGS) -Iinclude -o testintegration src/testintegration.cpp   bitpacking.o integratedbitpacking.o  simdbitpacking.o usimdbitpacking.o     simdintegratedbitpacking.o

libSIMDCompressionAndIntersection.a: $(OBJECTS)
	ar rvs $@ $^

libSIMDCompressionAndIntersection.so: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^


clean:
	rm -f *.o unit benchsearch testintegration testcodecs   simplesynth  compress uncompress budgetedtest ramtocache  entropy example benchintersection libSIMDCompressionAndIntersection.so libSIMDCompressionAndIntersection.a compflatstat




