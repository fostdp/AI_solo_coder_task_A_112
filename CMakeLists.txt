cmake_minimum_required(VERSION 3.15)
project(SlipMonitorSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++17")

add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED COMPONENTS 
    system 
    thread 
    chrono
)

find_package(OpenSSL REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

add_subdirectory(backend)
add_subdirectory(opc_simulator)

message(STATUS "==================================================")
message(STATUS "Slip Monitor System Configuration Complete")
message(STATUS "==================================================")
