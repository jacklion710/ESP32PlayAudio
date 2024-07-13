# Audio Playback System for ESP32

An audio engine based off the I2S driver library. Can play generative test tonesas well as wave files stored on the SPIFFS partition.

## Why?

Integrating a functional audio engine into any application is no trivial task. This repository is intended as a starting point for future DSP based applications using Arduino.

## Usage

Handling of stereo files is not fully supported at the moment. It is recommended to use single channel mono audio files at a sampling rate of 44100 and a bit depth of 16 bits per sample. 

Assuming you have uploaded some audio data to your SPIFFS partition properly you will be able to select a file by replacing the argument in the `printWAVHeader()` and `playAudioFile()` functions.

```ino
// Print WAV header
printWAVHeader("/YOUR_AUDIO_FILE.wav");

...

// Play audio file
playAudioFile("/YOUR_AUDIO_FILE.wav");
```

Compile and upload your sketch to the ESP32 and you should be able to hear the audio on startup.

## Contributions

This project is still a work in progress. Stereo audio can be handled better for one thing. The biggest issue currently is that the audio doesn't seem to play through the entire wave file. The longer the wave file the longer playback occurs although truncated. I suspect there may be an issue when uploading to SPIFFS and will have to investigate these issues further.

In the meantime, if anyone would like to contribute then please feel free to open a pull request.
