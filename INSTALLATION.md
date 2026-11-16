# Installation and flashing

## Hardware and accounts
- ESP32-2432S028 / ESP32-2432S028R Cheap Yellow Display
- USB data cable and 2.4 GHz Wi-Fi
- Spotify account and Spotify Developer application credentials
- Windows PC (or another machine) to run the companion Flask/Spotipy server

## Install tools

Install Visual Studio Code with PlatformIO IDE, or install PlatformIO Core:

```bash
python -m pip install --upgrade platformio
```

## Configure firmware

```bash
git clone https://github.com/okyanus-11/CYD_Spotify_Remote.git
cd CYD_Spotify_Remote
```

Copy `firmware/CYD_Spotify_Remote/config.example.h` to `firmware/CYD_Spotify_Remote/config.h`. Set the Wi-Fi credentials, the PC's current LAN IP in `SPOTIFY_SERVER_HOST`, and a long random `SPOTIFY_SERVER_TOKEN`. Configure `SPOTIFY_COVER_ROOT_CA` with the verified root CA for the artwork CDN; album-art HTTPS requests fail closed while it is empty.

## Configure and start the server

Copy `server/.env.example` to `server/.env`. Set Spotify Developer credentials and a strong `CYD_API_TOKEN`; use the same token in the firmware configuration. From the repository root:

```powershell
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r server/requirements.txt
python server/authorize.py
python server/server.py
```

Complete the Spotify authorization flow when prompted. Keep `.env`, tokens and client secrets private.

## Build and upload firmware

```bash
cd firmware
pio run
pio run -t upload --upload-port COMx
```

Replace `COMx` with the CYD's actual serial port. Start the server on the same trusted LAN and ensure the configured PC IP and port match.

## Network safety

The server uses bearer-token authentication for LAN requests, but LAN traffic is plain HTTP. Use a trusted private network only; do not port-forward the server to the internet. Hardware and live Spotify API behavior should be validated on the target device.
