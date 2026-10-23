# Hardware notes: Waveshare ESP32-S3-Touch-LCD-4B

Reference for the low-level setup in `firmware/src/hw.cpp`. Pins and panel values come from
[Waveshare's official examples](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4B) for this board.

## Board

| | |
|---|---|
| SoC | ESP32-S3, 16 MB flash, 8 MB octal PSRAM |
| Display | 4″ 480×480 IPS, ST7701S controller, 16-bit parallel RGB |
| Touch | GT911 on I²C (address 0x5D or 0x14) |
| I/O expander | TCA9554 on I²C, address 0x20 |
| USB | native USB (Espressif VID 0x303A), shows up as a serial port |

## Pins

| Function | GPIO |
|---|---|
| RGB red R0–R4 | 10, 11, 12, 13, 14 |
| RGB green G0–G5 | 21, 8, 18, 45, 38, 39 |
| RGB blue B0–B4 | 40, 41, 42, 2, 1 |
| PCLK / DE / HSYNC / VSYNC | 9 / 17 / 46 / 3 |
| Backlight (PWM, active low) | 4 |
| I²C SDA / SCL | 47 / 48 |

The ESP-IDF RGB driver expects data lines in the order B0–B4, G0–G5, R0–R4.

## I/O expander (TCA9554)

| Pin | Use |
|---|---|
| EXIO0 | LCD SPI chip select |
| EXIO1 | LCD SPI data |
| EXIO2 | LCD SPI clock |
| EXIO3 | speaker amplifier enable |
| EXIO4 | power key (input) |
| EXIO5 | touch reset |
| EXIO6 | touch interrupt |
| EXIO7 | LCD reset |

The ST7701S is configured once at startup over a 3-wire, 9-bit SPI bit-banged through EXIO0–2. The firmware only
switches those three pins to outputs and leaves the rest alone.

## Panel timing

| | |
|---|---|
| Pixel clock | **12 MHz** (about 42 frames per second) |
| HSYNC pulse / back porch / front porch | 8 / 50 / 10 |
| VSYNC pulse / back porch / front porch | 8 / 20 / 10 |
| Startup sequence | Waveshare's `st7701_type1_init_operations`, ending with colour inversion (`0x21`) and 18-bit format (`0x3A 0x60`) |

An earlier version used 16 MHz with community timing values. The frame buffer was correct, but the panel showed
flickering, cut text and jumps; Waveshare's values fixed it.

## Rotation

The ST7701S can only mirror, not rotate, so 90°/180°/270° are done in software: each screen is drawn upright into
a spare PSRAM buffer, then copied into the frame buffer turned (in 16×16 tiles to stay cache-friendly). Touch
coordinates are turned back the opposite way. Because the screen redraws only about once a minute, this costs
almost nothing.

## Frame buffers and memory bandwidth

- Two 480×480 RGB565 frame buffers in PSRAM (450 KB each), plus a 30-line bounce buffer in internal RAM.
- The panel is fed continuously from PSRAM, so heavy PSRAM traffic (full redraws, blending, flash writes) can make
  the picture slip. The driver's automatic restart then snaps it back, which shows as a jump.
- That's why the firmware redraws only when content changes, uses a backlight dip instead of blended transitions,
  and avoids flash writes on every Wi-Fi connection.
- Network traffic counts too: this framework keeps Wi-Fi buffers in PSRAM. The firmware therefore keeps blocks up
  to 32 KB (TLS, JSON) in internal RAM (`heap_caps_malloc_extmem_enable`). The settings page draws its live view
  itself from the display's data, so it doesn't copy pictures from the display.
- Flash access also pauses PSRAM: settings are saved only when they change (with the backlight briefly off), status
  requests never read flash, and updates are downloaded into PSRAM and written to flash in one go.
- Moving to a neighbouring screen, and the quick-settings panel, slide pictures that were drawn in advance (row
  copies), so nothing is re-rendered during an animation.

## Flash layout

`default_16MB.csv`: two 6.5 MB app slots (for over-the-air updates with rollback), NVS for settings, and a
data partition. Firmware is about 2.5 MB.

## Sensor bus (I2C on GPIO 47 / 48)

Found on this board with the USB command `i2c`:

| Address | Chip | Used for |
|---|---|---|
| 0x14 / 0x5D | GT911 | touch |
| 0x18 | ES8311 | audio output (not used yet; needs a speaker on the MX1.25 connector) |
| 0x20 | TCA9554 | IO expander (panel setup lines, speaker amplifier enable on pin 3) |
| 0x34 | AXP2101 | power and battery management (not used yet) |
| 0x40 | ES7210 | microphone input (wake on sound) |
| 0x51 | PCF85063 | battery-backed clock (not used yet) |
| 0x6B | QMI8658 | motion sensor (auto-rotation) |

**Motion sensor orientation** (gravity in the sensor's frame): +Y = upright (0°), −X = 90°, −Y = 180° (USB-C on the
left), +X = 270°.

## Audio (I2S)

ES7210 microphone input: MCLK GPIO 5, BCLK 16, LRCK 7, data in 15 (from Waveshare's board definition). The firmware
reads 16 kHz, 16-bit stereo and turns each 32 ms block into one loudness value.

## USB ports

- **USB** (the chip's own USB): flashing, the USB debug channel. Shows as "USB Serial Device" on Windows.
- **UART** (CH343 USB-serial chip): always able to flash the board, whatever the firmware does. To recover a board
  that doesn't answer on the other port: hold **BOOT** while plugging in, then flash.
