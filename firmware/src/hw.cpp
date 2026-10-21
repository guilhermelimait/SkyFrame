// Screen (ST7701S over RGB), backlight and GT911 touch for the Waveshare ESP32-S3-Touch-LCD-4B
#include "hw.h"
#include <Wire.h>
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include <atomic>

#define PIN_BL 4
#define PIN_SDA 47
#define PIN_SCL 48
#define EXP_ADDR 0x20   // TCA9554 IO expander
#define EX_LCD_CS 0
#define EX_LCD_SDA 1
#define EX_LCD_SCK 2

/* ---------- IO expander + 3-wire SPI (init commands only) ---------- */
static uint8_t exOut = 0xFF;
static void exWrite(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(EXP_ADDR);
  Wire.write(reg);
  Wire.write(v);
  Wire.endTransmission();
}
static void exPin(uint8_t bit, bool high) {
  if (high) exOut |= (1 << bit); else exOut &= ~(1 << bit);
  exWrite(0x01, exOut);
}
static void spi9(bool data, uint8_t v) {
  uint16_t w = (data ? 0x100 : 0) | v;
  for (int i = 8; i >= 0; i--) {
    exOut &= ~(1 << EX_LCD_SCK);
    if ((w >> i) & 1) exOut |= (1 << EX_LCD_SDA); else exOut &= ~(1 << EX_LCD_SDA);
    exWrite(0x01, exOut);
    exOut |= (1 << EX_LCD_SCK);
    exWrite(0x01, exOut);
  }
}
static void lcdCmd(uint8_t cmd, const uint8_t *d, uint8_t n) {
  exPin(EX_LCD_CS, false);
  spi9(false, cmd);
  for (uint8_t i = 0; i < n; i++) spi9(true, d[i]);
  exOut &= ~(1 << EX_LCD_SCK);
  exPin(EX_LCD_CS, true);
}

struct InitCmd { uint8_t cmd, len; uint8_t d[16]; uint16_t delayMs; };
// ST7701S sequence from ESPHome's mipi_rgb "ST7701S" model, as used for this board
// Waveshare's official sequence for this board (st7701_type1_init_operations in their Arduino examples)
static const InitCmd INIT[] = {
  {0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x10}},
  {0xC0, 2, {0x3B, 0x00}}, {0xC1, 2, {0x0D, 0x02}}, {0xC2, 2, {0x31, 0x05}}, {0xCD, 1, {0x08}},
  {0xB0, 16, {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}},
  {0xB1, 16, {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}},
  {0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x11}},
  {0xB0, 1, {0x60}}, {0xB1, 1, {0x32}}, {0xB2, 1, {0x07}}, {0xB3, 1, {0x80}}, {0xB5, 1, {0x49}},
  {0xB7, 1, {0x85}}, {0xB8, 1, {0x21}}, {0xC1, 1, {0x78}}, {0xC2, 1, {0x78}},
  {0xE0, 3, {0x00, 0x1B, 0x02}},
  {0xE1, 11, {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}},
  {0xE2, 12, {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}},
  {0xE3, 4, {0x00, 0x00, 0x11, 0x11}}, {0xE4, 2, {0x44, 0x44}},
  {0xE5, 16, {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}},
  {0xE6, 4, {0x00, 0x00, 0x11, 0x11}}, {0xE7, 2, {0x44, 0x44}},
  {0xE8, 16, {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}},
  {0xEB, 7, {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}}, {0xEC, 2, {0x3C, 0x00}},
  {0xED, 16, {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}},
  {0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x13}}, {0xE5, 1, {0xE4}},
  {0xFF, 5, {0x77, 0x01, 0x00, 0x00, 0x00}},   // back to the normal command page
  {0x21, 0, {}},                               // colour inversion (IPS panel)
  {0x3A, 1, {0x60}},                           // 18-bit interface
  {0x11, 0, {}, 120}, {0x29, 0, {}, 20},
};

/* ---------- RGB panel with two frame buffers ---------- */
static esp_lcd_panel_handle_t panel = nullptr;
static uint16_t *fb[2];
static int front = 0;
static SemaphoreHandle_t vsyncSem;

static bool IRAM_ATTR onFrameDone(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *) {
  BaseType_t woken = pdFALSE;
  xSemaphoreGiveFromISR(vsyncSem, &woken);
  return woken == pdTRUE;
}

static void panelInit() {
  esp_lcd_rgb_panel_config_t c = {};
  c.clk_src = LCD_CLK_SRC_DEFAULT;
  // timing from Waveshare's official example for this board (12 MHz, ~42 frames per second)
  c.timings.pclk_hz = 12 * 1000 * 1000;
  c.timings.h_res = LCD_W;
  c.timings.v_res = LCD_H;
  c.timings.hsync_pulse_width = 8;
  c.timings.hsync_back_porch = 50;
  c.timings.hsync_front_porch = 10;
  c.timings.vsync_pulse_width = 8;
  c.timings.vsync_back_porch = 20;
  c.timings.vsync_front_porch = 10;
  c.data_width = 16;
  c.bits_per_pixel = 16;
  c.num_fbs = 2;
  c.bounce_buffer_size_px = LCD_W * 30;
  c.dma_burst_size = 64;
  c.hsync_gpio_num = 46;
  c.vsync_gpio_num = 3;
  c.de_gpio_num = 17;
  c.pclk_gpio_num = 9;
  c.disp_gpio_num = -1;
  const int pins[16] = {40, 41, 42, 2, 1, 21, 8, 18, 45, 38, 39, 10, 11, 12, 13, 14};  // B0-4, G0-5, R0-4
  for (int i = 0; i < 16; i++) c.data_gpio_nums[i] = pins[i];
  c.flags.fb_in_psram = 1;
  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&c, &panel));
  vsyncSem = xSemaphoreCreateBinary();
  esp_lcd_rgb_panel_event_callbacks_t cbs = {};
  cbs.on_frame_buf_complete = onFrameDone;
  esp_lcd_rgb_panel_register_event_callbacks(panel, &cbs, nullptr);
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
  void *a, *b;
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel, 2, &a, &b));
  fb[0] = (uint16_t *)a;
  fb[1] = (uint16_t *)b;
  memset(fb[0], 0, LCD_W * LCD_H * 2);
  memset(fb[1], 0, LCD_W * LCD_H * 2);
}

uint16_t *hwBack() { return fb[front ^ 1]; }
uint16_t *hwFront() { return fb[front]; }

void hwPresent() {
  xSemaphoreTake(vsyncSem, 0);
  esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_W, LCD_H, fb[front ^ 1]);
  front ^= 1;
  xSemaphoreTake(vsyncSem, pdMS_TO_TICKS(40));   // let the swap happen before drawing into the old buffer
  xSemaphoreTake(vsyncSem, pdMS_TO_TICKS(40));
}

/* ---------- backlight (active-low PWM) ---------- */
static int blLevel = 0;
static std::atomic<int> flashDepth{0};
void hwBacklight(int pct) {
  blLevel = constrain(pct, 0, 100);
  if (!flashDepth) ledcWrite(PIN_BL, 1023 - (1023 * blLevel) / 100);
}

/* ---------- flash writes ---------- */
int hwResync() { return panel ? (int)esp_lcd_rgb_panel_restart(panel) : -1; }
void hwFlashBegin() {
  if (flashDepth++ == 0) ledcWrite(PIN_BL, 1023);   // dark while the picture may be out of step
}
void hwFlashEnd() {
  if (--flashDepth > 0) return;
  hwResync();
  delay(60);                                         // a couple of clean frames before the light comes back
  if (!flashDepth) ledcWrite(PIN_BL, 1023 - (1023 * blLevel) / 100);
}

/* ---------- GT911 ---------- */
static uint8_t gtAddr = 0;
static bool gtRead(uint16_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(gtAddr, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}
static void gtWrite(uint16_t reg, uint8_t v) {
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.write(v);
  Wire.endTransmission();
}
bool hwHasTouch() { return gtAddr != 0; }

static bool tDown = false;
static int tX = 0, tY = 0;
static uint32_t lastPoll = 0;
bool hwTouch(int &x, int &y) {
  if (gtAddr && millis() - lastPoll >= 15) {
    lastPoll = millis();
    uint8_t st, p[4];
    if (gtRead(0x814E, &st, 1) && (st & 0x80)) {
      uint8_t n = st & 0x0F;
      if (n > 0 && gtRead(0x8150, p, 4)) {
        tX = constrain(p[0] | (p[1] << 8), 0, LCD_W - 1);
        tY = constrain(p[2] | (p[3] << 8), 0, LCD_H - 1);
      }
      tDown = n > 0;
      gtWrite(0x814E, 0);
    }
  }
  x = tX; y = tY;
  return tDown;
}

void hwInit() {
  ledcAttach(PIN_BL, 5000, 10);
  hwBacklight(0);
  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  exOut = 0xFF & ~((1 << EX_LCD_SDA) | (1 << EX_LCD_SCK));
  exWrite(0x01, exOut);
  exWrite(0x03, 0xF8);   // EXIO0-2 outputs, the rest stay inputs
  for (const InitCmd &c : INIT) {
    lcdCmd(c.cmd, c.d, c.len);
    if (c.delayMs) delay(c.delayMs);
  }
  panelInit();
  for (uint8_t a : {0x5D, 0x14}) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { gtAddr = a; break; }
  }
}
