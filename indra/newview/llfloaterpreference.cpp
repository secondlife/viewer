/** 
 * @file llfloaterpreference.cpp
 * @brief Global preferences with and without persistence.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterpreference.h"

#include "message.h"

#include "llagent.h"
#include "llavatarconstants.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llcommandhandler.h"
#include "lldirpicker.h"
#include "llfeaturemanager.h"
#include "llfocusmgr.h"
//#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloaterabout.h"
#include "llfloaterhardwaresettings.h"
#include "llimfloater.h"
#include "llkeyboard.h"
#include "llmodaldialog.h"
#include "llnavigationbar.h"
#include "llnearbychat.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationtemplate.h"
#include "llpanellogin.h"
#include "llpanelvoicedevicesettings.h"
#include "llradiogroup.h"
#include "llsearchcombobox.h"
#include "llsky.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsliderctrl.h"
#include "llsidetray.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llviewermessage.h"
#include "llviewershadermgr.h"
#include "llvotree.h"
#include "llvosky.h"

// linden library includes
#include "llavatarnamecache.h"
#include "llerror.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llstring.h"

// project includes

#include "llbutton.h"
#include "llflexibleobject.h"
#include "lllineeditor.h"
#include "llresmgr.h"
#include "llspinctrl.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llvovolume.h"
#include "llwindow.h"
#include "llworld.h"
#include "pipeline.h"
#include "lluictrlfactory.h"
#include "llviewermedia.h"
#include "llpluginclassmedia.h"
#include "llteleporthistorystorage.h"

#include "lllogininstance.h"        // to check if logged in yet
#include "llsdserialize.h"

const F32 MAX_USER_FAR_CLIP = 512.f;
const F32 MIN_USER_FAR_CLIP = 64.f;

//control value for middle mouse as talk2push button
const static std::string MIDDLE_MOUSE_CV = "MiddleMouse";

class LLVoiceSetKeyDialog : public LLModalDialog
{
public:
	LLVoiceSetKeyDialog(const LLSD& key);
	~LLVoiceSetKeyDialog();
	
	/*virtual*/ BOOL postBuild();
	
	void setParent(LLFloaterPreference* parent) { mParent = parent; }
	
	BOOL handleKeyHere(KEY key, MASK mask);
	static void onCancel(void* user_data);
		
private:
	LLFloaterPreference* mParent;
};

LLVoiceSetKeyDialog::LLVoiceSetKeyDialog(const LLSD& key)
  : LLModalDialog(key),
	mParent(NULL)
{
}

//virtual
BOOL LLVoiceSetKeyDialog::postBuild()
{
	childSetAction("Cancel", onCancel, this);
	getChild<LLUICtrl>("Cancel")->setFocus(TRUE);
	
	gFocusMgr.setKeystrokesOnly(TRUE);
	
	return TRUE;
}

LLVoiceSetKeyDialog::~LLVoiceSetKeyDialog()
{
}

BOOL LLVoiceSetKeyDialog::handleKeyHere(KEY key, MASK mask)
{
	BOOL result = TRUE;
	
	if(key == 'Q' && mask == MASK_CONTROL)
	{
		result = FALSE;
	}
	else if (mParent)
	{
		mParent->setKey(key);
	}
	closeFloater();
	return result;
}

//static
void LLVoiceSetKeyDialog::onCancel(void* user_data)
{
	LLVoiceSetKeyDialog* self = (LLVoiceSetKeyDialog*)user_data;
	self->closeFloater();
}


// global functions 

// helper functions for getting/freeing the web browser media
// if creating/destroying these is too slow, we'll need to create
// a static member and update all our static callbacks

void handleNameTagOptionChanged(const LLSD& newvalue);	
void handleDisplayNamesOptionChanged(const LLSD& newvalue);	
bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response);

//bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);
//bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator);

bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( option == 0 ) // YES
	{
		// clean web
		LLViewerMedia::clearAllCaches();
		LLViewerMedia::clearAllCookies();
		
		// clean nav bar history
		LLNavigationBar::getInstance()->clearHistoryCache();
		
		// flag client texture cache for clearing next time the client runs
		gSavedSettings.setBOOL("PurgeCacheOnNextStartup", TRUE);
		LLNotificationsUtil::add("CacheWillClear");

		LLSearchHistory::getInstance()->clearHistory();
		LLSearchHistory::getInstance()->save();
		LLSearchComboBox* search_ctrl = LLNavigationBar::getInstance()->getChild<LLSearchComboBox>("search_combo_box");
		search_ctrl->clearHistory();

		LLTeleportHistoryStorage::getInstance()->purgeItems();
		LLTeleportHistoryStorage::getInstance()->save();
	}
	
	return false;
}

void handleNameTagOptionChanged(const LLSD& newvalue)
{
	LLVOAvatar::invalidateNameTags();
}

void handleDisplayNamesOptionChanged(const LLSD& newvalue)
{
	LLAvatarNameCache::setUseDisplayNames(newvalue.asBoolean());
	LLVOAvatar::invalidateNameTags();
}


/*bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option && floater )
	{
		if ( floater )
		{
			floater->setAllIgnored();
		//	LLFirstUse::disableFirstUse();
			floater->buildPopupLists();
		}
	}
	return false;
}

bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( 0 == option && floater )
	{
		if ( floater )
		{
			floater->resetAllIgnored();
			//LLFirstUse::resetFirstUse();
			floater->buildPopupLists();
		}
	}
	return false;
}
*/

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator)
{
	numerator = 0;
	denominator = 0;
	for (F32 test_denominator = 1.f; test_denominator < 30.f; test_denominator += 1.f)
	{
		if (fmodf((decimal_val * test_denominator) + 0.01f, 1.f) < 0.02f)
		{
			numerator = llround(decimal_val * test_denominator);
			denominator = llround(test_denominator);
			break;
		}
	}
}
// static
std::string LLFloaterPreference::sSkin = "";
//////////////////////////////////////////////
// LLFloaterPreference

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
	: LLFloater(key),
	mGotPersonalInfo(false),
	mOriginalIMViaEmail(false),
	mLanguageChanged(false),
	mAvatarDataInitialized(false),
	mDoubleClickActionDirty(false),
	mFavoritesRecordMayExist(false)
{
	
	//Build Floater is now Called from 	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
	
	static bool registered_dialog = false;
	if (!registered_dialog)
	{
		LLFloaterReg::add("voice_set_key", "floater_select_key.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLVoiceSetKeyDialog>);
		registered_dialog = true;
	}
	
	mCommitCallbackRegistrar.add("Pref.Apply",				boost::bind(&LLFloaterPreference::onBtnApply, this));
	mCommitCallbackRegistrar.add("Pref.Cancel",				boost::bind(&LLFloaterPreference::onBtnCancel, this));
	mCommitCallbackRegistrar.add("Pref.OK",					boost::bind(&LLFloaterPreference::onBtnOK, this));
	
//	mCommitCallbackRegistrar.add("Pref.ClearCache",				boost::bind(&LLFloaterPreference::onClickClearCache, this));
	mCommitCallbackRegistrar.add("Pref.WebClearCache",			boost::bind(&LLFloaterPreference::onClickBrowserClearCache, this));
	mCommitCallbackRegistrar.add("Pref.SetCache",				boost::bind(&LLFloaterPreference::onClickSetCache, this));
	mCommitCallbackRegistrar.add("Pref.ResetCache",				boost::bind(&LLFloaterPreference::onClickResetCache, this));
	mCommitCallbackRegistrar.add("Pref.ClickSkin",				boost::bind(&LLFloaterPreference::onClickSkin, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.SelectSkin",				boost::bind(&LLFloaterPreference::onSelectSkin, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetKey",			boost::bind(&LLFloaterPreference::onClickSetKey, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetMiddleMouse",	boost::bind(&LLFloaterPreference::onClickSetMiddleMouse, this));
//	mCommitCallbackRegistrar.add("Pref.ClickSkipDialogs",		boost::bind(&LLFloaterPreference::onClickSkipDialogs, this));
//	mCommitCallbackRegistrar.add("Pref.ClickResetDialogs",		boost::bind(&LLFloaterPreference::onClickResetDialogs, this));
	mCommitCallbackRegistrar.add("Pref.ClickEnablePopup",		boost::bind(&LLFloaterPreference::onClickEnablePopup, this));
	mCommitCallbackRegistrar.add("Pref.ClickDisablePopup",		boost::bind(&LLFloaterPreference::onClickDisablePopup, this));	
	mCommitCallbackRegistrar.add("Pref.LogPath",				boost::bind(&LLFloaterPreference::onClickLogPath, this));
	mCommitCallbackRegistrar.add("Pref.HardwareSettings",       boost::bind(&LLFloaterPreference::onOpenHardwareSettings, this));	
	mCommitCallbackRegistrar.add("Pref.HardwareDefaults",       boost::bind(&LLFloaterPreference::setHardwareDefaults, this));	
	mCommitCallbackRegistrar.add("Pref.VertexShaderEnable",     boost::bind(&LLFloaterPreference::onVertexShaderEnable, this));	
	mCommitCallbackRegistrar.add("Pref.WindowedMod",            boost::bind(&LLFloaterPreference::onCommitWindowedMode, this));	
	mCommitCallbackRegistrar.add("Pref.UpdateSliderText",       boost::bind(&LLFloaterPreference::onUpdateSliderText,this, _1,_2));	
	mCommitCallbackRegistrar.add("Pref.QualityPerformance",     boost::bind(&LLFloaterPreference::onChangeQuality, this, _2));	
	mCommitCallbackRegistrar.add("Pref.applyUIColor",			boost::bind(&LLFloaterPreference::applyUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.getUIColor",				boost::bind(&LLFloaterPreference::getUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.MaturitySettings",		boost::bind(&LLFloaterPreference::onChangeMaturity, this));
	mCommitCallbackRegistrar.add("Pref.BlockList",				boost::bind(&LLFloaterPreference::onClickBlockList, this));
	
	sSkin = gSavedSettings.getString("SkinCurrent");

	mCommitCallbackRegistrar.add("Pref.CommitDoubleClickChekbox",	boost::bind(&LLFloaterPreference::onDoubleClickCheckBox, this, _1));
	mCommitCallbackRegistrar.add("Pref.CommitRadioDoubleClick",	boost::bind(&LLFloaterPreference::onDoubleClickRadio, this));

	gSavedSettings.getControl("NameTagShowUsernames")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));	
	gSavedSettings.getControl("NameTagShowFriends")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));	
	gSavedSettings.getControl("UseDisplayNames")->getCommitSignal()->connect(boost::bind(&handleDisplayNamesOptionChanged,  _2));

	LLAvatarPropertiesProcessor::getInstance()->addObserver( gAgent.getID(), this );
}

void LLFloaterPreference::processProperties( void* pData, EAvatarProcessorType type )
{
	if ( APT_PROPERTIES == type )
	{
		const LLAvatarData* pAvatarData = static_cast<const LLAvatarData*>( pData );
		if( pAvatarData && gAgent.getID() == pAvatarData->avatar_id )
		{
			storeAvatarProperties( pAvatarData );
			processProfileProperties( pAvatarData );
		}
	}	
}

void LLFloaterPreference::storeAvatarProperties( const LLAvatarData* pAvatarData )
{
	if (gAgent.isInitialized() && (gAgent.getID() != LLUUID::null))
	{
		mAvatarProperties.avatar_id		= gAgent.getID();
		mAvatarProperties.image_id		= pAvatarData->image_id;
		mAvatarProperties.fl_image_id   = pAvatarData->fl_image_id;
		mAvatarProperties.about_text	= pAvatarData->about_text;
		mAvatarProperties.fl_about_text = pAvatarData->fl_about_text;
		mAvatarProperties.profile_url   = pAvatarData->profile_url;
		mAvatarProperties.flags		    = pAvatarData->flags;
		mAvatarProperties.allow_publish	= pAvatarData->flags & AVATAR_ALLOW_PUBLISH;

		mAvatarDataInitialized = true;
	}
}

void LLFloaterPreference::processProfileProperties(const LLAvatarData* pAvatarData )
{
	getChild<LLUICtrl>("online_searchresults")->setValue( (bool)(pAvatarData->flags & AVATAR_ALLOW_PUBLISH) );	
}

void LLFloaterPreference::saveAvatarProperties( void )
{
	mAvatarProperties.allow_publish = getChild<LLUICtrl>("online_searchresults")->getValue();
	if (mAvatarProperties.allow_publish)
	{
		mAvatarProperties.flags |= AVATAR_ALLOW_PUBLISH;
	}

	if (mAvatarDataInitialized)
	{
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesUpdate( &mAvatarProperties );
	}
}


BOOL LLFloaterPreference::postBuild()
{
	gSavedSettings.getControl("PlainTextChatHistory")->getSignal()->connect(boost::bind(&LLIMFloater::processChatHistoryStyleUpdate, _2));

	gSavedSettings.getControl("PlainTextChatHistory")->getSignal()->connect(boost::bind(&LLNearbyChat::processChatHistoryStyleUpdate, _2));

	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLIMFloater::processChatHistoryStyleUpdate, _2));

	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&LLNearbyChat::processChatHistoryStyleUpdate, _2));

	gSavedSettings.getControl("ChatBubbleOpacity")->getSignal()->connect(boost::bind(&LLFloaterPreference::onNameTagOpacityChange, this, _2));

	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (!tabcontainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
		tabcontainer->selectFirstTab();

	updateDoubleClickControls();

	getChild<LLUICtrl>("cache_location")->setEnabled(FALSE); // make it read-only but selectable (STORM-227)
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);

	getChild<LLComboBox>("language_combobox")->setCommitCallback(boost::bind(&LLFloaterPreference::onLanguageChange, this));

	// if floater is opened before login set default localized busy message
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		gSavedPerAccountSettings.setString("BusyModeResponse", LLTrans::getString("BusyModeResponseDefault"));
	}

	return TRUE;
}

void LLFloaterPreference::onBusyResponseChanged()
{
	// set "BusyResponseChanged" TRUE if user edited message differs from default, FALSE otherwise
	if(LLTrans::getString("BusyModeResponseDefault") != getChild<LLUICtrl>("busy_response")->getValue().asString())
	{
		gSavedPerAccountSettings.setBOOL("BusyResponseChanged", TRUE );
	}
	else
	{
		gSavedPerAccountSettings.setBOOL("BusyResponseChanged", FALSE );
	}
}

LLFloaterPreference::~LLFloaterPreference()
{
	// clean up user data
	LLComboBox* ctrl_window_size = getChild<LLComboBox>("windowsize combo");
	for (S32 i = 0; i < ctrl_window_size->getItemCount(); i++)
	{
		ctrl_window_size->setCurrentByIndex(i);
	}
}

void LLFloaterPreference::draw()
{
	BOOL has_first_selected = (getChildRef<LLScrollListCtrl>("disabled_popups").getFirstSelected()!=NULL);
	gSavedSettings.setBOOL("FirstSelectedDisabledPopups", has_first_selected);
	
	has_first_selected = (getChildRef<LLScrollListCtrl>("enabled_popups").getFirstSelected()!=NULL);
	gSavedSettings.setBOOL("FirstSelectedEnabledPopups", has_first_selected);
	
	LLFloater::draw();
}

void LLFloaterPreference::saveSettings()
{
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
	child_list_t::const_iterator end = tabcontainer->getChildList()->end();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->saveSettings();
	}
}	

void LLFloaterPreference::apply()
{
	LLAvatarPropertiesProcessor::getInstance()->addObserver( gAgent.getID(), this );
	
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (sSkin != gSavedSettings.getString("SkinCurrent"))
	{
		LLNotificationsUtil::add("ChangeSkin");
		refreshSkin(this);
	}
	// Call apply() on all panels that derive from LLPanelPreference
	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
		 iter != tabcontainer->getChildList()->end(); ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->apply();
	}
	// hardware menu apply
	LLFloaterHardwareSettings* hardware_settings = LLFloaterReg::getTypedInstance<LLFloaterHardwareSettings>("prefs_hardware_settings");
	if (hardware_settings)
	{
		hardware_settings->apply();
	}
	
	gViewerWindow->requestResolutionUpdate(); // for UIScaleFactor

	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());
	
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	setCacheLocation(cache_location);
	
	LLViewerMedia::setCookiesEnabled(getChild<LLUICtrl>("cookies_enabled")->getValue());
	
	if(hasChild("web_proxy_enabled") &&hasChild("web_proxy_editor") && hasChild("web_proxy_port"))
	{
		bool proxy_enable = getChild<LLUICtrl>("web_proxy_enabled")->getValue();
		std::string proxy_address = getChild<LLUICtrl>("web_proxy_editor")->getValue();
		int proxy_port = getChild<LLUICtrl>("web_proxy_port")->getValue();
		LLViewerMedia::setProxyConfig(proxy_enable, proxy_address, proxy_port);
	}
	
//	LLWString busy_response = utf8str_to_wstring(getChild<LLUICtrl>("busy_response")->getValue().asString());
//	LLWStringUtil::replaceTabsWithSpaces(busy_response, 4);

	gSavedSettings.setBOOL("PlainTextChatHistory", getChild<LLUICtrl>("plain_text_chat_history")->getValue().asBoolean());
	
	if(mGotPersonalInfo)
	{ 
//		gSavedSettings.setString("BusyModeResponse2", std::string(wstring_to_utf8str(busy_response)));
		bool new_im_via_email = getChild<LLUICtrl>("send_im_to_email")->getValue().asBoolean();
		bool new_hide_online = getChild<LLUICtrl>("online_visibility")->getValue().asBoolean();		
	
		if((new_im_via_email != mOriginalIMViaEmail)
			||(new_hide_online != mOriginalHideOnlineStatus))
		{
			// This hack is because we are representing several different 	 
			// possible strings with a single checkbox. Since most users 	 
			// can only select between 2 values, we represent it as a 	 
			// checkbox. This breaks down a little bit for liaisons, but 	 
			// works out in the end. 	 
			if(new_hide_online != mOriginalHideOnlineStatus) 	 
			{ 	 
				if(new_hide_online) mDirectoryVisibility = VISIBILITY_HIDDEN;
				else mDirectoryVisibility = VISIBILITY_DEFAULT;
			 //Update showonline value, otherwise multiple applys won't work
				mOriginalHideOnlineStatus = new_hide_online;
			} 	 
			gAgent.sendAgentUpdateUserInfo(new_im_via_email,mDirectoryVisibility);
		}
	}

	saveAvatarProperties();

	if (mDoubleClickActionDirty)
	{
		updateDoubleClickSettings();
		mDoubleClickActionDirty = false;
	}

	if (mFavoritesRecordMayExist && !gSavedPerAccountSettings.getBOOL("ShowFavoritesOnLogin"))
	{
		removeFavoritesRecordOfUser();		
	}
}

void LLFloaterPreference::removeFavoritesRecordOfUser()
{
	mFavoritesRecordMayExist = false;
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "stored_favorites.xml");
	LLSD fav_llsd;
	llifstream file;
	file.open(filename);
	if (!file.is_open()) return;
	LLSDSerialize::fromXML(fav_llsd, file);
	
	LLAvatarName av_name;
	LLAvatarNameCache::get( gAgentID, &av_name );
	if (fav_llsd.has(av_name.getLegacyName()))
	{
		fav_llsd.erase(av_name.getLegacyName());
	}
	
	llofstream out_file;
	out_file.open(filename);
	LLSDSerialize::toPrettyXML(fav_llsd, out_file);

}

void LLFloaterPreference::cancel()
{
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	// Call cancel() on all panels that derive from LLPanelPreference
	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
		iter != tabcontainer->getChildList()->end(); ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->cancel();
	}
	// hide joystick pref floater
	LLFloaterReg::hideInstance("pref_joystick");
	
	// cancel hardware menu
	LLFloaterHardwareSettings* hardware_settings = LLFloaterReg::getTypedInstance<LLFloaterHardwareSettings>("prefs_hardware_settings");
	if (hardware_settings)
	{
		hardware_settings->cancel();
	}
	
	// reverts any changes to current skin
	gSavedSettings.setString("SkinCurrent", sSkin);

	if (mDoubleClickActionDirty)
	{
		updateDoubleClickControls();
		mDoubleClickActionDirty = false;
	}
}

void LLFloaterPreference::onOpen(const LLSD& key)
{
	
	// this variable and if that follows it are used to properly handle busy mode response message
	static bool initialized = FALSE;
	// if user is logged in and we haven't initialized busy_response yet, do it
	if (!initialized && LLStartUp::getStartupState() == STATE_STARTED)
	{
		// Special approach is used for busy response localization, because "BusyModeResponse" is
		// in non-localizable xml, and also because it may be changed by user and in this case it shouldn't be localized.
		// To keep track of whether busy response is default or changed by user additional setting BusyResponseChanged
		// was added into per account settings.

		// initialization should happen once,so setting variable to TRUE
		initialized = TRUE;
		// this connection is needed to properly set "BusyResponseChanged" setting when user makes changes in
		// busy response message.
		gSavedPerAccountSettings.getControl("BusyModeResponse")->getSignal()->connect(boost::bind(&LLFloaterPreference::onBusyResponseChanged, this));
	}
	gAgent.sendAgentUserInfoRequest();

	/////////////////////////// From LLPanelGeneral //////////////////////////
	// if we have no agent, we can't let them choose anything
	// if we have an agent, then we only let them choose if they have a choice
	bool can_choose_maturity =
		gAgent.getID().notNull() &&
		(gAgent.isMature() || gAgent.isGodlike());
	
	LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
	LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest( gAgent.getID() );
	if (can_choose_maturity)
	{		
		// if they're not adult or a god, they shouldn't see the adult selection, so delete it
		if (!gAgent.isAdult() && !gAgent.isGodlikeWithoutAdminMenuFakery())
		{
			// we're going to remove the adult entry from the combo
			LLScrollListCtrl* maturity_list = maturity_combo->findChild<LLScrollListCtrl>("ComboBox");
			if (maturity_list)
			{
				maturity_list->deleteItems(LLSD(SIM_ACCESS_ADULT));
			}
		}
		getChildView("maturity_desired_combobox")->setVisible( true);
		getChildView("maturity_desired_textbox")->setVisible( false);
	}
	else
	{
		getChild<LLUICtrl>("maturity_desired_textbox")->setValue(maturity_combo->getSelectedItemLabel());
		getChildView("maturity_desired_combobox")->setVisible( false);
	}

	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		mFavoritesRecordMayExist = gSavedPerAccountSettings.getBOOL("ShowFavoritesOnLogin");
	}

	// Forget previous language changes.
	mLanguageChanged = false;

	// Display selected maturity icons.
	onChangeMaturity();
	
	// Enabled/disabled popups, might have been changed by user actions
	// while preferences floater was closed.
	buildPopupLists();

	LLPanelLogin::setAlwaysRefresh(true);
	refresh();
	
	// Make sure the current state of prefs are saved away when
	// when the floater is opened.  That will make cancel do its
	// job
	saveSettings();
}

void LLFloaterPreference::onVertexShaderEnable()
{
	refreshEnabledGraphics();
}

//static
void LLFloaterPreference::initBusyResponse()
	{
		if (!gSavedPerAccountSettings.getBOOL("BusyResponseChanged"))
		{
			//LLTrans::getString("BusyModeResponseDefault") is used here for localization (EXT-5885)
			gSavedPerAccountSettings.setString("BusyModeResponse", LLTrans::getString("BusyModeResponseDefault"));
		}
	}

void LLFloaterPreference::setHardwareDefaults()
{
	LLFeatureManager::getInstance()->applyRecommendedSettings();
	refreshEnabledGraphics();
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
	child_list_t::const_iterator end = tabcontainer->getChildList()->end();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->setHardwareDefaults();
	}
}

//virtual
void LLFloaterPreference::onClose(bool app_quitting)
{
	gSavedSettings.setS32("LastPrefTab", getChild<LLTabContainer>("pref core")->getCurrentPanelIndex());
	LLPanelLogin::setAlwaysRefresh(false);
	cancel();
}

void LLFloaterPreference::onOpenHardwareSettings()
{
	LLFloaterReg::showInstance("prefs_hardware_settings");
}
// static 
void LLFloaterPreference::onBtnOK()
{
	// commit any outstanding text entry
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	if (canClose())
	{
		saveSettings();
		apply();
		closeFloater(false);

		LLUIColorTable::instance().saveUserSettings();
		gSavedSettings.saveToFile( gSavedSettings.getString("ClientSettingsFile"), TRUE );
		std::string crash_settings_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
		// save all settings, even if equals defaults
		gCrashSettings.saveToFile(crash_settings_filename, FALSE);
	}
	else
	{
		// Show beep, pop up dialog, etc.
		llinfos << "Can't close preferences!" << llendl;
	}

	LLPanelLogin::updateLocationCombo( false );
}

// static 
void LLFloaterPreference::onBtnApply( )
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	apply();
	saveSettings();

	LLPanelLogin::updateLocationCombo( false );
}

// static 
void LLFloaterPreference::onBtnCancel()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
		refresh();
	}
	cancel();
	closeFloater();
}

// static 
void LLFloaterPreference::updateUserInfo(const std::string& visibility, bool im_via_email, const std::string& email)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if(instance)
	{
		instance->setPersonalInfo(visibility, im_via_email, email);	
	}
}


void LLFloaterPreference::refreshEnabledGraphics()
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if(instance)
	{
		instance->refresh();
		//instance->refreshEnabledState();
	}
	LLFloaterHardwareSettings* hardware_settings = LLFloaterReg::getTypedInstance<LLFloaterHardwareSettings>("prefs_hardware_settings");
	if (hardware_settings)
	{
		hardware_settings->refreshEnabledState();
	}
}

void LLFloaterPreference::onClickBrowserClearCache()
{
	LLNotificationsUtil::add("ConfirmClearBrowserCache", LLSD(), LLSD(), callback_clear_browser_cache);
}

// Called when user changes language via the combobox.
void LLFloaterPreference::onLanguageChange()
{
	// Let the user know that the change will only take effect after restart.
	// Do it only once so that we're not too irritating.
	if (!mLanguageChanged)
	{
		LLNotificationsUtil::add("ChangeLanguage");
		mLanguageChanged = true;
	}
}

void LLFloaterPreference::onNameTagOpacityChange(const LLSD& newvalue)
{
	LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>("background");
	if (color_swatch)
	{
		LLColor4 new_color = color_swatch->get();
		color_swatch->set( new_color.setAlpha(newvalue.asReal()) );
	}
}

void LLFloaterPreference::onClickSetCache()
{
	std::string cur_name(gSavedSettings.getString("CacheLocation"));
//	std::string cur_top_folder(gDirUtilp->getBaseFileName(cur_name));
	
	std::string proposed_name(cur_name);

	LLDirPicker& picker = LLDirPicker::instance();
	if (! picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	std::string dir_name = picker.getDirName();
	if (!dir_name.empty() && dir_name != cur_name)
	{
		std::string new_top_folder(gDirUtilp->getBaseFileName(dir_name));	
		LLNotificationsUtil::add("CacheWillBeMoved");
		gSavedSettings.setString("NewCacheLocation", dir_name);
		gSavedSettings.setString("NewCacheLocationTopFolder", new_top_folder);
	}
	else
	{
		std::string cache_location = gDirUtilp->getCacheDir();
		gSavedSettings.setString("CacheLocation", cache_location);
		std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
		gSavedSettings.setString("CacheLocationTopFolder", top_folder);
	}
}

void LLFloaterPreference::onClickResetCache()
{
	if (!gSavedSettings.getString("CacheLocation").empty())
	{
		gSavedSettings.setString("NewCacheLocation", "");
		gSavedSettings.setString("NewCacheLocationTopFolder", "");
	}
	
	LLNotificationsUtil::add("CacheWillBeMoved");
	std::string cache_location = gDirUtilp->getCacheDir(true);
	gSavedSettings.setString("CacheLocation", cache_location);
	std::string top_folder(gDirUtilp->getBaseFileName(cache_location));
	gSavedSettings.setString("CacheLocationTopFolder", top_folder);
}

void LLFloaterPreference::onClickSkin(LLUICtrl* ctrl, const LLSD& userdata)
{
	gSavedSettings.setString("SkinCurrent", userdata.asString());
	ctrl->setValue(userdata.asString());
}

void LLFloaterPreference::onSelectSkin()
{
	std::string skin_selection = getChild<LLRadioGroup>("skin_selection")->getValue().asString();
	gSavedSettings.setString("SkinCurrent", skin_selection);
}

void LLFloaterPreference::refreshSkin(void* data)
{
	LLPanel*self = (LLPanel*)data;
	sSkin = gSavedSettings.getString("SkinCurrent");
	self->getChild<LLRadioGroup>("skin_selection", true)->setValue(sSkin);
}


void LLFloaterPreference::buildPopupLists()
{
	LLScrollListCtrl& disabled_popups =
		getChildRef<LLScrollListCtrl>("disabled_popups");
	LLScrollListCtrl& enabled_popups =
		getChildRef<LLScrollListCtrl>("enabled_popups");
	
	disabled_popups.deleteAllItems();
	enabled_popups.deleteAllItems();
	
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		LLNotificationTemplatePtr templatep = iter->second;
		LLNotificationFormPtr formp = templatep->mForm;
		
		LLNotificationForm::EIgnoreType ignore = formp->getIgnoreType();
		if (ignore == LLNotificationForm::IGNORE_NO)
			continue;
		
		LLSD row;
		row["columns"][0]["value"] = formp->getIgnoreMessage();
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		row["columns"][0]["width"] = 400;
		
		LLScrollListItem* item = NULL;
		
		bool show_popup = !formp->getIgnored();
		if (!show_popup)
		{
			if (ignore == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
			{
				LLSD last_response = LLUI::sSettingGroups["config"]->getLLSD("Default" + templatep->mName);
				if (!last_response.isUndefined())
				{
					for (LLSD::map_const_iterator it = last_response.beginMap();
						 it != last_response.endMap();
						 ++it)
					{
						if (it->second.asBoolean())
						{
							row["columns"][1]["value"] = formp->getElement(it->first)["ignore"].asString();
							break;
						}
					}
				}
				row["columns"][1]["font"] = "SANSSERIF_SMALL";
				row["columns"][1]["width"] = 360;
			}
			item = disabled_popups.addElement(row);
		}
		else
		{
			item = enabled_popups.addElement(row);
		}
		
		if (item)
		{
			item->setUserdata((void*)&iter->first);
		}
	}
}

void LLFloaterPreference::refreshEnabledState()
{	
	LLComboBox* ctrl_reflections = getChild<LLComboBox>("Reflections");
	LLRadioGroup* radio_reflection_detail = getChild<LLRadioGroup>("ReflectionDetailRadio");
	
	// Reflections
	BOOL reflections = gSavedSettings.getBOOL("VertexShaderEnable") 
		&& gGLManager.mHasCubeMap
		&& LLCubeMap::sUseCubeMaps;
	ctrl_reflections->setEnabled(reflections);
	
	// Bump & Shiny	
	bool bumpshiny = gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump");
	getChild<LLCheckBoxCtrl>("BumpShiny")->setEnabled(bumpshiny ? TRUE : FALSE);
	
	radio_reflection_detail->setEnabled(reflections);
	
	// Avatar Mode
	// Enable Avatar Shaders
	LLCheckBoxCtrl* ctrl_avatar_vp = getChild<LLCheckBoxCtrl>("AvatarVertexProgram");
	// Avatar Render Mode
	LLCheckBoxCtrl* ctrl_avatar_cloth = getChild<LLCheckBoxCtrl>("AvatarCloth");

	S32 max_avatar_shader = LLViewerShaderMgr::instance()->mMaxAvatarShaderLevel;
	ctrl_avatar_vp->setEnabled((max_avatar_shader > 0) ? TRUE : FALSE);
	
	if (gSavedSettings.getBOOL("VertexShaderEnable") == FALSE || 
		gSavedSettings.getBOOL("RenderAvatarVP") == FALSE)
	{
		ctrl_avatar_cloth->setEnabled(false);
	} 
	else
	{
		ctrl_avatar_cloth->setEnabled(true);
	}
	
	// Vertex Shaders
	// Global Shader Enable
	LLCheckBoxCtrl* ctrl_shader_enable   = getChild<LLCheckBoxCtrl>("BasicShaders");
	// radio set for terrain detail mode
	LLRadioGroup*   mRadioTerrainDetail = getChild<LLRadioGroup>("TerrainDetailRadio");   // can be linked with control var

	ctrl_shader_enable->setEnabled(LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable"));
	
	BOOL shaders = ctrl_shader_enable->get();
	if (shaders)
	{
		mRadioTerrainDetail->setValue(1);
		mRadioTerrainDetail->setEnabled(FALSE);
	}
	else
	{
		mRadioTerrainDetail->setEnabled(TRUE);		
	}
	
	// WindLight
	LLCheckBoxCtrl* ctrl_wind_light = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	
	// *HACK just checks to see if we can use shaders... 
	// maybe some cards that use shaders, but don't support windlight
	ctrl_wind_light->setEnabled(ctrl_shader_enable->getEnabled() && shaders);

	//Deferred/SSAO/Shadows
	LLCheckBoxCtrl* ctrl_deferred = getChild<LLCheckBoxCtrl>("UseLightShaders");
	if (LLFeatureManager::getInstance()->isFeatureAvailable("RenderUseFBO") &&
	    LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") &&
		shaders)
	{
		BOOL enabled = (ctrl_wind_light->get()) ? TRUE : FALSE;

		ctrl_deferred->setEnabled(enabled);
	
		LLCheckBoxCtrl* ctrl_ssao = getChild<LLCheckBoxCtrl>("UseSSAO");
		LLComboBox* ctrl_shadow = getChild<LLComboBox>("ShadowDetail");

		enabled = enabled && LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferredSSAO") && (ctrl_deferred->get() ? TRUE : FALSE);
		
		ctrl_ssao->setEnabled(enabled);

		enabled = enabled && LLFeatureManager::getInstance()->isFeatureAvailable("RenderShadowDetail");

		ctrl_shadow->setEnabled(enabled);
	}


	// now turn off any features that are unavailable
	disableUnavailableSettings();

	getChildView("block_list")->setEnabled(LLLoginInstance::getInstance()->authSuccess());
}

void LLFloaterPreference::disableUnavailableSettings()
{	
	LLComboBox* ctrl_reflections   = getChild<LLComboBox>("Reflections");
	LLCheckBoxCtrl* ctrl_avatar_vp     = getChild<LLCheckBoxCtrl>("AvatarVertexProgram");
	LLCheckBoxCtrl* ctrl_avatar_cloth  = getChild<LLCheckBoxCtrl>("AvatarCloth");
	LLCheckBoxCtrl* ctrl_shader_enable = getChild<LLCheckBoxCtrl>("BasicShaders");
	LLCheckBoxCtrl* ctrl_wind_light    = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	LLCheckBoxCtrl* ctrl_avatar_impostors = getChild<LLCheckBoxCtrl>("AvatarImpostors");
	LLCheckBoxCtrl* ctrl_deferred = getChild<LLCheckBoxCtrl>("UseLightShaders");
	LLComboBox* ctrl_shadows = getChild<LLComboBox>("ShadowDetail");
	LLCheckBoxCtrl* ctrl_ssao = getChild<LLCheckBoxCtrl>("UseSSAO");

	// if vertex shaders off, disable all shader related products
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable"))
	{
		ctrl_shader_enable->setEnabled(FALSE);
		ctrl_shader_enable->setValue(FALSE);
		
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);
		
		ctrl_reflections->setEnabled(FALSE);
		ctrl_reflections->setValue(0);
		
		ctrl_avatar_vp->setEnabled(FALSE);
		ctrl_avatar_vp->setValue(FALSE);
		
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);

		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}
	
	// disabled windlight
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("WindLightUseAtmosShaders"))
	{
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);

		//deferred needs windlight, disable deferred
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}

	// disabled deferred
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred"))
	{
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}
	
	// disabled deferred SSAO
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferredSSAO"))
	{
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);
	}
	
	// disabled deferred shadows
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderShadowDetail"))
	{
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
	}

	// disabled reflections
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderReflectionDetail"))
	{
		ctrl_reflections->setEnabled(FALSE);
		ctrl_reflections->setValue(FALSE);
	}
	
	// disabled av
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderAvatarVP"))
	{
		ctrl_avatar_vp->setEnabled(FALSE);
		ctrl_avatar_vp->setValue(FALSE);
		
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);

		//deferred needs AvatarVP, disable deferred
		ctrl_shadows->setEnabled(FALSE);
		ctrl_shadows->setValue(0);
		
		ctrl_ssao->setEnabled(FALSE);
		ctrl_ssao->setValue(FALSE);

		ctrl_deferred->setEnabled(FALSE);
		ctrl_deferred->setValue(FALSE);
	}

	// disabled cloth
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderAvatarCloth"))
	{
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);
	}

	// disabled impostors
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderUseImpostors"))
	{
		ctrl_avatar_impostors->setEnabled(FALSE);
		ctrl_avatar_impostors->setValue(FALSE);
	}
}

void LLFloaterPreference::refresh()
{
	LLPanel::refresh();

	// sliders and their text boxes
	//	mPostProcess = gSavedSettings.getS32("RenderGlowResolutionPow");
	// slider text boxes
	updateSliderText(getChild<LLSliderCtrl>("ObjectMeshDetail",		true), getChild<LLTextBox>("ObjectMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("FlexibleMeshDetail",	true), getChild<LLTextBox>("FlexibleMeshDetailText",	true));
	updateSliderText(getChild<LLSliderCtrl>("TreeMeshDetail",		true), getChild<LLTextBox>("TreeMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("AvatarMeshDetail",		true), getChild<LLTextBox>("AvatarMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("TerrainMeshDetail",	true), getChild<LLTextBox>("TerrainMeshDetailText",		true));
	updateSliderText(getChild<LLSliderCtrl>("RenderPostProcess",	true), getChild<LLTextBox>("PostProcessText",			true));
	updateSliderText(getChild<LLSliderCtrl>("SkyMeshDetail",		true), getChild<LLTextBox>("SkyMeshDetailText",			true));
	
	refreshEnabledState();
}

void LLFloaterPreference::onCommitWindowedMode()
{
	refresh();
}

void LLFloaterPreference::onChangeQuality(const LLSD& data)
{
	U32 level = (U32)(data.asReal());
	LLFeatureManager::getInstance()->setGraphicsLevel(level, true);
	refreshEnabledGraphics();
	refresh();
}

void LLFloaterPreference::onClickSetKey()
{
	LLVoiceSetKeyDialog* dialog = LLFloaterReg::showTypedInstance<LLVoiceSetKeyDialog>("voice_set_key", LLSD(), TRUE);
	if (dialog)
	{
		dialog->setParent(this);
	}
}

void LLFloaterPreference::setKey(KEY key)
{
	getChild<LLUICtrl>("modifier_combo")->setValue(LLKeyboard::stringFromKey(key));
	// update the control right away since we no longer wait for apply
	getChild<LLUICtrl>("modifier_combo")->onCommit();
}

void LLFloaterPreference::onClickSetMiddleMouse()
{
	LLUICtrl* p2t_line_editor = getChild<LLUICtrl>("modifier_combo");

	// update the control right away since we no longer wait for apply
	p2t_line_editor->setControlValue(MIDDLE_MOUSE_CV);

	//push2talk button "middle mouse" control value is in English, need to localize it for presentation
	LLPanel* advanced_preferences = dynamic_cast<LLPanel*>(p2t_line_editor->getParent());
	if (advanced_preferences)
	{
		p2t_line_editor->setValue(advanced_preferences->getString("middle_mouse"));
	}
}
/*
void LLFloaterPreference::onClickSkipDialogs()
{
	LLNotificationsUtil::add("SkipShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_skip_dialogs, _1, _2, this));
}

void LLFloaterPreference::onClickResetDialogs()
{
	LLNotificationsUtil::add("ResetShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_reset_dialogs, _1, _2, this));
}
 */

void LLFloaterPreference::onClickEnablePopup()
{	
	LLScrollListCtrl& disabled_popups = getChildRef<LLScrollListCtrl>("disabled_popups");
	
	std::vector<LLScrollListItem*> items = disabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
		//gSavedSettings.setWarning(templatep->mName, TRUE);
		std::string notification_name = templatep->mName;
		LLUI::sSettingGroups["ignores"]->setBOOL(notification_name, TRUE);
	}
	
	buildPopupLists();
}

void LLFloaterPreference::onClickDisablePopup()
{	
	LLScrollListCtrl& enabled_popups = getChildRef<LLScrollListCtrl>("enabled_popups");
	
	std::vector<LLScrollListItem*> items = enabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator itor;
	for (itor = items.begin(); itor != items.end(); ++itor)
	{
		LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate(*(std::string*)((*itor)->getUserdata()));
		templatep->mForm->setIgnored(true);
	}
	
	buildPopupLists();
}

void LLFloaterPreference::resetAllIgnored()
{
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			iter->second->mForm->setIgnored(false);
		}
	}
}

void LLFloaterPreference::setAllIgnored()
{
	for (LLNotifications::TemplateMap::const_iterator iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd();
		 ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			iter->second->mForm->setIgnored(true);
		}
	}
}

void LLFloaterPreference::onClickLogPath()
{
	std::string proposed_name(gSavedPerAccountSettings.getString("InstantMessageLogPath"));	 
	
	LLDirPicker& picker = LLDirPicker::instance();
	if (!picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	gSavedPerAccountSettings.setString("InstantMessageLogPath", picker.getDirName());
}

void LLFloaterPreference::setPersonalInfo(const std::string& visibility, bool im_via_email, const std::string& email)
{
	mGotPersonalInfo = true;
	mOriginalIMViaEmail = im_via_email;
	mDirectoryVisibility = visibility;
	
	if(visibility == VISIBILITY_DEFAULT)
	{
		mOriginalHideOnlineStatus = false;
		getChildView("online_visibility")->setEnabled(TRUE); 	 
	}
	else if(visibility == VISIBILITY_HIDDEN)
	{
		mOriginalHideOnlineStatus = true;
		getChildView("online_visibility")->setEnabled(TRUE); 	 
	}
	else
	{
		mOriginalHideOnlineStatus = true;
	}
	
	getChildView("include_im_in_chat_history")->setEnabled(TRUE);
	getChildView("show_timestamps_check_im")->setEnabled(TRUE);
	getChildView("friends_online_notify_checkbox")->setEnabled(TRUE);
	
	getChild<LLUICtrl>("online_visibility")->setValue(mOriginalHideOnlineStatus); 	 
	getChild<LLUICtrl>("online_visibility")->setLabelArg("[DIR_VIS]", mDirectoryVisibility);
	getChildView("send_im_to_email")->setEnabled(TRUE);
	getChild<LLUICtrl>("send_im_to_email")->setValue(im_via_email);
	getChildView("plain_text_chat_history")->setEnabled(TRUE);
	getChild<LLUICtrl>("plain_text_chat_history")->setValue(gSavedSettings.getBOOL("PlainTextChatHistory"));
	getChildView("log_instant_messages")->setEnabled(TRUE);
//	getChildView("log_chat")->setEnabled(TRUE);
//	getChildView("busy_response")->setEnabled(TRUE);
//	getChildView("log_instant_messages_timestamp")->setEnabled(TRUE);
//	getChildView("log_chat_timestamp")->setEnabled(TRUE);
	getChildView("log_chat_IM")->setEnabled(TRUE);
	getChildView("log_date_timestamp")->setEnabled(TRUE);
	
//	getChild<LLUICtrl>("busy_response")->setValue(gSavedSettings.getString("BusyModeResponse2"));
	
	getChildView("favorites_on_login_check")->setEnabled(TRUE);
	getChildView("log_nearby_chat")->setEnabled(TRUE);
	getChildView("log_instant_messages")->setEnabled(TRUE);
	getChildView("show_timestamps_check_im")->setEnabled(TRUE);
	getChildView("log_path_string")->setEnabled(FALSE);// LineEditor becomes readonly in this case.
	getChildView("log_path_button")->setEnabled(TRUE);
	childEnable("logfile_name_datestamp");	
	std::string display_email(email);
	getChild<LLUICtrl>("email_address")->setValue(display_email);

}

void LLFloaterPreference::onUpdateSliderText(LLUICtrl* ctrl, const LLSD& name)
{
	std::string ctrl_name = name.asString();
	
	if((ctrl_name =="" )|| !hasChild(ctrl_name, true))
		return;
	
	LLTextBox* text_box = getChild<LLTextBox>(name.asString());
	LLSliderCtrl* slider = dynamic_cast<LLSliderCtrl*>(ctrl);
	updateSliderText(slider, text_box);
}

void LLFloaterPreference::updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box)
{
	if(text_box == NULL || ctrl== NULL)
		return;
	
	// get range and points when text should change
	F32 value = (F32)ctrl->getValue().asReal();
	F32 min = ctrl->getMinValue();
	F32 max = ctrl->getMaxValue();
	F32 range = max - min;
	llassert(range > 0);
	F32 midPoint = min + range / 3.0f;
	F32 highPoint = min + (2.0f * range / 3.0f);
	
	// choose the right text
	if(value < midPoint)
	{
		text_box->setText(LLTrans::getString("GraphicsQualityLow"));
	} 
	else if (value < highPoint)
	{
		text_box->setText(LLTrans::getString("GraphicsQualityMid"));
	}
	else
	{
		text_box->setText(LLTrans::getString("GraphicsQualityHigh"));
	}
}

void LLFloaterPreference::onChangeMaturity()
{
	U8 sim_access = gSavedSettings.getU32("PreferredMaturity");

	getChild<LLIconCtrl>("rating_icon_general")->setVisible(sim_access == SIM_ACCESS_PG
															|| sim_access == SIM_ACCESS_MATURE
															|| sim_access == SIM_ACCESS_ADULT);

	getChild<LLIconCtrl>("rating_icon_moderate")->setVisible(sim_access == SIM_ACCESS_MATURE
															|| sim_access == SIM_ACCESS_ADULT);

	getChild<LLIconCtrl>("rating_icon_adult")->setVisible(sim_access == SIM_ACCESS_ADULT);
}

// FIXME: this will stop you from spawning the sidetray from preferences dialog on login screen
// but the UI for this will still be enabled
void LLFloaterPreference::onClickBlockList()
{
	// don't create side tray on demand
	if (LLSideTray::instanceCreated())
	{
		LLSideTray::getInstance()->showPanel("panel_block_list_sidetray");
	}
}

void LLFloaterPreference::onDoubleClickCheckBox(LLUICtrl* ctrl)
{
	if (!ctrl) return;
	mDoubleClickActionDirty = true;
	LLRadioGroup* radio_double_click_action = getChild<LLRadioGroup>("double_click_action");
	if (!radio_double_click_action) return;
	// select default value("teleport") in radio-group.
	radio_double_click_action->setSelectedIndex(0);
	// set radio-group enabled depending on state of checkbox
	radio_double_click_action->setEnabled(ctrl->getValue());
}

void LLFloaterPreference::onDoubleClickRadio()
{
	mDoubleClickActionDirty = true;
}

void LLFloaterPreference::updateDoubleClickSettings()
{
	LLCheckBoxCtrl* double_click_action_cb = getChild<LLCheckBoxCtrl>("double_click_chkbox");
	if (!double_click_action_cb) return;
	bool enable = double_click_action_cb->getValue().asBoolean();

	LLRadioGroup* radio_double_click_action = getChild<LLRadioGroup>("double_click_action");
	if (!radio_double_click_action) return;
	
	// enable double click radio-group depending on state of checkbox
	radio_double_click_action->setEnabled(enable);
	
	if (!enable)
	{
		// set double click action settings values to false if checkbox was unchecked
		gSavedSettings.setBOOL("DoubleClickAutoPilot", false);
		gSavedSettings.setBOOL("DoubleClickTeleport", false);
	}
	else
	{
		std::string selected = radio_double_click_action->getValue().asString();
		bool teleport_selected = selected == "radio_teleport";
		// set double click action settings values depending on chosen radio-button
		gSavedSettings.setBOOL( "DoubleClickTeleport", teleport_selected );
		gSavedSettings.setBOOL( "DoubleClickAutoPilot", !teleport_selected );
	}
}

void LLFloaterPreference::updateDoubleClickControls()
{
	// check is one of double-click actions settings enabled
	bool double_click_action_enabled = gSavedSettings.getBOOL("DoubleClickAutoPilot") || gSavedSettings.getBOOL("DoubleClickTeleport");
	LLCheckBoxCtrl* double_click_action_cb = getChild<LLCheckBoxCtrl>("double_click_chkbox");
	if (double_click_action_cb)
	{
		// check checkbox if one of double-click actions settings enabled, uncheck otherwise
		double_click_action_cb->setValue(double_click_action_enabled);
	}
	LLRadioGroup* double_click_action_radio = getChild<LLRadioGroup>("double_click_action");
	if (!double_click_action_radio) return;
	// set radio-group enabled if one of double-click actions settings enabled
	double_click_action_radio->setEnabled(double_click_action_enabled);
	// select button in radio-group depending on setting
	double_click_action_radio->setSelectedIndex(gSavedSettings.getBOOL("DoubleClickAutoPilot"));
}

void LLFloaterPreference::applyUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLUIColorTable::instance().setColor(param.asString(), LLColor4(ctrl->getValue()));
}

void LLFloaterPreference::getUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLColorSwatchCtrl* color_swatch = (LLColorSwatchCtrl*) ctrl;
	color_swatch->setOriginal(LLUIColorTable::instance().getColor(param.asString()));
}

void LLFloaterPreference::setCacheLocation(const LLStringExplicit& location)
{
	LLUICtrl* cache_location_editor = getChild<LLUICtrl>("cache_location");
	cache_location_editor->setValue(location);
	cache_location_editor->setToolTip(location);
}

//----------------------------------------------------------------------------
static LLRegisterPanelClassWrapper<LLPanelPreference> t_places("panel_preference");
LLPanelPreference::LLPanelPreference()
: LLPanel()
{
	mCommitCallbackRegistrar.add("Pref.setControlFalse",	boost::bind(&LLPanelPreference::setControlFalse,this, _2));
	mCommitCallbackRegistrar.add("Pref.updateMediaAutoPlayCheckbox",	boost::bind(&LLPanelPreference::updateMediaAutoPlayCheckbox, this, _1));
}

//virtual
BOOL LLPanelPreference::postBuild()
{

	////////////////////// PanelVoice ///////////////////
	if(hasChild("voice_unavailable"))
	{
		BOOL voice_disabled = gSavedSettings.getBOOL("CmdLineDisableVoice");
		getChildView("voice_unavailable")->setVisible( voice_disabled);
		getChildView("enable_voice_check")->setVisible( !voice_disabled);
	}
	
	//////////////////////PanelSkins ///////////////////
	
	if (hasChild("skin_selection"))
	{
		LLFloaterPreference::refreshSkin(this);

		// if skin is set to a skin that no longer exists (silver) set back to default
		if (getChild<LLRadioGroup>("skin_selection")->getSelectedIndex() < 0)
		{
			gSavedSettings.setString("SkinCurrent", "default");
			LLFloaterPreference::refreshSkin(this);
		}

	}

	if(hasChild("online_visibility") && hasChild("send_im_to_email"))
	{
		getChild<LLUICtrl>("email_address")->setValue(getString("log_in_to_change") );
//		getChild<LLUICtrl>("busy_response")->setValue(getString("log_in_to_change"));		
	}
	
	//////////////////////PanelPrivacy ///////////////////
	if (hasChild("media_enabled"))
	{
		bool media_enabled = gSavedSettings.getBOOL("AudioStreamingMedia");
		
		getChild<LLCheckBoxCtrl>("media_enabled")->set(media_enabled);
		getChild<LLCheckBoxCtrl>("autoplay_enabled")->setEnabled(media_enabled);
	}
	if (hasChild("music_enabled"))
	{
		getChild<LLCheckBoxCtrl>("music_enabled")->set(gSavedSettings.getBOOL("AudioStreamingMusic"));
	}
	if (hasChild("voice_call_friends_only_check"))
	{
		getChild<LLCheckBoxCtrl>("voice_call_friends_only_check")->setCommitCallback(boost::bind(&showFriendsOnlyWarning, _1, _2));
	}
	if (hasChild("favorites_on_login_check"))
	{
		getChild<LLCheckBoxCtrl>("favorites_on_login_check")->setCommitCallback(boost::bind(&showFavoritesOnLoginWarning, _1, _2));
	}

	// Panel Advanced
	if (hasChild("modifier_combo"))
	{
		//localizing if push2talk button is set to middle mouse
		if (MIDDLE_MOUSE_CV == getChild<LLUICtrl>("modifier_combo")->getValue().asString())
		{
			getChild<LLUICtrl>("modifier_combo")->setValue(getString("middle_mouse"));
		}
	}

	apply();
	return true;
}

void LLPanelPreference::apply()
{
	// no-op
}

void LLPanelPreference::saveSettings()
{
	// Save the value of all controls in the hierarchy
	mSavedValues.clear();
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLColorSwatchCtrl* color_swatch = dynamic_cast<LLColorSwatchCtrl *>(curview);
		if (color_swatch)
		{
			mSavedColors[color_swatch->getName()] = color_swatch->get();
		}
		else
		{
			LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
			if (ctrl)
			{
				LLControlVariable* control = ctrl->getControlVariable();
				if (control)
				{
					mSavedValues[control] = control->getValue();
				}
			}
		}
			
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}	
}

void LLPanelPreference::showFriendsOnlyWarning(LLUICtrl* checkbox, const LLSD& value)
{
	if (checkbox && checkbox->getValue())
	{
		LLNotificationsUtil::add("FriendsAndGroupsOnly");
	}
}

void LLPanelPreference::showFavoritesOnLoginWarning(LLUICtrl* checkbox, const LLSD& value)
{
	if (checkbox && checkbox->getValue())
	{
		LLNotificationsUtil::add("FavoritesOnLogin");
	}
}

void LLPanelPreference::cancel()
{
	for (control_values_map_t::iterator iter =  mSavedValues.begin();
		 iter !=  mSavedValues.end(); ++iter)
	{
		LLControlVariable* control = iter->first;
		LLSD ctrl_value = iter->second;
		control->set(ctrl_value);
	}

	for (string_color_map_t::iterator iter = mSavedColors.begin();
		 iter != mSavedColors.end(); ++iter)
	{
		LLColorSwatchCtrl* color_swatch = findChild<LLColorSwatchCtrl>(iter->first);
		if(color_swatch)
		{
			color_swatch->set(iter->second);
			color_swatch->onCommit();
		}
	}
}

void LLPanelPreference::setControlFalse(const LLSD& user_data)
{
	std::string control_name = user_data.asString();
	LLControlVariable* control = findControl(control_name);
	
	if (control)
		control->set(LLSD(FALSE));
}

void LLPanelPreference::updateMediaAutoPlayCheckbox(LLUICtrl* ctrl)
{
	std::string name = ctrl->getName();

	// Disable "Allow Media to auto play" only when both
	// "Streaming Music" and "Media" are unchecked. STORM-513.
	if ((name == "enable_music") || (name == "enable_media"))
	{
		bool music_enabled = getChild<LLCheckBoxCtrl>("enable_music")->get();
		bool media_enabled = getChild<LLCheckBoxCtrl>("enable_media")->get();

		getChild<LLCheckBoxCtrl>("media_auto_play_btn")->setEnabled(music_enabled || media_enabled);
	}
}

static LLRegisterPanelClassWrapper<LLPanelPreferenceGraphics> t_pref_graph("panel_preference_graphics");

BOOL LLPanelPreferenceGraphics::postBuild()
{
	return LLPanelPreference::postBuild();
}
void LLPanelPreferenceGraphics::draw()
{
	LLPanelPreference::draw();
	
	LLButton* button_apply = findChild<LLButton>("Apply");
	
	if(button_apply && button_apply->getVisible())
	{
		bool enable = hasDirtyChilds();

		button_apply->setEnabled(enable);

	}
}
bool LLPanelPreferenceGraphics::hasDirtyChilds()
{
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			if(ctrl->isDirty())
				return true;
		}
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}	
	return false;
}

void LLPanelPreferenceGraphics::resetDirtyChilds()
{
	std::list<LLView*> view_stack;
	view_stack.push_back(this);
	while(!view_stack.empty())
	{
		// Process view on top of the stack
		LLView* curview = view_stack.front();
		view_stack.pop_front();

		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			ctrl->resetDirty();
		}
		// Push children onto the end of the work stack
		for (child_list_t::const_iterator iter = curview->getChildList()->begin();
			 iter != curview->getChildList()->end(); ++iter)
		{
			view_stack.push_back(*iter);
		}
	}	
}
void LLPanelPreferenceGraphics::apply()
{
	resetDirtyChilds();
	LLPanelPreference::apply();
}
void LLPanelPreferenceGraphics::cancel()
{
	resetDirtyChilds();
	LLPanelPreference::cancel();
}
void LLPanelPreferenceGraphics::saveSettings()
{
	resetDirtyChilds();
	LLPanelPreference::saveSettings();
}
void LLPanelPreferenceGraphics::setHardwareDefaults()
{
	resetDirtyChilds();
	LLPanelPreference::setHardwareDefaults();
}
