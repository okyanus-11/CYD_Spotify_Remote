"""LAN authentication regression tests for the Spotify companion server."""

import os
import unittest
from unittest.mock import patch

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
