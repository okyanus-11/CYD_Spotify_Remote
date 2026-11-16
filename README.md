# CYD Spotify Remote

Touchscreen Spotify controller for the ESP32-2432S028 with a Flask/Spotipy
server for Windows.

## Build

Verified with PlatformIO: RAM 51,316 bytes, flash 980,265 bytes.

## Setup

Copy `firmware/CYD_Spotify_Remote/config.example.h` to `config.h` and fill in
local values. Set `SPOTIFY_COVER_ROOT_CA` to the verified PEM root CA used by
the Spotify artwork CDN; album-art downloads fail closed until this is set and
the device clock is synchronized. Copy `server/.env.example` to
`server/.env`, install `server/requirements.txt`, run `authorize.py` once,
then `server.py`.

## Fixes

Pause-safe album art, offline retry, configurable network settings, shared SPI
bus handling and safer server errors.

## Limits

Not tested on hardware or the live Spotify API. Album-art TLS certificate
verification requires a correctly configured trusted root CA. No OTA updates.
