cmake_minimum_required(VERSION 3.5)
project(sql_cpp_wrapper)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_library(CPPDB_LIB NAMES cppdb)

find_library(CPPDB_PG_DRIVER NAMES cppdb_postgresql PATHS /usr/lib/cppdb /usr/local/lib/cppdb)

if(NOT CPPDB_PG_DRIVER)
    message(FATAL_ERROR 
        "\n[ERRO CRÍTICO] Driver PostgreSQL do CppDB não encontrado!"
        "\nPara resolver, instale os pacotes necessários:"
        "\n    sudo apt update"
        "\n    sudo apt install libcppdb-dev libcppdb-postgresql0"
        "\n"
    )
endif()

add_library(sql_wrapper INTERFACE)
add_library(sql_cpp_wrapper::sql_cpp_wrapper ALIAS sql_wrapper)

target_include_directories(sql_wrapper INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CPPDB_INCLUDE_DIR}>
)

target_link_libraries(sql_wrapper INTERFACE ${CPPDB_LIB})

# --- Teste Local ---
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(test_app main.cpp)
    target_link_libraries(test_app PRIVATE sql_cpp_wrapper::sql_cpp_wrapper)
endif()
