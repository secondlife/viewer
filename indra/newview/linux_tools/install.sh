#!/bin/bash

# Install the Second Life Viewer. This script can install the viewer both
# system-wide and for an individual user.

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

    install_to_prefix "$HOME/.secondlife-install"
    $HOME/.secondlife-install/etc/refresh_desktop_app_entry.sh
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
    ${install_prefix}/etc/refresh_desktop_app_entry.sh
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

function check_media_dependencies()
{
    # libcef.so (used for in-viewer web media -- login page, prim/parcel
    # media) depends on NSS/NSPR at runtime but does not bundle them; this
    # matches CEF's own upstream distribution convention (treated as "the
    # OS already has these"), not a gap in this package. Most desktop Linux
    # installs already have them (Firefox depends on the same libraries),
    # but a minimal or server install may not, and the resulting failure is
    # silent -- the media component just exits almost instantly with no
    # media ever appearing, no obvious error shown to the user.
    command -v ldconfig >/dev/null 2>&1 || return 0

    local missing=""
    ldconfig -p | grep -q 'libnss3\.so'  || missing="${missing}libnss3 "
    ldconfig -p | grep -q 'libnspr4\.so' || missing="${missing}libnspr4 "

    if [ -n "$missing" ]; then
        warn "Missing system libraries needed for in-viewer web media: ${missing}"
        warn "Install them with your distro's package manager, e.g.:"
        warn "  sudo apt install libnss3 libnspr4      (Debian/Ubuntu)"
        warn "  sudo dnf install nss nspr              (Fedora)"
        warn "Without them, web media (login page, prim/parcel media) will not appear."
        echo
    fi
}

check_media_dependencies

if [ "$UID" == "0" ]; then
    root_install
else
    homedir_install
fi
