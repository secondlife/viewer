/** 
 * @file llfloaterpreference.cpp
 * @brief Global preferences with and without persistence.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llfloatervoicedevicesettings.h"
#include "llimfloater.h"
#include "llkeyboard.h"
#include "llmodaldialog.h"
#include "llnavigationbar.h"
#include "llnearbychat.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanellogin.h"
#include "llradiogroup.h"
#include "llsearchcombobox.h"
#include "llsky.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsliderctrl.h"
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

const F32 MAX_USER_FAR_CLIP = 512.f;
const F32 MIN_USER_FAR_CLIP = 64.f;

const S32 ASPECT_RATIO_STR_LEN = 100;

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
// 	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_select_key.xml", NULL);
}

//virtual
BOOL LLVoiceSetKeyDialog::postBuild()
{
	childSetAction("Cancel", onCancel, this);
	childSetFocus("Cancel");
	
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
viewer_media_t get_web_media();
bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response);

//bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);
//bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);

void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator);

viewer_media_t get_web_media()
{
	viewer_media_t media_source = LLViewerMedia::newMediaImpl(LLUUID::null);
	media_source->initializeMedia("text/html");
	return media_source;
}


bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( option == 0 ) // YES
	{
		// clean web
		viewer_media_t media_source = get_web_media();
		if (media_source && media_source->hasMedia())
			media_source->getMediaPlugin()->clear_cache();
		
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
	S32 name_tag_option = S32(newvalue);
	if(name_tag_option==2)
	{
		gSavedSettings.setBOOL("SmallAvatarNames", TRUE);
	}
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
F32 LLFloaterPreference::sAspectRatio = 0.0;
//////////////////////////////////////////////
// LLFloaterPreference

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
	: LLFloater(key),
	mGotPersonalInfo(false),
	mOriginalIMViaEmail(false)
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
	mCommitCallbackRegistrar.add("Pref.UpdateMeterText",		boost::bind(&LLFloaterPreference::updateMeterText, this, _1));	
	mCommitCallbackRegistrar.add("Pref.HardwareSettings",       boost::bind(&LLFloaterPreference::onOpenHardwareSettings, this));	
	mCommitCallbackRegistrar.add("Pref.HardwareDefaults",       boost::bind(&LLFloaterPreference::setHardwareDefaults, this));	
	mCommitCallbackRegistrar.add("Pref.VertexShaderEnable",     boost::bind(&LLFloaterPreference::onVertexShaderEnable, this));	
	mCommitCallbackRegistrar.add("Pref.WindowedMod",            boost::bind(&LLFloaterPreference::onCommitWindowedMode, this));	
	mCommitCallbackRegistrar.add("Pref.UpdateSliderText",       boost::bind(&LLFloaterPreference::onUpdateSliderText,this, _1,_2));	
	mCommitCallbackRegistrar.add("Pref.AutoDetectAspect",       boost::bind(&LLFloaterPreference::onCommitAutoDetectAspect, this));	
	mCommitCallbackRegistrar.add("Pref.ParcelMediaAutoPlayEnable",       boost::bind(&LLFloaterPreference::onCommitParcelMediaAutoPlayEnable, this));	
	mCommitCallbackRegistrar.add("Pref.MediaEnabled",           boost::bind(&LLFloaterPreference::onCommitMediaEnabled, this));	
	mCommitCallbackRegistrar.add("Pref.MusicEnabled",           boost::bind(&LLFloaterPreference::onCommitMusicEnabled, this));	
	mCommitCallbackRegistrar.add("Pref.onSelectAspectRatio",    boost::bind(&LLFloaterPreference::onKeystrokeAspectRatio, this));	
	mCommitCallbackRegistrar.add("Pref.QualityPerformance",     boost::bind(&LLFloaterPreference::onChangeQuality, this, _2));	
	mCommitCallbackRegistrar.add("Pref.applyUIColor",			boost::bind(&LLFloaterPreference::applyUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("Pref.getUIColor",				boost::bind(&LLFloaterPreference::getUIColor, this ,_1, _2));
	
	sSkin = gSavedSettings.getString("SkinCurrent");
	
	gSavedSettings.getControl("AvatarNameTagMode")->getCommitSignal()->connect(boost::bind(&handleNameTagOptionChanged,  _2));
}

BOOL LLFloaterPreference::postBuild()
{
	gSavedSettings.getControl("PlainTextChatHistory")->getSignal()->connect(boost::bind(&LLIMFloater::processChatHistoryStyleUpdate, _2));

	gSavedSettings.getControl("PlainTextChatHistory")->getSignal()->connect(boost::bind(&LLNearbyChat::processChatHistoryStyleUpdate, _2));

	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (!tabcontainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
		tabcontainer->selectFirstTab();
	S32 show_avatar_nametag_options = gSavedSettings.getS32("AvatarNameTagMode");
	handleNameTagOptionChanged(LLSD(show_avatar_nametag_options));

	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	childSetText("cache_location", cache_location);

	return TRUE;
}

LLFloaterPreference::~LLFloaterPreference()
{
	// clean up user data
	LLComboBox* ctrl_aspect_ratio = getChild<LLComboBox>( "aspect_ratio");
	LLComboBox* ctrl_window_size = getChild<LLComboBox>("windowsize combo");
	for (S32 i = 0; i < ctrl_aspect_ratio->getItemCount(); i++)
	{
		ctrl_aspect_ratio->setCurrentByIndex(i);
	}
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
	
	LLFloaterVoiceDeviceSettings* voice_device_settings = LLFloaterReg::findTypedInstance<LLFloaterVoiceDeviceSettings>("pref_voicedevicesettings");
	if(voice_device_settings)
	{
		voice_device_settings->apply();
	}
	
	gViewerWindow->requestResolutionUpdate(); // for UIScaleFactor

	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());
	
	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	childSetText("cache_location", cache_location);		
	
	viewer_media_t media_source = get_web_media();
	if (media_source && media_source->hasMedia())
	{
		media_source->getMediaPlugin()->enable_cookies(childGetValue("cookies_enabled"));
		if(hasChild("web_proxy_enabled") &&hasChild("web_proxy_editor") && hasChild("web_proxy_port"))
		{
			bool proxy_enable = childGetValue("web_proxy_enabled");
			std::string proxy_address = childGetValue("web_proxy_editor");
			int proxy_port = childGetValue("web_proxy_port");
			media_source->getMediaPlugin()->proxy_setup(proxy_enable, proxy_address, proxy_port);
		}
	}
	
//	LLWString busy_response = utf8str_to_wstring(getChild<LLUICtrl>("busy_response")->getValue().asString());
//	LLWStringUtil::replaceTabsWithSpaces(busy_response, 4);

	gSavedSettings.setBOOL("PlainTextChatHistory", childGetValue("plain_text_chat_history").asBoolean());
	
	if(mGotPersonalInfo)
	{ 
//		gSavedSettings.setString("BusyModeResponse2", std::string(wstring_to_utf8str(busy_response)));
		bool new_im_via_email = childGetValue("send_im_to_email").asBoolean();
		bool new_hide_online = childGetValue("online_visibility").asBoolean();		
	
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

	applyResolution();
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
	
	LLFloaterVoiceDeviceSettings* voice_device_settings = LLFloaterReg::findTypedInstance<LLFloaterVoiceDeviceSettings>("pref_voicedevicesettings");
	if (voice_device_settings)
	{
		voice_device_settings ->cancel();
	}
	
	LLFloaterReg::hideInstance("pref_voicedevicesettings");
	
	gSavedSettings.setF32("FullScreenAspectRatio", sAspectRatio);

}

void LLFloaterPreference::onOpen(const LLSD& key)
{
	gAgent.sendAgentUserInfoRequest();

	/////////////////////////// From LLPanelGeneral //////////////////////////
	// if we have no agent, we can't let them choose anything
	// if we have an agent, then we only let them choose if they have a choice
	bool can_choose_maturity =
		gAgent.getID().notNull() &&	(gAgent.isMature() || gAgent.isGodlike());
	
	LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
	
	if (can_choose_maturity)
	{		
		// if they're not adult or a god, they shouldn't see the adult selection, so delete it
		if (!gAgent.isAdult() && !gAgent.isGodlike())
		{
			// we're going to remove the adult entry from the combo. This obviously depends
			// on the order of items in the XML file, but there doesn't seem to be a reasonable
			// way to depend on the field in XML called 'name'.
			maturity_combo->remove(0);
		}
		childSetVisible("maturity_desired_combobox", true);
		childSetVisible("maturity_desired_textbox", false);		
	}
	else
	{
		childSetText("maturity_desired_textbox",  maturity_combo->getSelectedItemLabel());
		childSetVisible("maturity_desired_combobox", false);
	}
	
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

void LLFloaterPreference::setHardwareDefaults()
{
	LLFeatureManager::getInstance()->applyRecommendedSettings();
	refreshEnabledGraphics();
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
		if (cur_focus->acceptsTextInput())
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

	LLPanelLogin::refreshLocation( false );
}

// static 
void LLFloaterPreference::onBtnApply( )
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	apply();
	saveSettings();

	LLPanelLogin::refreshLocation( false );
}

// static 
void LLFloaterPreference::onBtnCancel()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus->acceptsTextInput())
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
		instance->refreshEnabledState();
	}
	LLFloaterHardwareSettings* hardware_settings = LLFloaterReg::getTypedInstance<LLFloaterHardwareSettings>("prefs_hardware_settings");
	if (hardware_settings)
	{
		hardware_settings->refreshEnabledState();
	}
}

void LLFloaterPreference::updateMeterText(LLUICtrl* ctrl)
{
	// get our UI widgets
	LLSliderCtrl* slider = (LLSliderCtrl*) ctrl;

	LLTextBox* m1 = getChild<LLTextBox>("DrawDistanceMeterText1");
	LLTextBox* m2 = getChild<LLTextBox>("DrawDistanceMeterText2");

	// toggle the two text boxes based on whether we have 1 or two digits
	F32 val = slider->getValueF32();
	bool two_digits = val < 100;
	m1->setVisible(two_digits);
	m2->setVisible(!two_digits);
}

void LLFloaterPreference::onClickBrowserClearCache()
{
	LLNotificationsUtil::add("ConfirmClearBrowserCache", LLSD(), LLSD(), callback_clear_browser_cache);
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
		
		bool show_popup = LLUI::sSettingGroups["ignores"]->getBOOL(templatep->mName);
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
			item = disabled_popups.addElement(row,
											  ADD_SORTED);
		}
		else
		{
			item = enabled_popups.addElement(row,
											 ADD_SORTED);
		}
		
		if (item)
		{
			item->setUserdata((void*)&iter->first);
		}
	}
}

void LLFloaterPreference::refreshEnabledState()
{	
	LLCheckBoxCtrl* ctrl_reflections = getChild<LLCheckBoxCtrl>("Reflections");
	LLRadioGroup* radio_reflection_detail = getChild<LLRadioGroup>("ReflectionDetailRadio");
	
	// Reflections
	BOOL reflections = gSavedSettings.getBOOL("VertexShaderEnable") 
		&& gGLManager.mHasCubeMap
		&& LLCubeMap::sUseCubeMaps;
	ctrl_reflections->setEnabled(reflections);
	
	// Bump & Shiny	
	bool bumpshiny = gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump");
	getChild<LLCheckBoxCtrl>("BumpShiny")->setEnabled(bumpshiny ? TRUE : FALSE);
	
	radio_reflection_detail->setEnabled(ctrl_reflections->get() && reflections);
	
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
	// now turn off any features that are unavailable
	disableUnavailableSettings();
}

void LLFloaterPreference::disableUnavailableSettings()
{	
	LLCheckBoxCtrl* ctrl_reflections   = getChild<LLCheckBoxCtrl>("Reflections");
	LLCheckBoxCtrl* ctrl_avatar_vp     = getChild<LLCheckBoxCtrl>("AvatarVertexProgram");
	LLCheckBoxCtrl* ctrl_avatar_cloth  = getChild<LLCheckBoxCtrl>("AvatarCloth");
	LLCheckBoxCtrl* ctrl_shader_enable = getChild<LLCheckBoxCtrl>("BasicShaders");
	LLCheckBoxCtrl* ctrl_wind_light    = getChild<LLCheckBoxCtrl>("WindLightUseAtmosShaders");
	LLCheckBoxCtrl* ctrl_avatar_impostors = getChild<LLCheckBoxCtrl>("AvatarImpostors");

	// if vertex shaders off, disable all shader related products
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("VertexShaderEnable"))
	{
		ctrl_shader_enable->setEnabled(FALSE);
		ctrl_shader_enable->setValue(FALSE);
		
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);
		
		ctrl_reflections->setEnabled(FALSE);
		ctrl_reflections->setValue(FALSE);
		
		ctrl_avatar_vp->setEnabled(FALSE);
		ctrl_avatar_vp->setValue(FALSE);
		
		ctrl_avatar_cloth->setEnabled(FALSE);
		ctrl_avatar_cloth->setValue(FALSE);
	}
	
	// disabled windlight
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("WindLightUseAtmosShaders"))
	{
		ctrl_wind_light->setEnabled(FALSE);
		ctrl_wind_light->setValue(FALSE);
	}
	
	// disabled reflections
	if(!LLFeatureManager::getInstance()->isFeatureAvailable("RenderWaterReflections"))
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

void LLFloaterPreference::onCommitAutoDetectAspect()
{
	BOOL auto_detect = getChild<LLCheckBoxCtrl>("aspect_auto_detect")->get();
	F32 ratio;
	
	if (auto_detect)
	{
		S32 numerator = 0;
		S32 denominator = 0;
		
		// clear any aspect ratio override
		gViewerWindow->mWindow->setNativeAspectRatio(0.f);
		fractionFromDecimal(gViewerWindow->mWindow->getNativeAspectRatio(), numerator, denominator);
		
		std::string aspect;
		if (numerator != 0)
		{
			aspect = llformat("%d:%d", numerator, denominator);
		}
		else
		{
			aspect = llformat("%.3f", gViewerWindow->mWindow->getNativeAspectRatio());
		}
		
		getChild<LLComboBox>( "aspect_ratio")->setLabel(aspect);
		
		ratio = gViewerWindow->mWindow->getNativeAspectRatio();
		gSavedSettings.setF32("FullScreenAspectRatio", ratio);
	}
}

void LLFloaterPreference::onCommitParcelMediaAutoPlayEnable()
{
	BOOL autoplay = getChild<LLCheckBoxCtrl>("autoplay_enabled")->get();
		
	gSavedSettings.setBOOL(LLViewerMedia::AUTO_PLAY_MEDIA_SETTING, autoplay);

	lldebugs << "autoplay now = " << int(autoplay) << llendl;
}

void LLFloaterPreference::onCommitMediaEnabled()
{
	LLCheckBoxCtrl *media_enabled_ctrl = getChild<LLCheckBoxCtrl>("media_enabled");
	bool enabled = media_enabled_ctrl->get();
	gSavedSettings.setBOOL("AudioStreamingMedia", enabled);
}

void LLFloaterPreference::onCommitMusicEnabled()
{
	LLCheckBoxCtrl *music_enabled_ctrl = getChild<LLCheckBoxCtrl>("music_enabled");
	bool enabled = music_enabled_ctrl->get();
	gSavedSettings.setBOOL("AudioStreamingMusic", enabled);
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

// static
// DEV-24146 -  needs to be removed at a later date. jan-2009
void LLFloaterPreference::cleanupBadSetting()
{
	if (gSavedPerAccountSettings.getString("BusyModeResponse2") == "|TOKEN COPY BusyModeResponse|")
	{
		llwarns << "cleaning old BusyModeResponse" << llendl;
		gSavedPerAccountSettings.setString("BusyModeResponse2", gSavedPerAccountSettings.getText("BusyModeResponse"));
	}
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
	childSetValue("modifier_combo", LLKeyboard::stringFromKey(key));
	// update the control right away since we no longer wait for apply
	getChild<LLUICtrl>("modifier_combo")->onCommit();
}

void LLFloaterPreference::onClickSetMiddleMouse()
{
	childSetValue("modifier_combo", "MiddleMouse");
	// update the control right away since we no longer wait for apply
	getChild<LLUICtrl>("modifier_combo")->onCommit();
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
		//gSavedSettings.setWarning(templatep->mName, TRUE);
		std::string notification_name = templatep->mName;
		LLUI::sSettingGroups["ignores"]->setBOOL(notification_name, FALSE);
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
			LLUI::sSettingGroups["ignores"]->setBOOL(iter->first, TRUE);
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
			LLUI::sSettingGroups["ignores"]->setBOOL(iter->first, FALSE);
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
	std::string chat_log_dir = picker.getDirName();
	std::string chat_log_top_folder= gDirUtilp->getBaseFileName(chat_log_dir);
	gSavedPerAccountSettings.setString("InstantMessageLogPath",chat_log_dir);
	gSavedPerAccountSettings.setString("InstantMessageLogFolder",chat_log_top_folder);
}

void LLFloaterPreference::setPersonalInfo(const std::string& visibility, bool im_via_email, const std::string& email)
{
	mGotPersonalInfo = true;
	mOriginalIMViaEmail = im_via_email;
	mDirectoryVisibility = visibility;
	
	if(visibility == VISIBILITY_DEFAULT)
	{
		mOriginalHideOnlineStatus = false;
		childEnable("online_visibility"); 	 
	}
	else if(visibility == VISIBILITY_HIDDEN)
	{
		mOriginalHideOnlineStatus = true;
		childEnable("online_visibility"); 	 
	}
	else
	{
		mOriginalHideOnlineStatus = true;
	}
	
	childEnable("include_im_in_chat_history");
	childEnable("show_timestamps_check_im");
	childEnable("friends_online_notify_checkbox");
	
	childSetValue("online_visibility", mOriginalHideOnlineStatus); 	 
	childSetLabelArg("online_visibility", "[DIR_VIS]", mDirectoryVisibility);
	childEnable("send_im_to_email");
	childSetValue("send_im_to_email", im_via_email);
	childEnable("plain_text_chat_history");
	childSetValue("plain_text_chat_history", gSavedSettings.getBOOL("PlainTextChatHistory"));
	childEnable("log_instant_messages");
//	childEnable("log_chat");
//	childEnable("busy_response");
//	childEnable("log_instant_messages_timestamp");
//	childEnable("log_chat_timestamp");
	childEnable("log_chat_IM");
	childEnable("log_date_timestamp");
	
//	childSetText("busy_response", gSavedSettings.getString("BusyModeResponse2"));
	
	childEnable("log_nearby_chat");
	childEnable("log_instant_messages");
	childEnable("show_timestamps_check_im");
	childEnable("log_path_string");
	childEnable("log_path_button");
	
	std::string display_email(email);
	childSetText("email_address",display_email);

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

void LLFloaterPreference::onKeystrokeAspectRatio()
{
	getChild<LLCheckBoxCtrl>("aspect_auto_detect")->set(FALSE);
}

void LLFloaterPreference::applyResolution()
{
	LLComboBox* ctrl_aspect_ratio = getChild<LLComboBox>( "aspect_ratio");
	gGL.flush();
	char aspect_ratio_text[ASPECT_RATIO_STR_LEN];		/*Flawfinder: ignore*/
	if (ctrl_aspect_ratio->getCurrentIndex() == -1)
	{
		// *Can't pass const char* from c_str() into strtok
		strncpy(aspect_ratio_text, ctrl_aspect_ratio->getSimple().c_str(), sizeof(aspect_ratio_text) -1);	/*Flawfinder: ignore*/
		aspect_ratio_text[sizeof(aspect_ratio_text) -1] = '\0';
		char *element = strtok(aspect_ratio_text, ":/\\");
		if (!element)
		{
			sAspectRatio = 0.f; // will be clamped later
		}
		else
		{
			LLLocale locale(LLLocale::USER_LOCALE);
			sAspectRatio = (F32)atof(element);
		}
		
		// look for denominator
		element = strtok(NULL, ":/\\");
		if (element)
		{
			LLLocale locale(LLLocale::USER_LOCALE);
			
			F32 denominator = (F32)atof(element);
			if (denominator != 0.f)
			{
				sAspectRatio /= denominator;
			}
		}
	}
	else
	{
		sAspectRatio = (F32)ctrl_aspect_ratio->getValue().asReal();
	}
	
	// presumably, user entered a non-numeric value if aspect_ratio == 0.f
	if (sAspectRatio != 0.f)
	{
		sAspectRatio = llclamp(sAspectRatio, 0.2f, 5.f);
		gSavedSettings.setF32("FullScreenAspectRatio", sAspectRatio);
	}
	
	// Screen resolution
	S32 num_resolutions;
	LLWindow::LLWindowResolution* supported_resolutions = 
	gViewerWindow->getWindow()->getSupportedResolutions(num_resolutions);
	S32 resIndex = getChild<LLComboBox>("fullscreen combo")->getCurrentIndex();
	if (resIndex == -1)
	{
		// use highest resolution if nothing selected
		resIndex = num_resolutions - 1;
	}
	gSavedSettings.setS32("FullScreenWidth", supported_resolutions[resIndex].mWidth);
	gSavedSettings.setS32("FullScreenHeight", supported_resolutions[resIndex].mHeight);
	
	gViewerWindow->requestResolutionUpdate(gSavedSettings.getBOOL("WindowFullScreen"));
	
	send_agent_update(TRUE);
	
	// Update enable/disable
	refresh();
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


//----------------------------------------------------------------------------
static LLRegisterPanelClassWrapper<LLPanelPreference> t_places("panel_preference");
LLPanelPreference::LLPanelPreference()
: LLPanel()
{
	mCommitCallbackRegistrar.add("Pref.setControlFalse",	boost::bind(&LLPanelPreference::setControlFalse,this, _2));
}

//virtual
BOOL LLPanelPreference::postBuild()
{

	////////////////////// PanelVoice ///////////////////
	if(hasChild("voice_unavailable"))
	{
		BOOL voice_disabled = gSavedSettings.getBOOL("CmdLineDisableVoice");
		childSetVisible("voice_unavailable", voice_disabled);
		childSetVisible("enable_voice_check", !voice_disabled);
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
		childSetText("email_address",getString("log_in_to_change") );
//		childSetText("busy_response", getString("log_in_to_change"));		
	}


	if(hasChild("aspect_ratio"))
	{
		// We used to set up fullscreen resolution and window size
		// controls here, see LLFloaterWindowSize::initWindowSizeControls()
		
		if (gSavedSettings.getBOOL("FullScreenAutoDetectAspectRatio"))
		{
			LLFloaterPreference::sAspectRatio = gViewerWindow->getDisplayAspectRatio();
		}
		else
		{
			LLFloaterPreference::sAspectRatio = gSavedSettings.getF32("FullScreenAspectRatio");
		}

		getChild<LLComboBox>("aspect_ratio")->setTextEntryCallback(boost::bind(&LLPanelPreference::setControlFalse, this, LLSD("FullScreenAutoDetectAspectRatio") ));	
		

		S32 numerator = 0;
		S32 denominator = 0;
		fractionFromDecimal(LLFloaterPreference::sAspectRatio, numerator, denominator);		
		
		LLUIString aspect_ratio_text = getString("aspect_ratio_text");
		if (numerator != 0)
		{
			aspect_ratio_text.setArg("[NUM]", llformat("%d",  numerator));
			aspect_ratio_text.setArg("[DEN]", llformat("%d",  denominator));
		}	
		else
		{
			aspect_ratio_text = llformat("%.3f", LLFloaterPreference::sAspectRatio);
		}
		
		LLComboBox* ctrl_aspect_ratio = getChild<LLComboBox>( "aspect_ratio");
		//mCtrlAspectRatio->setCommitCallback(onSelectAspectRatio, this);
		// add default aspect ratios
		ctrl_aspect_ratio->add(aspect_ratio_text, &LLFloaterPreference::sAspectRatio, ADD_TOP);
		ctrl_aspect_ratio->setCurrentByIndex(0);
		
		refresh();
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
