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

