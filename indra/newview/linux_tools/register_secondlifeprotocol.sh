#!/bin/bash

# Register a protocol handler (default: handle_secondlifeprotocol.sh) for
# URLs of the form secondlife://...
#
# Ask the Desktop Environment to change it's SLURL handler to this viewer. May prompt the user to select their preferred handler or do so silently depending on Desktop Environment.
xdg-settings set default-url-scheme-handler secondlife com.secondlife.indra.viewer.desktop
