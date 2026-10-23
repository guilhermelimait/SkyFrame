# Third-party notices

SkyFrame itself is MIT licensed (see [LICENSE](LICENSE)). These parts keep their own licenses:

- Inter font: SIL Open Font License 1.1, see firmware/data/OFL.txt
- ES7210 audio chip driver (firmware/src/es7210.cpp, es7210.h, audio_hal.h): Espressif MIT license (for use on
  Espressif products), Copyright Espressif Systems; license text in the file headers
- stb_truetype: public domain or MIT, see the end of firmware/src/stb_truetype.h
- World map data (firmware/tools/land-110m.json and the generated firmware/src/land_dots.h):
  world-atlas, ISC license, Copyright 2013-2019 Michael Bostock,
  see firmware/tools/world-atlas-LICENSE.txt; derived from Natural Earth (public domain)
- Libraries downloaded at build time (ArduinoJson, QRCode, Arduino-ESP32 / ESP-IDF):
  their own licenses, listed under "Credits" in README.md
