#include <Arduino.h>
#include "pv_porcupine.h"
#include "params_.h"  // твой wake word

#include "driver/i2s.h"

// ===== I2S (INMP441) конфигурация =====
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14

i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = 16000,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S_MSB,  // только MSB
  .intr_alloc_flags = 0,
  .dma_buf_count = 4,
  .dma_buf_len = 512,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

i2s_pin_config_t pin_config = {
  .bck_io_num = I2S_SCK,
  .ws_io_num = I2S_WS,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_SD
};

#define FRAME_LENGTH 512
int16_t pcm[FRAME_LENGTH];

pv_porcupine_t* porcupine = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Инициализация I2S
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  pv_porcupine_t* porcupine = NULL;  // handle
  float sensitivity[1] = { 0.5f };   // массив float с одной чувствительностью

  pv_status_t status = pv_porcupine_init(
    KEYWORD_ARRAY,         // указатель на байтовую модель wake-word
    KEYWORD_ARRAY_LENGTH,  // размер модели
    sensitivity,           // указатель на float array чувствительности
    1,                     // количество ключевых слов
    &porcupine             // handle
  );

  if (status != PV_STATUS_SUCCESS) {
    Serial.println("Failed to init Porcupine!");
    while (1)
      ;
  }

  Serial.println("Listening for wake word...");
}

void loop() {
  size_t bytesRead;
  i2s_read(I2S_NUM_0, pcm, sizeof(pcm), &bytesRead, portMAX_DELAY);

  int32_t result = 0;
  pv_status_t status = pv_porcupine_process(porcupine, pcm, &result);

  if (status == PV_STATUS_SUCCESS && result > 0) {
    Serial.println("Wake word detected!");
    // Тут можно включать свет, реле, отправлять сигнал и т.д.
  }
}
