class LLMediaAPI(object):
    """
    LEAP API extension for querying embedded browser and media content.
    Enables test automation for web-based UI components like the login splash page.
    """
    def __init__(self, viewer):
        self.viewer = viewer

    def getMediaInfo(self, path):
        """
        Returns current URL, MIME type, and loading status for the media widget at the specified path.
        """
        try:
            # Query the viewer core for metadata (URL, MIME, Status) of the media widget at the XUI path
            return self.viewer.get_media_info(path)
        except Exception as e:
            # Return error state if the widget path is invalid or media info is unavailable
            return {"success": False, "error": str(e)}

    def getMediaText(self, path):
        """
        Returns the visible text content from the embedded media or browser page at the specified path.
        """
        try:
            # Extract and return rendered DOM text from the browser instance for automated verification
            text_content = self.viewer.get_media_text(path)
            return {"text": text_content, "success": True}
        except Exception as e:
            # Handle potential failures in DOM access or text extraction
            return {"success": False, "error": str(e)}