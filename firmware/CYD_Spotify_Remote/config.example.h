// Copy to config.h and fill in locally. config.h is git-ignored.
#pragma once

#define WIFI_SSID            "YOUR_WIFI_SSID"
#define WIFI_PASSWORD        "YOUR_WIFI_PASSWORD"

// IPv4 address or hostname of the Windows PC running server/server.py.
// Do NOT include http:// and do not reuse an old address without checking it.
#define SPOTIFY_SERVER_HOST  "YOUR_PC_LAN_IP"
#define SPOTIFY_SERVER_PORT  5000

// Must match CYD_API_TOKEN in server/.env. Replace with a long random secret.
#define SPOTIFY_SERVER_TOKEN "CHANGE_ME_WITH_A_LONG_RANDOM_TOKEN"


// Required for verified HTTPS album-art downloads. Paste the PEM root CA that
// validates the Spotify artwork CDN certificate chain. Empty fails closed.
#define SPOTIFY_COVER_ROOT_CA ""
