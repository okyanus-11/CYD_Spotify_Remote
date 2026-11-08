#!/usr/bin/env python3
"""One-time Spotify browser authorization for the CYD remote server."""
from __future__ import annotations

import os

from dotenv import load_dotenv
from spotipy.oauth2 import SpotifyOAuth

load_dotenv()

SCOPE = (
    "user-read-playback-state "
    "user-read-currently-playing "
    "user-modify-playback-state"
)


def main() -> None:
    required = {
        "SPOTIFY_CLIENT_ID": os.getenv("SPOTIFY_CLIENT_ID"),
        "SPOTIFY_CLIENT_SECRET": os.getenv("SPOTIFY_CLIENT_SECRET"),
        "SPOTIFY_REDIRECT_URI": os.getenv("SPOTIFY_REDIRECT_URI"),
    }
    missing = [name for name, value in required.items() if not value]
    if missing:
        raise SystemExit("Missing in .env: " + ", ".join(missing))

    cache_path = os.getenv("SPOTIFY_CACHE_PATH", ".spotify_cache")
    auth = SpotifyOAuth(
        client_id=required["SPOTIFY_CLIENT_ID"],
        client_secret=required["SPOTIFY_CLIENT_SECRET"],
        redirect_uri=required["SPOTIFY_REDIRECT_URI"],
        scope=SCOPE,
        cache_path=cache_path,
        open_browser=True,
    )

    # Opens the browser (or prints the URL); the token is cached on success.
    token = auth.get_access_token(as_dict=False)
    if token:
        print(f"Spotify authorization complete. Token cached at {cache_path}")
    else:
        print("No new token was needed; the existing cache is valid.")


if __name__ == "__main__":
    main()
