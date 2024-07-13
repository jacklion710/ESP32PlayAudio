#include <Arduino.h>
#include <driver/i2s.h>
#include <SPIFFS.h>

// Audio configuration
#define SAMPLE_RATE     44100
#define I2S_NUM         I2S_NUM_0
#define I2S_DOUT        17  // DIN
#define I2S_BCLK        19  // BCLK
#define I2S_LRC         18  // LRC

// Structure to hold WAV format information
struct WAVFormat {
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
};

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100); // Wait for Serial to be ready

  Serial.println("Starting up...");

  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // List SPIFFS files
  listSPIFFSFiles();

  // Print WAV header
  printWAVHeader("/beat2.wav");

  // Configure I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Changed to mono
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 64,  // Increase DMA buffer count
    .dma_buf_len = 1024,  // Increase DMA buffer length
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  // Install and start I2S driver
  esp_err_t result = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.printf("Error installing I2S driver: %d\n", result);
    return;
  }

  // Set I2S pins
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  result = i2s_set_pin(I2S_NUM, &pin_config);
  if (result != ESP_OK) {
    Serial.printf("Error setting I2S pins: %d\n", result);
    return;
  }

  // Play test tone
//  playTestTone();

  // Play audio file
  playAudioFile("/beat2.wav");
}

void loop() {
  // Nothing to do here
}

struct WAVHeader {
  char riff[4];
  uint32_t overallSize;
  char wave[4];
  char fmt[4];
  uint32_t fmtLength;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
};

void playAudioFile(const char* filename) {
  File audioFile = openAudioFile(filename);
  if (!audioFile) return;

  if (!validateWAVHeader(audioFile)) {
    audioFile.close();
    return;
  }

  WAVFormat format;
  if (!processFmtChunk(audioFile, &format)) {
    audioFile.close();
    return;
  }

  uint32_t dataSize;
  if (!findDataChunk(audioFile, &dataSize)) {
    audioFile.close();
    return;
  }

  Serial.printf("Starting playback. Data size: %u bytes\n", dataSize);
  Serial.printf("Sample rate: %u Hz, Channels: %d, Bits per sample: %d\n",
                format.sampleRate, format.numChannels, format.bitsPerSample);

  playAudioData(audioFile, format, dataSize);

  audioFile.close();
}

// Function to open audio file
File openAudioFile(const char* filename) {
  Serial.printf("Playing file: %s\n", filename);
  File audioFile = SPIFFS.open(filename, "r");
  if (!audioFile) {
    Serial.println("Failed to open file for reading");
  }
  return audioFile;
}

// Function to validate WAV header
bool validateWAVHeader(File &audioFile) {
  char header[4];
  audioFile.read((uint8_t*)header, 4);
  if (strncmp(header, "RIFF", 4) != 0) {
    Serial.println("Not a valid WAV file - RIFF missing");
    return false;
  }

  audioFile.seek(audioFile.position() + 4);

  audioFile.read((uint8_t*)header, 4);
  if (strncmp(header, "WAVE", 4) != 0) {
    Serial.println("Not a valid WAV file - WAVE missing");
    return false;
  }

  return true;
}

// Function to process fmt chunk
bool processFmtChunk(File &audioFile, WAVFormat *format) {
  char header[4];
  uint32_t chunkSize;

  while (audioFile.available()) {
    audioFile.read((uint8_t*)header, 4);
    audioFile.read((uint8_t*)&chunkSize, 4);

    if (strncmp(header, "fmt ", 4) == 0) {
      audioFile.read((uint8_t*)&format->audioFormat, 2);
      audioFile.read((uint8_t*)&format->numChannels, 2);
      audioFile.read((uint8_t*)&format->sampleRate, 4);
      audioFile.read((uint8_t*)&format->byteRate, 4);
      audioFile.read((uint8_t*)&format->blockAlign, 2);
      audioFile.read((uint8_t*)&format->bitsPerSample, 2);
  
      Serial.printf("WAV file details:\n");
      Serial.printf("  Audio Format: %d\n", format->audioFormat);
      Serial.printf("  Channels: %d\n", format->numChannels);
      Serial.printf("  Sample Rate: %d Hz\n", format->sampleRate);
      Serial.printf("  Byte Rate: %d\n", format->byteRate);
      Serial.printf("  Block Align: %d\n", format->blockAlign);
      Serial.printf("  Bits per Sample: %d\n", format->bitsPerSample);

      Serial.printf("WAV file details: Sample rate: %u Hz, Channels: %d, Bits per sample: %d\n",
                    format->sampleRate, format->numChannels, format->bitsPerSample);

      if (format->sampleRate != 44100 || (format->numChannels != 1 && format->numChannels != 2) || format->bitsPerSample != 16) {
        Serial.println("WAV file format is not supported. Please use 44.1kHz, 16-bit, mono or stereo.");
        return false;
      }

      return true;
    } else {
      audioFile.seek(audioFile.position() + chunkSize);
    }
  }

  Serial.println("fmt chunk not found");
  return false;
}

// Function to find data chunk
bool findDataChunk(File &audioFile, uint32_t *dataSize) {
  char header[4];
  uint32_t chunkSize;
  
  audioFile.seek(12); // Skip RIFF header

  while (audioFile.available()) {
    audioFile.read((uint8_t*)header, 4);
    audioFile.read((uint8_t*)&chunkSize, 4);

    if (strncmp(header, "data", 4) == 0) {
      *dataSize = chunkSize;
      uint32_t remainingFileSize = audioFile.size() - audioFile.position();
      if (*dataSize > remainingFileSize) {
        Serial.printf("Warning: Data chunk size (%u) larger than remaining file size (%u). Adjusting.\n", *dataSize, remainingFileSize);
        *dataSize = remainingFileSize;
      }
      Serial.printf("Data chunk found. Size: %u bytes\n", *dataSize);
      return true;
    } else {
      audioFile.seek(audioFile.position() + chunkSize);
    }
  }

  Serial.println("Data chunk not found");
  return false;
}

// Function to play audio data
void playAudioData(File &audioFile, WAVFormat &format, uint32_t dataSize) {
  Serial.printf("Starting playback. Data size: %u bytes\n", dataSize);
  Serial.printf("Sample rate: %u Hz, Channels: %d, Bits per sample: %d\n",
                format.sampleRate, format.numChannels, format.bitsPerSample);

  float expectedDuration = (float)dataSize / format.byteRate;
  Serial.printf("Expected duration: %.2f seconds\n", expectedDuration);

  const int bufferSize = 1024; // Smaller buffer for more frequent updates
  uint8_t* buffer = (uint8_t*) malloc(bufferSize);
  if (!buffer) {
    Serial.println("Failed to allocate memory for audio buffer");
    return;
  }

  int totalBytesRead = 0;
  int totalBytesWritten = 0;
  unsigned long startTime = millis();

  while (totalBytesRead < dataSize && audioFile.available()) {
    int bytesToRead = min(dataSize - totalBytesRead, (uint32_t)bufferSize);
    int bytesRead = audioFile.read(buffer, bytesToRead);
    if (bytesRead == 0) {
      Serial.println("Reached end of file");
      break;
    }

    totalBytesRead += bytesRead;

    size_t bytesWritten = 0;
    esp_err_t result = i2s_write(I2S_NUM, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
    if (result != ESP_OK) {
      Serial.printf("Error in i2s_write: %d\n", result);
    }
    totalBytesWritten += bytesWritten;

    if (totalBytesRead % (bufferSize * 10) == 0) {
      Serial.printf("Progress: %d/%u bytes (%.1f%%)\n", 
                    totalBytesRead, dataSize, (float)totalBytesRead/dataSize*100);
    }

    // Add a small delay to prevent overwhelming the I2S driver
    delayMicroseconds(100);
  }

  free(buffer);

  unsigned long endTime = millis();
  float actualDuration = (endTime - startTime) / 1000.0;
  
  Serial.printf("Finished playing audio. Total bytes read: %d, Total bytes written: %d\n", 
                totalBytesRead, totalBytesWritten);
  Serial.printf("Actual playback duration: %.2f seconds\n", actualDuration);
}

void printWAVHeader(const char* filename) {
  File audioFile = SPIFFS.open(filename, "r");
  if (!audioFile) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("File contents (first 44 bytes):");
  for (int i = 0; i < 44; i++) {
    if (audioFile.available()) {
      byte b = audioFile.read();
      Serial.printf("%02X ", b);
      if ((i + 1) % 16 == 0) Serial.println();
    } else {
      Serial.println("End of file reached unexpectedly");
      break;
    }
  }
  Serial.println();

  audioFile.close();
}

void listSPIFFSFiles() {
  Serial.println("Files in SPIFFS:");
  
  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("- Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("- Not a directory");
    return;
  }

  File file = root.openNextFile();
  if (!file) {
    Serial.println("- Empty directory");
  }

  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }

  Serial.println("---------------");
}

void playTestTone() {
  Serial.println("Playing test tone");
  const int duration = 3; // seconds
  const int frequency = 440; // Hz
  const int samplesPerCycle = SAMPLE_RATE / frequency;
  int16_t* samples = (int16_t*)malloc(samplesPerCycle * sizeof(int16_t));

  if (!samples) {
    Serial.println("Failed to allocate memory for samples");
    return;
  }

  // Generate one cycle of sine wave
  for (int i = 0; i < samplesPerCycle; i++) {
    samples[i] = 32767 * sin(2 * PI * i / samplesPerCycle);
  }

  // Play the tone
  for (int i = 0; i < duration * SAMPLE_RATE / samplesPerCycle; i++) {
    size_t bytesWritten = 0;
    esp_err_t result = i2s_write(I2S_NUM, samples, samplesPerCycle * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    if (result != ESP_OK) {
      Serial.printf("Error in i2s_write: %d\n", result);
    }
  }

  free(samples);
  Serial.println("Test tone finished");
}
