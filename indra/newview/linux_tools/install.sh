#!/bin/bash

# Install the Second Life Viewer. This script can install the viewer both
# system-wide and for an individual user.

build_data_file="build_data.json"
if [ -f "${build_data_file}" ]; then
    version=$(sed -n 's/.*"Version"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${build_data_file}")
    channel=$(sed -n 's/.*"Channel"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${build_data_file}")
    installdir_name=$(echo "$channel" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' )-install
else
    echo "Error: File ${build_data_file} not found." >&2
    exit 1
fi

echo "Installing ${channel} version ${version}"

VT102_STYLE_NORMAL='\E[0m'
VT102_COLOR_RED='\E[31m'

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`
tarball_path=${RUN_PATH}

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
        install_to_prefix "$XDG_DATA_HOME/$installdir_name" #$XDG_DATA_HOME is a synonym for $HOME/.local/share/ unless the user has specified otherwise (unlikely).
    else
        install_to_prefix "$HOME/.local/share/$installdir_name" #XDG_DATA_HOME not set, so use default path as defined by XDG spec.
    fi
}

function root_install()
{
    local default_prefix="/opt/secondlife-install"

    echo -n "Enter the desired installation directory [${default_prefix}]: ";
    read
    if [[ "$REPLY" = "" ]] ; then
	local install_prefix=$default_prefix
    else
	local install_prefix=$REPLY
    fi

    install_to_prefix "$install_prefix"

    mkdir -p /usr/local/share/applications
}

function install_to_prefix()
{
    test -e "$1" && backup_previous_installation "$1"
    mkdir -p "$1" || die "Failed to create installation directory!"

    echo " - Installing to $1"

    cp -a "${tarball_path}"/* "$1/" || die "Failed to complete the installation!"
    
    "$1"/etc/refresh_desktop_app_entry.sh || echo "Failed to integrate into DE via XDG."
    set_slurl_handler "$1"
}

function backup_previous_installation()
{
    local backup_dir="$1".backup-$(date -I)
    echo " - Backing up previous installation to $backup_dir"

    mv "$1" "$backup_dir" || die "Failed to create backup of existing installation!"
}

set_slurl_handler()
{
    install_dir=$1
    echo
    prompt "Would you like to set Second Life as your default SLurl handler? [Y/N]: "
    if [ $? -eq 0 ]; then
	exit 0
    fi
    "$install_dir"/etc/register_secondlifeprotocol.sh #Successful association comes with a notification to the user.
}


if [ "$UID" == "0" ]; then
    root_install
else
    homedir_install
fi
