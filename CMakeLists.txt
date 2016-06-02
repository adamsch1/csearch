project(tool)
set(CMAKE_C_FLAGS "-fpic -mavx -std=c11 -pedantic -O3 -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-g")
set(CMAKE_CXX_FLAGS "-g -std=c++11")

set(source  tool.cc ifile.cc merge.cc dsort.cc streamvbytedelta.c streamvbyte.c )

add_executable(tool ${source})
