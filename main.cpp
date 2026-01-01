#include "peaudio.h"

int main(){
  std::cout << "Audio Test Started" << std::endl;
  std::cout << "Recording Audio..." << std::endl;
  PEAudoBuffer buffer = record_audio(1, 5, 44100, 1);
  std::cout << "Playing back recorded audio (from buffer)..." << std::endl;
  playback_buffer(buffer);
  save_wav("output.wav", buffer);
  std::cout << "Playing back recorded audio (from file)..." << std::endl;
  playback_file("output.wav");
  return 0;
} 
