#!/bin/bash

# Register a protocol handler (default: handle_secondlifeprotocol.sh) for
# URLs of the form secondlife://...
#

HANDLER="$1"

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`

install_prefix="$(realpath -- "${RUN_PATH}/..")"

cd "${RUN_PATH}/.."

if [ -z "$HANDLER" ]; then
    HANDLER=$install_prefix/etc/handle_secondlifeprotocol.sh
fi

function install_desktop_entry()
{
    local installation_prefix="$1"
    local desktop_entries_dir="$2"

    local desktop_entry="\
[Desktop Entry]\n\
Name=Second Life SLURL handler\n\
Path=${installation_prefix}\n\
Exec=${HANDLER} %u\n\
Icon=${installation_prefix}/secondlife_icon.png\n\
Terminal=false\n\
Type=Application\n\
StartupNotify=true\n\
StartupWMClass="com.secondlife.indra.viewer"\n\
NoDisplay=true\n\
MimeType=x-scheme-handler/secondlife\n\
X-Desktop-File-Install-Version=3.0"

    echo " - Installing protocol entries in ${desktop_entries_dir}"
    WORK_DIR=`mktemp -d`
    PROTOCOL_HANDLER="secondlife-protocol.desktop"
    echo -e $desktop_entry > "${WORK_DIR}/${PROTOCOL_HANDLER}" || "Failed to create desktop file!"
    desktop-file-install --dir="${desktop_entries_dir}" "${WORK_DIR}/${PROTOCOL_HANDLER}" || "Failed to install desktop file!"
    rm -r $WORK_DIR

    xdg-mime default "${desktop_entries_dir}/${PROTOCOL_HANDLER}" x-scheme-handler/secondlife

    update-desktop-database "${desktop_entries_dir}"
}

if [ "$UID" == "0" ]; then
    # system-wide
    install_desktop_entry "$install_prefix" /usr/local/share/applications
else
    # user-specific
    install_desktop_entry "$install_prefix" "$HOME/.local/share/applications"
fi
