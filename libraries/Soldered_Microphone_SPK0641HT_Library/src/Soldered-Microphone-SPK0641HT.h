/**
 **************************************************
 *
 * @file        Soldered-Microphone-SPK0641HT.h
 * @brief       Header file for sensor specific code.
 *
 *
 * @copyright GNU General Public License v3.0
 * @authors     Josip Šimun Kuči @ soldered.com
 ***************************************************/

#ifndef _SOLDERED_MICROPHONE_
#define _SOLDERED_MICROPHONE_

#include "Arduino.h"

// Platform-specific includes
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include "driver/gpio.h"
#include "driver/i2s_common.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#endif

/**
 * @class Microphone
 * @brief Class for interfacing with PDM microphones
 *
 * This class provides a unified interface for working with PDM microphones
 * across different hardware platforms (ESP32, RP2040, RP2350).
 */
class Microphone
{
  public:
    /**
     * @brief Initialize the PDM microphone
     *
     * @param dataPin Pin number for PDM data input
     * @param clockPin Pin number for PDM clock output
     * @param sampleRate Sample rate in Hz (default: 16000)
     * @param bitDepth Bit depth (8, 16, 24, or 32 bits, default: 16)
     * @param bufferSize Size of the internal buffer in samples (default: 512)
     */
    void begin(uint8_t dataPin, uint8_t clockPin, int sampleRate = 16000, uint8_t bitDepth = 16,
               uint16_t bufferSize = 512);

    // ---- Data access ----

    /**
     * @brief Read audio samples in blocking mode
     *
     * @param dst Destination buffer for audio samples
     * @param samples Number of samples to read
     * @return size_t Number of samples actually read
     */
    size_t read(int16_t *dst, size_t samples); // blocking

    /**
     * @brief Read audio samples in non-blocking mode
     *
     * @param dst Destination buffer for audio samples
     * @param samples Number of samples to read
     * @return size_t Number of samples actually read (0 if no data available)
     */
    size_t readNonBlocking(int16_t *dst, size_t samples); // immediate

    // ---- Controls ----

    /**
     * @brief Get the current sample rate
     *
     * @return int Current sample rate in Hz
     */
    int sampleRate() const;

    /**
     * @brief Set the sample rate
     *
     * @param sr New sample rate in Hz
     */
    void setSampleRate(int sr);

    /**
     * @brief Enable or disable the high-pass filter
     *
     * @param enable True to enable HPF, false to disable
     */
    void setHPF(bool enable);

    /**
     * @brief Set the software gain (linear scale)
     *
     * @param gain Gain value (1.0 = no gain)
     */
    void setGain(float gain);

    /**
     * @brief Set the software gain in decibels
     *
     * @param db Gain value in decibels (0dB = no gain)
     */
    void setGainDb(float db)
    {
        setGain(powf(10.0f, db / 20.0f));
    }

    // ---- DSP Utils ----

    /**
     * @brief Calculate RMS (Root Mean Square) value of audio data
     *
     * @param data Pointer to audio data
     * @param samples Number of samples to process
     * @return float RMS value normalized to 0.0-1.0 range
     */
    float rms(const int16_t *data, size_t samples);

    /**
     * @brief Find the peak amplitude in audio data
     *
     * @param data Pointer to audio data
     * @param samples Number of samples to process
     * @return int16_t Peak amplitude value
     */
    int16_t peak(const int16_t *data, size_t samples);

    /**
     * @brief Calculate dBFS (decibels relative to full scale)
     *
     * @param data Pointer to audio data
     * @param samples Number of samples to process
     * @return float dBFS value
     */
    float dbfs(const int16_t *data, size_t samples);

  private:
    // ---- Internal helpers ----

    /**
     * @brief Process audio data with DSP effects
     *
     * Applies high-pass filtering, gain, and clipping
     *
     * @param data Pointer to audio data
     * @param samples Number of samples to process
     */
    void processAudio(int16_t *data, size_t samples);

    // ---- State ----

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    i2s_chan_config_t chan_cfg;                                      ///< I2S channel configuration for ESP32
    i2s_chan_handle_t rx_handle = nullptr;                           ///< I2S channel handle for ESP32
    i2s_data_bit_width_t currentBitDepth = I2S_DATA_BIT_WIDTH_16BIT; ///< Current bit depth for ESP32
#else
    uint8_t currentBitDepth;       ///< Current bit depth (8, 16, 24, or 32 bits)
#endif

    int currentSampleRate = 16000; ///< Current sample rate in Hz
    float softwareGain = 1.0f;     ///< Current software gain (linear scale)

    bool hpfEnabled = false; ///< High-pass filter enabled state
    float hpfState = 0.0f;   ///< High-pass filter internal state
    float prevSample = 0.0f; ///< Previous sample for filter processing
};

#endif

