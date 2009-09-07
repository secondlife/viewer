/**
 * @file lltoastgroupnotifypanel.h
 * @brief Panel for group notify toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLGROUPNOTIFY_H
#define LL_LLGROUPNOTIFY_H

#include "llfontgl.h"
#include "lltoastpanel.h"
#include "lldarray.h"
#include "lltimer.h"
#include "llviewermessage.h"
#include "llnotifications.h"

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
	LLToastGroupNotifyPanel(LLNotificationPtr& notification);

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
