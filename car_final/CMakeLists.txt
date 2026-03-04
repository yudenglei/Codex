cmake_minimum_required(VERSION 3.16)
project(cae_car_final LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cae_core INTERFACE)
target_include_directories(cae_core INTERFACE include)

add_executable(cae_demo src/main.cpp)
target_link_libraries(cae_demo PRIVATE cae_core)

add_executable(cae_tests tests/test_core.cpp)
target_link_libraries(cae_tests PRIVATE cae_core)

enable_testing()
add_test(NAME cae_tests COMMAND cae_tests)
