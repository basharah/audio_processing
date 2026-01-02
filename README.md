# Audio Processing

## Description
Small C++ audio utility library built on miniaudio for recording, playback, and simple DSP.

## Features
See `FEATURES.md` for a full list.

## TODO
Planned work is tracked in `TODO.md`.

## Mel Spectrogram Output Layout
- `MelSpectrogram.mel_spec` is stored as `[mel_index, frame]` in row-major order (`mel_index * frames + frame`).
- `MelSpectrogram.normalized` is stored as `[frame, mel_index]` in row-major order (`frame * n_mels + mel_index`).

## Setup
- CMake 3.10+ and a C++17 compiler.
- macOS links CoreAudio frameworks via CMake.

## Build
```bash
./build.sh
```

## Test
```bash
./build.sh --run-tests
```
