# docker compose -f docker/compose.yml run gcc12-final /bin/bash

# mkdir -p build/debug && cd build/debug && cmake -DCMAKE_BUILD_TYPE=Debug ../.. && cmake --build . -j12 --
# mkdir -p build/release && cd build/release && cmake -DCMAKE_BUILD_TYPE=Release ../.. && cmake --build . -j12 --

# cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON ../..
# cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON ../..

# cmake --build . -j12 --
# cmake --build . -j12 --target clean
# cmake --build . -j12 --target my_executable
# cmake --build . -j12 --clean-first --

# cmake --find-package -DNAME=Boost -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST
# cmake -L

# find src/ -type f -executable
# tree ../.. -I build -P *.md -A --dirsfirst
# tree ../.. -L 2 -A --dirsfirst
# du -h --max-depth=1

# --------------------------------------------------------------------------------------
# project settings
# --------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)

# Set the project name
project(cxx_lab LANGUAGES CXX)

# Set C++ standard to 20 or later
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the install path for the library
set(CMAKE_INSTALL_RPATH "/usr/local/lib")
set(CMAKE_BUILD_RPATH "/usr/local/lib")

# Add the modules directory to the include path
include_directories(${CMAKE_SOURCE_DIR}/src/modules)
include_directories(/usr/local/include/mongocxx/v_noabi)
include_directories(/usr/local/include/bsoncxx/v_noabi)

# Set CMake policy CMP0167 to NEW
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# --------------------------------------------------------------------------------------
# find packages
# --------------------------------------------------------------------------------------

# Add Boost library
find_package(Boost REQUIRED COMPONENTS system thread)
message(STATUS "Boost Version: ${Boost_VERSION}")
message(STATUS "Boost Libraries: ${Boost_LIBRARIES}")

find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL Version: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")

# Find Protobuf
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Protobuf Version: ${Protobuf_VERSION}")

# Find gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "gRPC Version: ${gRPC_VERSION}")

# Find absl
find_package(absl CONFIG REQUIRED)
message(STATUS "ABSL Version: ${absl_VERSION}")

# --------------------------------------------------------------------------------------
# add sub-modules
# --------------------------------------------------------------------------------------

# Add the source directory
add_subdirectory(src)
