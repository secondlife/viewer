/**
 * @file llpanelmediasettingspermissions.h
 * @brief LLPanelMediaSettingsPermissions class definition
 *
 * note that "permissions" tab is really "Controls" tab - refs to 'perms' and
 * 'permissions' not changed to 'controls' since we don't want to change 
 * shared files in server code and keeping everything the same seemed best.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLPANELMEDIAMEDIASETTINGSPERMISSIONS_H
#define LL_LLPANELMEDIAMEDIASETTINGSPERMISSIONS_H

#include "llpanel.h"
#include "lluuid.h"

class LLComboBox;
class LLCheckBoxCtrl;
class LLNameBox;

class LLPanelMediaSettingsPermissions : public LLPanel
{
public:
	LLPanelMediaSettingsPermissions();
	~LLPanelMediaSettingsPermissions();
	
	BOOL postBuild();
	virtual void draw();
	
	// XXX TODO: put these into a common parent class?
	// Hook that the floater calls before applying changes from the panel
	void preApply();
	// Function that asks the panel to fill in values associated with the panel
	// 'include_tentative' means fill in tentative values as well, otherwise do not
	void getValues(LLSD &fill_me_in, bool include_tentative = true);
	// Hook that the floater calls after applying changes to the panel
	void postApply();
	
	static void initValues( void* userdata, const LLSD& media_settings, bool editable );
	static void clearValues( void* userdata,  bool editable);
	
private:
	LLComboBox* mControls;
	LLCheckBoxCtrl* mPermsOwnerInteract;
	LLCheckBoxCtrl* mPermsOwnerControl;
	LLNameBox* mPermsGroupName;
	LLCheckBoxCtrl* mPermsGroupInteract;
	LLCheckBoxCtrl* mPermsGroupControl;
	LLCheckBoxCtrl* mPermsWorldInteract;
	LLCheckBoxCtrl* mPermsWorldControl;
};

#endif  // LL_LLPANELMEDIAMEDIASETTINGSPERMISSIONS_H
