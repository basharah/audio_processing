add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../external/miniaudio)

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

add_library(peaudio STATIC peaudio.cpp)
target_include_directories(peaudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(peaudio PUBLIC miniaudio)
