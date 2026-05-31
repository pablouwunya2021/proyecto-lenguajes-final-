cmake_minimum_required(VERSION 3.20)
project(LenguajesCore VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-g -O0 -Wall -Wextra -Wno-unused-parameter)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

set(YALEX_SOURCES
    src/yalex/regex_engine.cpp
    src/yalex/nfa.cpp
    src/yalex/dfa.cpp
    src/yalex/tokenizer.cpp
    src/yalex/yalex_reader.cpp
)

set(YAPAR_SOURCES
    src/yapar/grammar.cpp
    src/yapar/lr0_automaton.cpp
    src/yapar/slr1_table.cpp
    src/yapar/ll1_table.cpp
    src/yapar/lalr_table.cpp
    src/yapar/conflict_resolver.cpp
    src/yapar/parser.cpp
)

add_library(lenguajes_core STATIC ${YALEX_SOURCES} ${YAPAR_SOURCES})
target_link_libraries(lenguajes_core nlohmann_json::nlohmann_json Threads::Threads)

add_executable(yalex_cli src/main_yalex.cpp)
target_link_libraries(yalex_cli lenguajes_core)

add_executable(yapar_cli src/main_yapar.cpp)
target_link_libraries(yapar_cli lenguajes_core)

enable_testing()
