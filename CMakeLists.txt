cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(peaudio VERSION 1.0)

add_subdirectory(peaudio)
add_executable(pe main.cpp)

target_link_libraries(pe PUBLIC peaudio)
