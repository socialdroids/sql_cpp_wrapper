cmake_minimum_required(VERSION 3.8)
project(sql_cpp_wrapper)

include(FetchContent)

# --- 1. Verificação do Driver Postgres ---
find_package(PostgreSQL QUIET)
if(NOT PostgreSQL_FOUND)
  message(
    WARNING
      "Driver PostgreSQL não detectado. No Ubuntu: sudo apt install libcppdb-postgresql0 libpq-dev"
  )
endif()

# --- 2. Gerenciamento da CPPDB ---
find_library(CPPDB_LIB cppdb)

if(NOT CPPDB_LIB)
  message(STATUS "cppdb não encontrada. Baixando via FetchContent...")

  set(SQLITE_BACKEND
      OFF
      CACHE BOOL "" FORCE)
  set(POSTGRESQL_BACKEND
      ON
      CACHE BOOL "" FORCE)

  FetchContent_Declare(
    cppdb_ext
    GIT_REPOSITORY https://github.com/artyom-beilis/cppdb.git
    GIT_TAG master)
  FetchContent_MakeAvailable(cppdb_ext)

  set(CPPDB_TARGETS cppdb cppdb_postgresql)
  set(CPPDB_INCLUDE_DIR ${cppdb_ext_SOURCE_DIR})

  # --- TRUQUE PARA O GERADOR DE EXPRESSÃO ---
  # Criamos uma propriedade no nível de projeto para indicar que o driver deve
  # ser copiado
  set_property(GLOBAL PROPERTY CPPDB_DRIVER_TARGET "cppdb_postgresql")
else()
  set(CPPDB_TARGETS ${CPPDB_LIB})
  set(CPPDB_INCLUDE_DIR "")
  set_property(GLOBAL PROPERTY CPPDB_DRIVER_TARGET "") # Vazio se for sistema
endif()

# --- 3. Função de Automação para o Usuário ---
function(sql_wrapper_setup_drivers target_name)
  get_property(DRIVER_TARGET GLOBAL PROPERTY CPPDB_DRIVER_TARGET)

  if(DRIVER_TARGET)
    # Usamos uma verificação mais robusta: se DRIVER_TARGET estiver preenchido,
    # significa que nós mesmos o compilamos.
    add_custom_command(
      TARGET ${target_name}
      POST_BUILD
      # O truque aqui é usar o comando condicional do próprio Shell/CMake para
      # evitar que o Generator Expression quebre se o alvo sumir do escopo
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:${DRIVER_TARGET}> $<TARGET_FILE_DIR:${target_name}>
      COMMENT "Copiando driver SQL compilado para ${target_name}..."
      COMMAND_EXPAND_LISTS # Ajuda na avaliação das expressões
    )
  endif()
endfunction()
# --- 4. Definição da sua Biblioteca ---
add_library(sql_wrapper INTERFACE)
add_library(sql_cpp_wrapper::sql_cpp_wrapper ALIAS sql_wrapper)

target_include_directories(
  sql_wrapper INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                        $<BUILD_INTERFACE:${CPPDB_INCLUDE_DIR}>)

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
