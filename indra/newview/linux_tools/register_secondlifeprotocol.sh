#!/bin/bash

# Register a protocol handler (default: handle_secondlifeprotocol.sh) for
# URLs of the form secondlife://...
#

HANDLER="$1"

RUN_PATH=`dirname "$0" || echo .`
cd "${RUN_PATH}"

if [ -z "$HANDLER" ]; then
    HANDLER=`pwd`/handle_secondlifeprotocol.sh
fi

# Register handler for GNOME-aware apps
LLGCONFTOOL2=gconftool-2
if which ${LLGCONFTOOL2} >/dev/null; then
    (${LLGCONFTOOL2} -s -t string /desktop/gnome/url-handlers/secondlife/command "${HANDLER} \"%s\"" && ${LLGCONFTOOL2} -s -t bool /desktop/gnome/url-handlers/secondlife/enabled true) || echo Warning: Did not register secondlife:// handler with GNOME: ${LLGCONFTOOL2} failed.
else
    echo Warning: Did not register secondlife:// handler with GNOME: ${LLGCONFTOOL2} not found.
fi

# Register handler for KDE-aware apps
if [ -z "$KDEHOME" ]; then
    KDEHOME=~/.kde
fi
LLKDEPROTDIR=${KDEHOME}/share/services
if [ -d "$LLKDEPROTDIR" ]; then
    LLKDEPROTFILE=${LLKDEPROTDIR}/secondlife.protocol
    cat > ${LLKDEPROTFILE} <<EOF || echo Warning: Did not register secondlife:// handler with KDE: Could not write ${LLKDEPROTFILE}
[Protocol]
exec=${HANDLER} '%u'
protocol=secondlife
input=none
output=none
helper=true
listing=
reading=false
writing=false
makedir=false
deleting=false
EOF
else
    echo Warning: Did not register secondlife:// handler with KDE: Directory $LLKDEPROTDIR does not exist.
fi
