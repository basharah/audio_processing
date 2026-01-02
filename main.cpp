#include "peaudio.h"
#include "peprocess.h"

#include <iostream>
#include <string>

int main(){
  std::cout << "Audio Test Started" << std::endl;
  std::cout << "Recording Audio..." << std::endl;
  PEAudoBuffer buffer = record_audio(1, 5, 44100, 1);
  // add sample rate to. file name (read from buffer)
  std::cout << "Saving Audio to file..." << std::endl;
  std::string output_path = "output_" + std::to_string(buffer.sample_rate) + ".wav";
  save_wav(output_path.c_str(), buffer);
  // delete the buffer after saving
  buffer.samples.clear();
  std::cout << "Loading Audio from file..." << std::endl;
  PEAudoBuffer loaded_buffer = load_wav(output_path.c_str());
  std::cout << "Resampling Audio to 22050 Hz..." << std::endl;
  PEAudoBuffer resampled_buffer = resample_buffer(loaded_buffer, 22050);
  std::cout << "Saving Resampled Audio to file..." << std::endl;
  std::string resampled_output_path = "output_" + std::to_string(resampled_buffer.sample_rate) + ".wav";
  save_wav(resampled_output_path.c_str(), resampled_buffer);
  return 0;
} 
