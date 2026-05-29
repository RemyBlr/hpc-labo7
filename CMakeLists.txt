cmake_minimum_required(VERSION 3.10)
project(FindKmers C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native")

# version mono-threadee
add_executable(k-mer src/main_monothread.c)

# --- Configuration OpenMP pour AppleClang + libomp (Homebrew) ---
# Recupere le prefixe Homebrew (différent sur Apple Silicon vs Intel)
execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# version multithreadee
add_executable(k-mer-mt src/main_multithread.c)
target_compile_options(k-mer-mt PRIVATE -Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include)
target_link_directories(k-mer-mt PRIVATE ${LIBOMP_PREFIX}/lib)
target_link_libraries(k-mer-mt PRIVATE omp)