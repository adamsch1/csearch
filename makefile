#
.SUFFIXES: .cpp .o .c .h .cc

DEBUG=1
# replace the CXX variable with a path to a C++11 compatible compiler.
# to build an aligned version, add -DUSE_ALIGNED=1
CXX ?= g++-4.7
ifeq ($(DEBUG),1)
    CXXFLAGS = -fpic -mavx -std=c++11  -Weffc++ -pedantic -ggdb -DDEBUG=1 -D_GLIBCXX_DEBUG -Wall -Wextra
    CCFLAGS = -fpic -mavx  -std=c11 -pedantic -ggdb -DDEBUG=1 -D_GLIBCXX_DEBUG -Wall -Wextra
else
    CXXFLAGS = -fpic -mavx -std=c++11  -Weffc++ -pedantic -O3 -Wall -Wextra
    CCFLAGS = -fpic -mavx -std=c11 -pedantic -O3 -Wall -Wextra
endif #debug


streamvbytedelta.o: streamvbytedelta.c $(HEADERS)
	  $(CC) $(CFLAGS) -c streamvbytedelta.c -Iinclude 

streamvbyte.o: streamvbyte.c $(HEADERS)
	  $(CC) $(CFLAGS) -c streamvbyte.c -Iinclude  


all: tool
	echo "please run unit tests by running the unit executable"

tool:  $(HEADERS) tool.c  $(OBJECTS)
	$(CC) $(CCFLAGS)  -o tool tool.c streamvbytedelta.c streamvbyte.c buf_ring.c $(OBJECTS) -lgumbo -lcurl


clean:
	rm -f *.o unit benchsearch testintegration testcodecs   simplesynth  compress uncompress budgetedtest ramtocache  entropy example benchintersection libSIMDCompressionAndIntersection.so libSIMDCompressionAndIntersection.a compflatstat

