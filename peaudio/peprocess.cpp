#include "peprocess.h"

#include <algorithm>
#include <cmath>
#include <complex>
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

static float hz_to_mel(float hz) {
  return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

static float mel_to_hz(float mel) {
  return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

static std::vector<float> mixdown_mono(const PEAudoBuffer& input) {
  std::vector<float> mono;
  if (input.samples.empty() || input.channels == 0) {
    return mono;
  }
  if (input.channels == 1) {
    mono = input.samples;
    return mono;
  }
  const std::size_t frames = input.samples.size() / input.channels;
  mono.resize(frames);
  for (std::size_t frame = 0; frame < frames; ++frame) {
    float sum = 0.0f;
    std::size_t idx = frame * input.channels;
    for (std::size_t ch = 0; ch < input.channels; ++ch) {
      sum += input.samples[idx + ch];
    }
    mono[frame] = sum / static_cast<float>(input.channels);
  }
  return mono;
}

static std::vector<float> make_hann_window(std::uint32_t win_length) {
  std::vector<float> window(win_length, 0.0f);
  if (win_length == 0) {
    return window;
  }
  if (win_length == 1) {
    window[0] = 1.0f;
    return window;
  }
  for (std::uint32_t n = 0; n < win_length; ++n) {
    window[n] = 0.5f - 0.5f * std::cos(2.0f * 3.14159265f * n / (win_length - 1));
  }
  return window;
}

static std::vector<float> make_mel_filterbank(std::uint32_t sample_rate,
                                              std::uint32_t n_fft,
                                              std::uint32_t total_mels,
                                              float fmin, float fmax) {
  const std::uint32_t bins = n_fft / 2 + 1;
  std::vector<float> bank(total_mels * bins, 0.0f);
  if (total_mels == 0 || bins == 0 || sample_rate == 0) {
    return bank;
  }

  const float nyquist = 0.5f * static_cast<float>(sample_rate);
  float fmin_clamped = std::max(0.0f, fmin);
  float fmax_clamped = fmax <= 0.0f ? nyquist : std::min(fmax, nyquist);
  if (fmax_clamped <= fmin_clamped) {
    return bank;
  }

  float mel_min = hz_to_mel(fmin_clamped);
  float mel_max = hz_to_mel(fmax_clamped);

  std::vector<float> mel_points(total_mels + 2);
  for (std::uint32_t i = 0; i < total_mels + 2; ++i) {
    mel_points[i] = mel_min + (mel_max - mel_min) * (static_cast<float>(i) /
                                                     static_cast<float>(total_mels + 1));
  }

  std::vector<float> hz_points(total_mels + 2);
  for (std::uint32_t i = 0; i < total_mels + 2; ++i) {
    hz_points[i] = mel_to_hz(mel_points[i]);
  }

  std::vector<float> bin_freqs(bins);
  for (std::uint32_t i = 0; i < bins; ++i) {
    bin_freqs[i] = static_cast<float>(i) * sample_rate / static_cast<float>(n_fft);
  }

  for (std::uint32_t m = 0; m < total_mels; ++m) {
    float left = hz_points[m];
    float center = hz_points[m + 1];
    float right = hz_points[m + 2];
    for (std::uint32_t k = 0; k < bins; ++k) {
      float freq = bin_freqs[k];
      float weight = 0.0f;
      if (freq >= left && freq <= center) {
        weight = (freq - left) / (center - left);
      } else if (freq > center && freq <= right) {
        weight = (right - freq) / (right - center);
      }
      if (weight < 0.0f) {
        weight = 0.0f;
      }
      bank[m * bins + k] = weight;
    }
  }

  return bank;
}

static MelSpectrogram compute_mel_spectrogram(const PEAudoBuffer& input,
                                              std::uint32_t n_fft,
                                              std::uint32_t hop_length,
                                              std::uint32_t win_length,
                                              std::uint32_t n_mels,
                                              std::uint32_t total_mels,
                                              float fmin, float fmax,
                                              float power) {
  MelSpectrogram result;
  if (input.samples.empty() || input.sample_rate == 0 || input.channels == 0 ||
      n_fft == 0 || hop_length == 0 || win_length == 0 || n_mels == 0 ||
      total_mels == 0 || n_mels > total_mels || win_length > n_fft) {
    return result;
  }

  std::vector<float> mono = mixdown_mono(input);
  if (mono.size() < win_length) {
    return result;
  }

  const std::uint32_t bins = n_fft / 2 + 1;
  const std::size_t frames = 1 + (mono.size() - win_length) / hop_length;
  if (frames == 0) {
    return result;
  }

  std::vector<float> window = make_hann_window(win_length);
  std::vector<float> filterbank = make_mel_filterbank(
      input.sample_rate, n_fft, total_mels, fmin, fmax);

  if (filterbank.empty()) {
    return result;
  }

  result.frames = static_cast<std::uint32_t>(frames);
  result.n_mels = n_mels;
  result.total_mels = total_mels;
  result.mel_spec.assign(total_mels * frames, 0.0f);

  std::vector<float> fft_input(n_fft, 0.0f);
  std::vector<float> spectrum(bins, 0.0f);

  for (std::size_t frame = 0; frame < frames; ++frame) {
    std::size_t start = frame * hop_length;
    for (std::uint32_t i = 0; i < win_length; ++i) {
      fft_input[i] = mono[start + i] * window[i];
    }
    for (std::uint32_t i = win_length; i < n_fft; ++i) {
      fft_input[i] = 0.0f;
    }

    for (std::uint32_t k = 0; k < bins; ++k) {
      float re = 0.0f;
      float im = 0.0f;
      for (std::uint32_t n = 0; n < n_fft; ++n) {
        float angle = -2.0f * 3.14159265f * k * n / static_cast<float>(n_fft);
        re += fft_input[n] * std::cos(angle);
        im += fft_input[n] * std::sin(angle);
      }
      float mag = std::sqrt(re * re + im * im);
      spectrum[k] = power == 1.0f ? mag : mag * mag;
    }

    for (std::uint32_t m = 0; m < total_mels; ++m) {
      float sum = 0.0f;
      const float* weights = &filterbank[m * bins];
      for (std::uint32_t k = 0; k < bins; ++k) {
        sum += weights[k] * spectrum[k];
      }
      result.mel_spec[m * frames + frame] = sum;
    }
  }

  return result;
}

MelSpectrogram log_mel_spectrogram(const PEAudoBuffer& input,
                                   std::uint32_t n_fft,
                                   std::uint32_t hop_length,
                                   std::uint32_t win_length,
                                   std::uint32_t n_mels,
                                   std::uint32_t total_mels,
                                   float fmin,
                                   float fmax) {
  MelSpectrogram result = compute_mel_spectrogram(
      input, n_fft, hop_length, win_length, n_mels, total_mels, fmin, fmax, 2.0f);
  if (result.mel_spec.empty()) {
    return result;
  }

  const std::size_t frames = result.frames;
  result.normalized.assign(frames * n_mels, 0.0f);

  float ref = 0.0f;
  for (std::uint32_t m = 0; m < n_mels; ++m) {
    for (std::size_t frame = 0; frame < frames; ++frame) {
      float v = result.mel_spec[m * frames + frame];
      if (v > ref) {
        ref = v;
      }
    }
  }

  const float amin = 1e-10f;
  if (ref < amin) {
    ref = amin;
  }

  for (std::uint32_t m = 0; m < n_mels; ++m) {
    for (std::size_t frame = 0; frame < frames; ++frame) {
      float v = result.mel_spec[m * frames + frame];
      float val = std::max(v, amin);
      float db = 10.0f * std::log10(val / ref);
      result.normalized[frame * n_mels + m] = db;
    }
  }

  return result;
}

MelSpectrogram pcen_mel_spectrogram(const PEAudoBuffer& input,
                                    std::uint32_t n_fft,
                                    std::uint32_t hop_length,
                                    std::uint32_t win_length,
                                    std::uint32_t n_mels,
                                    std::uint32_t total_mels,
                                    float fmin,
                                    float fmax) {
  MelSpectrogram result = compute_mel_spectrogram(
      input, n_fft, hop_length, win_length, n_mels, total_mels, fmin, fmax, 1.0f);
  if (result.mel_spec.empty()) {
    return result;
  }

  const std::size_t frames = result.frames;
  result.normalized.assign(frames * n_mels, 0.0f);

  const float gain = 0.8f;
  const float bias = 10.0f;
  const float power = 0.25f;
  const float eps = 1e-6f;
  const float scale = std::ldexp(1.0f, 31);
  const float t = 0.06f;
  const float b = std::exp(-static_cast<float>(hop_length) / (t * input.sample_rate));

  std::vector<float> m_smooth(n_mels, 0.0f);

  for (std::size_t frame = 0; frame < frames; ++frame) {
    for (std::uint32_t m = 0; m < n_mels; ++m) {
      float x = result.mel_spec[m * frames + frame] * scale;
      float smooth = (frame == 0) ? x : (1.0f - b) * x + b * m_smooth[m];
      m_smooth[m] = smooth;
      float denom = std::pow(eps + smooth, gain);
      float pcen = std::pow(x / denom + bias, power) - std::pow(bias, power);
      result.normalized[frame * n_mels + m] = pcen;
    }
  }

  return result;
}
