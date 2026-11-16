# CYD Spotify Remote

Touchscreen Spotify controller for the ESP32-2432S028 with a Flask/Spotipy
server for Windows.

## Appearance

Tap **THEME** in the top-right of the header to cycle Graphite, Midnight and
Light palettes. The selected palette is saved in ESP32 Preferences.

## Build

Verified with PlatformIO: RAM 51,316 bytes, flash 980,265 bytes.

## Setup

Copy `firmware/CYD_Spotify_Remote/config.example.h` to `config.h` and fill in
local values. Set `SPOTIFY_COVER_ROOT_CA` to the verified PEM root CA used by
the Spotify artwork CDN; album-art downloads fail closed until this is set and
the device clock is synchronized. Copy `server/.env.example` to
`server/.env`, replace its `CYD_API_TOKEN` placeholder with a long random
secret, and put the same value in `SPOTIFY_SERVER_TOKEN` in `config.h`.
Install `server/requirements.txt`, run `authorize.py` once, then
`server.py`. The server requires this token for LAN requests and only allows
tokenless access over loopback.

## Limits

Not tested on hardware or the live Spotify API. The LAN control API uses a
bearer token over trusted-network HTTP (do not port-forward it). Album-art TLS
verification requires a correctly configured trusted root CA. No OTA updates.
