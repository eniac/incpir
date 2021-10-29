project(DPFPIR)
#cmake_minimum_required(VERSION 3.4.1)

ADD_SUBDIRECTORY(proto)

enable_language(C CXX)
set(CMAKE_CXX_STANDARD 14)


set(SRCS
    AES.cpp
    Defines.cpp
    Log.cpp
    PRNG.cpp
    dpf.cpp
    hashdatastore.cpp)

set(CMAKE_C_FLAGS "-ffunction-sections -Wall  -maes -msse2 -msse4.1 -mavx2 -mpclmul -Wfatal-errors -pthread -Wno-strict-overflow  -fPIC -Wno-ignored-attributes")
set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS}  -std=c++14 -O2")

# Select flags.
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O2 -g -ggdb -rdynamic")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g -ggdb -rdynamic")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/proto)

add_executable(dpf_pir ${SRCS} main.cpp)
target_link_libraries(dpf_pir proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(bench ${SRCS} bench.cpp)
target_link_libraries(bench proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(dpf_tests ${SRCS} test.cpp)
target_link_libraries(dpf_tests proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(server ${SRCS} net_server.cpp)
target_link_libraries(server proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(client ${SRCS} net_client.cpp)
target_link_libraries(client proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)