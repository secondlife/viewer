/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"
#include "lltabcontainervertical.h"

class LLPanelGeneral;
class LLPanelInput;
class LLPanelDisplay;
class LLPanelDisplay2;
class LLPanelDisplay3;
class LLPanelAudioPrefs;
class LLPanelDebug;
class LLPanelNetwork;
class LLMessageSystem;
class LLPrefsChat;
class LLPrefsIM;
class LLPanelMsgs;
//class LLPanelWeb;		// Web prefs removed from Loopy build 
class LLScrollListCtrl;

class LLPreferenceCore
{

public:
	LLPreferenceCore(LLTabContainerCommon* tab_container, LLButton * default_btn);
	~LLPreferenceCore();

	void apply();
	void cancel();

	LLTabContainerCommon* getTabContainer() { return mTabContainer; }

	void setPersonalInfo(
		const char* visibility,
		BOOL im_via_email,
		const char* email);

	static void onTabChanged(void* user_data, bool from_click);

private:
	LLTabContainerCommon	*mTabContainer;
	LLPanelGeneral	        *mGeneralPanel;
	LLPanelInput			*mInputPanel;
	LLPanelNetwork	        *mNetworkPanel;
	LLPanelDisplay	        *mDisplayPanel;
	LLPanelDisplay2			*mDisplayPanel2;
	LLPanelDisplay3			*mDisplayPanel3;
	LLPanelAudioPrefs		*mAudioPanel;
//	LLPanelDebug			*mDebugPanel;
	LLPrefsChat				*mPrefsChat;
	LLPrefsIM				*mPrefsIM;
	LLPanelMsgs				*mMsgPanel;
//	LLPanelWeb*				mWebPanel;
};

// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater
{
public: 
	LLFloaterPreference();
	~LLFloaterPreference();

	void apply();
	void cancel();
	virtual BOOL postBuild();
	static void show(void*);

	// static data update, called from message handler
	static void updateUserInfo(
		const char* visibility,
		BOOL im_via_email,
		const char* email);

protected:
	LLPreferenceCore		*mPreferenceCore;

	/*virtual*/ void		draw();

	LLButton*	mAboutBtn;
	LLButton	*mOKBtn;
	LLButton	*mCancelBtn;
	LLButton	*mApplyBtn;

	static void		onClickAbout(void*);
	static void		onBtnOK(void*);
	static void		onBtnCancel(void*);
	static void		onBtnApply(void*);

	static LLFloaterPreference* sInstance;
};

#endif  // LL_LLPREFERENCEFLOATER_H
