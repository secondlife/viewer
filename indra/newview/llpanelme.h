/** 
 * @file llpanelme.h
 * @brief Side tray "Me" (My Profile) panel
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

#ifndef LL_LLPANELMEPROFILE_H
#define LL_LLPANELMEPROFILE_H

#include "llpanel.h"
#include "llpanelprofile.h"

class LLAvatarName;
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
	/*virtual*/ void onClose(const LLSD& key);

	void onAvatarNameChanged();

protected:	

	/*virtual*/void resetData();

	void processProfileProperties(const LLAvatarData* avatar_data);
	void onNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

private:
	void initTexturePickerMouseEvents();
	void onTexturePickerMouseEnter(LLUICtrl* ctrl);
	void onTexturePickerMouseLeave(LLUICtrl* ctrl);
	void onClickSetName();
	void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);

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
