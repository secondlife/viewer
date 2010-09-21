/**
 * @file llpanelmediasettingssecurity.h
 * @brief LLPanelMediaSettingsSecurity class definition
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

#ifndef LL_LLPANELMEDIAMEDIASETTINGSSECURITY_H
#define LL_LLPANELMEDIAMEDIASETTINGSSECURITY_H

#include "llpanel.h"

class LLCheckBoxCtrl;
class LLScrollListCtrl;
class LLTextBox;
class LLFloaterMediaSettings;

class LLPanelMediaSettingsSecurity : public LLPanel
{
public:
	LLPanelMediaSettingsSecurity();
	~LLPanelMediaSettingsSecurity();
	
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
	
	static void initValues( void* userdata, const LLSD& media_settings, bool editable);
	static void clearValues( void* userdata, bool editable);
	void addWhiteListEntry( const std::string& url );
	void setParent( LLFloaterMediaSettings* parent );
	bool urlPassesWhiteList( const std::string& test_url );
	const std::string makeValidUrl( const std::string& src_url );

	void updateWhitelistEnableStatus();	

protected:
	LLFloaterMediaSettings* mParent;
	
private:
	enum ColumnIndex 
	{
		ICON_COLUMN = 0,
		ENTRY_COLUMN = 1,
	};

	LLCheckBoxCtrl* mEnableWhiteList;
	LLScrollListCtrl* mWhiteListList;
	LLTextBox* mHomeUrlFailsWhiteListText;

	static void onBtnAdd(void*);
	static void onBtnDel(void*);
};

#endif  // LL_LLPANELMEDIAMEDIASETTINGSSECURITY_H
