#pragma once

#include "miniaudio.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

class PEAudio{
  public:
    void boo();
}; 

int test_audio();
int list_audio_devices();

struct SynthesisData {
  float phase = 0.0f;
  float frequency = 440.0f;
};

struct PEAudoBuffer {
  std::vector<float> samples;
  std::uint32_t sample_rate = 0;
  std::uint32_t channels = 0;
};

struct CaptureData {
  float* samples = nullptr;
  std::size_t frames_capacity = 0;
  std::size_t frames_written = 0;
  ma_uint32 channels = 0;
};


PEAudoBuffer record_audio(int capture_device_index, int seconds,
                         std::uint32_t sample_rate = 44100,
                         std::uint32_t channels = 1);

int save_wav(const char* output_path, const PEAudoBuffer& buffer);
int playback_buffer(const PEAudoBuffer& buffer);
int playback_file(const char* input_path);
