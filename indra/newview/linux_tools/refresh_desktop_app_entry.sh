#!/bin/bash

SCRIPTSRC=$(readlink -f "$0" || echo "$0")
RUN_PATH=$(dirname "${SCRIPTSRC}" || echo .)

install_prefix="$(realpath -- "${RUN_PATH}/..")"

build_data_file="${install_prefix}/build_data.json"
if [ -f "${build_data_file}" ]; then
    version=$(sed -n 's/.*"Version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${build_data_file}")
    channel_base=$(sed -n 's/.*"Channel Base"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${build_data_file}")
    channel=$(sed -n 's/.*"Channel"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${build_data_file}")
    desktopfilename=$(echo "$channel" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' )-viewer
else
    echo "Error: File ${build_data_file} not found." >&2
    exit 1
fi

# Check for the Release channel. This channel should not have the channel name in its launcher.
if [ "$channel" = "$channel_base Release" ]; then
    launcher_name="$channel_base"
else
    launcher_name=$channel
fi

function install_desktop_entry()
{
    local installation_prefix="${1}"
    local desktop_entries_dir="${2}"

    local desktop_entry="\
[Desktop Entry]\n\
Version=1.4\n\
Name=${launcher_name}\n\
GenericName=Second Life Viewer\n\
Comment=Client for the Online Virtual World, Second Life\n\
Path=${installation_prefix}\n\
Exec=${installation_prefix}/secondlife\n\
Icon=${desktopfilename}\n\
Terminal=false\n\
Type=Application\n\
Categories=Game;Simulation;\n\
StartupNotify=true\n\
StartupWMClass=\"com.secondlife.indra.viewer\"\n\
X-Desktop-File-Install-Version=3.0
PrefersNonDefaultGPU=true\n\
Actions=DefaultGPU;AssociateMIME;\n\
\n\
[Desktop Action DefaultGPU]\n\
Exec=env __GLX_VENDOR_LIBRARY_NAME=\"\" ${installation_prefix}/secondlife\n\
Name=Launch on default GPU\n\
\n\
[Desktop Action AssociateMIME]\n\
Exec=${installation_prefix}/etc/register_secondlifeprotocol.sh\n\
Name=Associate SLURLs"
#The above adds some options when the shortcut is right-clicked, to launch on the default (usually integrated) GPU, and to force MIME type association.

#The "PrefersNonDefaultGPU" line should automatically run the viewer on the most powerful GPU in the system, if it is not default. If it is, this is ignored.

# NOTE: DO NOT CHANGE THE "GenericName" FIELD - ONLY CHANGE THE "Name" FIELD. (This is to ensure that searching "Second Life" will show all the viewers installed in a user's system, regardless of their canonical name.)
    
    printf "Installing menu entries via XDG..."
    printf "%b" "${desktop_entry}" > "${installation_prefix}/${desktopfilename}".desktop || echo "Failed to install application menu!"
	xdg-icon-resource install --novendor --size 256 "${installation_prefix}/secondlife_icon.png" "${desktopfilename}"
	#NOTE: Above command takes the path to the icon to install && The name of the icon to be used by XDG. This should always be in the format of "x-Viewer" to avoid potential naming conflicts, as per XDG spec.
	xdg-desktop-menu install --novendor "${installation_prefix}"/"${desktopfilename}".desktop
}

if [ "$UID" == "0" ]; then
    # system-wide
    install_desktop_entry "$install_prefix" /usr/local/share/applications
else
    # user-specific
    install_desktop_entry "$install_prefix" "$HOME/.local/share/applications"
fi
