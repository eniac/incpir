cmake_minimum_required(VERSION 3.4)
project(CK)

ADD_SUBDIRECTORY(proto)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS}  -std=c++14 -O2")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/proto)

add_executable(CK main.cpp src/pir.hpp src/pir.cpp src/puncprf.hpp src/puncprf.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(CK proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(server src/eval_server.cpp src/pir.hpp src/pir.cpp src/puncprf.hpp src/puncprf.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(server proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(online_client_eval src/online_client_eval.cpp src/pir.hpp src/pir.cpp src/puncprf.hpp src/puncprf.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(online_client_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

add_executable(offline_client_eval src/offline_client_eval.cpp src/pir.hpp src/pir.cpp src/puncprf.hpp src/puncprf.cpp
        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

target_link_libraries(offline_client_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

#add_executable(refresh_server_eval src/refresh_server_eval.cpp src/pir.hpp src/pir.cpp src/puncprf.hpp src/puncprf.cpp
#       src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

#target_link_libraries(refresh_server_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

#add_executable(refresh_client_eval src/refresh_client_eval.cpp src/pir.hpp src/pir.cpp src/puncprf.hpp src/puncprf.cpp
#        src/client.hpp src/client.cpp src/server.hpp src/server.cpp)

#target_link_libraries(refresh_client_eval proto ${PROTOBUF_LIBRARY} m ssl crypto pthread)

