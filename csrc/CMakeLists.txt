cmake_minimum_required(VERSION 3.21)
project(SA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")


include(FetchContent)
FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
)
FetchContent_MakeAvailable(pybind11)

find_package(Python REQUIRED COMPONENTS Interpreter Development)

pybind11_add_module(
    SA_token SHARED SA.cpp
)

target_include_directories(SA_token PRIVATE
    ${Python_INCLUDE_DIRS}
)

target_link_libraries(SA_token PRIVATE
    pybind11::module
)

#rm -rf build && cmake -B build && cmake --build build