# Changelog

## 1.3.2 (2026-10-23)

- **Updates ask first and can be cancelled.** *Install now* opens a box covering the page ("Install v1.3.2?") with
  **Cancel** and **Install**; while it checks and downloads, **Cancel update** stops it cleanly (the download is
  thrown away and the display carries on). Once the new firmware is being written to flash it can't be cancelled,
  so the button disappears for those few seconds.
- **Fix: "Couldn't read the release info".** GitHub sometimes sends the release description in chunks, which the
  display read as broken JSON; it's now read through the HTTP library, with 3 tries on a flaky connection.
- The settings page is never kept by the browser (it always matches the firmware running), and it reloads itself
  after an update so the new version's page appears.
- **Fix: the Brightness slider seemed to do nothing at night.** It only changed the daytime level; during night
  hours it now sets the night level (and says so), so the change shows and stays.
- **Fix: the sound meter kept the microphone on with *Wake on sound* off.** The meter (and the microphone) now only
  run when the option is on; the page says "Off" otherwise.
- USB debug command `check`: look for a new version without installing.

## 1.3.1 (2026-10-23)

- **Fix: opening *Screen time* in quick settings restarted the display.** The fifth page had no picture memory
  (the list still had four pages); every page now gets its own, however many there are.

## 1.3 (2026-10-23)

### On the display
- **Quick settings:** swipe down from the top edge of any screen. Four tiles (brightness, night mode, clock, screen
  time) open pages that slide in; swipe left to right to go back, right to left for the next option, up to close.
  Brightness changes live as you drag; night mode has its hours and its own brightness together. Settings are saved
  once, when the panel closes (flash writes disturb the screen).
- **Screens slide:** drag a screen sideways and it follows your finger, with the next or previous screen coming in;
  taps slide quickly, and the automatic change slides gently instead of blinking to black. The neighbouring screens
  are drawn ahead of time, so only finished pictures move (no waiting, no glitches).
- Smoother animation: each frame waits for one screen refresh instead of two.
- **Quick settings in five rows** (icon, name, value): brightness, night mode, wake on sound, clock, screen time.
- **Wake on sound:** at night, the screen stays at its night level and uses the day brightness for a minute when
  there's sound around. It learns the room's background noise and reacts to sound above it (sensitivity low, medium,
  high; a live meter on its page). The microphone only runs while it's needed; only loudness is measured, nothing is
  recorded or sent.
- **Auto-rotation:** the board's motion sensor turns the picture to match how the display stands (after it's been
  steady for a moment; not while it lies flat or is touched). New default *Auto* in Settings → Rotation; picking a
  fixed angle turns it off.

### Fixes and settings page
- **Fix: glitches after changing the rotation (or interval, brightness, screen order).** Every settings change
  started a full data refresh (about five HTTPS downloads), and that Wi-Fi traffic disturbed the screen. Only a
  new place, units or world-clock cities refresh data now; other changes just redraw, and right away.
- **Install now starts at once.** The display goes straight to the update (no data refresh first) and shows its
  update screen immediately.
- **The settings page locks while updating:** a full-screen overlay from *Install now* until the display is back
  on the new version, with download progress, the install step and the restart; it closes with "Updated to …",
  or shows the reason if the update didn't finish. Opening the page during an update shows it too.

## 1.2 (2026-10-21)

### Display
- **Fix: the screen glitched constantly while the settings page was open.** Every status request (every 2 s)
  read the settings storage in flash to report whether the Wi-Fi password is encrypted, and flash reads pause the
  panel's picture memory. That answer is now worked out once at start-up. Found by testing with Wi-Fi forced off
  and on, and with the page open and closed.
- The firmware is confirmed to the bootloader only once after an update, not on every data update.

### Settings page
- The header shows the version and whether it's up to date as soon as the page opens.
- A **yellow bar** at the top replaces the update popup: it says when a new version is out, offers **Install now**
  and *What's new*, shows download and install progress, and stays until the display is updated.

### Updates
- **Fix: updates could not be downloaded.** GitHub sends release files through a redirect to its download
  servers, and the HTTP library reconnected there without the certificate list, so the secure connection was
  refused ("Couldn't download the signature"). Redirects are now followed with a fresh, certificate-checked
  connection for each step. Tested end to end: found, downloaded, signature checked, installed, restarted.
- **The screen no longer goes wild during an update.** The update is downloaded into memory first (the screen
  stays calm and shows progress), its signature is checked, and only then written to flash in a few seconds with
  the backlight off. Nothing touches flash unless the signature is valid.
- **Displays on 1.1 need one USB flash to get 1.2**, because 1.1 itself can't download updates. From 1.2 on,
  updates install by themselves again.
- Update errors now show the HTTP code, and the USB debug channel can start an update (`update`) and shows its log.
- New `update-test` build (`pio run -e update-test -t upload`): reports version 1.0.9, so you can test the
  whole update path against the latest real release without publishing anything.

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
  **Install now** when one is available.
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
