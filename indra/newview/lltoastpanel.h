/** 
 * @file lltoastpanel.h
 * @brief Creates a panel of a specific kind for a toast.
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

#ifndef LL_LLTOASTPANEL_H
#define LL_LLTOASTPANEL_H

#include "llpanel.h"
#include "lltextbox.h"
#include "llnotificationptr.h"

#include <string>

/**
 * Base class for all panels that can be added to the toast.
 * All toast panels should contain necessary logic for representing certain notification
 * but shouldn't contain logic related to this panel lifetime control and positioning
 * on the parent view.
 */
class LLToastPanel : public LLPanel {
public:
	LLToastPanel(const LLNotificationPtr&);
	virtual ~LLToastPanel() = 0;

	virtual std::string getTitle();
	virtual const LLUUID& getID();

	static const S32 MIN_PANEL_HEIGHT;

	/**
	 * Builder method for constructing notification specific panels.
	 * Normally type of created panels shouldn't be publicated and should be hidden
	 * from other functionality.
	 */
	static LLToastPanel* buidPanelFromNotification(
			const LLNotificationPtr& notification);
protected:
	LLNotificationPtr mNotification;
	void snapToMessageHeight(LLTextBase* message, S32 maxLineCount);
};

#endif /* LL_TOASTPANEL_H */
