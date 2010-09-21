-- First, convert the disk image to "read-write" format with Disk Utility or hdiutil
-- Mount the image, open the disk image window in the Finder and make it frontmost, then run this script from inside Script Editor
-- After running the script, unmount the disk image, re-mount it, and copy the .DS_Store file off from the command line.

tell application "Finder"
	
	set foo to every item in front window
	repeat with i in foo
		if the name of i is "Applications" then
			set the position of i to {391, 165}
		else if the name of i ends with ".app" then
			set the position of i to {121, 166}
		end if
	end repeat
	
	-- There doesn't seem to be a way to set the background picture with applescript, but all the saved .DS_Store files should already have that set correctly.
	
	set foo to front window
	set current view of foo to icon view
	set toolbar visible of foo to false
	set statusbar visible of foo to false
	set the bounds of foo to {100, 100, 600, 399}
	
	-- set the position of front window to {100, 100}
	-- get {name, position} of every item of front window
	
	get properties of front window
end tell
