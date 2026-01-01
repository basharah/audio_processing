cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(peaudio VERSION 1.0)

add_subdirectory(peaudio)
add_executable(pe main.cpp)

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio)
find_package(Threads REQUIRED)
if(WIN32)
  target_link_libraries(miniaudio INTERFACE winmm)
elseif(APPLE)
  target_link_libraries(miniaudio INTERFACE
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework CoreFoundation"
  )
elseif (UNIX) 
  target_link_libraries(miniaudio INTERFACE Threads::Threads dl m)
endif()

target_link_libraries(pe PRIVATE miniaudio)
target_link_libraries(pe PUBLIC peaudio)
