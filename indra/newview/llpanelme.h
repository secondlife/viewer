/** 
 * @file llpanelme.h
 * @brief Side tray "Me" (My Profile) panel
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

#ifndef LL_LLPANELMEPROFILE_H
#define LL_LLPANELMEPROFILE_H

#include "llpanel.h"
#include "llpanelavatar.h"

class LLPanelMyProfileEdit;
class LLPanelProfile;
class LLIconCtrl;

/**
* Panel for displaying Agent's profile, it consists of two sub panels - Profile
* and Picks. 
* LLPanelMe allows user to edit his profile and picks.
*/
class LLPanelMe : public LLPanelProfile
{
	LOG_CLASS(LLPanelMe);

public:

	LLPanelMe();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ bool notifyChildren(const LLSD& info);

	/*virtual*/ BOOL postBuild();

private:

	void buildEditPanel();

	void onEditProfileClicked();
	void onEditAppearanceClicked();
	void onSaveChangesClicked();
	void onCancelClicked();

	LLPanelMyProfileEdit *  mEditPanel;

};

class LLPanelMyProfileEdit : public LLPanelMyProfile
{
	LOG_CLASS(LLPanelMyProfileEdit);

public:

	LLPanelMyProfileEdit();

	/*virtual*/void processProperties(void* data, EAvatarProcessorType type);
	
	/*virtual*/BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& key);

protected:	

	/*virtual*/void resetData();

	void processProfileProperties(const LLAvatarData* avatar_data);

private:
	void initTexturePickerMouseEvents();
	void onTexturePickerMouseEnter(LLUICtrl* ctrl);
	void onTexturePickerMouseLeave(LLUICtrl* ctrl);
	void onClickSetName();

	/**
	 * Enabled/disables controls to prevent overwriting edited data upon receiving
	 * current data from server.
	 */
	void enableEditing(bool enable);

private:
	// map TexturePicker name => Edit Icon pointer should be visible while hovering Texture Picker
	typedef std::map<std::string, LLIconCtrl*> texture_edit_icon_map_t;
	texture_edit_icon_map_t mTextureEditIconMap;
};

#endif // LL_LLPANELMEPROFILE_H
