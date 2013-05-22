/** 
 * @file llfloatermediasettings.cpp
 * @brief Tabbed dialog for media settings - class definition
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

#ifndef LL_LLFLOATERMEDIASETTINGS_H
#define LL_LLFLOATERMEDIASETTINGS_H

#include "llfloater.h"
#include "lltabcontainer.h"

class LLPanelMediaSettingsGeneral;
class LLPanelMediaSettingsSecurity;
class LLPanelMediaSettingsPermissions;

class LLFloaterMediaSettings : 
	public LLFloater
{
public: 
	LLFloaterMediaSettings(const LLSD& key);
	~LLFloaterMediaSettings();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);

	static LLFloaterMediaSettings* getInstance();
	static bool instanceExists();
	static void apply();
	static void initValues( const LLSD& media_settings , bool editable);
	static void clearValues( bool editable);

	LLPanelMediaSettingsSecurity* getPanelSecurity(){return mPanelMediaSettingsSecurity;};	
	const std::string getHomeUrl();	
	//bool passesWhiteList( const std::string& test_url );

	virtual void	draw();

	bool mIdenticalHasMediaInfo;
	bool mMultipleMedia;
	bool mMultipleValidMedia;
	
protected:
	LLButton *mOKBtn;
	LLButton *mCancelBtn;
	LLButton *mApplyBtn;

	LLTabContainer *mTabContainer;
	LLPanelMediaSettingsGeneral* mPanelMediaSettingsGeneral;
	LLPanelMediaSettingsSecurity* mPanelMediaSettingsSecurity;
	LLPanelMediaSettingsPermissions* mPanelMediaSettingsPermissions;

	static void onBtnOK(void*);
	static void onBtnCancel(void*);
	static void onBtnApply(void*);
	static void onTabChanged(void* user_data, bool from_click);
	void commitFields();

	static LLFloaterMediaSettings* sInstance;

private:

	bool haveValuesChanged() const;
	
	LLSD mInitialValues;
	bool mWaitingToClose;
};

#endif  // LL_LLFLOATERMEDIASETTINGS_H
