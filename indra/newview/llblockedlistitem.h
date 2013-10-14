/**
 * @file llviewerobjectlistitem.h
 * @brief viewer object list item header file
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
#ifndef LLVIEWEROBJECTLISTITEM_H_
#define LLVIEWEROBJECTLISTITEM_H_

#include "llmutelist.h"
#include "llpanel.h"
#include "llstyle.h"
#include "lltextbox.h"
#include "lliconctrl.h"

/**
 * This class represents items of LLBlockList, which represents
 * contents of LLMuteList. LLMuteList "consists" of LLMute items.
 * Each LLMute represents either blocked avatar or object and
 * stores info about mute type (avatar or object)
 *
 * Each item consists if object/avatar icon and object/avatar name
 *
 * To create a blocked list item just need to pass LLMute pointer
 * and appropriate block list item will be created depending on
 * LLMute type (LLMute::EType) and other LLMute's info
 */
class LLBlockedListItem : public LLPanel
{
public:

	LLBlockedListItem(const LLMute* item);
	virtual BOOL postBuild();

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);

	virtual void setValue(const LLSD& value);

	void 					highlightName(const std::string& highlited_text);
	const std::string&		getName() const { return mItemName; }
	const LLMute::EType&	getType() const { return mMuteType; }
	const LLUUID&			getUUID() const { return mItemID;	}

private:

	LLTextBox*		mTitleCtrl;
	const LLUUID	mItemID;
	std::string		mItemName;
	LLMute::EType	mMuteType;

};

#endif /* LLVIEWEROBJECTLISTITEM_H_ */
