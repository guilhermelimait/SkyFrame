# Security

SkyFrame is a home gadget and a proof of concept: no accounts, no cloud, nothing reachable from the internet.
This page explains what it protects, how, and where the limits are. It's provided as is, with no warranty.

## What's stored on the device

| Data | How it's stored |
|---|---|
| Wi-Fi network name | plain (needed to connect, not secret) |
| Wi-Fi password | encrypted, see below |
| Pairing code for the settings page | plain in flash; also shown on the screen when you hold it |
| City, world clocks, countdown, display settings | plain |

Nothing else is stored, and the device doesn't collect or send any usage data.

## What leaves your home

- Your chosen city's **coordinates** go to [Open-Meteo](https://open-meteo.com/) for weather and air quality, and the
  three world-clock cities' coordinates for their time zones (Open-Meteo also sees your home's public IP address)
- Requests to [wheretheiss.at](https://wheretheiss.at/) for the Space Station track (no personal data)
- Requests to `pool.ntp.org` / `time.google.com` / `time.cloudflare.com` for the time
- Requests to GitHub for the latest SkyFrame release: once a night if automatic updates are on (the default), and
  when the settings page is opened (at most every 10 minutes)

City searches on the settings page go from **your browser** to Open-Meteo's geocoding service, not through the device.
Everything else the settings page needs, including its font, comes from the device itself.

## Protections

### Settings page
- Needs the **6-digit pairing code** in every request. The code is shown only on the device itself (hold the screen
  for 1 second), so being on the same Wi-Fi isn't enough.
- **Five wrong codes** lock the settings API for a minute.
- The code can be changed from the settings page (other paired browsers then need the new code).

### First-time setup
- The **SkyFrame-Setup** hotspot uses **WPA2** with a fresh random password each time, shown on the screen
  and in its QR code. The Wi-Fi password you type travels over an encrypted link.

### Stored Wi-Fi password
- On the first connection the chip generates a random **256-bit key** and burns it into a free **eFuse key block**
  with purpose `HMAC_UP` and **read protection**: software can't read it back, it can only ask the chip's HMAC
  hardware to use it.
- The password is encrypted with **AES-256-GCM** using `HMAC(chip key, label)` as the key. A copy of the flash
  is useless without that particular chip.
- When an older plaintext password is found, it's encrypted and the whole settings storage (NVS) is **erased and
  rewritten**: deleting an NVS entry alone leaves the old bytes physically in flash.
- The Wi-Fi driver is told not to keep its own copy of passwords in flash.

The eFuse write is **one-time and permanent**. It uses one of the ESP32-S3's six key blocks and doesn't affect
booting, USB flashing or updates.

### Internet connections
- All HTTPS requests **verify server certificates** against the Mozilla root bundle built into ESP-IDF.

### Firmware updates
- Releases must be signed with an **ECDSA P-256** key; the public half is built into the firmware
  (`firmware/src/ota_pubkey.h`). Unsigned or wrongly signed files are rejected before they're installed.
- Only **newer** versions are installed.
- A new version must complete a successful internet update within **10 minutes**, otherwise the bootloader
  **rolls back** to the previous one.

## Known limits

- **Plain HTTP on your local network.** The settings page and pairing code aren't encrypted between your browser
  and the device. Someone already able to watch traffic on your Wi-Fi could capture the code.
- **Physical access.** Someone who has the board *and* flashes their own firmware onto it can make the chip use its
  key and recover the Wi-Fi password. Preventing that needs **Secure Boot + flash encryption**, which is permanent
  and locks the board to signed firmware forever; this project doesn't enable it.
- **Lockout can be abused.** Anyone on your Wi-Fi can send wrong codes on purpose to keep the settings page locked.
  It can't unlock anything, but it can be a nuisance; a guest network for the display avoids it.
- **USB debug channel.** With a cable, someone can screenshot what the display shows (including the pairing-code
  screen) and change the displayed screen. It can't read the Wi-Fi password.

**Recommendation:** put the display on a **guest or IoT Wi-Fi network**. Then even in the worst case its password
can't open your main network, computers or phones.

## Your signing key

`skyframe-keys/ota_signing_key.pem` (created by `tools/keygen.py`, outside the repository) is the only thing
that can produce updates your display will accept.

- Keep a **backup** somewhere safe (password manager, USB stick).
- **Never** upload it: `.gitignore` excludes `*.pem`, and the key isn't inside the repository folder at all.
- If it's ever leaked, create a new key and re-flash the board over USB with firmware containing the new public key.

## Reporting a problem

This is a proof of concept shared as is, without support (see *Project status* in the README), so reports
may not be answered or fixed. If you still want to flag a security issue, please use a **private security
advisory** on this repository (*Security → Report a vulnerability*) rather than a public issue, so others aren't
exposed while it's open. If you rely on this project, review the code and apply fixes in your own fork.
