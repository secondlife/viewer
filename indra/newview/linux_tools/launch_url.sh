#!/bin/sh
# bash v1.14+ expected

# This script loads a web page in the 'default' graphical web browser.
# It MUST return immediately (or soon), so the browser should be
# launched in the background (thus no text-only browsers).
# This script does not trust the URL to be well-escaped or shell-safe.
#
# On Unixoids we try, in order of decreasing priority:
# - $BROWSER if set (preferred)
# - kfmclient openURL
# - x-www-browser
# - opera
# - firefox
# - mozilla
# - netscape

URL="$1"

if [ -z "$URL" ]; then
    echo Usage: $0 URL
    exit
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
    echo "Couldn't find the browser specified by \$BROWSER ($BROWSER)"
    echo "Trying some others..."
fi

# else kfmclient
# (embodies KDE concept of 'preferred browser')
if which kfmclient >/dev/null; then
    kfmclient openURL "$URL" &
    exit
fi

# else x-www-browser
# (Debianesque idea of a working X browser)
if which x-www-browser >/dev/null; then
    x-www-browser "$URL" &
    exit
fi

# else opera
# (if user has opera in their path, they probably went to the
# trouble of installing it -> prefer it)
if which opera >/dev/null; then
    opera "$URL" &
    exit
fi

# else firefox
if which firefox >/dev/null; then
    firefox "$URL" &
    exit
fi

# else mozilla
if which mozilla >/dev/null; then
    mozilla "$URL" &
    exit
fi

# else netscape
if which netscape >/dev/null; then
    netscape "$URL" &
    exit
fi

echo 'Failed to find a known browser.  Please consider setting the $BROWSER environment variable.'

# end.
