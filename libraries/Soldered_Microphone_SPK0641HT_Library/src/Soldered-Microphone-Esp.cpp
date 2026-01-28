/**
 **************************************************
 *
 * @file        Soldered-Microphone-Esp.cpp
 * @brief       Example functions to overload in base class.
 *
 *
 * @copyright GNU General Public License v3.0
 * @authors     @ soldered.com
 ***************************************************/

#include "Soldered-Microphone-SPK0641HT.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

/**
 * @brief Initialize the PDM microphone for ESP32
 *
 * @param dataPin Pin number for PDM data
 * @param clockPin Pin number for PDM clock
 * @param sampleRate Desired sample rate in Hz
 * @param bitDepth Bit depth (8, 16, 24, or 32 bits)
 * @param bufferSize Size of the PDM buffer (not used in this implementation)
 */
void Microphone::begin(uint8_t dataPin, uint8_t clockPin, int sampleRate, uint8_t bitDepth, uint16_t bufferSize)
{
    // Allocate an RX channel
    chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    // Convert bit depth to I2S format
    switch (bitDepth)
    {
    case 8:
        currentBitDepth = I2S_DATA_BIT_WIDTH_8BIT;
        break;
    case 16:
        currentBitDepth = I2S_DATA_BIT_WIDTH_16BIT;
        break;
    case 24:
        currentBitDepth = I2S_DATA_BIT_WIDTH_24BIT;
        break;
    case 32:
        currentBitDepth = I2S_DATA_BIT_WIDTH_32BIT;
        break;
    }

    // Build the PDM RX configuration
    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(sampleRate),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(currentBitDepth, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {.clk = (gpio_num_t)clockPin, .din = (gpio_num_t)dataPin, .invert_flags = {.clk_inv = false}},
    };

    // Initialize and enable the PDM RX channel
    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    // Set initial state variables
    currentSampleRate = sampleRate;
    softwareGain = 1.0f;
    hpfEnabled = false;
    hpfState = 0.0f;
}

// --------------------- Data Access -------------------------


/**
 * @brief Read audio samples in blocking mode
 *
 * @param dst Destination buffer for audio samples
 * @param samples Number of samples to read
 * @return size_t Number of samples actually read
 */
size_t Microphone::read(int16_t *dst, size_t samples)
{
    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(rx_handle, dst, samples * sizeof(int16_t), &bytes_read,
                                     portMAX_DELAY); // blocking
    size_t gotSamples = bytes_read / sizeof(int16_t);

    processAudio(dst, gotSamples);
    return gotSamples;
}

/**
 * @brief Read audio samples in non-blocking mode
 *
 * @param dst Destination buffer for audio samples
 * @param samples Number of samples to read
 * @return size_t Number of samples actually read
 */
size_t Microphone::readNonBlocking(int16_t *dst, size_t samples)
{
    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(rx_handle, dst, samples * sizeof(int16_t), &bytes_read,
                                     0); // no wait
    if (err != ESP_OK)
        return 0;

    size_t gotSamples = bytes_read / sizeof(int16_t);
    processAudio(dst, gotSamples);
    return gotSamples;
}

// --------------------- Controls -------------------------

/**
 * @brief Get the current sample rate
 *
 * @return int Current sample rate in Hz
 */
int Microphone::sampleRate() const
{
    return currentSampleRate;
}

/**
 * @brief Set the sample rate
 *
 * @param sr New sample rate in Hz
 */
void Microphone::setSampleRate(int sr)
{
    // Reconfigure I²S with new sample rate (if valid)
    // You may want to allow only enum values: 8k, 16k, 44.1k, 48k etc.
    ESP_ERROR_CHECK(i2s_channel_disable(rx_handle));

    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(sr),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(currentBitDepth, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {/* reuse stored pins */},
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    currentSampleRate = sr;
}

/**
 * @brief Enable or disable the high-pass filter
 *
 * @param enable True to enable HPF, false to disable
 */
void Microphone::setHPF(bool enable)
{
    hpfEnabled = enable;
    hpfState = 0.0f; // reset
}

/**
 * @brief Set the software gain
 *
 * @param gain Gain value
 */
void Microphone::setGain(float gain)
{
    softwareGain = gain;
}

// --------------------- DSP / Utils -------------------------

/**
 * @brief Calculate RMS (Root Mean Square) value of audio data
 *
 * @param data Pointer to audio data
 * @param samples Number of samples to process
 * @return float RMS value normalized to 0.0-1.0 range
 */
float Microphone::rms(const int16_t *data, size_t samples)
{
    double sum = 0;
    for (size_t i = 0; i < samples; i++)
    {
        sum += (double)data[i] * data[i];
    }
    return sqrt(sum / samples) / 32768.0f; // normalize
}

/**
 * @brief Find the peak amplitude in audio data
 *
 * @param data Pointer to audio data
 * @param samples Number of samples to process
 * @return int16_t Peak amplitude value
 */
int16_t Microphone::peak(const int16_t *data, size_t samples)
{
    int16_t peakVal = 0;
    for (size_t i = 0; i < samples; i++)
    {
        int16_t v = abs(data[i]);
        if (v > peakVal)
            peakVal = v;
    }
    return peakVal;
}

/**
 * @brief Calculate dBFS (decibels relative to full scale)
 *
 * @param data Pointer to audio data
 * @param samples Number of samples to process
 * @return float dBFS value
 */
float Microphone::dbfs(const int16_t *data, size_t samples)
{
    float r = rms(data, samples);
    return 20.0f * log10f(r + 1e-9f); // avoid log(0)
}

// --------------------- Internal -------------------------

/**
 * @brief Process audio data with DSP effects
 *
 * Applies high-pass filtering and gain
 *
 * @param data Pointer to audio data
 * @param samples Number of samples to process
 */
void Microphone::processAudio(int16_t *data, size_t samples)
{
    for (size_t i = 0; i < samples; i++)
    {
        float x = (float)data[i];

        // HPF (one-pole DC removal)
        if (hpfEnabled)
        {
            float y = x - hpfState + 0.995f * prevSample;
            hpfState = x;
            prevSample = y;
            x = y;
        }

        // Apply gain
        x *= softwareGain;

        // Clip back to int16
        if (x > 32767.0f)
            x = 32767.0f;
        if (x < -32768.0f)
            x = -32768.0f;
        data[i] = (int16_t)x;
    }
}

#endif