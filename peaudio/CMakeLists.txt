add_library(peaudio STATIC peaudio.cpp)
target_include_directories(peaudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

