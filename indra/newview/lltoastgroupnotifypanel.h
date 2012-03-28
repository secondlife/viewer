/**
 * @file lltoastgroupnotifypanel.h
 * @brief Panel for group notify toasts.
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

#ifndef LL_LLGROUPNOTIFY_H
#define LL_LLGROUPNOTIFY_H

#include "llfontgl.h"
#include "lltoastpanel.h"
#include "lldarray.h"
#include "lltimer.h"
#include "llviewermessage.h"
#include "llnotificationptr.h"

class LLButton;

/**
 * Toast panel for group notification.
 *
 * Replaces class LLGroupNotifyBox.
 */
class LLToastGroupNotifyPanel
:	public LLToastPanel
{
public:
	void close();

	static bool onNewNotification(const LLSD& notification);


	// Non-transient messages.  You can specify non-default button
	// layouts (like one for script dialogs) by passing various
	// numbers in for "layout".
	LLToastGroupNotifyPanel(const LLNotificationPtr& notification);

	/*virtual*/ ~LLToastGroupNotifyPanel();
protected:
	void onClickOk();
	void onClickAttachment();
private:
	static bool isAttachmentOpenable(LLAssetType::EType);

	static const S32 DEFAULT_MESSAGE_MAX_LINE_COUNT;

	LLButton* mSaveInventoryBtn;

	LLUUID mGroupID;
	LLOfferInfo* mInventoryOffer;
};

// This view contains the stack of notification windows.
//extern LLView* gGroupNotifyBoxView;

const S32 GROUP_LAYOUT_DEFAULT = 0;
const S32 GROUP_LAYOUT_SCRIPT_DIALOG = 1;

#endif
