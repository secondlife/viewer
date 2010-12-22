/**
 * @file lltoastscripttextbox.h
 * @brief Panel for script llTextBox dialogs
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLTOASTSCRIPTTEXTBOX_H
#define LL_LLTOASTSCRIPTTEXTBOX_H

#include "lltoastnotifypanel.h"
#include "llnotificationptr.h"

/**
 * Toast panel for scripted llTextbox notifications.
 */
class LLToastScriptTextbox
:	public LLToastPanel
{
public:
	void close();

	static bool onNewNotification(const LLSD& notification);

	// Non-transient messages.  You can specify non-default button
	// layouts (like one for script dialogs) by passing various
	// numbers in for "layout".
	LLToastScriptTextbox(const LLNotificationPtr& notification);

	/*virtual*/ ~LLToastScriptTextbox();

private:

	void onClickSubmit();
	void onClickIgnore();

	static const S32 DEFAULT_MESSAGE_MAX_LINE_COUNT;
};

#endif
