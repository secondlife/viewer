/** 
 * @file llfloatermediasettings.cpp
 * @brief Tabbed dialog for media settings - class definition
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

	virtual BOOL postBuild();
	static LLFloaterMediaSettings* getInstance();
	static void apply();
	static void initValues( const LLSD& media_settings );
	static void clearValues();
	void enableOkApplyBtns( bool enable );
	LLPanelMediaSettingsSecurity* getPanelSecurity(){return mPanelMediaSettingsSecurity;};

protected:
	LLButton *mOKBtn;
	LLButton *mCancelBtn;
	LLButton *mApplyBtn;

	LLTabContainer *mTabContainer;
	LLPanelMediaSettingsGeneral* mPanelMediaSettingsGeneral;
	LLPanelMediaSettingsSecurity* mPanelMediaSettingsSecurity;
	LLPanelMediaSettingsPermissions* mPanelMediaSettingsPermissions;

	void		onClose();
	static void onBtnOK(void*);
	static void onBtnCancel(void*);
	static void onBtnApply(void*);
	static void onTabChanged(void* user_data, bool from_click);
	void commitFields();

	static LLFloaterMediaSettings* sInstance;

private:
	bool mWaitingToClose;
};

#endif  // LL_LLFLOATERMEDIASETTINGS_H
