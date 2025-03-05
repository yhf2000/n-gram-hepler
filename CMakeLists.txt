cmake_minimum_required(VERSION 3.21)
project(SA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

include(FetchContent)
FetchContent_Declare(
    abseil
    GIT_REPOSITORY https://ghfast.top/https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20240116.1
)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_MakeAvailable(abseil)


find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

pybind11_add_module(
    SA_token
    SHARED
    SA.cpp
)

target_include_directories(SA_token PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann
    ${Python_INCLUDE_DIRS}
)

target_link_libraries(SA_token PRIVATE
    pybind11::module
    absl::flat_hash_map
)

#rm -rf build && cmake -B build && cmake --build build