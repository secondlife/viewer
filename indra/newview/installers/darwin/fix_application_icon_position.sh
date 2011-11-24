# just run this script each time after you change the installer's name to fix the icon misalignment 
#!/bin/bash
cp -r ../../../../build-darwin-i386/newview/*.dmg ~/Desktop/TempBuild.dmg
hdid ~/Desktop/TempBuild.dmg
open -a finder /Volumes/Second\ Life\ Installer
osascript dmg-cleanup.applescript
umount /Volumes/Second\ Life\ Installer/
hdid ~/Desktop/TempBuild.dmg
open -a finder /Volumes/Second\ Life\ Installer
#cp /Volumes/Second\ Life\ Installer/.DS_Store ~/Desktop/_DS_Store
#chflags nohidden ~/Desktop/_DS_Store
#cp ~/Desktop/_DS_Store ./firstlook-dmg/_DS_Store
#cp ~/Desktop/_DS_Store ./publicnightly-dmg/_DS_Store
#cp ~/Desktop/_DS_Store ./release-dmg/_DS_Store
#cp ~/Desktop/_DS_Store ./releasecandidate-dmg/_DS_Store
#umount /Volumes/Second\ Life\ Installer/
#rm ~/Desktop/_DS_Store ~/Desktop/TempBuild.dmg
