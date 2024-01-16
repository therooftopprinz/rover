cmake_minimum_required(VERSION 3.10)

project(rover)

include(ExternalProject)
include(FetchContent)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_PREFIX_PATH dependencies_install)

FetchContent_Declare(
    bfc
    GIT_REPOSITORY https://github.com/therooftopprinz/BFC.git
    GIT_TAG master)
FetchContent_MakeAvailable(bfc)

# cd dependencies_src/websocketpp/
# mkdir build && cd build
# cmake -DCMAKE_INSTALL_PREFIX=../../../dependencies_install/websocketpp ..
# cmake --build . --target install

find_package(websocketpp REQUIRED)

file(GLOB_RECURSE rover_src src/*.cpp)

add_executable(rover ${rover_src})

target_link_libraries(rover  PRIVATE bfc websocketpp::websocketpp pthread wiringPi)
target_include_directories(rover PRIVATE src dependencies_src/asio/include)

target_compile_features(rover PRIVATE cxx_std_17)
