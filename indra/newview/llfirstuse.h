/** 
 * @file llfirstuse.h
 * @brief Methods that spawn "first-use" dialogs.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
*/

class LLFirstUse
{
public:
/*	
	// Add a config variable to be reset on resetFirstUse()
	static void addConfigVariable(const std::string& var);
	
	// Sets all controls back to show the dialogs.
	static void disableFirstUse();
	static void resetFirstUse();

	// These methods are called each time the appropriate action is
	// taken.  The functions themselves handle only showing the dialog
	// the first time, or subsequent times if the user wishes.
	static void useBalanceIncrease(S32 delta);
	static void useBalanceDecrease(S32 delta);
	static void useSit();
	static void useMap();
	static void useGoTo();
	static void useBuild();
//	static void useLeftClickNoHit();
	static void useTeleport();
*/ 
	static void useOverrideKeys();
/*	
	static void useAttach();
	static void useAppearance();
	static void useInventory();
 */
	static void useSandbox();
	
/*
	static void useFlexible();
	static void useDebugMenus();
	static void useSculptedPrim();
	static void useMedia();
	
protected:
	static std::set<std::string> sConfigVariables;
*/
};

#endif
// EOF
