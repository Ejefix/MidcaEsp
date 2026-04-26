/**
 **************************************************
 *
 * @file        voiceToWAV_ESP32.ino
 * 
 * @brief       Example showing how to Initialize a microphone and save
 *              its output as a WAV file to the SD card
 *
 * @link        solde.red/333357
 *
 * @authors     Josip Šimun Kuči @ soldered.com
 ***************************************************/

#include <Arduino.h>
#include "Soldered-Microphone-SPK0641HT.h"
#include "SdFat.h"  // Advanced SD card library for ESP32

// Pin configuration - adjust these for your specific ESP32 board
constexpr int PDM_CLK = 4;     // PDM clock pin connected to microphone
constexpr int PDM_DIN = 27;    // PDM data pin connected to microphone  
constexpr int SD_CS = 5;       // Chip select pin for SD card module

// SD card configuration - automatically selects best interface
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)  // Use SDIO if available (faster)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(16))  // Dedicated SPI
#else
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(16))  // Shared SPI
#endif

// SD card and file objects using SdFat library
SdFs sd;          // SD card filesystem object
FsFile wavFile;   // File object for WAV recording

// Microphone object - interfaces with your hardware microphone
Microphone mic;

// Audio recording settings - adjust these for different quality levels
const int sampleRate = 32000;     // Higher sample rate = better quality (16000-48000)
const int numChannels = 1;        // 1 channel = mono recording
const int bitDepth = 16;          // 16-bit audio quality
const int numSamples = 512;       // Number of samples to read per batch

/**
 * WAV file header structure
 * Standard format that audio players expect - contains audio metadata
 */
struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};          // "RIFF" file identifier
  uint32_t chunkSize = 36;                      // File size minus 8 bytes
  char wave[4] = {'W', 'A', 'V', 'E'};          // "WAVE" format type
  char fmt[4] = {'f', 'm', 't', ' '};           // "fmt " subchunk
  uint32_t fmtChunkSize = 16;                   // Size of format data (16 for PCM)
  uint16_t audioFormat = 1;                     // 1 = PCM (uncompressed audio)
  uint16_t numChannels = 1;                     // Number of audio channels
  uint32_t sampleRate = 32000;                  // Samples per second
  uint32_t byteRate = 128000;                   // Bytes per second
  uint16_t blockAlign = 4;                      // Bytes per sample frame
  uint16_t bitsPerSample = 32;                  // Bits per sample
  char data[4] = {'d', 'a', 't', 'a'};          // "data" subchunk identifier
  uint32_t dataChunkSize = 0;                   // Size of audio data (updated later)
};

WavHeader wavHeader;  // Create WAV header instance

// Recording state management
enum RecState { IDLE, RECORDING, STOPPING };    // Different recording states
RecState recState = IDLE;                       // Start in idle mode
bool hasSDCard = false;                         // SD card detection flag
unsigned long recordingStartTime = 0;           // Track recording start time
const unsigned long maxRecordingTime = 30000;   // Maximum 30 second recording
char currentFilename[15] = "";                  // Current recording filename

/**
 * Update WAV header with current audio settings
 * Ensures the header matches our actual recording parameters
 */
void updateWavHeader() {
  wavHeader.sampleRate = sampleRate;
  wavHeader.numChannels = numChannels;
  wavHeader.bitsPerSample = bitDepth;
  wavHeader.byteRate = sampleRate * numChannels * (bitDepth / 8);
  wavHeader.blockAlign = numChannels * (bitDepth / 8);
}

/**
 * Write WAV header to the file
 * Returns true if successful, false if failed
 */
bool writeWavHeader() {
  if (!wavFile.isOpen()) return false;
  
  updateWavHeader();
  if (!wavFile.seek(0)) {
    Serial.println("Error seeking to file start!");
    return false;
  }
  
  size_t bytesWritten = wavFile.write((uint8_t *)&wavHeader, sizeof(WavHeader));
  if (bytesWritten != sizeof(WavHeader)) {
    Serial.println("Error writing WAV header!");
    return false;
  }
  
  return true;
}

/**
 * Start a new recording session
 * Creates a new WAV file with unique name and prepares it for writing
 */
bool startRecording() {
  if (!hasSDCard) {
    Serial.println("No SD card detected! Insert card and restart.");
    return false;
  }
  
  // Generate unique filename (REC000.WAV, REC001.WAV, etc.)
  int fileIndex = 0;
  char filename[15];
  
  do {
    snprintf(filename, sizeof(filename), "REC%03d.WAV", fileIndex++);
  } while (sd.exists(filename) && fileIndex < 1000);
  
  // Open file for writing (create new file, overwrite if exists)
  if (!wavFile.open(filename, O_RDWR | O_CREAT | O_TRUNC)) {
    Serial.println("Error creating file! Check SD card permissions.");
    return false;
  }
  
  strncpy(currentFilename, filename, sizeof(currentFilename));
  
  // Initialize header values
  wavHeader.dataChunkSize = 0;
  wavHeader.chunkSize = 36;
  
  // Write initial header
  if (!writeWavHeader()) {
    wavFile.close();
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
 * Begins the process of finalizing the WAV file
 */
void stopRecording() {
  if (recState != RECORDING) return;
  
  recState = STOPPING;
  Serial.println("Stopping recording...");
}

/**
 * Complete the recording process
 * Updates WAV header with correct file sizes and closes the file
 */
void completeRecording() {
  if (!wavFile.isOpen()) return;
  
  // Get current file position (which equals file size)
  uint32_t fileSize = wavFile.position();
  wavHeader.dataChunkSize = fileSize - sizeof(WavHeader);
  wavHeader.chunkSize = wavHeader.dataChunkSize + 36;
  
  // Write updated header with correct sizes
  if (!writeWavHeader()) {
    Serial.println("Error updating WAV header!");
  }
  
  // Close file
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
  
  Serial.println("\n=== ESP32 Microphone WAV Recorder ===");
  Serial.println("This example records audio to SD card as WAV files");
  
  // Initialize SD card using SdFat library
  Serial.println("Initializing SD card...");
  
  if (!sd.cardBegin(SD_CONFIG)) {
    Serial.println("SD card initialization failed!");
    Serial.println("Check: 1) SD card inserted 2) Correct CS pin 3) Proper wiring");
    hasSDCard = false;
  } else {
    // Initialize filesystem volume
    if (!sd.volumeBegin()) {
      Serial.println("Failed to initialize filesystem!");
      Serial.println("Try reformatting SD card as FAT32");
      hasSDCard = false;
    } else {
      hasSDCard = true;
      Serial.println("SD Card ready for recording!");
    }
  }
  
  // Initialize microphone with our settings
  Serial.println("Initializing microphone...");
  mic.begin(PDM_DIN, PDM_CLK, sampleRate, 16, 512);
  mic.setHPF(true);       // Enable high-pass filter to remove low-frequency noise
  mic.setGainDb(10.0f);   // Apply +10dB gain for better recording volume
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
  Serial.println("================================");
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
    
    // Read samples from microphone
    size_t count = mic.read(samples, numSamples);
    
    // Write to file if we have data
    if (count > 0) {
      size_t bytesWritten = wavFile.write((uint8_t *)samples, count * sizeof(int16_t));
      
      if (bytesWritten != count * sizeof(int16_t)) {
        Serial.println("Write error! SD card may be full or too slow.");
        stopRecording();
      }
    }
    
    // Check for maximum recording time
    if (millis() - recordingStartTime >= maxRecordingTime) {
      Serial.println("30 second time limit reached");
      stopRecording();
    }

  } else if (recState == STOPPING) {
    // Finalize the recording
    completeRecording();
  }
  
  // Show recording progress every second
  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate > 1000) {
    lastStatusUpdate = millis();
    
    if (recState == RECORDING) {
      unsigned long elapsed = (millis() - recordingStartTime) / 1000;
      unsigned long remaining = (maxRecordingTime / 1000) - elapsed;
      Serial.printf("Recording: %lu sec, Remaining: %lu sec\n", elapsed, remaining);
    }
  }
  
  // Small delay to prevent overwhelming the system
  delay(10);
}