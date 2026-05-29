cmake_minimum_required(VERSION 3.10)
project(FindKmers C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native")

find_package(OpenMP REQUIRED)

# version mono-threadée
add_executable(k-mer src/main_monothread.c)

# version multithreadée
add_executable(k-mer-mt src/main_multithread.c)
target_link_libraries(k-mer-mt PUBLIC OpenMP::OpenMP_C)