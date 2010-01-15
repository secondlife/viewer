/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
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

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"

class LLPanelPreference;
class LLPanelLCD;
class LLPanelDebug;
class LLMessageSystem;
class LLScrollListCtrl;
class LLSliderCtrl;
class LLSD;
class LLTextBox;

typedef enum
	{
		GS_LOW_GRAPHICS,
		GS_MID_GRAPHICS,
		GS_HIGH_GRAPHICS,
		GS_ULTRA_GRAPHICS
		
	} EGraphicsSettings;


// Floater to control preferences (display, audio, bandwidth, general.
class LLFloaterPreference : public LLFloater
{
public: 
	LLFloaterPreference(const LLSD& key);
	~LLFloaterPreference();

	void apply();
	void cancel();
	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	// static data update, called from message handler
	static void updateUserInfo(const std::string& visibility, bool im_via_email, const std::string& email);

	// refresh all the graphics preferences menus
	static void refreshEnabledGraphics();
	
protected:	
	void		onBtnOK();
	void		onBtnCancel();
	void		onBtnApply();

	void		onClickBrowserClearCache();

	// if the custom settings box is clicked
	void onChangeCustom();
	void updateMeterText(LLUICtrl* ctrl);
	void onOpenHardwareSettings();
	/// callback for defaults
	void setHardwareDefaults();
	// callback for when client turns on shaders
	void onVertexShaderEnable();
	
	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.	
	void saveSettings();
		

public:

	void onClickSetCache();
	void onClickResetCache();
	void onClickSkin(LLUICtrl* ctrl,const LLSD& userdata);
	void onSelectSkin();
	void onClickSetKey();
	void setKey(KEY key);
	void onClickSetMiddleMouse();
	void onClickSkipDialogs();
	void onClickResetDialogs();
	void onClickEnablePopup();
	void onClickDisablePopup();	
	void resetAllIgnored();
	void setAllIgnored();
	void onClickLogPath();	
	void enableHistory();
	void setPersonalInfo(const std::string& visibility, bool im_via_email, const std::string& email);
	void refreshEnabledState();
	void disableUnavailableSettings();
	void onCommitWindowedMode();
	void refresh();	// Refresh enable/disable
	// if the quality radio buttons are changed
	void onChangeQuality(const LLSD& data);
	
	void updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box);
	void onUpdateSliderText(LLUICtrl* ctrl, const LLSD& name);
	void onKeystrokeAspectRatio();
//	void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator);

	void onCommitAutoDetectAspect();
	void onCommitParcelMediaAutoPlayEnable();
	void onCommitMediaEnabled();
	void applyResolution();
	void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
	void getUIColor(LLUICtrl* ctrl, const LLSD& param);	
	
	void buildPopupLists();
	static void refreshSkin(void* data);
	static void cleanupBadSetting();
	static F32 sAspectRatio;	
private:
	static std::string sSkin;
	bool mGotPersonalInfo;
	bool mOriginalIMViaEmail;
	
	bool mOriginalHideOnlineStatus;
	std::string mDirectoryVisibility;
};

class LLPanelPreference : public LLPanel
{
public:
	LLPanelPreference();
	/*virtual*/ BOOL postBuild();
	
	virtual void apply();
	virtual void cancel();
	void setControlFalse(const LLSD& user_data);

	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.
	virtual void saveSettings();
	
private:
	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;

	typedef std::map<std::string, LLColor4> string_color_map_t;
	string_color_map_t mSavedColors;
};

#endif  // LL_LLPREFERENCEFLOATER_H
