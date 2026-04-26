/**
 **************************************************
 *
 * @file        voiceToWAV_RP2350.ino
 * 
 * @brief       Example showing how to Initialize a microphone and save
 *              its output as a WAV file to the SD card
 *
 * @link        solde.red/333357
 *
 * @authors     Josip Šimun Kuči @ soldered.com
 ***************************************************/

#include <SD.h>
#include "Soldered-Microphone-SPK0641HT.h"

// Pin configuration - change these if using different pins
constexpr int PDM_CLK = 4;    // PDM clock pin connected to microphone
constexpr int PDM_DIN = 7;    // PDM data pin connected to microphone
constexpr int SD_CS = 1;      // Chip select pin for SD card module
const int _MISO = 0;          // SPI MISO pin (data from SD card to microcontroller)
const int _MOSI = 3;          // SPI MOSI pin (data from microcontroller to SD card)
const int _SCK = 2;           // SPI clock pin

// Create microphone object - this interfaces with your hardware microphone
Microphone mic;

// Audio recording settings - you can adjust these values
const int sampleRate = 16000;     // Higher = better quality but larger files (8000-44100 typical)
const int numChannels = 1;        // 1 for mono, 2 for stereo (most microphones are mono)
const int bitDepth = 16;          // 16-bit is CD quality, 8-bit uses less space
const int numSamples = 512;       // How many samples to read at once (affects performance)

// File handling variables
File wavFile;                     // File object for our recording
bool hasSDCard = false;           // Tracks if SD card is detected
char currentFilename[15] = "";    // Stores the current filename

/**
 * WAV file header structure
 * This format is standard for WAV files and contains information
 * about the audio like sample rate, bit depth, etc.
 */
struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};          // "RIFF" identifier
  uint32_t chunkSize = 36;                      // File size minus 8 bytes
  char wave[4] = {'W', 'A', 'V', 'E'};          // "WAVE" format
  char fmt[4] = {'f', 'm', 't', ' '};           // "fmt " subchunk
  uint32_t fmtChunkSize = 16;                   // Size of format subchunk
  uint16_t audioFormat = 1;                     // 1 = PCM (uncompressed audio)
  uint16_t numChannels = 1;                     // Number of audio channels
  uint32_t sampleRate = 16000;                  // Samples per second
  uint32_t byteRate = 16000 * 1 * 16 / 8;       // Bytes per second
  uint16_t blockAlign = 1 * 16 / 8;             // Bytes per sample frame
  uint16_t bitsPerSample = 16;                  // Bits per sample
  char data[4] = {'d', 'a', 't', 'a'};          // "data" subchunk
  uint32_t dataChunkSize = 0;                   // Size of audio data
};

WavHeader wavHeader;  // Create a WAV header instance

// Recording state management
enum RecState { IDLE, RECORDING, STOPPING };    // Different recording states
RecState recState = IDLE;                       // Start in idle state
unsigned long recordingStartTime = 0;           // Track when recording started
const unsigned long maxRecordingTime = 30000;   // Maximum 30 second recording

/**
 * Update WAV header with current audio settings
 * This ensures the header matches our recording parameters
 */
void updateWavHeader() {
  wavHeader.sampleRate = sampleRate;
  wavHeader.numChannels = numChannels;
  wavHeader.bitsPerSample = bitDepth;
  wavHeader.byteRate = sampleRate * numChannels * (bitDepth / 8);
  wavHeader.blockAlign = numChannels * (bitDepth / 8);
}

/**
 * Write the WAV header to the file
 * Returns true if successful, false if failed
 */
bool writeWavHeader() {
  if (!wavFile) return false;
  updateWavHeader();
  wavFile.seek(0);
  size_t bytesWritten = wavFile.write((uint8_t*)&wavHeader, sizeof(WavHeader));
  return bytesWritten == sizeof(WavHeader);
}

/**
 * Start a new recording session
 * Creates a new WAV file with a unique name and prepares it for writing
 */
bool startRecording() {
  if (!hasSDCard) {
    Serial.println("No SD card detected! Insert card and restart.");
    return false;
  }

  // Generate a unique filename (REC000.WAV, REC001.WAV, etc.)
  int fileIndex = 0;
  char filename[15];
  do {
    snprintf(filename, sizeof(filename), "REC%03d.WAV", fileIndex++);
  } while (SD.exists(filename) && fileIndex < 1000);

  // Open the file for writing
  wavFile = SD.open(filename, FILE_WRITE);
  if (!wavFile) {
    Serial.println("Failed to create file! Is SD card write-protected?");
    return false;
  }

  // Save filename and reset header values
  strncpy(currentFilename, filename, sizeof(currentFilename));
  wavHeader.dataChunkSize = 0;
  wavHeader.chunkSize = 36;

  // Write the header to file
  if (!writeWavHeader()) {
    wavFile.close();
    Serial.println("Failed to write WAV header!");
    return false;
  }

  // Start recording
  recState = RECORDING;
  recordingStartTime = millis();
  Serial.printf("Recording started: %s\n", filename);
  return true;
}

/**
 * Stop the current recording
 * This begins the process of finalizing the WAV file
 */
void stopRecording() {
  if (recState == RECORDING) {
    recState = STOPPING;
    Serial.println("Stopping recording...");
  }
}

/**
 * Complete the recording process
 * Updates the WAV header with correct file sizes and closes the file
 */
void completeRecording() {
  if (!wavFile) return;

  // Calculate file size and update header
  uint32_t fileSize = wavFile.size();
  wavHeader.dataChunkSize = fileSize - sizeof(WavHeader);
  wavHeader.chunkSize = wavHeader.dataChunkSize + 36;

  // Write updated header and close file
  wavFile.seek(0);
  wavFile.write((uint8_t*)&wavHeader, sizeof(WavHeader));
  wavFile.close();

  Serial.printf("Recording saved: %s (%lu bytes)\n", currentFilename, fileSize);
  recState = IDLE;
}

/**
 * Setup function - runs once at startup
 * Initializes all hardware components and prepares for recording
 */
void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor to connect
  
  // Configure pins for SD card
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  SPI.setRX(_MISO);
  SPI.setTX(_MOSI);
  SPI.setSCK(_SCK);

  Serial.println("\n=== RP2040 Microphone WAV Recorder ===");
  Serial.println("This example records audio to SD card as WAV files");

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    Serial.println("Check: 1) SD card inserted 2) Correct CS pin 3) Card formatted");
    hasSDCard = false;
  } else {
    Serial.println("SD card ready for recording!");
    hasSDCard = true;
  }

  // Initialize microphone with our settings
  Serial.println("Initializing microphone...");
  mic.begin(PDM_DIN, PDM_CLK, sampleRate, bitDepth, numSamples);
  mic.setHPF(true);       // Enable high-pass filter to remove low-frequency noise
  mic.setGainDb(10.0f);   // Apply +10dB gain to make recording louder
  Serial.println("Microphone ready!");

  // Prepare WAV header with our settings
  updateWavHeader();

  // Show user instructions
  Serial.println("\n=== How to Use ===");
  Serial.println("1. Open serial monitor (115200 baud)");
  Serial.println("2. Press 'r' to start recording");
  Serial.println("3. Press 'r' again to stop (or wait 30 seconds)");
  Serial.println("4. Files saved as REC000.WAV, REC001.WAV, etc.");
  Serial.println("5. Play WAV files on computer or phone");
  Serial.println("======================================");
}

/**
 * Main program loop - handles recording, user input, and file management
 * Runs continuously after setup()
 */
void loop() {
  // Check for user commands from serial monitor
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'r' || command == 'R') {
      if (recState == IDLE) {
        startRecording();
      } else if (recState == RECORDING) {
        stopRecording();
      }
    }
  }

  // Handle recording state
  if (recState == RECORDING) {
    // Buffer for audio samples
    int16_t samples[numSamples];
    
    // Read samples from microphone (this function waits for data)
    size_t count = mic.read(samples, numSamples);
    
    if (count > 0) {
      // Write samples to WAV file
      size_t written = wavFile.write((uint8_t*)samples, count * sizeof(int16_t));
      if (written != count * sizeof(int16_t)) {
        Serial.println("Write error! SD card may be full or too slow.");
        stopRecording();
      }
    }

    // Stop automatically after 30 seconds
    if (millis() - recordingStartTime >= maxRecordingTime) {
      Serial.println("30 second time limit reached");
      stopRecording();
    }

  } else if (recState == STOPPING) {
    // Finalize the recording
    completeRecording();
  }

  // Show recording progress every second
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    if (recState == RECORDING) {
      unsigned long elapsed = (millis() - recordingStartTime) / 1000;
      unsigned long remaining = (maxRecordingTime / 1000) - elapsed;
      Serial.printf("Recording: %lus, Remaining: %lus\n", elapsed, remaining);
    }
  }

  // Small delay to prevent overwhelming the system
  delay(10);
}