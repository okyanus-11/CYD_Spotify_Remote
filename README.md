# CYD Spotify Remote

ESP32-2432S028 touchscreen controller for a LAN companion server. It displays track metadata and album art and sends playback and volume commands.

## Dependencies

- ESP32 Arduino core (WiFi, HTTPClient, WiFiClientSecure)
- TFT_eSPI
- XPT2046_Touchscreen
- TJpg_Decoder

Configure TFT_eSPI for the target CYD board.

## Setup

1. Set the Wi-Fi and server placeholders near the top of `CYD_Spotify_Remote.ino` locally.
2. Run the standalone server from the companion repository.
3. Upload the sketch to the CYD and verify that the computer and device share a trusted LAN.

The included server is a route-compatible stub: it does not authenticate with Spotify or control a Spotify client. Wire those actions into the server separately if that functionality is required.

## Verification

The sketch was source-reviewed but not compiled or hardware-tested because no Arduino toolchain or CYD hardware was available. The sketch uses `setInsecure()` for album-art HTTPS and should not be treated as secure for sensitive traffic.

