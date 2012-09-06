#!/bin/bash

# Simple wrapper for secondlife utilities.

## - Avoids an often-buggy X feature that doesn't really benefit us anyway.
export SDL_VIDEO_X11_DGAMOUSE=0

## - Works around a problem with misconfigured 64-bit systems not finding GL
export LIBGL_DRIVERS_PATH="${LIBGL_DRIVERS_PATH}":/usr/lib64/dri:/usr/lib32/dri:/usr/lib/dri

## - The 'scim' GTK IM module widely crashes the viewer.  Avoid it.
if [ "$GTK_IM_MODULE" = "scim" ]; then
    export GTK_IM_MODULE=xim
fi

EXECUTABLE="$(basename "$0")-bin"
SCRIPTSRC="$(readlink -f "$0" || echo "$0")"
RUN_PATH="$(dirname "${SCRIPTSRC}" || echo .)"
cd "${RUN_PATH}"

export LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH}"

# Run the program.
"./$EXECUTABLE" "$@"
exit $?
