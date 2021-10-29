CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
project(incrementalPIR)

ADD_SUBDIRECTORY(proto)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/lib/cmake/")
set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/proto)

set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS}  -std=c++14 -O2")

add_executable(incrementalPIR main.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(incrementalPIR proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(online_server_eval src/online_server_eval.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(online_server_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(online_client_eval src/online_client_eval.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(online_client_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(offline_server_eval src/offline_server_eval.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(offline_server_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(offline_client_eval src/offline_client_eval.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(offline_client_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(refresh_server_eval src/refresh_server_eval.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(refresh_server_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(refresh_client_eval src/refresh_client_eval.cpp src/adprp.hpp src/adprp.cpp
        src/pir.hpp src/pir.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(refresh_client_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)
