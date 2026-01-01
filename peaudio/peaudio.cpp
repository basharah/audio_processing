#define MINIAUDIO_IMPLEMENTATION

#include <atomic>
#include <cmath>
#include <cstring>
#include <thread>

#include <iostream>
#include "peaudio.h"


void PEAudio::boo(){
  std::cout << "Boo\n";
} 



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

int test_audio() {
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

int list_audio_devices() {
  ma_context context;
  if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
    return -1;
  }

  ma_device_info* playbackInfos;
  ma_uint32 playbackCount;
  ma_device_info* captureInfos;
  ma_uint32 captureCount;

  if (ma_context_get_devices(&context, &playbackInfos, &playbackCount,
                             &captureInfos, &captureCount) != MA_SUCCESS) {
    ma_context_uninit(&context);
    return -1;
  }

  std::cout << "Playback devices:\n";
  for (ma_uint32 i = 0; i < playbackCount; ++i) {
    std::cout << "  [" << i << "] " << playbackInfos[i].name << "\n";
  }

  std::cout << "Capture devices:\n";
  for (ma_uint32 i = 0; i < captureCount; ++i) {
    std::cout << "  [" << i << "] " << captureInfos[i].name << "\n";
  }

  ma_context_uninit(&context);
  return 0;
}


void capture_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
  (void)output;
  CaptureData* data = (CaptureData*)device->pUserData;
  if (data == NULL || input == NULL || data->samples == NULL) {
    return;
  }

  if (data->frames_written >= data->frames_capacity) {
    return;
  }

  ma_uint32 frames_to_write = frameCount;
  if (data->frames_written + frames_to_write > data->frames_capacity) {
    frames_to_write = (ma_uint32)(data->frames_capacity - data->frames_written);
  }

  std::size_t sample_offset = data->frames_written * data->channels;
  std::size_t sample_count = (std::size_t)frames_to_write * data->channels;
  std::memcpy(data->samples + sample_offset, input, sample_count * sizeof(float));
  data->frames_written += frames_to_write;
}

PEAudoBuffer record_audio(int capture_device_index, int seconds,
                          std::uint32_t sample_rate,
                          std::uint32_t channels) {
  PEAudoBuffer result;
  if (seconds <= 0 || sample_rate == 0 || channels == 0) {
    return result;
  }

  ma_context context;
  if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
    return result;
  }

  ma_device_info* playbackInfos;
  ma_uint32 playbackCount;
  ma_device_info* captureInfos;
  ma_uint32 captureCount;
  if (ma_context_get_devices(&context, &playbackInfos, &playbackCount,
                             &captureInfos, &captureCount) != MA_SUCCESS) {
    ma_context_uninit(&context);
    return result;
  }

  if (capture_device_index < 0 ||
      (ma_uint32)capture_device_index >= captureCount) {
    ma_context_uninit(&context);
    return result;
  }

  std::size_t total_frames = (std::size_t)seconds * sample_rate;
  result.samples.resize(total_frames * channels);
  result.sample_rate = sample_rate;
  result.channels = channels;

  CaptureData data;
  data.samples = result.samples.data();
  data.frames_capacity = total_frames;
  data.channels = channels;

  ma_device_config config = ma_device_config_init(ma_device_type_capture);
  config.capture.format = ma_format_f32;
  config.capture.channels = channels;
  config.sampleRate = sample_rate;
  config.dataCallback = capture_data_callback;
  config.pUserData = &data;
  config.capture.pDeviceID = &captureInfos[capture_device_index].id;

  ma_device device;
  if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
    ma_context_uninit(&context);
    return PEAudoBuffer();
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    ma_device_uninit(&device);
    ma_context_uninit(&context);
    return PEAudoBuffer();
  }

  std::this_thread::sleep_for(std::chrono::seconds(seconds));

  ma_device_uninit(&device);
  ma_context_uninit(&context);

  if (data.frames_written < data.frames_capacity) {
    result.samples.resize(data.frames_written * channels);
  }

  return result;
}

int save_wav(const char* output_path, const PEAudoBuffer& buffer) {
  if (output_path == NULL || buffer.samples.empty() ||
      buffer.sample_rate == 0 || buffer.channels == 0) {
    return -1;
  }

  ma_encoder_config encoder_config = ma_encoder_config_init(
      ma_encoding_format_wav, ma_format_f32, buffer.channels, buffer.sample_rate);

  ma_encoder encoder;
  if (ma_encoder_init_file(output_path, &encoder_config, &encoder) != MA_SUCCESS) {
    return -1;
  }

  ma_uint64 frame_count = buffer.samples.size() / buffer.channels;
  ma_encoder_write_pcm_frames(&encoder, buffer.samples.data(), frame_count, NULL);
  ma_encoder_uninit(&encoder);
  return 0;
}

struct PlaybackBufferData {
  const float* samples = nullptr;
  std::size_t total_frames = 0;
  std::size_t frame_index = 0;
  ma_uint32 channels = 0;
  std::atomic<bool> done{false};
};

void playback_buffer_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
  (void)input;
  PlaybackBufferData* data = (PlaybackBufferData*)device->pUserData;
  float* out = (float*)output;
  if (data == NULL || data->samples == NULL || out == NULL) {
    return;
  }

  ma_uint32 frames_to_write = frameCount;
  if (data->frame_index + frames_to_write > data->total_frames) {
    frames_to_write = (ma_uint32)(data->total_frames - data->frame_index);
  }

  std::size_t sample_offset = data->frame_index * data->channels;
  std::size_t sample_count = (std::size_t)frames_to_write * data->channels;
  if (sample_count > 0) {
    std::memcpy(out, data->samples + sample_offset, sample_count * sizeof(float));
  }

  if (frames_to_write < frameCount) {
    std::size_t remaining_samples = (std::size_t)(frameCount - frames_to_write) * data->channels;
    std::memset(out + sample_count, 0, remaining_samples * sizeof(float));
    data->done.store(true);
  }

  data->frame_index += frames_to_write;
}

int playback_buffer(const PEAudoBuffer& buffer) {
  if (buffer.samples.empty() || buffer.sample_rate == 0 || buffer.channels == 0) {
    return -1;
  }

  PlaybackBufferData data;
  data.samples = buffer.samples.data();
  data.total_frames = buffer.samples.size() / buffer.channels;
  data.channels = buffer.channels;

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = buffer.channels;
  config.sampleRate = buffer.sample_rate;
  config.dataCallback = playback_buffer_callback;
  config.pUserData = &data;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    return -1;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    ma_device_uninit(&device);
    return -1;
  }

  while (!data.done.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ma_device_uninit(&device);
  return 0;
}

struct PlaybackFileData {
  ma_decoder decoder;
  ma_uint32 channels = 0;
  std::atomic<bool> done{false};
};

void playback_file_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
  (void)input;
  PlaybackFileData* data = (PlaybackFileData*)device->pUserData;
  float* out = (float*)output;
  if (data == NULL || out == NULL) {
    return;
  }

  ma_uint64 frames_read = 0;
  ma_decoder_read_pcm_frames(&data->decoder, out, frameCount, &frames_read);

  if (frames_read < frameCount) {
    std::size_t written_samples = (std::size_t)frames_read * data->channels;
    std::size_t remaining_samples = (std::size_t)(frameCount - frames_read) * data->channels;
    std::memset(out + written_samples, 0, remaining_samples * sizeof(float));
    data->done.store(true);
  }
}

int playback_file(const char* input_path) {
  if (input_path == NULL) {
    return -1;
  }

  PlaybackFileData data;
  ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 0, 0);
  if (ma_decoder_init_file(input_path, &decoder_config, &data.decoder) != MA_SUCCESS) {
    return -1;
  }
  data.channels = data.decoder.outputChannels;

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = data.decoder.outputChannels;
  config.sampleRate = data.decoder.outputSampleRate;
  config.dataCallback = playback_file_callback;
  config.pUserData = &data;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    ma_decoder_uninit(&data.decoder);
    return -1;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    ma_device_uninit(&device);
    ma_decoder_uninit(&data.decoder);
    return -1;
  }

  while (!data.done.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ma_device_uninit(&device);
  ma_decoder_uninit(&data.decoder);
  return 0;
}
