#pragma once

#include "peaudio.h"

#include <cstdint>
#include <vector>

PEAudoBuffer resample_buffer(const PEAudoBuffer& input,
                             std::uint32_t target_sample_rate);

struct FilterState {
  std::vector<float> prev_in;
  std::vector<float> prev_out;
};

PEAudoBuffer low_pass_filter(const PEAudoBuffer& input, float cutoff_hz,
                             FilterState* state = nullptr);
PEAudoBuffer high_pass_filter(const PEAudoBuffer& input, float cutoff_hz,
                              FilterState* state = nullptr);

PEAudoBuffer stereo_to_mono(const PEAudoBuffer& input);

PEAudoBuffer butterworth_filter(const PEAudoBuffer& input, float cutoff_hz,
                                const char* type, int order = 4);
