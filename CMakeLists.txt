cmake_minimum_required(VERSION 3.13)
project(cpwconv C)

find_package(LibXml2 REQUIRED)
message(STATUS LIBXML2_FOUND = ${LIBXML2_FOUND})


set(CMAKE_C_STANDARD 11)

add_executable(cpwconv main.c)
target_link_libraries(cpwconv ${LIBXML2_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LIBXML2_INCLUDE_DIR})
