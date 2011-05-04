/** 
 * @file llfloaterpreference.h
 * @brief LLPreferenceCore class definition
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

#ifndef LL_LLFLOATERPREFERENCE_H
#define LL_LLFLOATERPREFERENCE_H

#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"

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
class LLFloaterPreference : public LLFloater, public LLAvatarPropertiesObserver
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
	
	// translate user's busy response message according to current locale if message is default, otherwise do nothing
	static void initBusyResponse();

	void processProperties( void* pData, EAvatarProcessorType type );
	void processProfileProperties(const LLAvatarData* pAvatarData );
	void storeAvatarProperties( const LLAvatarData* pAvatarData );
	void saveAvatarProperties( void );

protected:	
	void		onBtnOK();
	void		onBtnCancel();
	void		onBtnApply();

	void		onClickBrowserClearCache();
	void		onLanguageChange();
	void		onNameTagOpacityChange(const LLSD& newvalue);

	// set value of "BusyResponseChanged" in account settings depending on whether busy response
	// string differs from default after user changes.
	void onBusyResponseChanged();
	// if the custom settings box is clicked
	void onChangeCustom();
	void updateMeterText(LLUICtrl* ctrl);
	void onOpenHardwareSettings();
	/// callback for defaults
	void setHardwareDefaults();
	// callback for when client turns on shaders
	void onVertexShaderEnable();
	// callback for changing double click action checkbox
	void onDoubleClickCheckBox(LLUICtrl* ctrl);
	// callback for selecting double click action radio-button
	void onDoubleClickRadio();
	// updates double-click action settings depending on controls from preferences
	void updateDoubleClickSettings();
	// updates double-click action controls depending on values from settings.xml
	void updateDoubleClickControls();
	
	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.	
	void saveSettings();
		

public:

	void setCacheLocation(const LLStringExplicit& location);

	void onClickSetCache();
	void onClickResetCache();
	void onClickSkin(LLUICtrl* ctrl,const LLSD& userdata);
	void onSelectSkin();
	void onClickSetKey();
	void setKey(KEY key);
	void onClickSetMiddleMouse();
//	void onClickSkipDialogs();
//	void onClickResetDialogs();
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
//	void fractionFromDecimal(F32 decimal_val, S32& numerator, S32& denominator);

	void onCommitParcelMediaAutoPlayEnable();
	void onCommitMediaEnabled();
	void onCommitMusicEnabled();
	void applyResolution();
	void onChangeMaturity();
	void onClickBlockList();
	void applyUIColor(LLUICtrl* ctrl, const LLSD& param);
	void getUIColor(LLUICtrl* ctrl, const LLSD& param);
	
	void buildPopupLists();
	static void refreshSkin(void* data);
private:
	static std::string sSkin;
	// set true if state of double-click action checkbox or radio-group was changed by user
	// (reset back to false on apply or cancel)
	bool mDoubleClickActionDirty;
	bool mGotPersonalInfo;
	bool mOriginalIMViaEmail;
	bool mLanguageChanged;
	bool mAvatarDataInitialized;
	
	bool mOriginalHideOnlineStatus;
	std::string mDirectoryVisibility;
	
	LLAvatarData mAvatarProperties;
};

class LLPanelPreference : public LLPanel
{
public:
	LLPanelPreference();
	/*virtual*/ BOOL postBuild();
	
	virtual ~LLPanelPreference();

	virtual void apply();
	virtual void cancel();
	void setControlFalse(const LLSD& user_data);
	virtual void setHardwareDefaults(){};

	// Disables "Allow Media to auto play" check box only when both
	// "Streaming Music" and "Media" are unchecked. Otherwise enables it.
	void updateMediaAutoPlayCheckbox(LLUICtrl* ctrl);

	// This function squirrels away the current values of the controls so that
	// cancel() can restore them.
	virtual void saveSettings();
	
	class Updater;
private:
	//for "Only friends and groups can call or IM me"
	static void showFriendsOnlyWarning(LLUICtrl*, const LLSD&);
	//for "Show my Favorite Landmarks at Login"
	static void showFavoritesOnLoginWarning(LLUICtrl* checkbox, const LLSD& value);

	typedef std::map<LLControlVariable*, LLSD> control_values_map_t;
	control_values_map_t mSavedValues;

	typedef std::map<std::string, LLColor4> string_color_map_t;
	string_color_map_t mSavedColors;

	Updater* mBandWidthUpdater;
};

class LLPanelPreferenceGraphics : public LLPanelPreference
{
public:
	BOOL postBuild();
	void draw();
	void apply();
	void cancel();
	void saveSettings();
	void setHardwareDefaults();
protected:
	bool hasDirtyChilds();
	void resetDirtyChilds();
	
};

#endif  // LL_LLPREFERENCEFLOATER_H
