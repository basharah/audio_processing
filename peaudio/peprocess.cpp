#include "peprocess.h"

#include <algorithm>
#include <cmath>
#include <cstring>

PEAudoBuffer resample_buffer(const PEAudoBuffer& input,
                             std::uint32_t target_sample_rate) {
  PEAudoBuffer output;
  if (input.samples.empty() || input.sample_rate == 0 ||
      input.channels == 0 || target_sample_rate == 0) {
    return output;
  }

  output.sample_rate = target_sample_rate;
  output.channels = input.channels;

  if (input.sample_rate == target_sample_rate) {
    output.samples = input.samples;
    return output;
  }

  const double rate_ratio =
      static_cast<double>(target_sample_rate) / input.sample_rate;
  const std::size_t input_frames = input.samples.size() / input.channels;
  const std::size_t output_frames =
      static_cast<std::size_t>(std::ceil(input_frames * rate_ratio));

  output.samples.resize(output_frames * output.channels);

  for (std::size_t out_frame = 0; out_frame < output_frames; ++out_frame) {
    double src_pos = out_frame / rate_ratio;
    std::size_t src_index = static_cast<std::size_t>(src_pos);
    double frac = src_pos - src_index;

    if (src_index >= input_frames) {
      src_index = input_frames - 1;
      frac = 0.0;
    }

    std::size_t src_index_next =
        std::min(src_index + 1, input_frames - 1);

    for (std::size_t ch = 0; ch < input.channels; ++ch) {
      float a = input.samples[src_index * input.channels + ch];
      float b = input.samples[src_index_next * input.channels + ch];
      float sample = static_cast<float>((1.0 - frac) * a + frac * b);
      output.samples[out_frame * output.channels + ch] = sample;
    }
  }

  return output;
}

static void init_filter_state(FilterState* state, std::uint32_t channels) {
  if (state == nullptr) {
    return;
  }
  if (state->prev_in.size() != channels) {
    state->prev_in.assign(channels, 0.0f);
  }
  if (state->prev_out.size() != channels) {
    state->prev_out.assign(channels, 0.0f);
  }
}

PEAudoBuffer low_pass_filter(const PEAudoBuffer& input, float cutoff_hz,
                             FilterState* state) {
  PEAudoBuffer output;
  if (input.samples.empty() || input.sample_rate == 0 || input.channels == 0 ||
      cutoff_hz <= 0.0f) {
    return output;
  }

  output.sample_rate = input.sample_rate;
  output.channels = input.channels;
  output.samples.resize(input.samples.size());

  init_filter_state(state, input.channels);
  std::vector<float> local_prev_out;
  std::vector<float> local_prev_in;
  float* prev_out = nullptr;
  float* prev_in = nullptr;
  if (state != nullptr) {
    prev_out = state->prev_out.data();
    prev_in = state->prev_in.data();
  } else {
    local_prev_out.assign(input.channels, 0.0f);
    local_prev_in.assign(input.channels, 0.0f);
    prev_out = local_prev_out.data();
    prev_in = local_prev_in.data();
  }

  const float dt = 1.0f / static_cast<float>(input.sample_rate);
  const float rc = 1.0f / (2.0f * 3.14159265f * cutoff_hz);
  const float alpha = dt / (rc + dt);

  const std::size_t frames = input.samples.size() / input.channels;
  for (std::size_t frame = 0; frame < frames; ++frame) {
    for (std::size_t ch = 0; ch < input.channels; ++ch) {
      std::size_t idx = frame * input.channels + ch;
      float x = input.samples[idx];
      float y = prev_out[ch] + alpha * (x - prev_out[ch]);
      output.samples[idx] = y;
      prev_out[ch] = y;
      prev_in[ch] = x;
    }
  }

  return output;
}

PEAudoBuffer high_pass_filter(const PEAudoBuffer& input, float cutoff_hz,
                              FilterState* state) {
  PEAudoBuffer output;
  if (input.samples.empty() || input.sample_rate == 0 || input.channels == 0 ||
      cutoff_hz <= 0.0f) {
    return output;
  }

  output.sample_rate = input.sample_rate;
  output.channels = input.channels;
  output.samples.resize(input.samples.size());

  init_filter_state(state, input.channels);
  std::vector<float> local_prev_out;
  std::vector<float> local_prev_in;
  float* prev_out = nullptr;
  float* prev_in = nullptr;
  if (state != nullptr) {
    prev_out = state->prev_out.data();
    prev_in = state->prev_in.data();
  } else {
    local_prev_out.assign(input.channels, 0.0f);
    local_prev_in.assign(input.channels, 0.0f);
    prev_out = local_prev_out.data();
    prev_in = local_prev_in.data();
  }

  const float dt = 1.0f / static_cast<float>(input.sample_rate);
  const float rc = 1.0f / (2.0f * 3.14159265f * cutoff_hz);
  const float alpha = rc / (rc + dt);

  const std::size_t frames = input.samples.size() / input.channels;
  for (std::size_t frame = 0; frame < frames; ++frame) {
    for (std::size_t ch = 0; ch < input.channels; ++ch) {
      std::size_t idx = frame * input.channels + ch;
      float x = input.samples[idx];
      float y = alpha * (prev_out[ch] + x - prev_in[ch]);
      output.samples[idx] = y;
      prev_out[ch] = y;
      prev_in[ch] = x;
    }
  }

  return output;
}

PEAudoBuffer stereo_to_mono(const PEAudoBuffer& input) {
  PEAudoBuffer output;
  if (input.samples.empty() || input.sample_rate == 0 || input.channels == 0) {
    return output;
  }

  if (input.channels == 1) {
    output = input;
    return output;
  }

  output.sample_rate = input.sample_rate;
  output.channels = 1;

  const std::size_t frames = input.samples.size() / input.channels;
  output.samples.resize(frames);

  for (std::size_t frame = 0; frame < frames; ++frame) {
    std::size_t idx = frame * input.channels;
    float sum = 0.0f;
    for (std::size_t ch = 0; ch < input.channels; ++ch) {
      sum += input.samples[idx + ch];
    }
    output.samples[frame] = sum / static_cast<float>(input.channels);
  }

  return output;
}

struct Biquad {
  float b0 = 0.0f;
  float b1 = 0.0f;
  float b2 = 0.0f;
  float a1 = 0.0f;
  float a2 = 0.0f;
};

static Biquad make_lowpass(float sample_rate, float cutoff_hz, float q) {
  const float w0 = 2.0f * 3.14159265f * cutoff_hz / sample_rate;
  const float cos_w0 = std::cos(w0);
  const float sin_w0 = std::sin(w0);
  const float alpha = sin_w0 / (2.0f * q);

  const float b0 = (1.0f - cos_w0) * 0.5f;
  const float b1 = 1.0f - cos_w0;
  const float b2 = (1.0f - cos_w0) * 0.5f;
  const float a0 = 1.0f + alpha;
  const float a1 = -2.0f * cos_w0;
  const float a2 = 1.0f - alpha;

  return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

static Biquad make_highpass(float sample_rate, float cutoff_hz, float q) {
  const float w0 = 2.0f * 3.14159265f * cutoff_hz / sample_rate;
  const float cos_w0 = std::cos(w0);
  const float sin_w0 = std::sin(w0);
  const float alpha = sin_w0 / (2.0f * q);

  const float b0 = (1.0f + cos_w0) * 0.5f;
  const float b1 = -(1.0f + cos_w0);
  const float b2 = (1.0f + cos_w0) * 0.5f;
  const float a0 = 1.0f + alpha;
  const float a1 = -2.0f * cos_w0;
  const float a2 = 1.0f - alpha;

  return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

static void run_sos_inplace(const std::vector<Biquad>& sos, float* data,
                            std::size_t frames, std::uint32_t channels) {
  for (const Biquad& bq : sos) {
    std::vector<float> z1(channels, 0.0f);
    std::vector<float> z2(channels, 0.0f);
    for (std::size_t frame = 0; frame < frames; ++frame) {
      for (std::size_t ch = 0; ch < channels; ++ch) {
        std::size_t idx = frame * channels + ch;
        float x = data[idx];
        float y = bq.b0 * x + z1[ch];
        z1[ch] = bq.b1 * x - bq.a1 * y + z2[ch];
        z2[ch] = bq.b2 * x - bq.a2 * y;
        data[idx] = y;
      }
    }
  }
}

static std::size_t make_odd_pad(const float* input, std::size_t frames,
                                std::uint32_t channels, std::size_t padlen,
                                std::vector<float>& padded) {
  if (frames == 0 || channels == 0) {
    return 0;
  }
  std::size_t pad = padlen;
  if (frames <= 1) {
    pad = 0;
  } else if (pad >= frames) {
    pad = frames - 1;
  }

  const std::size_t padded_frames = frames + 2 * pad;
  padded.assign(padded_frames * channels, 0.0f);

  for (std::size_t ch = 0; ch < channels; ++ch) {
    float first = input[ch];
    for (std::size_t i = 0; i < pad; ++i) {
      float x = input[(i + 1) * channels + ch];
      padded[i * channels + ch] = 2.0f * first - x;
    }

    for (std::size_t frame = 0; frame < frames; ++frame) {
      padded[(pad + frame) * channels + ch] = input[frame * channels + ch];
    }

    float last = input[(frames - 1) * channels + ch];
    for (std::size_t i = 0; i < pad; ++i) {
      float x = input[(frames - 2 - i) * channels + ch];
      padded[(pad + frames + i) * channels + ch] = 2.0f * last - x;
    }
  }

  return pad;
}

static void reverse_interleaved(float* data, std::size_t frames,
                                std::uint32_t channels) {
  for (std::size_t frame = 0; frame < frames / 2; ++frame) {
    for (std::size_t ch = 0; ch < channels; ++ch) {
      std::size_t a = frame * channels + ch;
      std::size_t b = (frames - 1 - frame) * channels + ch;
      std::swap(data[a], data[b]);
    }
  }
}

static std::vector<Biquad> make_butterworth_sos(std::uint32_t sample_rate,
                                                float cutoff_hz, bool is_low,
                                                int order) {
  std::vector<Biquad> sos;
  if (order <= 0 || (order % 2) != 0) {
    return sos;
  }

  const int sections = order / 2;
  sos.reserve(sections);

  for (int k = 1; k <= sections; ++k) {
    float angle = static_cast<float>((2 * k - 1) * 3.14159265 / (2.0 * order));
    float q = 1.0f / (2.0f * std::cos(angle));
    Biquad bq = is_low ? make_lowpass(sample_rate, cutoff_hz, q)
                       : make_highpass(sample_rate, cutoff_hz, q);
    sos.push_back(bq);
  }

  return sos;
}

PEAudoBuffer butterworth_filter(const PEAudoBuffer& input, float cutoff_hz,
                                const char* type, int order) {
  PEAudoBuffer output;
  if (input.samples.empty() || input.sample_rate == 0 || input.channels == 0 ||
      cutoff_hz <= 0.0f || type == nullptr || order <= 0) {
    return output;
  }

  const float nyquist = 0.5f * static_cast<float>(input.sample_rate);
  float normalized = cutoff_hz / nyquist;
  if (normalized >= 1.0f) {
    normalized = 0.999f;
  }
  const float clamped_cutoff = normalized * nyquist;

  bool is_low = std::strcmp(type, "low") == 0;
  bool is_high = std::strcmp(type, "high") == 0;
  if (!is_low && !is_high) {
    return output;
  }

  output = input;
  const std::size_t frames = output.samples.size() / output.channels;
  if (frames < 2 || (order % 2) != 0) {
    return output;
  }

  std::vector<Biquad> sos =
      make_butterworth_sos(output.sample_rate, clamped_cutoff, is_low, order);
  if (sos.empty()) {
    return PEAudoBuffer();
  }

  const std::size_t padlen = static_cast<std::size_t>(3 * order);
  std::vector<float> padded;
  std::size_t pad = make_odd_pad(output.samples.data(), frames, output.channels,
                                 padlen, padded);
  std::size_t padded_frames = frames + 2 * pad;

  run_sos_inplace(sos, padded.data(), padded_frames, output.channels);
  reverse_interleaved(padded.data(), padded_frames, output.channels);
  run_sos_inplace(sos, padded.data(), padded_frames, output.channels);
  reverse_interleaved(padded.data(), padded_frames, output.channels);

  output.samples.resize(frames * output.channels);
  for (std::size_t frame = 0; frame < frames; ++frame) {
    for (std::size_t ch = 0; ch < output.channels; ++ch) {
      output.samples[frame * output.channels + ch] =
          padded[(pad + frame) * output.channels + ch];
    }
  }

  return output;
}
