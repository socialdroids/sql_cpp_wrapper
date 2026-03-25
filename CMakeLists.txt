cmake_minimum_required(VERSION 3.7) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Dependencies
#sudo apt install libcppdb-dev libcppdb-postgresql0

project(sql_cpp_wrapper) 
include_directories(include)

find_library(CPPDB_LIB cppdb) 
if (NOT CPPDB_LIB)
    message(FATAL_ERROR "cppdb library not found") 
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${CPPDB_LIB})
