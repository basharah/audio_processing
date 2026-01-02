#include "peaudio.h"
#include "peprocess.h"

#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

TEST(PeAudioBuffer, SaveWavWritesFile) {
  PEAudoBuffer buffer;
  buffer.sample_rate = 44100;
  buffer.channels = 1;
  buffer.samples = {0.0f, 0.25f, -0.25f, 0.0f};

  std::filesystem::path path = std::filesystem::temp_directory_path() / "peaudio_test.wav";
  std::filesystem::remove(path);

  int result = save_wav(path.string().c_str(), buffer);
  EXPECT_EQ(result, 0);
  EXPECT_TRUE(std::filesystem::exists(path));
  EXPECT_GT(std::filesystem::file_size(path), 0u);

  std::filesystem::remove(path);
}

TEST(PeAudioBuffer, RecordAudioInvalidIndexReturnsEmpty) {
  PEAudoBuffer buffer = record_audio(-1, 1);
  EXPECT_TRUE(buffer.samples.empty());
  EXPECT_EQ(buffer.sample_rate, 0u);
  EXPECT_EQ(buffer.channels, 0u);
}

TEST(PeAudioBuffer, LoadWavRoundTrip) {
  PEAudoBuffer input;
  input.sample_rate = 22050;
  input.channels = 1;
  input.samples = {0.0f, 0.5f, -0.5f, 0.0f};

  std::filesystem::path path = std::filesystem::temp_directory_path() / "peaudio_roundtrip.wav";
  std::filesystem::remove(path);

  ASSERT_EQ(save_wav(path.string().c_str(), input), 0);

  PEAudoBuffer output = load_wav(path.string().c_str());
  EXPECT_EQ(output.sample_rate, input.sample_rate);
  EXPECT_EQ(output.channels, input.channels);
  ASSERT_EQ(output.samples.size(), input.samples.size());
  for (std::size_t i = 0; i < input.samples.size(); ++i) {
    EXPECT_FLOAT_EQ(output.samples[i], input.samples[i]);
  }

  std::filesystem::remove(path);
}

TEST(PeAudioProcess, ResampleBufferLinearUpsample) {
  PEAudoBuffer input;
  input.sample_rate = 2;
  input.channels = 1;
  input.samples = {0.0f, 1.0f};

  PEAudoBuffer output = resample_buffer(input, 4);

  ASSERT_EQ(output.sample_rate, 4u);
  ASSERT_EQ(output.channels, 1u);
  ASSERT_EQ(output.samples.size(), 4u);
  EXPECT_FLOAT_EQ(output.samples[0], 0.0f);
  EXPECT_FLOAT_EQ(output.samples[1], 0.5f);
  EXPECT_FLOAT_EQ(output.samples[2], 1.0f);
  EXPECT_FLOAT_EQ(output.samples[3], 1.0f);
}

TEST(PeAudioProcess, ResampleBufferSameRatePassThrough) {
  PEAudoBuffer input;
  input.sample_rate = 44100;
  input.channels = 2;
  input.samples = {0.1f, -0.1f, 0.2f, -0.2f};

  PEAudoBuffer output = resample_buffer(input, 44100);

  EXPECT_EQ(output.sample_rate, 44100u);
  EXPECT_EQ(output.channels, 2u);
  EXPECT_EQ(output.samples, input.samples);
}

TEST(PeAudioProcess, LowPassFilterStepResponse) {
  PEAudoBuffer input;
  input.sample_rate = 4;
  input.channels = 1;
  input.samples = {1.0f, 1.0f, 1.0f, 1.0f};

  const float cutoff_hz = 1.0f;
  PEAudoBuffer output = low_pass_filter(input, cutoff_hz);

  ASSERT_EQ(output.samples.size(), input.samples.size());

  const float dt = 1.0f / static_cast<float>(input.sample_rate);
  const float rc = 1.0f / (2.0f * 3.14159265f * cutoff_hz);
  const float alpha = dt / (rc + dt);

  float y = 0.0f;
  for (std::size_t i = 0; i < input.samples.size(); ++i) {
    y = y + alpha * (input.samples[i] - y);
    EXPECT_NEAR(output.samples[i], y, 1e-5f);
  }
}

TEST(PeAudioProcess, HighPassFilterStepResponse) {
  PEAudoBuffer input;
  input.sample_rate = 4;
  input.channels = 1;
  input.samples = {1.0f, 1.0f, 1.0f, 1.0f};

  const float cutoff_hz = 1.0f;
  PEAudoBuffer output = high_pass_filter(input, cutoff_hz);

  ASSERT_EQ(output.samples.size(), input.samples.size());

  const float dt = 1.0f / static_cast<float>(input.sample_rate);
  const float rc = 1.0f / (2.0f * 3.14159265f * cutoff_hz);
  const float alpha = rc / (rc + dt);

  float prev_out = 0.0f;
  float prev_in = 0.0f;
  for (std::size_t i = 0; i < input.samples.size(); ++i) {
    float y = alpha * (prev_out + input.samples[i] - prev_in);
    EXPECT_NEAR(output.samples[i], y, 1e-5f);
    prev_out = y;
    prev_in = input.samples[i];
  }
}

TEST(PeAudioProcess, StereoToMonoAveragesChannels) {
  PEAudoBuffer input;
  input.sample_rate = 48000;
  input.channels = 2;
  input.samples = {1.0f, 0.0f, 0.0f, 1.0f};

  PEAudoBuffer output = stereo_to_mono(input);

  ASSERT_EQ(output.channels, 1u);
  ASSERT_EQ(output.samples.size(), 2u);
  EXPECT_FLOAT_EQ(output.samples[0], 0.5f);
  EXPECT_FLOAT_EQ(output.samples[1], 0.5f);
}

TEST(PeAudioProcess, ButterworthInvalidTypeReturnsEmpty) {
  PEAudoBuffer input;
  input.sample_rate = 48000;
  input.channels = 1;
  input.samples = {0.0f, 1.0f, 0.0f, -1.0f};

  PEAudoBuffer output = butterworth_filter(input, 1000.0f, "band", 4);
  EXPECT_TRUE(output.samples.empty());
  EXPECT_EQ(output.sample_rate, 0u);
  EXPECT_EQ(output.channels, 0u);
}

TEST(PeAudioProcess, ButterworthHighCutoffPassesThrough) {
  PEAudoBuffer input;
  input.sample_rate = 48000;
  input.channels = 1;
  const std::size_t frames = 1024;
  input.samples.resize(frames);
  for (std::size_t i = 0; i < frames; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(input.sample_rate);
    input.samples[i] = std::sin(2.0f * 3.14159265f * 1000.0f * t);
  }

  PEAudoBuffer output = butterworth_filter(input, 22000.0f, "low", 4);

  ASSERT_EQ(output.samples.size(), input.samples.size());
  double dot = 0.0;
  double in_energy = 0.0;
  double out_energy = 0.0;
  for (std::size_t i = 0; i < input.samples.size(); ++i) {
    double x = input.samples[i];
    double y = output.samples[i];
    dot += x * y;
    in_energy += x * x;
    out_energy += y * y;
  }
  double denom = std::sqrt(in_energy * out_energy);
  ASSERT_GT(denom, 0.0);
  double corr = dot / denom;
  EXPECT_GT(corr, 0.99);
}

TEST(PeAudioProcess, ButterworthOddOrderReturnsInput) {
  PEAudoBuffer input;
  input.sample_rate = 48000;
  input.channels = 1;
  input.samples = {0.0f, 0.25f, -0.25f, 0.5f};

  PEAudoBuffer output = butterworth_filter(input, 1000.0f, "low", 3);

  EXPECT_EQ(output.sample_rate, input.sample_rate);
  EXPECT_EQ(output.channels, input.channels);
  EXPECT_EQ(output.samples, input.samples);
}
