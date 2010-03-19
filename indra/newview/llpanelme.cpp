/** 
 * @file llpanelme.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llpanelme.h"

// Viewer includes
#include "llpanelprofile.h"
#include "llavatarconstants.h"
#include "llagent.h"
#include "llagentwearables.h"
#include "llsidetray.h"
#include "llviewercontrol.h"

// Linden libraries
#include "llavatarnamecache.h"		// IDEVO
#include "llchat.h"					// IDEVO HACK
#include "lliconctrl.h"
#include "llnotificationsutil.h"	// IDEVO
#include "lltabcontainer.h"
#include "lltexturectrl.h"

#define PICKER_SECOND_LIFE "2nd_life_pic"
#define PICKER_FIRST_LIFE "real_world_pic"
#define PANEL_PROFILE "panel_profile"

static LLRegisterPanelClassWrapper<LLPanelMyProfileEdit> t_panel_me_profile_edit("edit_profile_panel");
static LLRegisterPanelClassWrapper<LLPanelMe> t_panel_me_profile("panel_me");

LLPanelMe::LLPanelMe(void) 
 : LLPanelProfile()
 , mEditPanel(NULL)
{
	setAvatarId(gAgent.getID());
}

BOOL LLPanelMe::postBuild()
{
	LLPanelProfile::postBuild();

	getTabContainer()[PANEL_PROFILE]->childSetAction("edit_profile_btn", boost::bind(&LLPanelMe::onEditProfileClicked, this), this);
	getTabContainer()[PANEL_PROFILE]->childSetAction("edit_appearance_btn", boost::bind(&LLPanelMe::onEditAppearanceClicked, this), this);

	return TRUE;
}

void LLPanelMe::onOpen(const LLSD& key)
{
	LLPanelProfile::onOpen(key);

	// Force Edit My Profile if this is the first time when user is opening Me Panel (EXT-5068)
	bool opened = gSavedSettings.getBOOL("MePanelOpened");
	// In some cases Side Tray my call onOpen() twice, check getCollapsed() to be sure this
	// is the last time onOpen() is called
	if( !opened && !LLSideTray::getInstance()->getCollapsed() )
	{
		buildEditPanel();
		openPanel(mEditPanel, getAvatarId());

		gSavedSettings.setBOOL("MePanelOpened", true);
	}
}

bool LLPanelMe::notifyChildren(const LLSD& info)
{
	if (info.has("task-panel-action") && info["task-panel-action"].asString() == "handle-tri-state")
	{
		// Implement task panel tri-state behavior.
		//
		// When the button of an active open task panel is clicked, side tray
		// calls notifyChildren() on the panel, passing task-panel-action=>handle-tri-state as an argument.
		// The task panel is supposed to handle this by reverting to the default view,
		// i.e. closing any dependent panels like "pick info" or "profile edit".

		bool on_default_view = true;

		const LLRect& task_panel_rect = getRect();
		for (LLView* child = getFirstChild(); child; child = findNextSibling(child))
		{
			LLPanel* panel = dynamic_cast<LLPanel*>(child);
			if (!panel)
				continue;

			// *HACK: implement panel stack instead (e.g. me->pick_info->pick_edit).
			if (panel->getRect().getWidth()  == task_panel_rect.getWidth()  &&
				panel->getRect().getHeight() == task_panel_rect.getHeight() &&
				panel->getVisible())
			{
				panel->setVisible(FALSE);
				on_default_view = false;
			}
		}
		
		if (on_default_view)
			LLSideTray::getInstance()->collapseSideBar();

		return true; // this notification is only supposed to be handled by task panels 
	}

	return LLPanel::notifyChildren(info);
}

void LLPanelMe::buildEditPanel()
{
	if (NULL == mEditPanel)
	{
		mEditPanel = new LLPanelMyProfileEdit();
		mEditPanel->childSetAction("save_btn", boost::bind(&LLPanelMe::onSaveChangesClicked, this), this);
		mEditPanel->childSetAction("cancel_btn", boost::bind(&LLPanelMe::onCancelClicked, this), this);
	}
}


void LLPanelMe::onEditProfileClicked()
{
	buildEditPanel();
	togglePanel(mEditPanel, getAvatarId()); // open
}

void LLPanelMe::onEditAppearanceClicked()
{
	if (gAgentWearables.areWearablesLoaded())
	{
		gAgent.changeCameraToCustomizeAvatar();
	}
}

void LLPanelMe::onSaveChangesClicked()
{
	LLAvatarData data = LLAvatarData();
	data.avatar_id = gAgent.getID();
	data.image_id = mEditPanel->getChild<LLTextureCtrl>(PICKER_SECOND_LIFE)->getImageAssetID();
	data.fl_image_id = mEditPanel->getChild<LLTextureCtrl>(PICKER_FIRST_LIFE)->getImageAssetID();
	data.about_text = mEditPanel->childGetValue("sl_description_edit").asString();
	data.fl_about_text = mEditPanel->childGetValue("fl_description_edit").asString();
	data.profile_url = mEditPanel->childGetValue("homepage_edit").asString();
	data.allow_publish = mEditPanel->childGetValue("show_in_search_checkbox");

	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesUpdate(&data);
	togglePanel(mEditPanel); // close
	onOpen(getAvatarId());
}

void LLPanelMe::onCancelClicked()
{
	togglePanel(mEditPanel); // close
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelMyProfileEdit::LLPanelMyProfileEdit() 
 : LLPanelMyProfile()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_edit_profile.xml");

	setAvatarId(gAgent.getID());
}

void LLPanelMyProfileEdit::onOpen(const LLSD& key)
{
	resetData();

	// Disable editing until data is loaded, or edited fields will be overwritten when data
	// is loaded.
	enableEditing(false);
	LLPanelMyProfile::onOpen(getAvatarId());
}

void LLPanelMyProfileEdit::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_PROPERTIES == type)
	{
		const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
		if(avatar_data && getAvatarId() == avatar_data->avatar_id)
		{
			// *TODO dzaporozhan
			// Workaround for ticket EXT-1099, waiting for fix for ticket EXT-1128
			enableEditing(true);
			processProfileProperties(avatar_data);
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
		}
	}
}

void LLPanelMyProfileEdit::processProfileProperties(const LLAvatarData* avatar_data)
{
	fillCommonData(avatar_data);

	// 'Home page' was hidden in LLPanelAvatarProfile::fillCommonData() to fix  EXT-4734
	// Show 'Home page' in Edit My Profile (EXT-4873)
	childSetVisible("homepage_edit", true);

	fillPartnerData(avatar_data);

	fillAccountStatus(avatar_data);

	childSetValue("show_in_search_checkbox", (BOOL)(avatar_data->flags & AVATAR_ALLOW_PUBLISH));

	// IDEVO - These fields do not seem to exist any more.
	//std::string full_name;
	//BOOL found = gCacheName->getFullName(avatar_data->avatar_id, full_name);
	//if (found)
	//{
	//	childSetTextArg("name_text", "[NAME]", full_name);
	//}
	LLAvatarNameCache::get(avatar_data->avatar_id,
		boost::bind(&LLPanelMyProfileEdit::onNameCache, this, _1, _2));
}

void LLPanelMyProfileEdit::onNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	getChild<LLUICtrl>("user_name")->setValue( av_name.mDisplayName );
	getChild<LLUICtrl>("user_slid")->setValue( av_name.mSLID );
}

BOOL LLPanelMyProfileEdit::postBuild()
{
	initTexturePickerMouseEvents();

	childSetTextArg("partner_edit_link", "[URL]", getString("partner_edit_link_url"));
	childSetTextArg("my_account_link", "[URL]", getString("my_account_link_url"));

	getChild<LLUICtrl>("set_name")->setCommitCallback(
		boost::bind(&LLPanelMyProfileEdit::onClickSetName, this));

	return LLPanelAvatarProfile::postBuild();
}
/**
 * Inits map with texture picker and appropriate edit icon.
 * Sets callbacks of Mouse Enter and Mouse Leave signals of Texture Pickers 
 */
void LLPanelMyProfileEdit::initTexturePickerMouseEvents()
{
	LLTextureCtrl* text_pic = getChild<LLTextureCtrl>(PICKER_SECOND_LIFE);	
	LLIconCtrl* text_icon = getChild<LLIconCtrl>("2nd_life_edit_icon");
	mTextureEditIconMap[text_pic->getName()] = text_icon;
	text_pic->setMouseEnterCallback(boost::bind(&LLPanelMyProfileEdit::onTexturePickerMouseEnter, this, _1));
	text_pic->setMouseLeaveCallback(boost::bind(&LLPanelMyProfileEdit::onTexturePickerMouseLeave, this, _1));
	text_icon->setVisible(FALSE);

	text_pic = getChild<LLTextureCtrl>(PICKER_FIRST_LIFE);
	text_icon = getChild<LLIconCtrl>("real_world_edit_icon");
	mTextureEditIconMap[text_pic->getName()] = text_icon;
	text_pic->setMouseEnterCallback(boost::bind(&LLPanelMyProfileEdit::onTexturePickerMouseEnter, this, _1));
	text_pic->setMouseLeaveCallback(boost::bind(&LLPanelMyProfileEdit::onTexturePickerMouseLeave, this, _1));
	text_icon->setVisible(FALSE);
}

void LLPanelMyProfileEdit::resetData()
{
	LLPanelMyProfile::resetData();

	//childSetTextArg("name_text", "[FIRST]", LLStringUtil::null);
	//childSetTextArg("name_text", "[LAST]", LLStringUtil::null);
	getChild<LLUICtrl>("user_name")->setValue( LLSD() );
	getChild<LLUICtrl>("user_slid")->setValue( LLSD() );
}

void LLPanelMyProfileEdit::onTexturePickerMouseEnter(LLUICtrl* ctrl)
{
	mTextureEditIconMap[ctrl->getName()]->setVisible(TRUE);
}
void LLPanelMyProfileEdit::onTexturePickerMouseLeave(LLUICtrl* ctrl)
{
	mTextureEditIconMap[ctrl->getName()]->setVisible(FALSE);
}

// IDEVO HACK
extern void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel);

void LLPanelMyProfileEdit::onCacheSetName(bool success,
										  const std::string& reason,
										  const LLSD& content)
{
	if (success)
	{
		// HACK: Use chat to invalidate names
		send_chat_from_viewer("refreshname", CHAT_TYPE_NORMAL, 0);

		// Re-fetch my name, as it may have been sanitized by the service
		LLAvatarNameCache::get(getAvatarId(),
			boost::bind(&LLPanelMyProfileEdit::onNameCache, this, _1, _2));
	}
	else
	{
		// JAMESDEBUG TODO: localize strings for reasons we couldn't
		// change the name
		LLNotificationsUtil::add("SetDisplayNameFailedGeneric");
		// TODO: SetDisplayNameFailedThrottle with [FREQUENCY]
		// TODO: SetDisplayNameFailedUnavailable
	}
}

void LLPanelMyProfileEdit::onDialogSetName(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLUUID agent_id = notification["payload"]["agent_id"];
		if (agent_id.isNull()) return;

		std::string display_name = response["display_name"].asString();
		LLAvatarNameCache::setDisplayName(agent_id, display_name,
			boost::bind(&LLPanelMyProfileEdit::onCacheSetName, this,
				_1, _2, _3));
	}
}

void LLPanelMyProfileEdit::onClickSetName()
{
	// IDEVO
	LLUUID agent_id = getAvatarId();
	std::string display_name;
	LLAvatarName av_name;
	if (LLAvatarNameCache::useDisplayNames()
		&& LLAvatarNameCache::get(agent_id, &av_name))
	{
		display_name = av_name.mDisplayName;
	}
	else
	{
		gCacheName->getFullName(agent_id, display_name);
	}

	if (!display_name.empty())
	{
		LLSD args;
		args["DISPLAY_NAME"] = display_name;
		LLSD payload;
		payload["agent_id"] = agent_id;
		LLNotificationsUtil::add("SetDisplayName", args, payload, 
			boost::bind(&LLPanelMyProfileEdit::onDialogSetName, this, _1, _2));
	}
}

void LLPanelMyProfileEdit::enableEditing(bool enable)
{
	childSetEnabled("2nd_life_pic", enable);
	childSetEnabled("real_world_pic", enable);
	childSetEnabled("sl_description_edit", enable);
	childSetEnabled("fl_description_edit", enable);
	childSetEnabled("homepage_edit", enable);
	childSetEnabled("show_in_search_checkbox", enable);
}
