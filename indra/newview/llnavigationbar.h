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
#include "llbutton.h"

class LLLocationInputCtrl;
class LLMenuGL;
class LLSearchEditor;
class LLSearchComboBox;

/**
 * This button is able to handle click-dragging mouse event.
 * It has appropriated signal for this event.
 * Dragging direction can be set from xml by attribute called 'direction'
 * 
 * *TODO: move to llui?  
 */

class LLPullButton : public LLButton
{
	LOG_CLASS(LLPullButton);
	
public:
	
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
		Optional<std::string>	direction; // left, right, down, up
		
		Params()
		:	direction("direction","down")
		{}
	};
	
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);

	boost::signals2::connection setClickDraggingCallback( const commit_signal_t::slot_type& cb );
	
	/* virtual*/ ~LLPullButton()
	{
		delete mClickDraggingSignal;
	}
	
protected:
	friend class LLUICtrlFactory;
	// convert string name into direction vector
	void setDirectionFromName(const std::string& name);
	LLPullButton(const LLPullButton::Params& params);
	
	commit_signal_t* mClickDraggingSignal;	
	LLVector2 mLastMouseDown;
	LLVector2 mDraggingDirection;
};

/**
 * Web browser-like navigation bar.
 */ 
class LLNavigationBar
	:	public LLPanel, public LLSingleton<LLNavigationBar>
{
	LOG_CLASS(LLNavigationBar);
	
public:
	LLNavigationBar();
	virtual ~LLNavigationBar();
	
	/*virtual*/ void	draw();
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	postBuild();
	/*virtual*/ void	setVisible(BOOL visible);

	void handleLoginComplete();
	void clearHistoryCache();

	void showNavigationPanel(BOOL visible);
	void showFavoritesPanel(BOOL visible);

	int getDefNavBarHeight();
	int getDefFavBarHeight();
	
private:

	void rebuildTeleportHistoryMenu();
	void showTeleportHistoryMenu(LLUICtrl* btn_ctrl);
	void invokeSearch(std::string search_text);
	// callbacks
	void onTeleportHistoryMenuItemClicked(const LLSD& userdata);
	void onTeleportHistoryChanged();
	void onBackButtonClicked();
	void onBackOrForwardButtonHeldDown(LLUICtrl* ctrl, const LLSD& param);
	void onNavigationButtonHeldUp(LLButton* nav_button);
	void onForwardButtonClicked();
	void onHomeButtonClicked();
	void onLocationSelection();
	void onLocationPrearrange(const LLSD& data);
	void onSearchCommit();
	void onTeleportFinished(const LLVector3d& global_agent_pos);
	void onTeleportFailed();
	void onRegionNameResponse(
			std::string typed_location,
			std::string region_name,
			LLVector3 local_coords,
			U64 region_handle, const std::string& url,
			const LLUUID& snapshot_id, bool teleport);

	void fillSearchComboBox();

	LLMenuGL*					mTeleportHistoryMenu;
	LLPullButton*				mBtnBack;
	LLPullButton*				mBtnForward;
	LLButton*					mBtnHome;
	LLSearchComboBox*			mSearchComboBox;
	LLLocationInputCtrl*		mCmbLocation;
	LLRect						mDefaultNbRect;
	LLRect						mDefaultFpRect;
	boost::signals2::connection	mTeleportFailedConnection;
	boost::signals2::connection	mTeleportFinishConnection;
	boost::signals2::connection	mHistoryMenuConnection;
	bool						mPurgeTPHistoryItems;
	// if true, save location to location history when teleport finishes
	bool						mSaveToLocationHistory;
};

#endif
