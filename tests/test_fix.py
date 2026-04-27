import unittest
from unittest.mock import MagicMock
from code import VoiceManager

class TestVoiceManager(unittest.TestCase):
    def setUp(self):
        self.manager = VoiceManager()
        # Mock internal UI methods to avoid side effects
        self.manager._create_indicator = MagicMock()
        self.manager._destroy_indicator = MagicMock()
        self.manager._update_indicator_ui = MagicMock()
        self.manager._start_voice_session = MagicMock()

    def test_on_participant_speech_initializes_participant(self):
        self.manager.on_participant_speech("user_1", True)
        self.assertIn("user_1", self.manager.participants)
        self.assertTrue(self.manager.participants["user_1"]["is_speaking"])
        self.manager._create_indicator.assert_called_once_with("user_1")
        self.manager._update_indicator_ui.assert_called_with("user_1", True)

    def test_clear_voice_indicators_purges_cache(self):
        self.manager.participants = {"user_1": {"is_speaking": True}, "user_2": {"is_speaking": False}}
        self.manager.clear_voice_indicators()
        self.assertEqual(len(self.manager.participants), 0)
        self.assertEqual(self.manager._destroy_indicator.call_count, 2)

    def test_handle_teleport_complete_clears_on_provider_switch(self):
        self.manager.current_provider = "vivox"
        self.manager.participants = {"user_1": {"is_speaking": True}}
        
        self.manager.handle_teleport_complete("webrtc")
        
        self.assertEqual(self.manager.current_provider, "webrtc")
        self.assertEqual(len(self.manager.participants), 0)
        self.manager._destroy_indicator.assert_called_once()
        self.manager._start_voice_session.assert_called_once()

    def test_handle_teleport_complete_no_clear_on_same_provider(self):
        self.manager.current_provider = "vivox"
        self.manager.participants = {"user_1": {"is_speaking": True}}
        
        self.manager.handle_teleport_complete("vivox")
        
        self.assertEqual(len(self.manager.participants), 1)
        self.manager._destroy_indicator.assert_not_called()

if __name__ == "__main__":
    unittest.main()