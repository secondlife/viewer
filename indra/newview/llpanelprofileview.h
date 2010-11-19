/** 
* @file llpanelprofileview.h
* @brief Side tray "Profile View" panel
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

#ifndef LL_LLPANELPROFILEVIEW_H
#define LL_LLPANELPROFILEVIEW_H

#include "llpanel.h"
#include "llpanelprofile.h"
#include "llavatarpropertiesprocessor.h"
#include "llagent.h"
#include "lltooldraganddrop.h"

class LLAvatarName;
class LLPanelProfile;
class LLPanelProfileTab;
class LLTextBox;
class AvatarStatusObserver;

/**
* Panel for displaying Avatar's profile. It consists of three sub panels - Profile,
* Picks and Notes.
*/
class LLPanelProfileView : public LLPanelProfile
{
	LOG_CLASS(LLPanelProfileView);
	friend class LLUICtrlFactory;
	friend class AvatarStatusObserver;

public:

	LLPanelProfileView();

	/*virtual*/ ~LLPanelProfileView();

	/*virtual*/ void onOpen(const LLSD& key);
	
	/*virtual*/ BOOL postBuild();

	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg)
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(getAvatarId(), gAgent.getSessionID(), drop,
				 cargo_type, cargo_data, accept);

		return TRUE;
	}


protected:

	void onBackBtnClick();
	void onCopyToClipboard();
	bool isGrantedToSeeOnlineStatus();

	/**
	 * Displays avatar's online status if possible.
	 *
	 * Requirements from EXT-3880:
	 * For friends:
	 * - Online when online and privacy settings allow to show
	 * - Offline when offline and privacy settings allow to show
	 * - Else: nothing
	 * For other avatars:
	 *  - Online when online and was not set in Preferences/"Only Friends & Groups can see when I am online"
	 *  - Else: Offline
	 */
	void updateOnlineStatus();
	void processOnlineStatus(bool online);

private:
	// LLCacheName will call this function when avatar name is loaded from server.
	// This is required to display names that have not been cached yet.
//	void onNameCache(
//		const LLUUID& id, 
//		const std::string& full_name,
//		bool is_group);
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

	LLTextBox* mStatusText;
	AvatarStatusObserver* mAvatarStatusObserver;
};

#endif //LL_LLPANELPROFILEVIEW_H
