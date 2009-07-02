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
#include "v3dmath.h"
#include "lluuid.h"
#include "llwebbrowserctrl.h"

#include "llavatarpropertiesprocessor.h"

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLDropTarget;
class LLInventoryItem;
class LLLineEditor;
class LLNameEditor;
class LLPanelAvatar;
class LLScrollListCtrl;
class LLTabContainer;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLUICtrl;
class LLViewerImage;
class LLViewerObject;
class LLMessageSystem;
class LLIconCtrl;
class LLWebBrowserCtrl;
class LLVector3d;
class LLFloaterReg;

class LLPanelMeProfile;
class LLPanelPick;
class LLAgent;

enum EOnlineStatus
{
	ONLINE_STATUS_NO      = 0,
	ONLINE_STATUS_YES     = 1
};


class LLPanelProfileTab 
	: public LLPanel
	, public LLAvatarPropertiesObserver
{
public:
	
	LLPanelProfileTab(const LLUUID& avatar_id);
	LLPanelProfileTab(const Params& params );

	void setAvatarId(const LLUUID& avatar_id);

	const LLUUID& getAvatarId(){return mAvatarId;}

	virtual void updateData() = 0;
	
	virtual void onActivate(const LLUUID& id);

	typedef enum e_profile_type
	{
		PT_UNKNOWN,
		PT_OWN,
		PT_OTHER
	} EProfileType;

	virtual void onAddFriend();

	virtual void onIM();

	virtual void onTeleport();

	virtual void clear(){};

protected:
	virtual ~LLPanelProfileTab();
	void setProfileType();

private:
	void scrollToTop();

protected:
	e_profile_type mProfileType;
	LLUUID mAvatarId;
};

class LLPanelAvatarProfile 
	: public LLPanelProfileTab
{
public:
	LLPanelAvatarProfile(const LLUUID& avatar_id = LLUUID::null);
	LLPanelAvatarProfile(const Params& params );
	~LLPanelAvatarProfile();
	
	static void* create(void* data);

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void updateData();

	void clear();

	virtual void clearControls();

	/*virtual*/ BOOL postBuild(void);
	/*virtual*/ void onOpen(const LLSD& key);

	void onAddFriendButtonClick();

	void onIMButtonClick();

	void onCallButtonClick();

	void onTeleportButtonClick();

	void onShareButtonClick();

private:
	bool isOwnProfile(){return PT_OWN == mProfileType;}
	bool isEditMode(){return mEditMode;}
	void updateChildrenList();
	void onStatusChanged();
	void onStatusMessageChanged();
	void setCaptionText(const LLAvatarData* avatar_data);

	static void onUrlTextboxClicked(std::string url);
	void onHomepageTextboxClicked();
	void onUpdateAccountTextboxClicked();
	void onMyAccountTextboxClicked();
	void onPartnerEditTextboxClicked();

protected:
	bool mEditMode;

private:
	bool mUpdated;
	LLComboBox * mStatusCombobox;
	LLLineEditor * mStatusMessage;
};


class LLPanelAvatarNotes 
	: public LLPanelProfileTab
{
public:
	LLPanelAvatarNotes(const LLUUID& id = LLUUID::null);
	LLPanelAvatarNotes(const Params& params );
	~LLPanelAvatarNotes();

	static void* create(void* data);

	void onActivate(const LLUUID& id);

	BOOL postBuild(void);

	void onCommitRights();

	void onCommitNotes();

	void clear();

	void processProperties(void* data, EAvatarProcessorType type);

	void updateData();

protected:

	void updateChildrenList();
};



// helper funcs
void add_left_label(LLPanel *panel, const std::string& name, S32 y);

#endif // LL_LLPANELAVATAR_H
