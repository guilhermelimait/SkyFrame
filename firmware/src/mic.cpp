// Room loudness from the board's microphone: ES7210 audio chip (Espressif's driver, es7210.cpp) and the ESP32's
// I2S input. A small task reads 32 ms blocks at 16 kHz and keeps one smoothed loudness value per channel.
// Pins from Waveshare's board definition: MCLK 5, BCLK 16, LRCK 7, data in 15.
#include "mic.h"
#include <Wire.h>
#include "driver/i2s_std.h"
#include "es7210.h"

static i2s_chan_handle_t rx = nullptr;
static TaskHandle_t task = nullptr;
static volatile bool running = false;
static volatile float level[2] = {-90, -90};
static volatile float floorDb = 0;   // 0 = not learned yet

static void micTask(void *) {
  static int16_t buf[512 * 2];   // 512 stereo frames
  uint32_t blocks = 0;
  while (running) {
    size_t got = 0;
    if (i2s_channel_read(rx, buf, sizeof buf, &got, pdMS_TO_TICKS(200)) != ESP_OK || got < sizeof buf) continue;
    for (int ch = 0; ch < 2; ch++) {
      int32_t mean = 0;
      for (int i = ch; i < 1024; i += 2) mean += buf[i];
      mean /= 512;
      double sum = 0;
      for (int i = ch; i < 1024; i += 2) { double v = buf[i] - mean; sum += v * v; }
      float db = 10 * log10f((float)(sum / 512) / (32768.f * 32768.f) + 1e-12f);
      // rise fast (a sound is noticed at once), fall slowly (quiet has to last)
      level[ch] = db > level[ch] ? db : level[ch] * 0.9f + db * 0.1f;
    }
    // background noise: follows quiet at once, rises only slowly (a sound has to last ~15 s to count as background)
    // (the first second is the chip warming up, near silence: skip it; then settle fast for 3 s, then slowly)
    float l = max(level[0], level[1]);
    if (++blocks <= 31) continue;
    if (floorDb == 0 || l < floorDb) floorDb = l;
    else floorDb = floorDb + (l - floorDb) * (blocks < 125 ? 0.1f : 0.002f);
  }
  task = nullptr;
  vTaskDelete(nullptr);
}

bool micStart() {
  if (running) return true;
  audio_hal_codec_config_t cfg = {};
  cfg.adc_input = AUDIO_HAL_ADC_INPUT_ALL;
  cfg.codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE;
  cfg.i2s_iface.mode = AUDIO_HAL_MODE_SLAVE;
  cfg.i2s_iface.fmt = AUDIO_HAL_I2S_NORMAL;
  cfg.i2s_iface.samples = AUDIO_HAL_16K_SAMPLES;
  cfg.i2s_iface.bits = AUDIO_HAL_BIT_LENGTH_16BITS;
  esp_err_t e = es7210_adc_init(&Wire, &cfg);
  e |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
  e |= es7210_adc_set_gain((es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2), GAIN_30DB);
  e |= es7210_adc_set_gain((es7210_input_mics_t)(ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4), GAIN_30DB);
  e |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);
  if (e != ESP_OK) return false;

  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
  if (i2s_new_channel(&cc, nullptr, &rx) != ESP_OK) return false;
  i2s_std_config_t sc = {};
  sc.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
  sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  sc.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  sc.gpio_cfg.mclk = GPIO_NUM_5;
  sc.gpio_cfg.bclk = GPIO_NUM_16;
  sc.gpio_cfg.ws = GPIO_NUM_7;
  sc.gpio_cfg.dout = I2S_GPIO_UNUSED;
  sc.gpio_cfg.din = GPIO_NUM_15;
  if (i2s_channel_init_std_mode(rx, &sc) != ESP_OK || i2s_channel_enable(rx) != ESP_OK) {
    i2s_del_channel(rx); rx = nullptr;
    return false;
  }
  running = true;
  xTaskCreatePinnedToCore(micTask, "mic", 4096, nullptr, 1, &task, 0);
  return true;
}

void micStop() {
  if (!running) return;
  running = false;
  while (task) delay(10);
  i2s_channel_disable(rx);
  i2s_del_channel(rx);
  rx = nullptr;
  es7210_adc_ctrl_state(AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_STOP);
  level[0] = -90;
  level[1] = -90;
  floorDb = 0;
}

bool micRunning() { return running; }
float micFloor() { return floorDb ? floorDb : -60; }
float micMargin(int sense) { return sense >= 2 ? 6 : sense == 1 ? 10 : 16; }
bool micHears(int sense) { return running && floorDb != 0 && micLevel() > floorDb + micMargin(sense); }
float micLevel(int ch) { return ch >= 0 ? level[ch & 1] : max(level[0], level[1]); }
