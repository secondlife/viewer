#!/bin/bash
# just run this script each time after you change the installer's name to fix the icon misalignment 
mydir="$(dirname "$0")"
# If there's more than one DMG in more than one build directory, pick the most
# recent one.
dmgfile="$(ls -t "$mydir/../../../../build-darwin-*/newview/*.dmg" | head -n 1)"
dmgwork="$HOME/Desktop/TempBuild.dmg"
mounted="/Volumes/Second Life Installer"
cp -r "$dmgfile" "$dmgwork"
hdid "$dmgwork"
open -a finder "$mounted"
osascript "$mydir/dmg-cleanup.applescript"
umount "$mounted"/
hdid "$dmgwork"
open -a finder "$mounted"
#cp "$mounted"/.DS_Store ~/Desktop/_DS_Store
#chflags nohidden ~/Desktop/_DS_Store
#cp ~/Desktop/_DS_Store "$mydir/firstlook-dmg/_DS_Store"
#cp ~/Desktop/_DS_Store "$mydir/publicnightly-dmg/_DS_Store"
#cp ~/Desktop/_DS_Store "$mydir/release-dmg/_DS_Store"
#cp ~/Desktop/_DS_Store "$mydir/releasecandidate-dmg/_DS_Store"
#umount "$mounted"/
#rm ~/Desktop/_DS_Store "$dmgwork"
