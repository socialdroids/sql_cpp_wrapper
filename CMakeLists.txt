# cmake_minimum_required(VERSION 3.7) set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#
# #Dependencies #sudo apt install libcppdb-dev libcppdb-postgresql0
#
# project(sql_cpp_wrapper) include_directories(include)
#
# find_library(CPPDB_LIB cppdb) if (NOT CPPDB_LIB) message(FATAL_ERROR "cppdb
# library not found") endif()
#
# add_executable(${PROJECT_NAME} main.cpp) target_link_libraries(${PROJECT_NAME}
# ${CPPDB_LIB})



# cmake_minimum_required(VERSION 3.5)
# project(sql_cpp_wrapper)
#
# include(FetchContent)
#
# find_package(PostgreSQL)
#
# if(NOT PostgreSQL_FOUND)
#   # Forçamos a exibição da mensagem ignorando o modo QUIET do CMake
#   message(STATUS "------------------------------------------------------------")
#   message(
#     WARNING
#       "\n[AVISO] Driver PostgreSQL não detectado no sistema!"
#       "\nSua biblioteca 'sql_cpp_wrapper' vai compilar, mas a conexão com bancos "
#       "\nPostgres falhará em tempo de execução (runtime) se o driver não estiver instalado."
#       "\nNo Ubuntu/Debian, resolva com:"
#       "\n    sudo apt install libcppdb-postgresql0 libpq-dev\n")
#   message(STATUS "------------------------------------------------------------")
# else()
#   message(
#     STATUS "[OK] Driver PostgreSQL encontrado em: ${PostgreSQL_LIBRARIES}")
# endif()
#
# find_library(CPPDB_LIB cppdb)
#
# if(NOT CPPDB_LIB)
#   message(
#     STATUS "cppdb não encontrada. Baixando e compilando automaticamente...")
#
#   set(SQLITE_BACKEND
#       OFF
#       CACHE BOOL "" FORCE) # desativar o que não quer
#   set(POSTGRESQL_BACKEND
#       ON
#       CACHE BOOL "" FORCE)
#
#   FetchContent_Declare(
#     cppdb_ext
#     GIT_REPOSITORY https://github.com/artyom-beilis/cppdb.git
#     GIT_TAG master # Ou uma tag específica
#   )
#
#   # Nota: CppDB usa CMake, o que facilita muito!
#   FetchContent_MakeAvailable(cppdb_ext)
#   set(CPPDB_LIB cppdb) # O target gerado pelo projeto deles
# else()
#   message(STATUS "cppdb encontrada no sistema: ${CPPDB_LIB}")
# endif()
#
# function(sql_wrapper_setup_drivers target_name)
#   add_custom_command(
#     TARGET ${target_name}
#     POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_FILE:cppdb_postgresql> $<TARGET_FILE_DIR:${target_name}>
#     COMMENT "Instalando drivers SQL para ${target_name}...")
# endfunction()
#
# # 1. Criar a biblioteca INTERFACE (header-only)
# add_library(sql_wrapper INTERFACE)
#
# # 1. Definir o Alias (O apelido que será usado no target_link_libraries)
# add_library(sql_cpp_wrapper::sql_cpp_wrapper ALIAS sql_wrapper)
#
# # 1. Definir onde estão os headers para quem for usar a lib O uso de
# #   BUILD_INTERFACE garante que funcione tanto localmente quanto via
# #   FetchContent
# target_include_directories(
#   sql_wrapper
#   INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#             $<BUILD_INTERFACE:${cppdb_ext_SOURCE_DIR}>)
#
# target_link_libraries(sql_wrapper INTERFACE cppdb cppdb_postgresql)
#
# if(NOT WIN32)
#     # Define que qualquer alvo que linkar com você deve olhar na própria pasta por .so
#     target_link_options(sql_wrapper INTERFACE "-Wl,-rpath,$ORIGIN")
# endif()
#
# # main.cpp apenas para testes locais ---
# if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
#   add_executable(test_app main.cpp)
#   target_link_libraries(test_app PRIVATE sql_cpp_wrapper::sql_cpp_wrapper)
#   sql_wrapper_setup_drivers(test_app)
# endif()

cmake_minimum_required(VERSION 3.14) # Recomendado 3.14 para FetchContent estável
project(sql_cpp_wrapper)

include(FetchContent)

# --- 1. Verificação do Driver Postgres ---
find_package(PostgreSQL QUIET)
if(NOT PostgreSQL_FOUND)
    message(WARNING "Driver PostgreSQL não detectado. No Ubuntu: sudo apt install libcppdb-postgresql0 libpq-dev")
endif()

# --- 2. Gerenciamento da CPPDB ---
find_library(CPPDB_LIB cppdb)

if(NOT CPPDB_LIB)
    message(STATUS "cppdb não encontrada. Baixando via FetchContent...")
    
    set(SQLITE_BACKEND OFF CACHE BOOL "" FORCE)
    set(POSTGRESQL_BACKEND ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        cppdb_ext
        GIT_REPOSITORY https://github.com/artyom-beilis/cppdb.git
        GIT_TAG master 
    )
    FetchContent_MakeAvailable(cppdb_ext)
    
    # Criamos variáveis para facilitar a exportação
    set(CPPDB_TARGETS cppdb cppdb_postgresql)
    set(CPPDB_INCLUDE_DIR ${cppdb_ext_SOURCE_DIR})
else()
    message(STATUS "cppdb encontrada no sistema: ${CPPDB_LIB}")
    set(CPPDB_TARGETS ${CPPDB_LIB})
    set(CPPDB_INCLUDE_DIR "") # Sistema já sabe onde estão os headers
endif()

# --- 3. Função de Automação para o Usuário ---
function(sql_wrapper_setup_drivers target_name)
    # Só tentamos copiar se estivermos usando a versão do FetchContent (onde o target existe)
    if(TARGET cppdb_postgresql)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:cppdb_postgresql> $<TARGET_FILE_DIR:${target_name}>
            COMMENT "Copiando driver SQL para a pasta do executável..."
        )
    endif()
endfunction()

# --- 4. Definição da sua Biblioteca ---
add_library(sql_wrapper INTERFACE)
add_library(sql_cpp_wrapper::sql_cpp_wrapper ALIAS sql_wrapper)

target_include_directories(sql_wrapper INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CPPDB_INCLUDE_DIR}>
)

target_link_libraries(sql_wrapper INTERFACE ${CPPDB_TARGETS})

if(NOT WIN32)
    target_link_options(sql_wrapper INTERFACE "-Wl,-rpath,$ORIGIN")
endif()

# --- 5. Teste Local ---
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(test_app main.cpp)
    target_link_libraries(test_app PRIVATE sql_cpp_wrapper::sql_cpp_wrapper)
    sql_wrapper_setup_drivers(test_app)
endif()
