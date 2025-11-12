#!/bin/env sh

# Register a protocol handler (default: handle_secondlifeprotocol.sh) for
# URLs of the form secondlife://...
#

desired_handler="${1}"

print() {
    log_prefix="RegisterSLProtocol:"
    printf "%s %s\n" "${log_prefix}" "$*"
}
run_path=$(dirname "$0" || echo .)
cd "${run_path}/.." || exit

if [ -z "${desired_handler}" ]; then
    desired_handler="$(pwd)/etc/handle_secondlifeprotocol.sh"
fi

# Ensure the handle_secondlifeprotocol.sh file is executeable (otherwise, xdg-mime won't work)
chmod +x "$desired_handler"

# Check if xdg-mime is present, if so, use it to register new protocol.
if command -v xdg-mime >/dev/null 2>&1; then
    urlhandler=$(xdg-mime query default x-scheme-handler/secondlife)
    localappdir="${HOME}/.local/share/applications"
    newhandler="secondlifeprotocol_$(basename "${PWD%}").desktop"
    handlerpath="${localappdir}/${newhandler}"
    cat >"${handlerpath}" <<EOFnew || print "Warning: Did not register secondlife:// handler with xdg-mime: Could not write $newhandler"
[Desktop Entry]
Version=1.4
Name="Second Life URL handler"
Comment="secondlife:// URL handler"
Type=Application
Exec=$desired_handler %u
Terminal=false
StartupNotify=true
NoDisplay=true
MimeType=x-scheme-handler/secondlife
EOFnew

    if [ -z "${urlhandler}" ]; then
        print "No SLURL handler currently registered, creating new..."
    else
        #xdg-mime uninstall $localappdir/$urlhandler
        #Clean up handlers from other viewers
        if [ "${urlhandler}" != "${newhandler}" ]; then
            print "Current SLURL Handler: ${urlhandler} - Setting ${newhandler} as the new default..."
            #mv "${localappdir}"/"${urlhandler}" "${localappdir}"/"${urlhandler}".bak #Old method, now replaced with XDG.
            xdg-desktop-menu install --novendor "${localappdir}"/"$urlhandler"
        else
            print "SLURL Handler has not changed, leaving as-is."
        fi
    fi
    xdg-mime default "${newhandler}" x-scheme-handler/secondlife
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "${localappdir}"
        print "Registered ${desired_handler} as secondlife:// protocol handler with xdg-mime."
    else
        print "Warning: Cannot update desktop database, command missing - installation may be incomplete."
    fi
else
    print "Warning: Did not register secondlife:// handler with xdg-mime: Package not found."
    #If XDG is not present, then do not handle, just warn.
fi
notify-send -t 5000 -i info "Second Life URL Handler" "SLURL association created"
