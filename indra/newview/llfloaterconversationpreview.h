/**
 * @file llfloaterconversationpreview.h
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LLFLOATERCONVERSATIONPREVIEW_H_
#define LLFLOATERCONVERSATIONPREVIEW_H_

#include "llchathistory.h"
#include "llfloater.h"

class LLSpinCtrl;

class LLFloaterConversationPreview : public LLFloater
{
public:

	LLFloaterConversationPreview(const LLSD& session_id);
	virtual ~LLFloaterConversationPreview(){};

	virtual BOOL postBuild();

	virtual void draw();
	virtual void onOpen(const LLSD& key);

private:
	void onMoreHistoryBtnClick();
	void showHistory();

	LLSpinCtrl*		mPageSpinner;
	LLChatHistory*	mChatHistory;
	LLUUID			mSessionID;
	int				mCurrentPage;
	int				mPageSize;

	std::list<LLSD> mMessages;
};

#endif /* LLFLOATERCONVERSATIONPREVIEW_H_ */
