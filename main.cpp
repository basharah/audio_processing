#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cmath>
#include <thread>

struct SynthesisData {
  float phase = 0.0f;
  float frequency = 440.0f;
};

void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount){
  SynthesisData* data = (SynthesisData*)device->pUserData;
  float* out = (float*) output;
  (void) input;

  const float sampleRate = (float)device->sampleRate;

  for(ma_uint32 i=0; i < frameCount; ++i){
    float sample = sinf(2.0f * 3.14159265f * data->frequency * data->phase);
    data->phase += 1.0f / sampleRate;
    
    // Stereo output
    out[i*2 + 0] = sample;
    out[i*2 + 1] = sample; 
  } 
}

int main(){
  SynthesisData data;
  
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
  config.sampleRate = 44100;
  config.dataCallback = data_callback;
  config.pUserData = &data;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS){
    return -1;
  } 

  ma_device_start(&device);

  // Play sound for 3 seconds
  std::this_thread::sleep_for(std::chrono::seconds(3));

  ma_device_uninit(&device);
  return 0;
} 
