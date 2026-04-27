import unittest
from unittest.mock import MagicMock
from code import LLMediaAPI

class TestLLMediaAPI(unittest.TestCase):
    def setUp(self):
        self.mock_viewer = MagicMock()
        self.api = LLMediaAPI(self.mock_viewer)

    def test_getMediaInfo_success(self):
        expected_data = {"url": "http://example.com", "mime": "text/html", "status": "loaded"}
        self.mock_viewer.get_media_info.return_value = expected_data
        result = self.api.getMediaInfo("test/path")
        self.assertEqual(result, expected_data)

    def test_getMediaInfo_exception(self):
        self.mock_viewer.get_media_info.side_effect = Exception("error")
        result = self.api.getMediaInfo("test/path")
        self.assertEqual(result, {"success": False, "error": "error"})

    def test_getMediaText_success(self):
        self.mock_viewer.get_media_text.return_value = "rendered content"
        result = self.api.getMediaText("test/path")
        self.assertEqual(result, {"text": "rendered content", "success": True})

    def test_getMediaText_exception(self):
        self.mock_viewer.get_media_text.side_effect = Exception("error")
        result = self.api.getMediaText("test/path")
        self.assertEqual(result, {"success": False, "error": "error"})

if __name__ == "__main__":
    unittest.main()