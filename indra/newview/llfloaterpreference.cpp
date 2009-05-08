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

#include "llfocusmgr.h"
#include "lltabcontainer.h"
#include "llfloaterreg.h"

#include "llagent.h"
#include "llcommandhandler.h"
#include "llfloaterabout.h"
#include "llfloaterhardwaresettings.h"
#include "llpanelnetwork.h"
#include "llpanelaudioprefs.h"
#include "llpaneldisplay.h"
#include "llpanelgeneral.h"
#include "llpanelinput.h"
#include "llpanellogin.h"
#include "llpanelmsgs.h"
#include "llpanelweb.h"
#include "llpanelskins.h"
#include "llprefschat.h"
#include "llprefsvoice.h"
#include "llprefsim.h"
#include "llviewercontrol.h"

//////////////////////////////////////////////
// LLFloaterPreference

LLFloaterPreference::LLFloaterPreference(const LLSD& key)
	: LLFloater(key),
	  mInputPanel(NULL),
	  mNetworkPanel(NULL),
	  mWebPanel(NULL),
	  mDisplayPanel(NULL),
	  mAudioPanel(NULL),
	  mPrefsChat(NULL),
	  mPrefsVoice(NULL),
	  mPrefsIM(NULL),
	  mMsgPanel(NULL),
	  mSkinsPanel(NULL)
{
	mFactoryMap["general"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelGeneral>);
	mFactoryMap["input"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelInput>);
	mFactoryMap["network"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelNetwork>);
	mFactoryMap["web"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelWeb>);
	mFactoryMap["display"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelDisplay>);
	mFactoryMap["audio"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelAudioPrefs>);
	mFactoryMap["chat"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPrefsChat>);
	mFactoryMap["voice"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPrefsVoice>);
	mFactoryMap["im"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPrefsIM>);
	mFactoryMap["msgs"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelMsgs>);
	mFactoryMap["skins"] = LLCallbackMap((LLCallbackMap::callback_t)LLCallbackMap::buildPanel<LLPanelSkins>);
	
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preferences.xml", FALSE);
}

BOOL LLFloaterPreference::postBuild()
{
	getChild<LLButton>("About...")->setClickedCallback(onClickAbout, this);
	getChild<LLButton>("Apply")->setClickedCallback(onBtnApply, this);
	getChild<LLButton>("Cancel")->setClickedCallback(onBtnCancel, this);
	getChild<LLButton>("OK")->setClickedCallback(onBtnOK, this);
	
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	if (!tabcontainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
		tabcontainer->selectFirstTab();

	// Panels that don't yet derive from LLPanelPreferenc
	// *TODO: Skinning - conver these to derive from LLPanelPreference
	mWebPanel = dynamic_cast<LLPanelWeb*>(getChild<LLPanel>("web"));
	mDisplayPanel = dynamic_cast<LLPanelDisplay*>(getChild<LLPanel>("display"));
	mAudioPanel = dynamic_cast<LLPanelAudioPrefs*>(getChild<LLPanel>("audio"));
	mPrefsChat = dynamic_cast<LLPrefsChat*>(getChild<LLPanel>("chat"));
	mPrefsVoice = dynamic_cast<LLPrefsVoice*>(getChild<LLPanel>("voice"));
	mPrefsIM = dynamic_cast<LLPrefsIM*>(getChild<LLPanel>("im"));
	mMsgPanel = dynamic_cast<LLPanelMsgs*>(getChild<LLPanel>("msgs"));
	mSkinsPanel = dynamic_cast<LLPanelSkins*>(getChild<LLPanel>("skins"));
	
	return TRUE;
}


LLFloaterPreference::~LLFloaterPreference()
{
}

void LLFloaterPreference::apply()
{
	LLTabContainer* tabcontainer = getChild<LLTabContainer>("pref core");
	// Call apply() on all panels that derive from LLPanelPreference
	for (child_list_t::const_iterator iter = tabcontainer->getChildList()->begin();
		 iter != tabcontainer->getChildList()->end(); ++iter)
	{
		LLView* view = *iter;
		LLPanelPreference* panel = dynamic_cast<LLPanelPreference*>(view);
		if (panel)
			panel->apply();
	}

	if (mWebPanel) mWebPanel->apply();
	if (mDisplayPanel) mDisplayPanel->apply();
	if (mAudioPanel) mAudioPanel->apply();
	if (mPrefsChat) mPrefsChat->apply();
	if (mPrefsVoice) mPrefsVoice->apply();
	if (mPrefsIM) mPrefsIM->apply();
	if (mMsgPanel) mMsgPanel->apply();
	if (mSkinsPanel) mSkinsPanel->apply();

	// hardware menu apply
	LLFloaterHardwareSettings::instance()->apply();
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

	if (mWebPanel) mWebPanel->apply();
	if (mDisplayPanel) mDisplayPanel->cancel();
	if (mAudioPanel) mAudioPanel->cancel();
	if (mPrefsChat) mPrefsChat->cancel();
	if (mPrefsVoice) mPrefsVoice->cancel();
	if (mPrefsIM) mPrefsIM->cancel();
	if (mMsgPanel) mMsgPanel->cancel();
	if (mSkinsPanel) mSkinsPanel->cancel();

	// cancel hardware menu
	LLFloaterHardwareSettings::instance()->cancel();
}

void LLFloaterPreference::onOpen(const LLSD& key)
{
	gAgent.sendAgentUserInfoRequest();
	LLPanelLogin::setAlwaysRefresh(true);
}

void LLFloaterPreference::onClose(bool app_quitting)
{
	gSavedSettings.setS32("LastPrefTab", getChild<LLTabContainer>("pref core")->getCurrentPanelIndex());
	LLPanelLogin::setAlwaysRefresh(false);
	cancel(); // will be a no-op if OK or apply was performed just prior.
	destroy();
}

// static
void LLFloaterPreference::onClickAbout(void*)
{
	LLFloaterAbout::showInstance();
}

// static 
void LLFloaterPreference::onBtnOK( void* userdata )
{
	LLFloaterPreference *fp =(LLFloaterPreference *)userdata;
	// commit any outstanding text entry
	if (fp->hasFocus())
	{
		LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	if (fp->canClose())
	{
		fp->apply();
		fp->closeFloater(false);

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


// static 
void LLFloaterPreference::onBtnApply( void* userdata )
{
	LLFloaterPreference *fp =(LLFloaterPreference *)userdata;
	if (fp->hasFocus())
	{
		LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	fp->apply();

	LLPanelLogin::refreshLocation( false );
}


// static 
void LLFloaterPreference::onBtnCancel( void* userdata )
{
	LLFloaterPreference *fp =(LLFloaterPreference *)userdata;
	if (fp->hasFocus())
	{
		LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	fp->closeFloater(); // side effect will also cancel any unsaved changes.
}


// static
void LLFloaterPreference::updateUserInfo(const std::string& visibility, bool im_via_email, const std::string& email)
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if(instance && instance->mPrefsIM)
	{
		instance->mPrefsIM->setPersonalInfo(visibility, im_via_email, email);
	}
}

// static
void LLFloaterPreference::refreshEnabledGraphics()
{
	LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
	if(instance)
	{
		LLFloaterHardwareSettings::instance()->refreshEnabledState();
		if (instance->mDisplayPanel)
			instance->mDisplayPanel->refreshEnabledState();
	}
}

//----------------------------------------------------------------------------

//virtual
BOOL LLPanelPreference::postBuild()
{
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
