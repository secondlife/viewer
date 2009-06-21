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
#include "llcombobox.h"
#include "llcommandhandler.h"
#include "lldirpicker.h"
#include "llfeaturemanager.h"
#include "llfocusmgr.h"
#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloaterabout.h"
#include "llfloaterhardwaresettings.h"
#include "llfloatervoicedevicesettings.h"
#include "llkeyboard.h"
#include "llmodaldialog.h"
#include "llpaneldisplay.h"
#include "llpanellogin.h"
#include "llradiogroup.h"
#include "llstylemap.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsliderctrl.h"
#include "lltabcontainer.h"
#include "lltexteditor.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"

class LLVoiceSetKeyDialog : public LLModalDialog
	{
	public:
		LLVoiceSetKeyDialog(LLFloaterPreference* parent);
		~LLVoiceSetKeyDialog();
		
		BOOL handleKeyHere(KEY key, MASK mask);
		static void onCancel(void* user_data);
		
	private:
		LLFloaterPreference* mParent;
	};

LLVoiceSetKeyDialog::LLVoiceSetKeyDialog(LLFloaterPreference* parent)
: LLModalDialog(LLStringUtil::null, 240, 100), mParent(parent)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_select_key.xml");
	childSetAction("Cancel", onCancel, this);
	childSetFocus("Cancel");
	
	gFocusMgr.setKeystrokesOnly(TRUE);
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
	else
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

void free_web_media(LLMediaBase *media_source);
void handleHTMLLinkColorChanged(const LLSD& newvalue);	
LLMediaBase *get_web_media();
bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response);

bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);
bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater);

LLMediaBase *get_web_media()
{
	LLMediaBase *media_source;
	LLMediaManager *mgr = LLMediaManager::getInstance();
	
	if (!mgr)
	{
		llwarns << "cannot get media manager" << llendl;
		return NULL;
	}
	
	media_source = mgr->createSourceFromMimeType("http", "text/html" );
	if ( !media_source )
	{
		llwarns << "media source create failed " << llendl;
		return NULL;
	}
	
	return media_source;
}

void free_web_media(LLMediaBase *media_source)
{
	if (!media_source)
		return;
	
	LLMediaManager *mgr = LLMediaManager::getInstance();
	if (!mgr)
	{
		llwarns << "cannot get media manager" << llendl;
		return;
	}
	
	mgr->destroySource(media_source);
}


bool callback_clear_browser_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if ( option == 0 ) // YES
	{
		LLMediaBase *media_source = get_web_media();
		if (media_source)
			media_source->clearCache();
		free_web_media(media_source);
	}
	return false;
}

void handleHTMLLinkColorChanged(const LLSD& newvalue)
{
	LLTextEditor::setLinkColor(LLColor4(newvalue));
	LLStyleMap::instance().update();
	
}

bool callback_skip_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option && floater )
	{
		if ( floater )
		{
			floater->setAllIgnored();
			LLFirstUse::disableFirstUse();
			LLFloaterPreference::buildLists(floater);
		}
	}
	return false;
}

bool callback_reset_dialogs(const LLSD& notification, const LLSD& response, LLFloaterPreference* floater)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if ( 0 == option && floater )
	{
		if ( floater )
		{
			floater->resetAllIgnored();
			LLFirstUse::resetFirstUse();
			LLFloaterPreference::buildLists(floater);
		}
	}
	return false;
}

// static
std::string LLFloaterPreference::sSkin = "";
//////////////////////////////////////////////
// LLFloaterPreference

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
	: LLFloater(key),
	mGotPersonalInfo(false),
	mOriginalIMViaEmail(false)
{
	//Build Floater is now Called from 	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
          
	mFactoryMap["display"]		= LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelDisplay>);               /// done fixing the callbacks


	mCommitCallbackRegistrar.add("Pref.Apply",				boost::bind(&LLFloaterPreference::onBtnApply, this));
	mCommitCallbackRegistrar.add("Pref.Cancel",				boost::bind(&LLFloaterPreference::onBtnCancel, this));
	mCommitCallbackRegistrar.add("Pref.OK",					boost::bind(&LLFloaterPreference::onBtnOK, this));
	
	mCommitCallbackRegistrar.add("Pref.ClearCache",				boost::bind(&LLFloaterPreference::onClickClearCache, (void*)NULL));
	mCommitCallbackRegistrar.add("Pref.WebClearCache",			boost::bind(&LLFloaterPreference::onClickBrowserClearCache, (void*)NULL));
	mCommitCallbackRegistrar.add("Pref.SetCache",				boost::bind(&LLFloaterPreference::onClickSetCache, this));
	mCommitCallbackRegistrar.add("Pref.ResetCache",				boost::bind(&LLFloaterPreference::onClickResetCache, this));
	mCommitCallbackRegistrar.add("Pref.ClickSkin",				boost::bind(&LLFloaterPreference::onClickSkin, this,_1, _2));
	mCommitCallbackRegistrar.add("Pref.SelectSkin",				boost::bind(&LLFloaterPreference::onSelectSkin, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetKey",			boost::bind(&LLFloaterPreference::onClickSetKey, this));
	mCommitCallbackRegistrar.add("Pref.VoiceSetMiddleMouse",	boost::bind(&LLFloaterPreference::onClickSetMiddleMouse, this));
	mCommitCallbackRegistrar.add("Pref.ClickSkipDialogs",		boost::bind(&LLFloaterPreference::onClickSkipDialogs, this));
	mCommitCallbackRegistrar.add("Pref.ClickResetDialogs",		boost::bind(&LLFloaterPreference::onClickResetDialogs, this));
	mCommitCallbackRegistrar.add("Pref.ClickEnablePopup",		boost::bind(&LLFloaterPreference::onClickEnablePopup, this));
	mCommitCallbackRegistrar.add("Pref.LogPath",				boost::bind(&LLFloaterPreference::onClickLogPath, this));
	mCommitCallbackRegistrar.add("Pref.Logging",				boost::bind(&LLFloaterPreference::onCommitLogging, this));
	mCommitCallbackRegistrar.add("Pref.OpenHelp",				boost::bind(&LLFloaterPreference::onOpenHelp, this));	
	mCommitCallbackRegistrar.add("Pref.ChangeCustom",			boost::bind(&LLFloaterPreference::onChangeCustom, this));	
	mCommitCallbackRegistrar.add("Pref.UpdateMeterText",		boost::bind(&LLFloaterPreference::updateMeterText, this, _1));	
	mCommitCallbackRegistrar.add("Pref.HardwareSettings",       boost::bind(&LLFloaterPreference::onOpenHardwareSettings, this));	
	mCommitCallbackRegistrar.add("Pref.HardwareDefaults",       boost::bind(&LLFloaterPreference::setHardwareDefaults, this));	
	mCommitCallbackRegistrar.add("Pref.VertexShaderEnable",     boost::bind(&LLFloaterPreference::onVertexShaderEnable, this));	
	gSavedSkinSettings.getControl("HTMLLinkColor")->getCommitSignal()->connect(boost::bind(&handleHTMLLinkColorChanged,  _2));
	//gSavedSettings.getControl("UseExternalBrowser")->getCommitSignal()->connect(boost::bind(&LLPanelPreference::handleUseExternalBrowserChanged, this));
	
}

BOOL LLFloaterPreference::postBuild()
{
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (!tabcontainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
		tabcontainer->selectFirstTab();
	
	
	return TRUE;
}

LLFloaterPreference::~LLFloaterPreference()
{
}
void LLFloaterPreference::draw()
{
	BOOL has_first_selected = (getChildRef<LLScrollListCtrl>("disabled_popups").getFirstSelected()!=NULL);
	gSavedSettings.setBOOL("FirstSelectedDisabledPopups", has_first_selected);
	
	
	LLFloater::draw();
}

void LLFloaterPreference::apply()
{
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (sSkin != gSavedSettings.getString("SkinCurrent"))
	{
		LLNotifications::instance().add("ChangeSkin");
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
	LLFloaterHardwareSettings::instance()->apply();
	
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
	
	LLMediaBase *media_source = get_web_media();
	if (media_source)
	{
		media_source->enableCookies(childGetValue("cookies_enabled"));
		if(hasChild("web_proxy_enabled") &&hasChild("web_proxy_editor") && hasChild("web_proxy_port"))
		{
			bool proxy_enable = childGetValue("web_proxy_enabled");
			std::string proxy_address = childGetValue("web_proxy_editor");
			
			int proxy_port = childGetValue("web_proxy_port");
			media_source->enableProxy(proxy_enable, proxy_address, proxy_port);
		}
	}
	free_web_media(media_source);
	
	LLTextEditor* busy = getChild<LLTextEditor>("busy_response");
	LLWString busy_response;
	if (busy) busy_response = busy->getWText(); 
	LLWStringUtil::replaceTabsWithSpaces(busy_response, 4);
	
	if(mGotPersonalInfo)
	{ 
		gSavedPerAccountSettings.setString("BusyModeResponse2", std::string(wstring_to_utf8str(busy_response)));
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
	LLFloaterHardwareSettings::instance()->cancel();   // TODO: angela  change the build of the floater to floater reg
	
	// reverts any changes to current skin
	gSavedSettings.setString("SkinCurrent", sSkin);
	
	LLFloaterVoiceDeviceSettings* voice_device_settings = LLFloaterReg::findTypedInstance<LLFloaterVoiceDeviceSettings>("pref_voicedevicesettings");
	if (voice_device_settings)
	{
		voice_device_settings ->cancel();
	}
	LLFloaterReg::hideInstance("pref_voicedevicesettings");
}

void LLFloaterPreference::onOpen(const LLSD& key)
{
	gAgent.sendAgentUserInfoRequest();
	LLPanelLogin::setAlwaysRefresh(true);
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
void LLFloaterPreference::onClose(bool app_quitting)
{
	gSavedSettings.setS32("LastPrefTab", getChild<LLTabContainer>("pref core")->getCurrentPanelIndex());
	LLPanelLogin::setAlwaysRefresh(false);
	cancel(); // will be a no-op if OK or apply was performed just prior.
	destroy();
}
void LLFloaterPreference::onOpenHardwareSettings()
{
	LLFloaterHardwareSettings::show();
}
// static 
void LLFloaterPreference::onBtnOK()
{
	// commit any outstanding text entry
	if (hasFocus())
	{
		LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	if (canClose())
	{
		apply();
		closeFloater(false);
		gSavedSettings.saveToFile( gSavedSettings.getString("ClientSettingsFile"), TRUE );
		gSavedSkinSettings.saveToFile(gSavedSettings.getString("SkinningSettingsFile") , TRUE );
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

void LLFloaterPreference::onOpenHelp()
{
	const char* xml_alert = "GraphicsPreferencesHelp";
	LLNotifications::instance().add(this->contextualNotification(xml_alert));
}

// static 
void LLFloaterPreference::onBtnApply( )
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	apply();

	LLPanelLogin::refreshLocation( false );
}

// static 
void LLFloaterPreference::onBtnCancel()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	closeFloater(); // side effect will also cancel any unsaved changes.
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


void LLFloaterPreference::onChangeCustom()
{
	// if custom is turned off, reset everything to defaults
	if (this && getChild<LLCheckBoxCtrl>("CustomSettings")->getValue())
	{
		U32 set = (U32)getChild<LLSliderCtrl>("QualityPerformanceSelection")->getValueF32();
		LLFeatureManager::getInstance()->setGraphicsLevel(set, true);	
		updateMeterText(getChild<LLSliderCtrl>("DrawDistance"));
	}

	refreshEnabledGraphics();
}
//////////////////////////////////////////////////////////////////////////
// static     Note:(angela) NOT touching LLPanelDisplay for this milestone (skinning-11)
void LLFloaterPreference::refreshEnabledGraphics()
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if(instance)
	{
		LLFloaterHardwareSettings::instance()->refreshEnabledState();
		
		LLTabContainer* tabcontainer = instance->getChild<LLTabContainer>("pref core");
		for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
			 iter != tabcontainer->getChildList()->end(); ++iter)
		{
			LLView* view = *iter;
			if(!view)
				return;
			if(view->getName()=="display")
			{
				LLPanelDisplay* display_panel = dynamic_cast<LLPanelDisplay*>(view);
				if(!display_panel)
					return;
				display_panel->refreshEnabledState();
			}	
		}
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

// static
void LLFloaterPreference::onClickClearCache(void*)
{
	// flag client cache for clearing next time the client runs
	gSavedSettings.setBOOL("PurgeCacheOnNextStartup", TRUE);
	LLNotifications::instance().add("CacheWillClear");
}

// static
void LLFloaterPreference::onClickBrowserClearCache(void*)
{
	LLNotifications::instance().add("ConfirmClearBrowserCache", LLSD(), LLSD(), callback_clear_browser_cache);
}

void LLFloaterPreference::onClickSetCache()
{
	std::string cur_name(gSavedSettings.getString("CacheLocation"));
	std::string proposed_name(cur_name);

	LLDirPicker& picker = LLDirPicker::instance();
	if (! picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}

	std::string dir_name = picker.getDirName();
	if (!dir_name.empty() && dir_name != cur_name)
	{
		childSetText("cache_location", dir_name);
		LLNotifications::instance().add("CacheWillBeMoved");
		gSavedSettings.setString("NewCacheLocation", dir_name);
	}
	else
	{
		std::string cache_location = gDirUtilp->getCacheDir();
		childSetText("cache_location", cache_location);
	}
}

void LLFloaterPreference::onClickResetCache()
{
	if (!gSavedSettings.getString("CacheLocation").empty())
	{
		gSavedSettings.setString("NewCacheLocation", "");
		LLNotifications::instance().add("CacheWillBeMoved");
	}
	std::string cache_location = gDirUtilp->getCacheDir(true);
	childSetText("cache_location", cache_location);
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

// static
void LLFloaterPreference::buildLists(void* data)
{
	LLPanel*self = (LLPanel*)data;
	LLScrollListCtrl& disabled_popups = self->getChildRef<LLScrollListCtrl>("disabled_popups");
	LLScrollListCtrl& enabled_popups = self->getChildRef<LLScrollListCtrl>("enabled_popups");
	
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
	LLVoiceSetKeyDialog* dialog = new LLVoiceSetKeyDialog(this);
	dialog->startModal();
}

void LLFloaterPreference::setKey(KEY key)
{
	childSetValue("modifier_combo", LLKeyboard::stringFromKey(key));
}

void LLFloaterPreference::onClickSetMiddleMouse()
{
	childSetValue("modifier_combo", "MiddleMouse");
}

void LLFloaterPreference::onClickSkipDialogs()
{
	LLNotifications::instance().add("SkipShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_skip_dialogs, _1, _2, this));
}

void LLFloaterPreference::onClickResetDialogs()
{
	LLNotifications::instance().add("ResetShowNextTimeDialogs", LLSD(), LLSD(), boost::bind(&callback_reset_dialogs, _1, _2, this));
}

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
	
	buildLists(this);
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
	std::string proposed_name(childGetText("log_path_string"));	 
	
	LLDirPicker& picker = LLDirPicker::instance();
	if (!picker.getDir(&proposed_name ) )
	{
		return; //Canceled!
	}
	
	childSetText("log_path_string", picker.getDirName());	 
}

void LLFloaterPreference::onCommitLogging()
{
	enableHistory();
}
void LLFloaterPreference::enableHistory()
{
	
	if (childGetValue("log_instant_messages").asBoolean() || childGetValue("log_chat").asBoolean())
	{
		childEnable("log_show_history");
		childEnable("log_path_button");
	}
	else
	{
		childDisable("log_show_history");
		childDisable("log_path_button");
	}
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
	childEnable("log_instant_messages");
	childEnable("log_chat");
	childEnable("busy_response");
	childEnable("log_instant_messages_timestamp");
	childEnable("log_chat_timestamp");
	childEnable("log_chat_IM");
	childEnable("log_date_timestamp");
	
	childSetText("busy_response", gSavedPerAccountSettings.getString("BusyModeResponse2"));
	
	enableHistory();
	std::string display_email(email);
	childSetText("email_address",display_email);

}


//----------------------------------------------------------------------------
static LLRegisterPanelClassWrapper<LLPanelPreference> t_places("panel_preference");
LLPanelPreference::LLPanelPreference()
: LLPanel()
{
	//
	mCommitCallbackRegistrar.add("setControlFalse",		boost::bind(&LLPanelPreference::setControlFalse,this, _2));
	
}
//virtual
BOOL LLPanelPreference::postBuild()
{
	if (hasChild("maturity_desired_combobox"))
	{

		/////////////////////////// From LLPanelGeneral //////////////////////////
		// if we have no agent, we can't let them choose anything
		// if we have an agent, then we only let them choose if they have a choice
		bool canChoose = gAgent.getID().notNull() &&
			(gAgent.isMature() || gAgent.isGodlike());

		if (canChoose)
		{

			// if they're not adult or a god, they shouldn't see the adult selection, so delete it
			if (!gAgent.isAdult() && !gAgent.isGodlike())
			{
				LLComboBox* pMaturityCombo = getChild<LLComboBox>("maturity_desired_combobox");
				// we're going to remove the adult entry from the combo. This obviously depends
				// on the order of items in the XML file, but there doesn't seem to be a reasonable
				// way to depend on the field in XML called 'name'.
				pMaturityCombo->remove(0);
			}
			childSetVisible("maturity_desired_combobox", true);
			childSetVisible("maturity_desired_textbox",	false);
		}
		else
		{
			childSetVisible("maturity_desired_combobox", false);
			std::string selectedItemLabel = getChild<LLComboBox>("maturity_desired_combobox")->getSelectedItemLabel();
			childSetValue("maturity_desired_textbox", selectedItemLabel);
			childSetVisible("maturity_desired_textbox",	true);
		}
	}
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
	////////////////////////Panel Popups/////////////////
	if(hasChild("disabled_popups") && hasChild("enabled_popups"))
	{
		LLFloaterPreference::buildLists(this);
	}
	//////
	if(hasChild("online_visibility") && hasChild("send_im_to_email"))
	{
		requires("online_visibility");
		requires("send_im_to_email");
		if (!checkRequirements())
		{
			return FALSE;
		}
		childSetText("email_address",getString("log_in_to_change") );
		childSetText("busy_response", getString("log_in_to_change"));
		
	}
	apply();
	return true;
}

void LLPanelPreference::apply()
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
		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(curview);
		if (ctrl)
		{
			LLControlVariable* control = ctrl->getControlVariable();
			if (control)
			{
				mSavedValues[control] = control->getValue();
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
}

void LLPanelPreference::setControlFalse(const LLSD& user_data)
{
	std::string control_name = user_data.asString();
	LLControlVariable* control = findControl(control_name);
	
	if (control)
		control->set(LLSD(FALSE));
}

