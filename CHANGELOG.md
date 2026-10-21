# Changelog

## 1.1 (2026-10-21)

### SkyFrame
- The project is now called **SkyFrame**: settings at `http://skyframe.local`, setup hotspot `SkyFrame-Setup`.
  Saved settings, Wi-Fi and the pairing code carry over.
- New **Stargazing tonight** screen replaces the focus timer: a 0–10 score for tonight from the hourly cloud
  forecast, the Moon's brightness and position, and real darkness (Sun below −12°), plus the best time to look up.

### Display
- Panel timing now uses Waveshare's official values for this board (12 MHz pixel clock, their sync timing and
  startup sequence). Fixes flickering, cut or distorted text, and picture jumps.
- Screens are redrawn only when something on them changes (usually once a minute) instead of every second.
- Screen changes dip the backlight instead of blending frames, which removes the "card on top of card" effect.
- Each internet update redraws the screen once, after all data has arrived.
- Less memory traffic from the network: no more picture copying for the settings page (see below),
  encryption buffers kept in internal RAM, and a bigger buffer for the panel.
- Saving a setting no longer leaves the picture shifted or glitching: writing to flash briefly stops the panel
  from reading its picture, so the backlight now goes off for a moment and the panel restarts cleanly afterwards
  (also kept in step while an update is written).
- New setting: rotate the picture 0°, 90°, 180° or 270°; touch and the settings preview follow the rotation.

### Wi-Fi
- Connects every 15 minutes to update, then switches the radio off.
- Holding the screen for 1 second turns Wi-Fi on for 10 minutes and shows the settings address.
- The Space Station position is interpolated from a 95-minute track downloaded at each update.
- "offline" only shows after more than an hour without a successful update.

### Security
- Settings page protected by a 6-digit pairing code shown on the screen (QR code pairs automatically).
  Only requests that send a wrong code count toward the lockout, so an old open browser tab can't lock you out.
- First-time setup hotspot uses WPA2 with a fresh random password shown on the screen.
- Wi-Fi password stored encrypted (AES-256-GCM) with a key locked in the chip's eFuses; older plaintext copies
  are wiped from flash.
- All HTTPS connections check server certificates.
- The Wi-Fi driver no longer keeps its own copy of passwords in flash.

### Settings page
- Redesigned as three tabs: **Now** (live preview, previous/next, brightness, hold), **Screens** (reorder by
  dragging, show a screen now, settings inside each screen's card) and **Device**.
- Saving shows "Saved" next to the setting and offers Undo; a short welcome on the first visit.
- Larger touch targets and keyboard support (arrow keys reorder screens).
- Opening the page checks GitHub for a new version (without installing, at most every 10 minutes) and offers
  **Install now** when one is available, with a popup the first time a new version is found.
- The page's font (Inter) is served by the display itself: the page loads nothing from the internet except
  city searches, and looks the same offline.
- The About buttons (GitHub, Ko-fi, MIT license) are the same size and aligned; they stack on phones.
- The live view is drawn by the page itself from the display's data (`/api/data`), with the same layout,
  fonts and astronomy as the display. It is sharp, follows screen changes at once and costs the display
  nothing. **Show exact picture** still fetches a real capture of the screen.

### Updates
- Updates come from the SkyFrame repository built into the firmware (`OTA_REPO`): nothing to configure.
- New *About* section in the settings page: version, MIT license, credits, and buttons for GitHub and Ko-fi.
- Signed firmware updates from GitHub Releases: nightly or on demand, with a progress screen and automatic rollback.
- New tools: `keygen.py` (signing key) and `release.py` (build + sign).

## 1.0

- First standalone firmware: all screens drawn on the board, data over Wi-Fi, settings page at
  `http://skyframe.local`, first-time Wi-Fi setup hotspot.
- 16 screens: home, forecast, hourly, sun, moon, sky map, planets, solar system, sky events, Space Station,
  world clocks, calendar, air quality, focus timer, countdown, quote.

## Before 1.0

- Browser version of the design (`web/skyframe.html`), including a NASA picture-of-the-day screen.
- A PC-driven version that streamed the browser page to the board over USB (retired).
