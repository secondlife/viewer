/** 
 * @file llpanelme.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llpanelme.h"

// Viewer includes
#include "llpanelprofile.h"
#include "llavatarconstants.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llhints.h"
#include "llsidetray.h"
#include "llviewercontrol.h"
#include "llviewerdisplayname.h"

// Linden libraries
#include "llavatarnamecache.h"		// IDEVO
#include "lliconctrl.h"
#include "llnotifications.h"
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

void LLPanelMe::onSaveChangesClicked()
{
	LLAvatarData data = LLAvatarData();
	data.avatar_id = gAgent.getID();
	data.image_id = mEditPanel->getChild<LLTextureCtrl>(PICKER_SECOND_LIFE)->getImageAssetID();
	data.fl_image_id = mEditPanel->getChild<LLTextureCtrl>(PICKER_FIRST_LIFE)->getImageAssetID();
	data.about_text = mEditPanel->getChild<LLUICtrl>("sl_description_edit")->getValue().asString();
	data.fl_about_text = mEditPanel->getChild<LLUICtrl>("fl_description_edit")->getValue().asString();
	data.profile_url = mEditPanel->getChild<LLUICtrl>("homepage_edit")->getValue().asString();
	data.allow_publish = mEditPanel->getChild<LLUICtrl>("show_in_search_checkbox")->getValue();

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
	buildFromFile( "panel_edit_profile.xml");

	setAvatarId(gAgent.getID());

	LLAvatarNameCache::addUseDisplayNamesCallback(boost::bind(&LLPanelMyProfileEdit::onAvatarNameChanged, this));
}

void LLPanelMyProfileEdit::onOpen(const LLSD& key)
{
	resetData();

	// Disable editing until data is loaded, or edited fields will be overwritten when data
	// is loaded.
	enableEditing(false);

	// force new avatar name fetch so we have latest update time
	LLAvatarNameCache::fetch(gAgent.getID()); 
	LLPanelMyProfile::onOpen(getAvatarId());
	
	LLAvatarName av_name;	
	if (LLAvatarNameCache::useDisplayNames())
	{
		if (LLAvatarNameCache::get(gAgent.getID(), &av_name) && av_name.mIsDisplayNameDefault)  	
		{
			LLFirstUse::setDisplayName();
		}
		else
		{
			LLFirstUse::setDisplayName(false);
		}
	}

	if (LLAvatarNameCache::useDisplayNames())
	{
		getChild<LLUICtrl>("user_label")->setVisible( true );
		getChild<LLUICtrl>("user_slid")->setVisible( true );
		getChild<LLUICtrl>("display_name_label")->setVisible( true );
		getChild<LLUICtrl>("set_name")->setVisible( true );
		getChild<LLUICtrl>("set_name")->setEnabled( true );
		getChild<LLUICtrl>("solo_user_name")->setVisible( false );
		getChild<LLUICtrl>("solo_username_label")->setVisible( false );
	}
	else
	{
		getChild<LLUICtrl>("user_label")->setVisible( false );
		getChild<LLUICtrl>("user_slid")->setVisible( false );
		getChild<LLUICtrl>("display_name_label")->setVisible( false );
		getChild<LLUICtrl>("set_name")->setVisible( false );
		getChild<LLUICtrl>("set_name")->setEnabled( false );
		getChild<LLUICtrl>("solo_user_name")->setVisible( true );
		getChild<LLUICtrl>("solo_username_label")->setVisible( true );
	}
}

void LLPanelMyProfileEdit::onClose(const LLSD& key)
{
	if (LLAvatarNameCache::useDisplayNames())
	{
		LLFirstUse::setDisplayName(false);
	}	
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
	getChildView("homepage_edit")->setVisible( true);

	fillPartnerData(avatar_data);

	fillAccountStatus(avatar_data);

	getChild<LLUICtrl>("show_in_search_checkbox")->setValue((BOOL)(avatar_data->flags & AVATAR_ALLOW_PUBLISH));

	LLAvatarNameCache::get(avatar_data->avatar_id,
		boost::bind(&LLPanelMyProfileEdit::onNameCache, this, _1, _2));
}

void LLPanelMyProfileEdit::onNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	getChild<LLUICtrl>("user_name")->setValue( av_name.mDisplayName );
	getChild<LLUICtrl>("user_slid")->setValue( av_name.mUsername );
	getChild<LLUICtrl>("user_name_small")->setValue( av_name.mDisplayName );
	getChild<LLUICtrl>("solo_user_name")->setValue( av_name.mDisplayName );


	if (LLAvatarNameCache::useDisplayNames())
	{
		getChild<LLUICtrl>("user_label")->setVisible( true );
		getChild<LLUICtrl>("user_slid")->setVisible( true );
		getChild<LLUICtrl>("display_name_label")->setVisible( true );
		getChild<LLUICtrl>("set_name")->setVisible( true );
		getChild<LLUICtrl>("set_name")->setEnabled( true );

		getChild<LLUICtrl>("solo_user_name")->setVisible( false );
		getChild<LLUICtrl>("solo_username_label")->setVisible( false );

		// show smaller display name if too long to display in regular size
		if (getChild<LLTextBox>("user_name")->getTextPixelWidth() > getChild<LLTextBox>("user_name")->getRect().getWidth())
		{
			getChild<LLUICtrl>("user_name_small")->setVisible( true );
			getChild<LLUICtrl>("user_name")->setVisible( false );
		}
		else
		{
			getChild<LLUICtrl>("user_name_small")->setVisible( false );
			getChild<LLUICtrl>("user_name")->setVisible( true );
		}
	}
	else
	{
		getChild<LLUICtrl>("user_label")->setVisible( false );
		getChild<LLUICtrl>("user_slid")->setVisible( false );
		getChild<LLUICtrl>("display_name_label")->setVisible( false );
		getChild<LLUICtrl>("set_name")->setVisible( false );
		getChild<LLUICtrl>("set_name")->setEnabled( false );
		
		getChild<LLUICtrl>("solo_user_name")->setVisible( true );
		getChild<LLUICtrl>("user_name_small")->setVisible( false );
		getChild<LLUICtrl>("user_name")->setVisible( false );
		getChild<LLUICtrl>("solo_username_label")->setVisible( true );
	}
}


void LLPanelMyProfileEdit::onAvatarNameChanged()
{
	LLAvatarNameCache::get(getAvatarId(),
		boost::bind(&LLPanelMyProfileEdit::onNameCache, this, _1, _2));
}

BOOL LLPanelMyProfileEdit::postBuild()
{
	initTexturePickerMouseEvents();

	getChild<LLUICtrl>("partner_edit_link")->setTextArg("[URL]", getString("partner_edit_link_url"));
	getChild<LLUICtrl>("my_account_link")->setTextArg("[URL]", getString("my_account_link_url"));

	getChild<LLUICtrl>("set_name")->setCommitCallback(
		boost::bind(&LLPanelMyProfileEdit::onClickSetName, this));

	LLHints::registerHintTarget("set_display_name", getChild<LLUICtrl>("set_name")->getHandle());
	LLViewerDisplayName::addNameChangedCallback(boost::bind(&LLPanelMyProfileEdit::onAvatarNameChanged, this));
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
	getChild<LLUICtrl>("solo_user_name")->setValue( LLSD() );
	getChild<LLUICtrl>("user_name_small")->setValue( LLSD() );
}

void LLPanelMyProfileEdit::onTexturePickerMouseEnter(LLUICtrl* ctrl)
{
	mTextureEditIconMap[ctrl->getName()]->setVisible(TRUE);
}
void LLPanelMyProfileEdit::onTexturePickerMouseLeave(LLUICtrl* ctrl)
{
	mTextureEditIconMap[ctrl->getName()]->setVisible(FALSE);
}

void LLPanelMyProfileEdit::onClickSetName()
{	
	LLAvatarNameCache::get(getAvatarId(), 
			boost::bind(&LLPanelMyProfileEdit::onAvatarNameCache,
				this, _1, _2));	

	LLFirstUse::setDisplayName(false);
}

void LLPanelMyProfileEdit::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	if (av_name.mDisplayName.empty())
	{
		// something is wrong, tell user to try again later
		LLNotificationsUtil::add("SetDisplayNameFailedGeneric");
		return;		
	}

	llinfos << "name-change now " << LLDate::now() << " next_update "
		<< LLDate(av_name.mNextUpdate) << llendl;
	F64 now_secs = LLDate::now().secondsSinceEpoch();

	if (now_secs < av_name.mNextUpdate)
	{
		// if the update time is more than a year in the future, it means updates have been blocked
		// show a more general message
        const int YEAR = 60*60*24*365; 
		if (now_secs + YEAR < av_name.mNextUpdate)
		{
			LLNotificationsUtil::add("SetDisplayNameBlocked");
			return;
		}
	}
	
	LLFloaterReg::showInstance("display_name");
}

void LLPanelMyProfileEdit::enableEditing(bool enable)
{
	getChildView("2nd_life_pic")->setEnabled(enable);
	getChildView("real_world_pic")->setEnabled(enable);
	getChildView("sl_description_edit")->setEnabled(enable);
	getChildView("fl_description_edit")->setEnabled(enable);
	getChildView("homepage_edit")->setEnabled(enable);
	getChildView("show_in_search_checkbox")->setEnabled(enable);
}
