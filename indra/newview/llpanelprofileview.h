/** 
* @file llpanelprofileview.h
* @brief Side tray "Profile View" panel
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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
