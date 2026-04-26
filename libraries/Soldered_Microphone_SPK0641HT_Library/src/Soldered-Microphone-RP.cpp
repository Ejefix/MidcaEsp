/**
 **************************************************
 *
 * @file        Soldered-Microphone-RP.cpp
 * @brief       RP2350 adaptation of Microphone class using PDM
 *
 * @copyright   GNU GPL v3.0
 ***************************************************/

#include "Soldered-Microphone-SPK0641HT.h"

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)

#include <PDM.h>

// Static variables for PDM audio processing
static volatile bool pdmAvailable = false; ///< Flag indicating PDM data is available
static volatile int pdmReadPosition = 0;   ///< Current read position in circular buffer
static volatile int pdmWritePosition = 0;  ///< Current write position in circular buffer
static const int BUFFER_SIZE = 1024;       ///< Size of the audio buffer
static int16_t audioBuffer[BUFFER_SIZE];   ///< Circular buffer for audio data

/**
 * @brief Callback function for PDM data reception
 *
 * This function is called by the PDM library when audio data is available.
 * It reads the data into a temporary buffer and transfers it to the circular buffer.
 */
static void onPDMData()
{
    int bytesAvailable = PDM.available();
    if (bytesAvailable > 0)
    {
        int samplesAvailable = bytesAvailable / sizeof(int16_t);
        int16_t tempBuffer[samplesAvailable];

        // Read data into temporary buffer
        PDM.read(tempBuffer, bytesAvailable);

        // Copy to circular buffer
        for (int i = 0; i < samplesAvailable; i++)
        {
            audioBuffer[pdmWritePosition] = tempBuffer[i];
            pdmWritePosition = (pdmWritePosition + 1) % BUFFER_SIZE;
        }

        pdmAvailable = true;
    }
}

/**
 * @brief Initialize the PDM microphone
 *
 * @param dataPin Pin number for PDM data
 * @param clockPin Pin number for PDM clock
 * @param sampleRate Desired sample rate in Hz
 * @param bitDepth Bit depth (currently fixed at 16-bit)
 * @param bufferSize Size of the PDM buffer in samples
 */
void Microphone::begin(uint8_t dataPin, uint8_t clockPin, int sampleRate, uint8_t bitDepth, uint16_t bufferSize)
{
    Serial.println("Setting up custom PDM microphone...");

    // Set pin configuration
    PDM.setDIN(dataPin);
    PDM.setCLK(clockPin);

    // Configure software state
    currentSampleRate = sampleRate;
    currentBitDepth = 16;
    softwareGain = 1.0f;
    hpfEnabled = false;
    hpfState = 0.0f;
    prevSample = 0.0f;

    // Clear buffer
    memset(audioBuffer, 0, sizeof(audioBuffer));
    pdmReadPosition = 0;
    pdmWritePosition = 0;

    // Set up the receive callback
    PDM.onReceive(onPDMData);

    // Set buffer size (in bytes)
    PDM.setBufferSize(bufferSize * 2); // 16-bit = 2 bytes per sample

    // Start PDM (mono only)
    if (!PDM.begin(1, sampleRate))
    {
        Serial.println("ERROR: PDM.begin() failed");
        while (1)
            ; // lock up
    }

    // Add a small delay to allow PDM to stabilize
    delay(100);

    Serial.println("PDM microphone initialized successfully.");
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
    unsigned long startTime = millis();
    size_t samplesRead = 0;

    while (samplesRead < samples && (millis() - startTime) < 1000)
    {
        if (pdmAvailable)
        {
            int availableSamples = (pdmWritePosition - pdmReadPosition + BUFFER_SIZE) % BUFFER_SIZE;
            if (availableSamples == 0)
            {
                pdmAvailable = false;
                continue;
            }

            int samplesToRead = min((size_t)availableSamples, samples - samplesRead);

            for (int i = 0; i < samplesToRead; i++)
            {
                dst[samplesRead + i] = audioBuffer[pdmReadPosition];
                pdmReadPosition = (pdmReadPosition + 1) % BUFFER_SIZE;
            }

            samplesRead += samplesToRead;
        }
    }

    processAudio(dst, samplesRead);
    return samplesRead;
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
    if (!pdmAvailable)
        return 0;

    int availableSamples = (pdmWritePosition - pdmReadPosition + BUFFER_SIZE) % BUFFER_SIZE;
    if (availableSamples == 0)
    {
        pdmAvailable = false;
        return 0;
    }

    int samplesToRead = min((size_t)availableSamples, samples);

    for (int i = 0; i < samplesToRead; i++)
    {
        dst[i] = audioBuffer[pdmReadPosition];
        pdmReadPosition = (pdmReadPosition + 1) % BUFFER_SIZE;
    }

    if (pdmReadPosition == pdmWritePosition)
    {
        pdmAvailable = false;
    }

    processAudio(dst, samplesToRead);
    return samplesToRead;
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
    // Don't change if already set
    if (sr == currentSampleRate)
        return;

    PDM.end();
    delay(50);

    if (!PDM.begin(1, sr))
    {
        Serial.println("ERROR: PDM re-initialization failed");
        while (1)
            ;
    }

    currentSampleRate = sr;
    delay(100); // Allow PDM to stabilize
}

/**
 * @brief Enable or disable the high-pass filter
 *
 * @param enable True to enable HPF, false to disable
 */
void Microphone::setHPF(bool enable)
{
    hpfEnabled = enable;
    hpfState = 0.0f;
    prevSample = 0.0f;
}

/**
 * @brief Set the software gain
 *
 * @param gain Gain value (limited to 0.1-10.0 range)
 */
void Microphone::setGain(float gain)
{
    // Limit gain to prevent excessive clipping
    softwareGain = constrain(gain, 0.1f, 10.0f);
}

// --------------------- DSP / Utils -------------------------

float Microphone::rms(const int16_t *data, size_t samples)
{
    double sum = 0;
    for (size_t i = 0; i < samples; i++)
    {
        sum += (double)data[i] * data[i];
    }
    return sqrt(sum / samples) / 32768.0f; // normalize
}

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

float Microphone::dbfs(const int16_t *data, size_t samples)
{
    float r = rms(data, samples);
    return 20.0f * log10f(r + 1e-9f); // avoid log(0)
}

// --------------------- Internal -------------------------

/**
 * @brief Process audio data with DSP effects
 *
 * Applies DC offset removal, high-pass filtering, gain, and clipping
 *
 * @param data Pointer to audio data
 * @param samples Number of samples to process
 */
void Microphone::processAudio(int16_t *data, size_t samples)
{
    if (samples == 0)
        return;

    // First, remove DC offset by calculating mean
    long sum = 0;
    for (size_t i = 0; i < samples; i++)
    {
        sum += data[i];
    }
    int16_t dcOffset = sum / samples;

    for (size_t i = 0; i < samples; i++)
    {
        float x = (float)(data[i] - dcOffset);

        // Improved HPF (first-order high-pass)
        if (hpfEnabled)
        {
            float y = 0.95f * (hpfState + x - prevSample);
            prevSample = x;
            hpfState = y;
            x = y;
        }

        // Apply gain with careful clipping
        x *= softwareGain;

        // Soft clipping to reduce harsh distortion
        if (x > 32767.0f)
            x = 32767.0f - (x - 32767.0f) * 0.5f;
        if (x < -32768.0f)
            x = -32768.0f - (x + 32768.0f) * 0.5f;

        // Final hard clip for safety
        x = constrain(x, -32768.0f, 32767.0f);

        data[i] = (int16_t)x;
    }
}

#endif