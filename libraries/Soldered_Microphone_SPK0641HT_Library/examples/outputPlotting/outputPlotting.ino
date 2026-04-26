/**
 **************************************************
 *
 * @file        outputPlotting.ino
 * 
 * @brief       Example showing how to Initialize a microphone and show the values
 *              sampled from it on the Serial plotter
 *
 * @link        solde.red/333357
 *
 * @authors     Josip Šimun Kuči @ soldered.com
 ***************************************************/
#include "Soldered-Microphone-SPK0641HT.h"

// Pin config (adjust for your wiring)
constexpr int PDM_CLK = 4;
constexpr int PDM_DIN = 19;

Microphone mic;

void setup() {
  Serial.begin(115200);
  
  // Start microphone at sample rate of 16kHz, 16 bit depth and 512 byte 
  // buffer size
  mic.begin(PDM_DIN, PDM_CLK, 16000, 16, 512);
  
  // Optional processing config
  mic.setHPF(true);         // remove DC
  mic.setGainDb(10.0f);      // unity gain
}

void loop() {
  // For plotter visualization, we print a range band
  int range_limit = 3000;
  Serial.print(-range_limit);
  Serial.print(" ");
  Serial.print(range_limit);
  Serial.print(" ");

  // Read samples (blocking)
  int16_t samples[256];
  size_t count = mic.read(samples, 256);

  // Print a few samples (don’t flood serial)
  for (size_t i = 0; i < min(count, (size_t)8); i++) {
    Serial.print(samples[i]);
    Serial.print(" ");
  }
  Serial.println();
}