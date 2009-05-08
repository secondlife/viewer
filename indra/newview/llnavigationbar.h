/** 
 * @file llnavigationbar.h
 * @brief Navigation bar definition
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
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

#ifndef LL_LLNAVIGATIONBAR_H
#define LL_LLNAVIGATIONBAR_H

#include "llpanel.h"

extern S32 NAVIGATION_BAR_HEIGHT;

class LLButton;
class LLLocationInputCtrl;
class LLMenuGL;
class LLLineEditor;

/**
 * Web browser-like navigation bar.
 */ 
class LLNavigationBar
:	public LLPanel
{
	LOG_CLASS(LLNavigationBar);

public:
	static LLNavigationBar* getInstance();
	virtual ~LLNavigationBar();
	
	/*virtual*/ void	draw();
	/*virtual*/ BOOL	postBuild();
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);

private:
	LLNavigationBar();

	void refreshLocation();
	void rebuildLocationHistory(std::string filter = "");
	void rebuildTeleportHistoryMenu();
	void showTeleportHistoryMenu();
	
	// callbacks
	bool onLocationContextMenuItemEnabled(const LLSD& userdata);
	void onLocationContextMenuItemClicked(const LLSD& userdata);
	void onTeleportHistoryMenuItemClicked(const LLSD& userdata);
	void onTeleportHistoryChanged();
	void onBackButtonClicked();
	void onBackOrForwardButtonHeldDown(const LLSD& param);
	void onForwardButtonClicked();
	void onHomeButtonClicked();
	void onInfoButtonClicked();
	void onHelpButtonClicked();
	void onLocationFocusReceived();
	void onLocationFocusLost();
	void onLocationSelection();
	void onLocationTextEntry(LLUICtrl* ctrl);
	void onLocationPrearrange(const LLSD& data);
	void onLocationHistoryLoaded();
	void onSearchCommit();
	static void onRegionNameResponse(
			LLVector3 local_coords, U64 region_handle, const std::string& url,
			const LLUUID& snapshot_id, bool teleport);

	static void teleport(std::string slurl);

	static LLNavigationBar *sInstance;
	
	LLMenuGL*				mLocationContextMenu;
	LLMenuGL*				mTeleportHistoryMenu;
	LLButton*				mBtnBack;
	LLButton*				mBtnForward;
	LLButton*				mBtnHome;
	LLButton*				mBtnInfo;
	LLButton*				mBtnHelp;
	LLLineEditor*			mLeSearch;
	LLLocationInputCtrl*	mCmbLocation;
};

#endif
