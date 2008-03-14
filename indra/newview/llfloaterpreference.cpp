/** 
 * @file llfloaterpreference.cpp
 * @brief LLPreferenceCore class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lldir.h"
#include "llfocusmgr.h"
#include "llscrollbar.h"
#include "llspinctrl.h"
#include "message.h"

#include "llcommandhandler.h"
#include "llfloaterabout.h"
#include "llfloaterpreference.h"
#include "llpanelnetwork.h"
#include "llpanelaudioprefs.h"
#include "llpaneldisplay.h"
#include "llpaneldebug.h"
#include "llpanelgeneral.h"
#include "llpanelinput.h"
#include "llpanellogin.h"
#include "llpanelLCD.h"
#include "llpanelmsgs.h"
#include "llpanelweb.h"
#include "llprefschat.h"
#include "llprefsvoice.h"
#include "llprefsim.h"
#include "llresizehandle.h"
#include "llresmgr.h"
#include "llassetstorage.h"
#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "llscrollcontainer.h"
#include "llfloaterhardwaresettings.h"

#if LL_WINDOWS
// for Logitech LCD keyboards / speakers
#ifndef LL_LOGITECH_LCD_H
#include "lllogitechlcd.h"
#endif
extern LLLCD	*gLcdScreen; 
#endif

const S32 PREF_BORDER = 4;
const S32 PREF_PAD = 5;
const S32 PREF_BUTTON_WIDTH = 70;
const S32 PREF_CATEGORY_WIDTH = 150;

const S32 PREF_FLOATER_MIN_HEIGHT = 2 * SCROLLBAR_SIZE + 2 * LLPANEL_BORDER_WIDTH + 96;

LLFloaterPreference* LLFloaterPreference::sInstance = NULL;


class LLPreferencesHandler : public LLCommandHandler
{
public:
	// don't allow from external browsers
	LLPreferencesHandler() : LLCommandHandler("preferences", false) { }
	bool handle(const LLSD& tokens, const LLSD& queryMap)
	{
		LLFloaterPreference::show(NULL);
		return true;
	}
};

LLPreferencesHandler gPreferencesHandler;


// Must be done at run time, not compile time. JC
S32 pref_min_width()
{
	return  
	2 * PREF_BORDER + 
	2 * PREF_BUTTON_WIDTH + 
	PREF_PAD + RESIZE_HANDLE_WIDTH +
	PREF_CATEGORY_WIDTH +
	PREF_PAD;
}

S32 pref_min_height()
{
	return
	2 * PREF_BORDER +
	3*(BTN_HEIGHT + PREF_PAD) +
	PREF_FLOATER_MIN_HEIGHT;
}


LLPreferenceCore::LLPreferenceCore(LLTabContainer* tab_container, LLButton * default_btn) :
	mTabContainer(tab_container),
	mGeneralPanel(NULL),
	mInputPanel(NULL),
	mNetworkPanel(NULL),
	mDisplayPanel(NULL),
	mAudioPanel(NULL),
	mMsgPanel(NULL),
	mLCDPanel(NULL)
{
	mGeneralPanel = new LLPanelGeneral();
	mTabContainer->addTabPanel(mGeneralPanel, mGeneralPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
	mGeneralPanel->setDefaultBtn(default_btn);

	mInputPanel = new LLPanelInput();
	mTabContainer->addTabPanel(mInputPanel, mInputPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
	mInputPanel->setDefaultBtn(default_btn);

	mNetworkPanel = new LLPanelNetwork();
	mTabContainer->addTabPanel(mNetworkPanel, mNetworkPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
	mNetworkPanel->setDefaultBtn(default_btn);

	mWebPanel = new LLPanelWeb();
	mTabContainer->addTabPanel(mWebPanel, mWebPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
	mWebPanel->setDefaultBtn(default_btn);

	mDisplayPanel = new LLPanelDisplay();
	mTabContainer->addTabPanel(mDisplayPanel, mDisplayPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
	mDisplayPanel->setDefaultBtn(default_btn);

	mAudioPanel = new LLPanelAudioPrefs();
	mTabContainer->addTabPanel(mAudioPanel, mAudioPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
	mAudioPanel->setDefaultBtn(default_btn);

	mPrefsChat = new LLPrefsChat();
	mTabContainer->addTabPanel(mPrefsChat->getPanel(), mPrefsChat->getPanel()->getLabel(), FALSE, onTabChanged, mTabContainer);
	mPrefsChat->getPanel()->setDefaultBtn(default_btn);

	mPrefsVoice = new LLPrefsVoice();
	mTabContainer->addTabPanel(mPrefsVoice, mPrefsVoice->getLabel(), FALSE, onTabChanged, mTabContainer);
	mPrefsVoice->setDefaultBtn(default_btn);

	mPrefsIM = new LLPrefsIM();
	mTabContainer->addTabPanel(mPrefsIM->getPanel(), mPrefsIM->getPanel()->getLabel(), FALSE, onTabChanged, mTabContainer);
	mPrefsIM->getPanel()->setDefaultBtn(default_btn);

#if LL_WINDOWS && LL_LCD_COMPILE

	// only add this option if we actually have a logitech keyboard / speaker set
	if (gLcdScreen->Enabled())
	{
		mLCDPanel = new LLPanelLCD();
		mTabContainer->addTabPanel(mLCDPanel, mLCDPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
		mLCDPanel->setDefaultBtn(default_btn);
	}

#else
	mLCDPanel = NULL;
#endif

	mMsgPanel = new LLPanelMsgs();
	mTabContainer->addTabPanel(mMsgPanel, mMsgPanel->getLabel(), FALSE, onTabChanged, mTabContainer);
	mMsgPanel->setDefaultBtn(default_btn);

	mTabContainer->selectTab(gSavedSettings.getS32("LastPrefTab"));
}

LLPreferenceCore::~LLPreferenceCore()
{
	if (mGeneralPanel)
	{
		delete mGeneralPanel;
		mGeneralPanel = NULL;
	}
	if (mInputPanel)
	{
		delete mInputPanel;
		mInputPanel = NULL;
	}
	if (mNetworkPanel)
	{
		delete mNetworkPanel;
		mNetworkPanel = NULL;
	}
	if (mDisplayPanel)
	{
		delete mDisplayPanel;
		mDisplayPanel = NULL;
	}

	if (mAudioPanel)
	{
		delete mAudioPanel;
		mAudioPanel = NULL;
	}
	if (mPrefsChat)
	{
		delete mPrefsChat;
		mPrefsChat = NULL;
	}
	if (mPrefsIM)
	{
		delete mPrefsIM;
		mPrefsIM = NULL;
	}
	if (mMsgPanel)
	{
		delete mMsgPanel;
		mMsgPanel = NULL;
	}
	if (mWebPanel)
	{
		delete mWebPanel;
		mWebPanel = NULL;
	}
}


void LLPreferenceCore::apply()
{
	mGeneralPanel->apply();
	mInputPanel->apply();
	mNetworkPanel->apply();
	mDisplayPanel->apply();
	mPrefsChat->apply();
	mPrefsVoice->apply();
	mPrefsIM->apply();
	mMsgPanel->apply();

	// hardware menu apply
	LLFloaterHardwareSettings::instance()->apply();

	mWebPanel->apply();
#if LL_WINDOWS && LL_LCD_COMPILE
	// only add this option if we actually have a logitech keyboard / speaker set
	if (gLcdScreen->Enabled())
	{
		mLCDPanel->apply();
	}
#endif
//	mWebPanel->apply();
}


void LLPreferenceCore::cancel()
{
	mGeneralPanel->cancel();
	mInputPanel->cancel();
	mNetworkPanel->cancel();
	mDisplayPanel->cancel();
	mAudioPanel->cancel();
	mPrefsChat->cancel();
	mPrefsVoice->cancel();
	mPrefsIM->cancel();
	mMsgPanel->cancel();

	// cancel hardware menu
	LLFloaterHardwareSettings::instance()->cancel();

	mWebPanel->cancel();
#if LL_WINDOWS && LL_LCD_COMPILE
	// only add this option if we actually have a logitech keyboard / speaker set
	if (gLcdScreen->Enabled())
	{
		mLCDPanel->cancel();
	}
#endif
//	mWebPanel->cancel();
}

// static
void LLPreferenceCore::onTabChanged(void* user_data, bool from_click)
{
	LLTabContainer* self = (LLTabContainer*)user_data;

	gSavedSettings.setS32("LastPrefTab", self->getCurrentPanelIndex());
}


void LLPreferenceCore::setPersonalInfo(
	const char* visibility,
	BOOL im_via_email,
	const char* email)
{
	mPrefsIM->setPersonalInfo(visibility, im_via_email, email);
}

void LLPreferenceCore::refreshEnabledGraphics()
{
	LLFloaterHardwareSettings::instance()->refreshEnabledState();
	mDisplayPanel->refreshEnabledState();
}

//////////////////////////////////////////////
// LLFloaterPreference

LLFloaterPreference::LLFloaterPreference()
{
	gUICtrlFactory->buildFloater(this, "floater_preferences.xml");
}

BOOL LLFloaterPreference::postBuild()
{
	requires("About...", WIDGET_TYPE_BUTTON);
	requires("OK", WIDGET_TYPE_BUTTON);
	requires("Cancel", WIDGET_TYPE_BUTTON);
	requires("Apply", WIDGET_TYPE_BUTTON);
	requires("pref core", WIDGET_TYPE_TAB_CONTAINER);

	if (!checkRequirements())
	{
		return FALSE;
	}

	mAboutBtn = LLUICtrlFactory::getButtonByName(this, "About...");
	mAboutBtn->setClickedCallback(onClickAbout, this);
	
	mApplyBtn = LLUICtrlFactory::getButtonByName(this, "Apply");
	mApplyBtn->setClickedCallback(onBtnApply, this);
		
	mCancelBtn = LLUICtrlFactory::getButtonByName(this, "Cancel");
	mCancelBtn->setClickedCallback(onBtnCancel, this);

	mOKBtn = LLUICtrlFactory::getButtonByName(this, "OK");
	mOKBtn->setClickedCallback(onBtnOK, this);
			
	mPreferenceCore = new LLPreferenceCore(
		LLUICtrlFactory::getTabContainerByName(this, "pref core"),
		getChild<LLButton>("OK")
		);
	
	sInstance = this;

	return TRUE;
}


LLFloaterPreference::~LLFloaterPreference()
{
	sInstance = NULL;
	delete mPreferenceCore;
}


void LLFloaterPreference::draw()
{
	if( getVisible() )
	{
		LLFloater::draw();
	}
}


void LLFloaterPreference::apply()
{
	this->mPreferenceCore->apply();
}


void LLFloaterPreference::cancel()
{
	this->mPreferenceCore->cancel();
}


// static
void LLFloaterPreference::show(void*)
{
	if (!sInstance)
	{
		new LLFloaterPreference();
		sInstance->center();
	}

	sInstance->open();		/* Flawfinder: ignore */

	if(!gAgent.getID().isNull())
	{
		// we're logged in, so we can get this info.
		gMessageSystem->newMessageFast(_PREHASH_UserInfoRequest);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
	}

	LLPanelLogin::setAlwaysRefresh(true);
}


// static
void LLFloaterPreference::onClickAbout(void*)
{
	LLFloaterAbout::show(NULL);
}


// static 
void LLFloaterPreference::onBtnOK( void* userdata )
{
	LLPanelLogin::refreshLocation( false );

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
		fp->close(false);

		gSavedSettings.saveToFile( gSavedSettings.getString("ClientSettingsFile"), TRUE );
		
		std::string crash_settings_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
		// save all settings, even if equals defaults
		gCrashSettings.saveToFile(crash_settings_filename.c_str(), FALSE);
	}
	else
	{
		// Show beep, pop up dialog, etc.
		llinfos << "Can't close preferences!" << llendl;
	}
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


void LLFloaterPreference::onClose(bool app_quitting)
{
	LLPanelLogin::setAlwaysRefresh(false);
	LLFloater::onClose(app_quitting);
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
	fp->cancel();
	fp->close();
}


// static
void LLFloaterPreference::updateUserInfo(
	const char* visibility,
	BOOL im_via_email,
	const char* email)
{
	if(sInstance && sInstance->mPreferenceCore)
	{
		sInstance->mPreferenceCore->setPersonalInfo(
			visibility, im_via_email, email);
	}
}

void LLFloaterPreference::refreshEnabledGraphics()
{
	sInstance->mPreferenceCore->refreshEnabledGraphics();
}
