/**
 * @file lltoastimpanel.h
 * @brief Panel for IM toasts.
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

#ifndef LLTOASTIMPANEL_H_
#define LLTOASTIMPANEL_H_


#include "lltoastpanel.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llavatariconctrl.h"

class LLGroupIconCtrl;

class LLToastIMPanel: public LLToastPanel 
{
public:
	struct Params
	{
		LLNotificationPtr	notification;
		LLUUID				avatar_id,
							session_id;
		std::string			from,
							time,
							message;

		Params() {}
	};

	LLToastIMPanel(LLToastIMPanel::Params &p);
	virtual ~LLToastIMPanel();
	/*virtual*/ BOOL 	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleToolTip(S32 x, S32 y, MASK mask);
private:
	void showInspector();

	void spawnNameToolTip();
	void spawnGroupIconToolTip();

	void initIcon();

	static const S32 DEFAULT_MESSAGE_MAX_LINE_COUNT;

	LLNotificationPtr	mNotification;
	LLUUID				mSessionID;
	LLUUID				mAvatarID;
	LLAvatarIconCtrl*	mAvatarIcon;
	LLGroupIconCtrl*	mGroupIcon;
	LLAvatarIconCtrl*	mAdhocIcon;
	LLTextBox*			mAvatarName;
	LLTextBox*			mTime;
	LLTextBox*			mMessage;
};

#endif // LLTOASTIMPANEL_H_


