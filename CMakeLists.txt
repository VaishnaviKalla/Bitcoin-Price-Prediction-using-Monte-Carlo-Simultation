cmake_minimum_required(VERSION 3.18)

# Detect CUDA option early so we can conditionally enable the language
option(USE_CUDA   "Enable CUDA GPU acceleration"  ON)
if(USE_CUDA)
    project(btc_monte_carlo LANGUAGES CXX CUDA)
else()
    project(btc_monte_carlo LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)

# ─────────────────────────────────────────────
# Build options
# ─────────────────────────────────────────────
option(USE_MPI    "Enable MPI distributed support" ON)
option(USE_OPENMP "Enable OpenMP multi-threading"  ON)

# ─────────────────────────────────────────────
# Packages
# ─────────────────────────────────────────────
if(USE_MPI)
    find_package(MPI REQUIRED)
    message(STATUS "MPI found: ${MPI_CXX_LIBRARIES}")
endif()

if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
    message(STATUS "OpenMP found: version ${OpenMP_CXX_VERSION}")
endif()

if(USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "CUDA Toolkit: ${CUDAToolkit_VERSION}")
    set(CMAKE_CUDA_ARCHITECTURES 70 75 80 86 89 90)
endif()

# ─────────────────────────────────────────────
# Includes
# ─────────────────────────────────────────────
include_directories(include)

# ─────────────────────────────────────────────
# Sources
# ─────────────────────────────────────────────
set(COMMON_SOURCES
    src/data_loader.cpp
    src/gbm_params.cpp
    src/statistics.cpp
    src/results_writer.cpp
    src/logger.cpp
)

set(CPU_SOURCES
    src/simulator_cpu.cpp
)

if(USE_CUDA)
    set(GPU_SOURCES
        src/simulator_gpu.cu
    )
endif()

# ─────────────────────────────────────────────
# Main executable
# ─────────────────────────────────────────────
if(USE_CUDA)
    add_executable(btc_sim
        src/main.cpp
        ${COMMON_SOURCES}
        ${CPU_SOURCES}
        ${GPU_SOURCES}
    )
    set_target_properties(btc_sim PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
    )
    target_compile_definitions(btc_sim PRIVATE USE_CUDA)
    target_link_libraries(btc_sim PRIVATE CUDA::curand CUDA::cudart)
else()
    add_executable(btc_sim
        src/main.cpp
        ${COMMON_SOURCES}
        ${CPU_SOURCES}
    )
endif()

if(USE_MPI)
    target_compile_definitions(btc_sim PRIVATE USE_MPI)
    target_link_libraries(btc_sim PRIVATE MPI::MPI_CXX)
    target_include_directories(btc_sim PRIVATE ${MPI_CXX_INCLUDE_DIRS})
endif()

if(USE_OPENMP)
    target_compile_definitions(btc_sim PRIVATE USE_OPENMP)
    target_link_libraries(btc_sim PRIVATE OpenMP::OpenMP_CXX)
endif()

target_compile_options(btc_sim PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-O3 -march=native -Wall -Wextra>
)

# ─────────────────────────────────────────────
# Install
# ─────────────────────────────────────────────
install(TARGETS btc_sim DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/btc_monte_carlo/data)

message(STATUS "")
message(STATUS "=== Build configuration ===")
message(STATUS "  CUDA   : ${USE_CUDA}")
message(STATUS "  MPI    : ${USE_MPI}")
message(STATUS "  OpenMP : ${USE_OPENMP}")
message(STATUS "===========================")
