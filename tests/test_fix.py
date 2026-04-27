import unittest
from code import Settings

class TestSettings(unittest.TestCase):
    def test_initialization(self):
        settings = Settings()
        self.assertEqual(settings.font_size, 12)
        self.assertEqual(settings.font_hinting, "slight")
        self.assertTrue(settings.anti_aliasing)

    def test_update_settings(self):
        settings = Settings()
        settings.update_settings({"font_size": 16, "anti_aliasing": False, "invalid_key": "ignore"})
        self.assertEqual(settings.font_size, 16)
        self.assertFalse(settings.anti_aliasing)
        self.assertFalse(hasattr(settings, "invalid_key"))

if __name__ == "__main__":
    unittest.main()