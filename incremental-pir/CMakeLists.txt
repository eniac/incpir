cmake_minimum_required(VERSION 3.4)
project (localbench)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/lib/cmake/")
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS}  -std=c++14 -O2")

add_executable(localbench main.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

include_directories("/usr/local/include")

target_link_libraries(localbench m ssl crypto pthread) 


