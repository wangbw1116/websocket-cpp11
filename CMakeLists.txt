cmake_minimum_required(VERSION 3.16)

project(ws_sample)

add_executable(ws_server ws_server.cpp)
add_executable(ws_client ws_client.cpp)

include_directories(asio/asio/include)
include_directories(websocketpp)
include_directories(json/single_include)

target_link_libraries(ws_server pthread)
target_link_libraries(ws_client pthread)