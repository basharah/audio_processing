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

struct MelSpectrogram {
  std::vector<float> mel_spec;
  std::vector<float> normalized;
  std::uint32_t frames = 0;
  std::uint32_t n_mels = 0;
  std::uint32_t total_mels = 0;
};

MelSpectrogram log_mel_spectrogram(const PEAudoBuffer& input,
                                   std::uint32_t n_fft,
                                   std::uint32_t hop_length,
                                   std::uint32_t win_length,
                                   std::uint32_t n_mels,
                                   std::uint32_t total_mels,
                                   float fmin,
                                   float fmax);

MelSpectrogram pcen_mel_spectrogram(const PEAudoBuffer& input,
                                    std::uint32_t n_fft,
                                    std::uint32_t hop_length,
                                    std::uint32_t win_length,
                                    std::uint32_t n_mels,
                                    std::uint32_t total_mels,
                                    float fmin,
                                    float fmax);
