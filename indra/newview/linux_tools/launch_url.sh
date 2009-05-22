#!/bin/bash

# This script loads a web page in the 'default' graphical web browser.
# It MUST return immediately (or soon), so the browser should be
# launched in the background (thus no text-only browsers).
# This script does not trust the URL to be well-escaped or shell-safe.
#
# On Unixoids we try, in order of decreasing priority:
# - $BROWSER if set (preferred)
# - Default GNOME browser
# - Default KDE browser
# - x-www-browser
# - The first browser in $BROWSER_COMMANDS that is found.

URL="$1"

if [ -z "$URL" ]; then
    echo "Usage: $(basename "$0") URL"
    exit
fi

# restore LD_LIBRARY_PATH from SAVED_LD_LIBRARY_PATH if it exists
if [ "${SAVED_LD_LIBRARY_PATH+isset}" = "isset" ]; then
    export LD_LIBRARY_PATH="${SAVED_LD_LIBRARY_PATH}"
    echo "$0: Restored library path to '${SAVED_LD_LIBRARY_PATH}'"
fi

# if $BROWSER is defined, use it.
XBROWSER=`echo "$BROWSER" |cut -f1 -d:`
if [ ! -z "$XBROWSER" ]; then
    XBROWSER_CMD=`echo "$XBROWSER" |cut -f1 -d' '`
    # look for $XBROWSER_CMD either literally or in PATH
    if [ -x "$XBROWSER_CMD" ] || which $XBROWSER_CMD >/dev/null; then
        # check for %s string, avoiding bash2-ism of [[ ]]
	if echo "$XBROWSER" | grep %s >/dev/null; then
	    # $XBROWSER has %s which needs substituting
	    echo "$URL" | xargs -r -i%s $XBROWSER &
	    exit
	fi
        # $XBROWSER has no %s, tack URL on the end instead
	$XBROWSER "$URL" &
	exit
    fi
    echo "$0: Couldn't find the browser specified by \$BROWSER ($BROWSER)"
    echo "$0: Trying some others..."
fi

# Launcher the default GNOME browser.
if [ ! -z "$GNOME_DESKTOP_SESSION_ID" ] && which gnome-open >/dev/null; then
    gnome-open "$URL" &
    exit
fi

# Launch the default KDE browser.
if [ ! -z "$KDE_FULL_SESSION" ] && which kfmclient >/dev/null; then
    kfmclient openURL "$URL" &
    exit
fi

# List of browser commands that will be tried in the order listed. x-www-browser
# will be tried first, which is a debian alternative.
BROWSER_COMMANDS="      \
    x-www-browser       \
    firefox             \
    mozilla-firefox     \
    iceweasel           \
    iceape              \
    opera               \
    epiphany-browser    \
    epiphany-gecko      \
    epiphany-webkit     \
    epiphany            \
    mozilla             \
    seamonkey           \
    galeon              \
    dillo               \
    netscape"
for browser_cmd in $BROWSER_COMMANDS; do
    if which $browser_cmd >/dev/null; then
	$browser_cmd "$URL" &
        exit
   fi
done

echo '$0: Failed to find a known browser.  Please consider setting the $BROWSER environment variable.'
exit 1
