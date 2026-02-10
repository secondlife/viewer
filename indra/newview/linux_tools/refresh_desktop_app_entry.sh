#!/bin/bash

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`

install_prefix="$(realpath -- "${RUN_PATH}/..")"

function install_desktop_entry()
{
    local installation_prefix="$1"
    local desktop_entries_dir="$2"

    printf "Installing menu entries via XDG..."
	xdg-icon-resource install --novendor --size 256 "${installation_prefix}/secondlife_icon.png" "com.secondlife.SecondLifeViewer"
	#NOTE: Above command takes the path to the icon to install && The name of the icon to be used by XDG. This should always be in the format of "xViewer" to avoid potential naming conflicts, as per XDG spec.
	xdg-desktop-menu install --novendor "${installation_prefix}"/etc/com.secondlife.SecondLifeViewer.desktop

    xdg-desktop-menu forceupdate #Above command should update the menu system, but do it a second time just in case.
}

if [ "$UID" == "0" ]; then
    # system-wide
    install_desktop_entry "$install_prefix" /usr/local/share/applications
else
    # user-specific
    install_desktop_entry "$install_prefix" "$HOME/.local/share/applications"
fi
