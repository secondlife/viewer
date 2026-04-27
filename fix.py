class Settings:
    def __init__(self):
        # Default font and UI settings
        self.font_size = 12
        # Font hinting options: "none", "slight", "medium", "full"
        self.font_hinting = "slight" # Updated default font hinting to 'slight' to improve UI text rendering and legibility
        self.anti_aliasing = True

    def update_settings(self, settings_dict):
        """
        Updates the settings with the provided dictionary.
        """
        for key, value in settings_dict.items():
            if hasattr(self, key):
                setattr(self, key, value) # Dynamically set attributes for UI configuration updates