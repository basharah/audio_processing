cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(peaudio VERSION 1.0)

add_subdirectory(peaudio)
add_executable(pe main.cpp)

target_link_libraries(pe PUBLIC peaudio)

option(PEAUDIO_BUILD_TESTS "Build tests" ON)
if(PEAUDIO_BUILD_TESTS)
  include(FetchContent)
  cmake_policy(SET CMP0135 NEW)
  set(CMAKE_WARN_DEPRECATED OFF)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  set(CMAKE_WARN_DEPRECATED ON)

  enable_testing()
  add_executable(peaudio_tests tests/peaudio_tests.cpp)
  target_link_libraries(peaudio_tests PRIVATE GTest::gtest_main peaudio)
  include(GoogleTest)
  gtest_discover_tests(peaudio_tests)
endif()
