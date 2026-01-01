#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <cmath>
#include <thread>

static float g_time = 0.0f;

void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount){
  float* out = (float*) output;
  (void) input;

  const float frequency = 440.0f;
  const float sampleRate = (float)device->sampleRate;

  for(ma_uint32 i=0; i < sampleRate; ++i){
    float sample = sinf(2.0f * 3.14159265f * frequency * g_time);
    g_time += 1.0f / sampleRate;
    out[i*2 + 0] = sample;
    out[i*2 + 1] = sample; 
  } 
}

int main(){
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
  config.sampleRate = 44100;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS){
    return -1;
  } 

  ma_device_start(&device);

  // Paly sound for 3 seconds
  std::this_thread::sleep_for(std::chrono::seconds(3));

  ma_device_uninit(&device);
  return 0;
} 
