cmake_minimum_required(VERSION 3.14)
project(networking VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Incluir headers
include_directories(include)

# Ejecutable del servidor
add_executable(server src/server.cpp)

# Ejecutable del cliente
add_executable(client src/client.cpp)

# Tests unitarios
enable_testing()
add_executable(unit_tests tests/test_network.cpp)
add_test(NAME unit_tests COMMAND unit_tests)

# Benchmarks
add_executable(benchmark benchmarks/benchmark.cpp)

