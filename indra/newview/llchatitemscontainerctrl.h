/** 
 * @file llchatitemscontainerctrl.h
 * @brief chat history scrolling panel implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLCHATITEMSCONTAINERCTRL_H_
#define LL_LLCHATITEMSCONTAINERCTRL_H_

#include "llchat.h"
#include "llpanel.h"
#include "llscrollbar.h"
#include "llviewerchat.h"
#include "lltoastpanel.h"

typedef enum e_show_item_header
{
	CHATITEMHEADER_SHOW_ONLY_NAME = 0,
	CHATITEMHEADER_SHOW_ONLY_ICON = 1,
	CHATITEMHEADER_SHOW_BOTH
} EShowItemHeader;

class LLNearbyChatToastPanel : public LLPanel
{
protected:
        LLNearbyChatToastPanel()
		: 
	mIsDirty(false),
	mSourceType(CHAT_SOURCE_OBJECT)
	{};
public:
	~LLNearbyChatToastPanel(){}
	
	static LLNearbyChatToastPanel* createInstance();

	const LLUUID& getFromID() const { return mFromID;}
	const std::string& getFromName() const { return mFromName; }
	
	//void	addText		(const std::string& message ,  const LLStyle::Params& input_params = LLStyle::Params());
	//void	setMessage	(const LLChat& msg);
	void	snapToMessageHeight	();

	bool	canAddText	();

	void	onMouseLeave	(S32 x, S32 y, MASK mask);
	void	onMouseEnter	(S32 x, S32 y, MASK mask);
	BOOL	handleMouseDown	(S32 x, S32 y, MASK mask);
	BOOL	handleMouseUp	(S32 x, S32 y, MASK mask);

	virtual BOOL postBuild();

	void	reshape		(S32 width, S32 height, BOOL called_from_parent = TRUE);

	void	setHeaderVisibility(EShowItemHeader e);
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);

	virtual void init(LLSD& data);
	virtual void addMessage(LLSD& data);

	virtual void draw();

	//*TODO REMOVE, why a dup of getFromID?
	const LLUUID&	messageID() const { return mFromID;}
private:
	LLUUID			mFromID;	// agent id or object id
	std::string		mFromName;
	EChatSourceType	mSourceType;
	


	bool mIsDirty;
};


#endif


