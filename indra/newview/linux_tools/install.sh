#!/bin/bash

# Install the Second Life Viewer. This script can install the viewer both
# system-wide and for an individual user.

echo "Installing ${channel} version ${version}"

VT102_STYLE_NORMAL='\E[0m'
VT102_COLOR_RED='\E[31m'

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`
tarball_path=${RUN_PATH}

build_data_file="${RUN_PATH}/build_data.json"
if [ -f "${build_data_file}" ]; then
    version=$(sed -n 's/.*"Version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${build_data_file}")
    channel=$(sed -n 's/.*"Channel"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${build_data_file}")
    installdir_name=$(echo "$channel" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' )-install
else
    echo "Error: File ${build_data_file} not found." >&2
    exit 1
fi

function prompt()
{
    local prompt=$1
    local input

    echo -n "$prompt"

    while read input; do
        case $input in
            [Yy]* )
                return 1
                ;;
            [Nn]* )
                return 0
                ;;
            * )
                echo "Please enter yes or no."
                echo -n "$prompt"
        esac
    done
}

function die()
{
    warn $1
    exit 1
}

function warn()
{
    echo -n -e $VT102_COLOR_RED
    echo $1
    echo -n -e $VT102_STYLE_NORMAL
}

function homedir_install()
{
    warn "You are not running as a privileged user, so you will only be able"
    warn "to install the Second Life Viewer in your home directory. If you"
    warn "would like to install the Second Life Viewer system-wide, please run"
    warn "this script as the root user, or with the 'sudo' command."
    echo

    prompt "Proceed with the installation? [Y/N]: "
    if [[ $? == 0 ]]; then
	exit 0
    fi

    if [ -d "$XDG_DATA_HOME" ] ; then
        local install_prefix="$XDG_DATA_HOME/$installdir_name" #$XDG_DATA_HOME is a synonym for $HOME/.local/share/ unless the user has specified otherwise (unlikely).
    else
        local install_prefix="$HOME/.local/share/$installdir_name"
    fi
    install_to_prefix "$install_prefix"
    update_desktop_entry "$install_prefix"
}

function root_install()
{
    local default_prefix="/opt/$installdir_name"

    echo -n "Enter the desired installation directory [${default_prefix}]: ";
    read
    if [[ "$REPLY" = "" ]] ; then
	local install_prefix=$default_prefix
    else
	local install_prefix=$REPLY
    fi

    install_to_prefix "$install_prefix"

    mkdir -p /usr/local/share/applications
    update_desktop_entry "$install_prefix"
}

function install_to_prefix()
{
    test -e "$1" && backup_previous_installation "$1"
    mkdir -p "$1" || die "Failed to create installation directory!"

    echo " - Installing to $1"

    cp -a "${tarball_path}"/* "$1/" || die "Failed to complete the installation!"
}

function backup_previous_installation()
{
    local backup_dir="$1".backup-$(date -I)
    echo " - Backing up previous installation to $backup_dir"

    mv "$1" "$backup_dir" || die "Failed to create backup of existing installation!"
}

#Below function is not currently used as the desktop environment should prompt the user to associate SLURLs upon first use following installation.
function set_slurl_handler()
{
    local install_prefix=$1
    echo
    prompt "Would you like to set Second Life as your default SLurl handler? [Y/N]: "
    if [ $? -eq 0 ]; then
	exit 0
    fi
    "${install_prefix}"/etc/register_secondlifeprotocol.sh #Should prompt the desktop environment to set association. Normally not needed as it will prompt upon the first use of a SLURL after installation.
}

function update_desktop_entry()
{
    local install_prefix=$1
    sed -i "s|@INSTALLATION_PREFIX@|$install_prefix|g" "$install_prefix/etc/com.secondlife.SecondLifeViewer.desktop"
    "${install_prefix}"/etc/refresh_desktop_app_entry.sh
}

if [ "$UID" == "0" ]; then
    root_install
else
    homedir_install
fi
