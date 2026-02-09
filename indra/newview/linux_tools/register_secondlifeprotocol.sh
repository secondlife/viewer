#!/bin/bash

# Register a protocol handler (default: handle_secondlifeprotocol.sh) for
# URLs of the form secondlife://...
#
# Instead of forcing through an association, this should get the desktop environment to prompt the user if they wish to change their SLURL handler.
xdg-settings set default-url-scheme-handler secondlife com.secondlife.SecondLifeViewer.desktop
