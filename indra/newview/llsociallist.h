/** 
* @file   llsociallist.h
* @brief  Header file for llsociallist
* @author Gilbert@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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
#ifndef LL_LLSOCIALLIST_H
#define LL_LLSOCIALLIST_H

#include "llflatlistview.h"
#include "llstyle.h"


/**
 * Generic list of avatars.
 * 
 * Updates itself when it's dirty, using optional name filter.
 * To initiate update, modify the UUID list and call setDirty().
 * 
 * @see getIDs()
 * @see setDirty()
 * @see setNameFilter()
 */

class LLAvatarIconCtrl;
class LLIconCtrl;
class LLOutputMonitorCtrl;

class LLSocialList : public LLFlatListViewEx
{
public:

	struct Params : public LLInitParam::Block<Params, LLFlatListViewEx::Params>
	{
	};

	LLSocialList(const Params&p);
	virtual	~LLSocialList();

	virtual void draw();
	void refresh();
	void addNewItem(const LLUUID& id, const std::string& name, BOOL is_online, EAddPosition pos = ADD_BOTTOM);



	std::string mNameFilter;
};

class LLSocialListItem : public LLPanel
{
	public:
	LLSocialListItem();
	~LLSocialListItem();

	BOOL postBuild();
	void setName(const std::string& name, const std::string& highlight = LLStringUtil::null);
	void setValue(const LLSD& value);
	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);

	LLTextBox * mLabelTextBox;
	std::string mLabel;
	LLStyle::Params mLabelTextBoxStyle;


	LLAvatarIconCtrl * mIcon;
	LLTextBox * mLastInteractionTime;
	LLIconCtrl * mIconPermissionOnline;
	LLIconCtrl * mIconPermissionMap;
	LLIconCtrl * mIconPermissionEditMine;
	LLIconCtrl * mIconPermissionEditTheirs;
	LLOutputMonitorCtrl * mSpeakingIndicator;
	LLButton * mInfoBtn;
	LLButton * mProfileBtn;
};	


#endif // LL_LLSOCIALLIST_H
