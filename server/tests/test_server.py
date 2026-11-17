"""LAN authentication regression tests for the Spotify companion server."""

import os
import unittest
from unittest.mock import patch

import server
from server import app


class AccessControlTests(unittest.TestCase):
    def setUp(self):
        self.client = app.test_client()

    def test_tokenless_loopback_access_is_allowed(self):
        with patch.dict(os.environ, {}, clear=False):
            os.environ.pop("CYD_API_TOKEN", None)
            response = self.client.get(
                "/health", environ_base={"REMOTE_ADDR": "127.0.0.1"}
            )
        self.assertEqual(response.status_code, 200)

    def test_tokenless_remote_access_is_rejected(self):
        with patch.dict(os.environ, {}, clear=False):
            os.environ.pop("CYD_API_TOKEN", None)
            response = self.client.get(
                "/health", environ_base={"REMOTE_ADDR": "192.168.1.20"}
            )
        self.assertEqual(response.status_code, 503)

    def test_configured_token_is_required(self):
        with patch.dict(os.environ, {"CYD_API_TOKEN": "test-secret"}):
            missing = self.client.get("/health")
            wrong = self.client.get(
                "/health", headers={"Authorization": "Bearer wrong"}
            )
            correct = self.client.get(
                "/health", headers={"Authorization": "Bearer test-secret"}
            )
        self.assertEqual(missing.status_code, 401)
        self.assertEqual(wrong.status_code, 401)
        self.assertEqual(correct.status_code, 200)


if __name__ == "__main__":
    unittest.main()



class PlaybackMethodTests(unittest.TestCase):
    class FakeSpotify:
        def __init__(self):
            self.volume_value = 0
            self.calls = []

        def current_playback(self):
            return {
                "is_playing": False,
                "device": {"id": "device-1", "volume_percent": self.volume_value},
            }

        def start_playback(self, **kwargs):
            self.calls.append(("start", kwargs))

        def pause_playback(self):
            self.calls.append(("pause", {}))

        def previous_track(self):
            self.calls.append(("previous", {}))

        def next_track(self):
            self.calls.append(("next", {}))

        def volume(self, value):
            self.volume_value = value
            self.calls.append(("volume", value))

    def setUp(self):
        self.client = app.test_client()
        self.spotify = self.FakeSpotify()

    def test_mutating_routes_are_post_only_and_accept_firmware_token(self):
        routes = ("/play", "/previous", "/next", "/volume/up", "/volume/down")
        with patch.dict(os.environ, {"CYD_API_TOKEN": "test-secret"}), patch(
            "server.get_spotify", return_value=self.spotify
        ):
            for route in routes:
                with self.subTest(route=route):
                    get_response = self.client.get(
                        route, headers={"X-CYD-Token": "test-secret"}
                    )
                    self.assertEqual(get_response.status_code, 405)
                    post_response = self.client.post(
                        route, headers={"X-CYD-Token": "test-secret"}
                    )
                    self.assertEqual(post_response.status_code, 200)

    def test_volume_up_respects_zero_volume(self):
        with patch.dict(os.environ, {"CYD_API_TOKEN": "test-secret"}), patch(
            "server.get_spotify", return_value=self.spotify
        ):
            response = self.client.post(
                "/volume/up", headers={"Authorization": "Bearer test-secret"}
            )
        self.assertEqual(response.status_code, 200)
        self.assertEqual(self.spotify.volume_value, 10)
