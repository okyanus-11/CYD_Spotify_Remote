import os
import unittest
from unittest.mock import Mock, patch

import server as app_server


class ControlApiTests(unittest.TestCase):
    api_token = "0123456789abcdef0123456789abcdef"
    control_paths = ("/play", "/previous", "/next", "/volume/up", "/volume/down")

    def setUp(self):
        self.client = app_server.app.test_client()
        self.previous_spotify = app_server._spotify
        app_server._spotify = Mock()

    def tearDown(self):
        app_server._spotify = self.previous_spotify

    def test_control_routes_reject_missing_token_configuration(self):
        with patch.dict(os.environ, {"CYD_API_TOKEN": ""}):
            responses = [self.client.post(path) for path in self.control_paths]

        for response in responses:
            self.assertEqual(response.status_code, 503)
            self.assertNotIn("CYD_API_TOKEN", response.get_data(as_text=True))

    def test_control_routes_reject_weak_token_configuration(self):
        with patch.dict(os.environ, {"CYD_API_TOKEN": "short"}):
            responses = [
                self.client.post(
                    path, headers={"X-CYD-Token": "short"}
                )
                for path in self.control_paths
            ]

        self.assertTrue(all(response.status_code == 503 for response in responses))

    def test_control_routes_reject_invalid_token(self):
        with patch.dict(os.environ, {"CYD_API_TOKEN": self.api_token}):
            response = self.client.post(
                "/play", headers={"X-CYD-Token": "incorrect"}
            )

        self.assertEqual(response.status_code, 401)
        self.assertNotIn(self.api_token, response.get_data(as_text=True))
        app_server._spotify.pause_playback.assert_not_called()

    def test_get_is_not_allowed_for_control_routes(self):
        with patch.dict(os.environ, {"CYD_API_TOKEN": self.api_token}):
            responses = [self.client.get(path) for path in self.control_paths]

        self.assertTrue(all(response.status_code == 405 for response in responses))

    def test_valid_token_can_control_playback(self):
        app_server._spotify.current_playback.return_value = {"is_playing": True}
        with patch.dict(os.environ, {"CYD_API_TOKEN": self.api_token}):
            response = self.client.post(
                "/play", headers={"X-CYD-Token": self.api_token}
            )

        self.assertEqual(response.status_code, 200)
        app_server._spotify.pause_playback.assert_called_once_with()

    def test_volume_up_from_zero_increments_from_zero(self):
        app_server._spotify.current_playback.return_value = {
            "device": {"id": "device-id", "volume_percent": 0}
        }
        with patch.dict(os.environ, {"CYD_API_TOKEN": self.api_token}):
            response = self.client.post(
                "/volume/up", headers={"X-CYD-Token": self.api_token}
            )

        self.assertEqual(response.status_code, 200)
        app_server._spotify.volume.assert_called_once_with(10)

    def test_spotify_exception_is_not_returned_to_control_client(self):
        app_server._spotify.current_playback.side_effect = RuntimeError(
            self.api_token
        )
        with patch.dict(os.environ, {"CYD_API_TOKEN": self.api_token}):
            response = self.client.post(
                "/play", headers={"X-CYD-Token": self.api_token}
            )

        self.assertEqual(response.status_code, 409)
        self.assertNotIn(self.api_token, response.get_data(as_text=True))


if __name__ == "__main__":
    unittest.main()
