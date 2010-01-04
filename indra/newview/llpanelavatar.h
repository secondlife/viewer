/** 
 * @file llpanelavatar.h
 * @brief LLPanelAvatar and related class definitions
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELAVATAR_H
#define LL_LLPANELAVATAR_H

#include "llpanel.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"

class LLComboBox;
class LLLineEditor;
class LLToggleableMenu;

enum EOnlineStatus
{
	ONLINE_STATUS_NO      = 0,
	ONLINE_STATUS_YES     = 1
};

/**
* Base class for any Profile View or My Profile Panel.
*/
class LLPanelProfileTab
	: public LLPanel
	, public LLAvatarPropertiesObserver
{
public:

	/**
	 * Sets avatar ID, sets panel as observer of avatar related info replies from server.
	 */
	virtual void setAvatarId(const LLUUID& id);

	/**
	 * Returns avatar ID.
	 */
	virtual const LLUUID& getAvatarId() { return mAvatarId; }

	/**
	 * Sends update data request to server.
	 */
	virtual void updateData() = 0;

	/**
	 * Clears panel data if viewing avatar info for first time and sends update data request.
	 */
	virtual void onOpen(const LLSD& key);

	/**
	 * Profile tabs should close any opened panels here.
	 *
	 * Called from LLPanelProfile::onOpen() before opening new profile.
	 * See LLPanelPicks::onClosePanel for example. LLPanelPicks closes picture info panel
	 * before new profile is displayed, otherwise new profile will 
	 * be hidden behind picture info panel.
	 */
	virtual void onClosePanel() {}

	/**
	 * Resets controls visibility, state, etc.
	 */
	virtual void resetControls(){};

	/**
	 * Clears all data received from server.
	 */
	virtual void resetData(){};

	/*virtual*/ ~LLPanelProfileTab();

protected:

	LLPanelProfileTab();

	/**
	 * Scrolls panel to top when viewing avatar info for first time.
	 */
	void scrollToTop();

	virtual void onMapButtonClick();

	virtual void updateButtons();

private:

	LLUUID mAvatarId;
};

/**
* Panel for displaying Avatar's first and second life related info.
*/
class LLPanelAvatarProfile
	: public LLPanelProfileTab
	, public LLFriendObserver
{
public:
	LLPanelAvatarProfile();
	/*virtual*/ ~LLPanelAvatarProfile();

	/*virtual*/ void onOpen(const LLSD& key);

	/**
	 * LLFriendObserver trigger
	 */
	virtual void changed(U32 mask);

	/*virtual*/ void setAvatarId(const LLUUID& id);

	/**
	 * Processes data received from server.
	 */
	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void updateData();

	/*virtual*/ void resetControls();

	/*virtual*/ void resetData();

protected:

	/**
	 * Process profile related data received from server.
	 */
	virtual void processProfileProperties(const LLAvatarData* avatar_data);

	/**
	 * Processes group related data received from server.
	 */
	virtual void processGroupProperties(const LLAvatarGroups* avatar_groups);

	/**
	 * Fills common for Avatar profile and My Profile fields.
	 */
	virtual void fillCommonData(const LLAvatarData* avatar_data);

	/**
	 * Fills partner data.
	 */
	virtual void fillPartnerData(const LLAvatarData* avatar_data);

	/**
	 * Fills Avatar's online status.
	 */
	virtual void fillOnlineStatus(const LLAvatarData* avatar_data);

	/**
	 * Fills account status.
	 */
	virtual void fillAccountStatus(const LLAvatarData* avatar_data);

	/**
	 * Opens "Pay Resident" dialog.
	 */
	void pay();

	/**
	 * opens inventory and IM for sharing items
	 */
	void share();

	void kick();
	void freeze();
	void unfreeze();
	void csr();
	

	bool enableGod();


	void onUrlTextboxClicked(const std::string& url);
	void onHomepageTextboxClicked();
	void onAddFriendButtonClick();
	void onIMButtonClick();
	void onCallButtonClick();
	void onTeleportButtonClick();
	void onShareButtonClick();
	void onOverflowButtonClicked();

private:

	typedef std::map< std::string,LLUUID>	group_map_t;
	group_map_t 			mGroups;

	LLToggleableMenu*		mProfileMenu;
};

/**
 * Panel for displaying own first and second life related info.
 */
class LLPanelMyProfile
	: public LLPanelAvatarProfile
{
public:
	LLPanelMyProfile();

	/*virtual*/ BOOL postBuild();

protected:

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ void processProfileProperties(const LLAvatarData* avatar_data);

	/**
	 * Fills Avatar status data.
	 */
	virtual void fillStatusData(const LLAvatarData* avatar_data);

	/*virtual*/ void resetControls();

protected:

	void onStatusChanged();
	void onStatusMessageChanged();

private:

	LLComboBox* mStatusCombobox;
};

/**
* Panel for displaying Avatar's notes and modifying friend's rights.
*/
class LLPanelAvatarNotes 
	: public LLPanelProfileTab
	, public LLFriendObserver
{
public:
	LLPanelAvatarNotes();
	/*virtual*/ ~LLPanelAvatarNotes();

	virtual void setAvatarId(const LLUUID& id);

	/** 
	 * LLFriendObserver trigger
	 */
	virtual void changed(U32 mask);

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	/*virtual*/ void updateData();

protected:

	/*virtual*/ void resetControls();

	/*virtual*/ void resetData();

	/**
	 * Fills rights data for friends.
	 */
	void fillRightsData();

	void rightsConfirmationCallback(const LLSD& notification,
			const LLSD& response, S32 rights);
	void confirmModifyRights(bool grant, S32 rights);
	void onCommitRights();
	void onCommitNotes();

	void onAddFriendButtonClick();
	void onIMButtonClick();
	void onCallButtonClick();
	void onTeleportButtonClick();
	void onShareButtonClick();
};

#endif // LL_LLPANELAVATAR_H
