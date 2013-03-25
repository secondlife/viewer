/**
 * @file llconversationloglistitem.h
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

#ifndef LLCONVERSATIONLOGLISTITEM_H_
#define LLCONVERSATIONLOGLISTITEM_H_

#include "llfloaterimsession.h"
#include "llpanel.h"

class LLTextBox;
class LLConversation;

/**
 * This class is a visual representation of LLConversation, each of which is LLConversationLog entry.
 * LLConversationLogList consists of these LLConversationLogListItems.
 * LLConversationLogListItem consists of:
 *		conversaion_type_icon
 *		conversaion_name
 *		conversaion_date
 * Also LLConversationLogListItem holds pointer to its LLConversationLog.
 */

class LLConversationLogListItem : public LLPanel
{
public:
	LLConversationLogListItem(const LLConversation* conversation);
	virtual ~LLConversationLogListItem();

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);

	virtual void setValue(const LLSD& value);

	virtual BOOL postBuild();

	void onIMFloaterShown(const LLUUID& session_id);
	void onRemoveBtnClicked();

	const LLConversation* getConversation() const { return mConversation; }

	void highlightNameDate(const std::string& highlited_text);

	void onDoubleClick();

	/**
	 * updates string value of last interaction time from conversation
	 */
	void updateTimestamp();
	void updateName();
	void updateOfflineIMs();

private:

	void initIcons();

	const LLConversation* mConversation;

	LLTextBox*		mConversationName;
	LLTextBox*		mConversationDate;

	boost::signals2::connection mIMFloaterShowedConnection;
};

#endif /* LLCONVERSATIONLOGITEM_H_ */
