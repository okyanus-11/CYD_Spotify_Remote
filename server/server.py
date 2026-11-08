#!/usr/bin/env python3
"""CYD Spotify Remote companion server.

The ESP32 firmware sends simple GET requests and expects the legacy
``song|artist|cover`` now-playing format.  This server keeps that contract and
appends ``|1`` (playing) or ``|0`` (paused) so the display can update its
PLAY/PAUSE button without redrawing the album art.

Setup
    1. Create a Spotify app: https://developer.spotify.com/dashboard
    2. Add http://127.0.0.1:8888/callback as a redirect URI.
    3. Copy .env.example to .env and fill in the values.
    4. python -m pip install -r requirements.txt
    5. python authorize.py          # one-time browser login, writes the cache
    6. python server.py --port 5000

Set SPOTIFY_CACHE_PATH if you want the token cache somewhere else.  Never
commit .env or the token cache.
"""
from __future__ import annotations

import os
import threading
import time

from flask import Flask, Response, jsonify
from dotenv import load_dotenv

load_dotenv()

SCOPE = (
    "user-read-playback-state "
    "user-read-currently-playing "
    "user-modify-playback-state"
)
CACHE_TTL_SECONDS = 2.0

app = Flask(__name__)

_lock = threading.Lock()
_cache = {"at": 0.0, "payload": None}
_last_good = {"song": "Nothing playing", "artist": "", "cover": "", "playing": False}
_spotify = None
_init_error = None


def _env(name: str, default: str | None = None) -> str | None:
    value = os.getenv(name, default)
    return value.strip() if isinstance(value, str) else value


def _cache_path() -> str:
    return _env("SPOTIFY_CACHE_PATH", ".spotify_cache") or ".spotify_cache"


def _missing_settings() -> list[str]:
    required = {
        "SPOTIFY_CLIENT_ID": _env("SPOTIFY_CLIENT_ID"),
        "SPOTIFY_CLIENT_SECRET": _env("SPOTIFY_CLIENT_SECRET"),
        "SPOTIFY_REDIRECT_URI": _env("SPOTIFY_REDIRECT_URI"),
    }
    return [name for name, value in required.items() if not value]


def get_spotify():
    """Return a Spotipy client, or None with a human-readable reason."""
    global _spotify, _init_error

    if _spotify is not None:
        return _spotify

    missing = _missing_settings()
    if missing:
        _init_error = "Missing Spotify settings: " + ", ".join(missing)
        return None

    cache_path = _cache_path()
    if not os.path.exists(cache_path):
        _init_error = (
            f"No Spotify token cache at {cache_path}. "
            "Run 'python authorize.py' once on this computer."
        )
        return None

    try:
        import spotipy
        from spotipy.oauth2 import SpotifyOAuth

        auth = SpotifyOAuth(
            client_id=_env("SPOTIFY_CLIENT_ID"),
            client_secret=_env("SPOTIFY_CLIENT_SECRET"),
            redirect_uri=_env("SPOTIFY_REDIRECT_URI"),
            scope=SCOPE,
            cache_path=cache_path,
            open_browser=False,
        )
        _spotify = spotipy.Spotify(auth_manager=auth)
        _init_error = None
    except Exception as exc:  # noqa: BLE001 - report setup errors to the LAN client
        _spotify = None
        _init_error = f"Spotify initialisation failed: {exc}"

    return _spotify


def _sanitize(value, limit: int = 200) -> str:
    text = str(value or "").replace("|", "/").replace("\r", " ").replace("\n", " ")
    return text.strip()[:limit]


def _error(message: str, status: int = 503):
    return jsonify(ok=False, error=message), status


def _read_now_playing() -> tuple[dict, str | None]:
    global _last_good

    sp = get_spotify()
    if sp is None:
        return dict(_last_good), _init_error

    try:
        playback = sp.current_playback()
    except Exception as exc:  # noqa: BLE001 - keep the legacy text route alive
        return dict(_last_good), f"Spotify request failed: {exc}"

    # current_playback() still returns the item while paused, which is what
    # keeps the cover on screen.  Fall back to the last known track when the
    # account briefly reports nothing.
    if playback and playback.get("item"):
        item = playback["item"]
        album = item.get("album") or {}
        images = album.get("images") or []
        payload = {
            "song": _sanitize(item.get("name"), 120) or "Unknown song",
            "artist": _sanitize(
                ", ".join(a.get("name", "") for a in item.get("artists", [])), 120
            ),
            "cover": _sanitize(images[0].get("url") if images else "", 500),
            "playing": bool(playback.get("is_playing")),
        }
        with _lock:
            _last_good = payload
        return dict(payload), None

    return dict(_last_good), None


def _cached_now_playing() -> tuple[dict, str | None]:
    with _lock:
        fresh = (
            _cache["payload"] is not None
            and time.time() - _cache["at"] < CACHE_TTL_SECONDS
        )
        payload = dict(_cache["payload"]) if fresh else None

    if payload is not None:
        return payload, None

    payload, error = _read_now_playing()
    with _lock:
        _cache["at"] = time.time()
        _cache["payload"] = dict(payload)
    return payload, error


def _require_device(sp):
    playback = sp.current_playback()
    if not playback:
        raise RuntimeError("Nothing is playing on Spotify")
    device = playback.get("device") or {}
    if not device.get("id"):
        raise RuntimeError("No active Spotify device")
    return playback, device


@app.get("/")
def home():
    return "CYD SPOTIFY SERVER OK\n", 200, {"Content-Type": "text/plain"}


@app.get("/health")
def health():
    missing = _missing_settings()
    return jsonify(
        ok=not missing,
        credentials_configured=not missing,
        token_cache=os.path.exists(_cache_path()),
        cache_path=_cache_path(),
        error=_init_error,
    )


@app.get("/play")
def play():
    sp = get_spotify()
    if sp is None:
        return _error(_init_error)
    try:
        playback = sp.current_playback()
        if playback and playback.get("is_playing"):
            sp.pause_playback()
        elif playback and (playback.get("device") or {}).get("id"):
            sp.start_playback(device_id=playback["device"]["id"])
        else:
            sp.start_playback()
        return "OK"
    except Exception as exc:  # noqa: BLE001
        return _error(f"Play/pause failed: {exc}", 409)


@app.get("/previous")
def previous():
    sp = get_spotify()
    if sp is None:
        return _error(_init_error)
    try:
        sp.previous_track()
        return "OK"
    except Exception as exc:  # noqa: BLE001
        return _error(f"Previous failed: {exc}", 409)


@app.get("/next")
def next_track():
    sp = get_spotify()
    if sp is None:
        return _error(_init_error)
    try:
        sp.next_track()
        return "OK"
    except Exception as exc:  # noqa: BLE001
        return _error(f"Next failed: {exc}", 409)


def _change_volume(delta: int):
    sp = get_spotify()
    if sp is None:
        return _error(_init_error)
    try:
        _playback, device = _require_device(sp)
        current = int(device.get("volume_percent") or 50)
        sp.volume(max(0, min(100, current + delta)))
        return "OK"
    except Exception as exc:  # noqa: BLE001
        return _error(f"Volume change failed: {exc}", 409)


@app.get("/volume/up")
def volume_up():
    return _change_volume(10)


@app.get("/volume/down")
def volume_down():
    return _change_volume(-10)


@app.get("/now-playing")
def now_playing():
    payload, error = _cached_now_playing()
    playing = "1" if payload.get("playing") else "0"
    body = (
        f"{_sanitize(payload.get('song'), 120)}|"
        f"{_sanitize(payload.get('artist'), 120)}|"
        f"{_sanitize(payload.get('cover'), 500)}|{playing}"
    )
    response = Response(body, mimetype="text/plain")
    response.headers["Cache-Control"] = "no-store"
    if error:
        response.headers["X-CYD-Error"] = error[:200]
    return response


@app.get("/spotify-test")
def spotify_test():
    sp = get_spotify()
    if sp is None:
        return _error(_init_error)
    try:
        user = sp.current_user()
        return f"Connected as {user.get('display_name') or user.get('id')}!"
    except Exception as exc:  # noqa: BLE001
        return _error(f"Spotify test failed: {exc}", 409)


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description="CYD Spotify Remote server")
    parser.add_argument("--host", default=os.getenv("CYD_SERVER_HOST", "0.0.0.0"))
    parser.add_argument(
        "--port", type=int, default=int(os.getenv("CYD_SERVER_PORT", "5000"))
    )
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    print(f"CYD Spotify server listening on http://{args.host}:{args.port}")
    print("Set SPOTIFY_* values in .env; run authorize.py once.")
    app.run(host=args.host, port=args.port, debug=args.debug)


if __name__ == "__main__":
    main()
