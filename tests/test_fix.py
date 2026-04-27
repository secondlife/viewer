import unittest
from unittest.mock import patch, MagicMock
import os
import code

class TestLauncher(unittest.TestCase):
    @patch('platform.system')
    @patch('subprocess.run')
    def test_launch_viewer_linux(self, mock_run, mock_system):
        mock_system.return_value = "Linux"
        code.launch_viewer("http://example.com")
        mock_run.assert_called_with(["./secondlife-bin", "--login-url", "http://example.com"], check=True)

    @patch('platform.system')
    @patch('subprocess.run')
    @patch('time.time')
    @patch.dict(os.environ, {}, clear=False)
    def test_launch_viewer_mac(self, mock_time, mock_run, mock_system):
        mock_system.return_value = "Darwin"
        mock_time.return_value = 1000
        code.launch_viewer("http://example.com")
        
        self.assertEqual(os.environ.get("NSAppSleepDisabled"), "YES")
        expected_args = [
            "./Second Life.app/Contents/MacOS/Second Life",
            "--login-url", "http://example.com?v=1000",
            "--disable-gpu-compositing",
            "--disable-gpu-rasterization"
        ]
        mock_run.assert_called_with(expected_args, check=True)

    @patch('subprocess.run')
    def test_launch_viewer_not_found(self, mock_run):
        mock_run.side_effect = FileNotFoundError
        # Should catch exception and print error instead of crashing
        code.launch_viewer("http://example.com")

if __name__ == "__main__":
    unittest.main()