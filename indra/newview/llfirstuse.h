/** 
 * @file llfirstuse.h
 * @brief Methods that spawn "first-use" dialogs.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLFIRSTUSE_H
#define LL_LLFIRSTUSE_H

#include <vector>
#include "llstring.h"

/*
1.  On first use of 'sit here', explain how to get up and rotate view. 

2.  On first use of map, explain dbl-click = telport, how hubs/beacons work,
click-drag to move map

3.  First use of pie 'Go To', explain other ways to move around 

4.  First use of 'Create' or 'Edit', explain build toolbar, that you can create
things if build enabled, edit things you own, and that you can ESC to exit it. 

5.  First use of 'Talk to' explain difference between that and regular chat,
reduced range, how to leave conversation, arrow keys to orbit. 

6.  First lft-click that does nothing (land,object):  Explain that rgt-click
gives menu, lft-click interacts or moves if physical

7.  On first receipt of L$ (not rez/derez) explain that objects or people may
give you L$, and how to give someone or something money ('Pay...'). 

8.  After first teleporting and being sent to nearest hub, a dialog explaining
how to find and move toward the beacon. 

9.  On first accept/auto-accept permissions, explain that some objects may be
activated by entering mouselook 'M', or may override your movement keys with
other functions. 

10.  FIrst use of 'wear' or drag object from inventory onto self: 'You can
attach objects to your body by dragging ontl yourelf of rgt-clk->wear from
object or from inventory.

11.  FIrst time you run the client on a system without QuickTime installed.

12. First time you create a flexible object.

13. First time you open the debug menus (ctrl-alt-shift D)

14. First time you create/edit a sculpted prim.

15. Explanation of Speak&flyout buttons.
*/

class LLNotification;


class LLFirstUse
{
public:

	// Add a config variable to be reset on resetFirstUse()
	static void addConfigVariable(const std::string& var);
	
	// Sets all controls back to show the dialogs.
	static void disableFirstUse();
	static void resetFirstUse();

	static void otherAvatarChatFirst(bool enable = true);
	static void speak(bool enable = true);
	static void sit(bool enable = true);
	static void notUsingDestinationGuide(bool enable = true);
	static void notUsingSidePanel(bool enable = true);
	static void notMoving(bool enable = true);
	static void viewPopup(bool enable = true);
	static void newInventory(bool enable = true);
	static void receiveLindens(bool enable = true);
	static void setDisplayName(bool enable = true);
	static void useSandbox();
	
protected:
	static void firstUseNotification(const std::string& control_var, bool enable, const std::string& notification_name, LLSD args = LLSD(), LLSD payload = LLSD());
	static std::set<std::string> sConfigVariables;

	static void init();
	static bool processNotification(const LLSD& notify);
};

#endif
// EOF
