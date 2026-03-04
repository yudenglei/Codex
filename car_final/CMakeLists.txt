cmake_minimum_required(VERSION 3.16)
project(cae_car_final LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CapnProto QUIET)

add_library(cae_core
  src/capnp_io.cpp
)
target_include_directories(cae_core PUBLIC include)

if(CapnProto_FOUND)
  capnp_generate_cpp(CAR_CAPNP_SRCS CAR_CAPNP_HDRS car_pcb.capnp)
  target_sources(cae_core PRIVATE ${CAR_CAPNP_SRCS} ${CAR_CAPNP_HDRS})
  target_link_libraries(cae_core PUBLIC CapnProto::capnp CapnProto::kj)
  target_compile_definitions(cae_core PUBLIC CAE_ENABLE_CAPNP=1)
endif()

add_executable(cae_demo src/main.cpp)
target_link_libraries(cae_demo PRIVATE cae_core)

add_executable(cae_tests tests/test_core.cpp)
target_link_libraries(cae_tests PRIVATE cae_core)

enable_testing()
add_test(NAME cae_tests COMMAND cae_tests)
