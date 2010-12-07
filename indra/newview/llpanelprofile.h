/** 
* @file llpanelprofile.h
* @brief Profile panel
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELPROFILE_H
#define LL_LLPANELPROFILE_H

#include "llpanel.h"
#include "llpanelavatar.h"

class LLTabContainer;

std::string getProfileURL(const std::string& agent_name);

/**
* Base class for Profile View and My Profile.
*/
class LLPanelProfile : public LLPanel
{
	LOG_CLASS(LLPanelProfile);

public:
	/*virtual*/ BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& key);

	virtual void togglePanel(LLPanel*, const LLSD& key = LLSD());

	virtual void openPanel(LLPanel* panel, const LLSD& params);

	virtual void closePanel(LLPanel* panel);

	S32 notifyParent(const LLSD& info);

protected:

	LLPanelProfile();

	virtual void onTabSelected(const LLSD& param);

	virtual void setAllChildrenVisible(BOOL visible);

	LLTabContainer* getTabCtrl() { return mTabCtrl; }

	const LLUUID& getAvatarId() { return mAvatarId; }

	void setAvatarId(const LLUUID& avatar_id) { mAvatarId = avatar_id; }

	typedef std::map<std::string, LLPanelProfileTab*> profile_tabs_t;

	profile_tabs_t& getTabContainer() { return mTabContainer; }

private:

	LLTabContainer* mTabCtrl;	
	profile_tabs_t mTabContainer;
	LLUUID mAvatarId;
};

#endif //LL_LLPANELPROFILE_H
