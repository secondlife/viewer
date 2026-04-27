import logging

class VoiceManager:
    """
    Manages the lifecycle of voice sessions and participant indicators (voice dots).
    """
    def __init__(self):
        self.logger = logging.getLogger("VoiceManager")
        self.participants = {}
        self.current_provider = None

    def handle_teleport_complete(self, new_provider):
        """
        Triggered when a teleport is finished to re-initialize voice for the new region.
        """
        self.logger.info(f"Teleport complete. New voice provider: {new_provider}")

        # If switching providers (e.g., Vivox to WebRTC), clear the participant mapping.
        # Stale mapping from the previous provider prevents new voice dots from appearing in the UI.
        if self.current_provider and self.current_provider != new_provider:
            self.logger.debug(f"Provider switch detected ({self.current_provider} -> {new_provider}).")
            self.clear_voice_indicators() # Fix: Clear all indicators to ensure new ones are created for WebRTC
            self.current_provider = new_provider

        self._start_voice_session()

    def clear_voice_indicators(self):
        """
        Removes all active voice indicators and purges the participant cache.
        """
        for participant_id in list(self.participants.keys()):
            self._destroy_indicator(participant_id)
        self.participants.clear() # Fix: Reset tracking map to allow fresh registration in the new session

    def on_participant_speech(self, participant_id, is_speaking):
        """
        Callback from the voice engine when a participant's speech state changes.
        """
        if participant_id not in self.participants:
            # Lazily initialize the participant record and UI dot
            self.participants[participant_id] = {"is_speaking": False}
            self._create_indicator(participant_id)
        
        self.participants[participant_id]["is_speaking"] = is_speaking
        self._update_indicator_ui(participant_id, is_speaking)

    def _start_voice_session(self):
        # Implementation for session startup logic
        pass

    def _create_indicator(self, participant_id):
        # UI implementation for creating the green voice dot
        pass

    def _destroy_indicator(self, participant_id):
        # UI implementation for removing the voice dot
        pass

    def _update_indicator_ui(self, participant_id, is_speaking):
        # UI implementation for animating green bars or dot visibility
        pass