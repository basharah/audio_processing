#include "peaudio.h"

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
