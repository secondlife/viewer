/** 
 * @file llnavigationbar.h
 * @brief Navigation bar definition
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
 * Dragging direction can be set from xml attribute called 'direction'
 * 
 * *TODO: move to llui?  
 */

class LLPullButton: public LLButton
{
	LOG_CLASS(LLPullButton);

public:
	struct Params: public LLInitParam::Block<Params, LLButton::Params>
	{
		Optional<std::string> direction; // left, right, down, up

		Params() 
		:	direction("direction", "down")
		{
		}
	};
	
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);

	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);

	boost::signals2::connection setClickDraggingCallback(const commit_signal_t::slot_type& cb);

protected:
	friend class LLUICtrlFactory;
	// convert string name into direction vector
	void setDirectionFromName(const std::string& name);
	LLPullButton(const LLPullButton::Params& params);

	commit_signal_t mClickDraggingSignal;
	LLVector2 mLastMouseDown;
	LLVector2 mDraggingDirection;
};

/**
 * Web browser-like navigation bar.
 */ 
class LLNavigationBar
	:	public LLPanel, public LLSingleton<LLNavigationBar>, private LLDestroyClass<LLNavigationBar>
{
	LOG_CLASS(LLNavigationBar);
	friend class LLDestroyClass<LLNavigationBar>;
	
public:
	LLNavigationBar();
	virtual ~LLNavigationBar();
	
	/*virtual*/ void	draw();
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	postBuild();
	/*virtual*/ void	setVisible(BOOL visible);

	void handleLoginComplete();
	void clearHistoryCache();

	int getDefNavBarHeight();
	int getDefFavBarHeight();
	
private:
	// the distance between navigation panel and favorites panel in pixels
	const static S32 FAVBAR_TOP_PADDING = 10;

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
	void onTeleportFinished(const LLVector3d& global_agent_pos);
	void onTeleportFailed();
	void onRegionNameResponse(
			std::string typed_location,
			std::string region_name,
			LLVector3 local_coords,
			U64 region_handle, const std::string& url,
			const LLUUID& snapshot_id, bool teleport);

	static void destroyClass()
	{
		if (LLNavigationBar::instanceExists())
		{
			LLNavigationBar::getInstance()->setEnabled(FALSE);
		}
	}

	LLMenuGL*					mTeleportHistoryMenu;
	LLPullButton*				mBtnBack;
	LLPullButton*				mBtnForward;
	LLButton*					mBtnHome;
	LLLocationInputCtrl*		mCmbLocation;
	LLRect						mDefaultNbRect;
	LLRect						mDefaultFpRect;
	boost::signals2::connection	mTeleportFailedConnection;
	boost::signals2::connection	mTeleportFinishConnection;
	boost::signals2::connection	mHistoryMenuConnection;
	// if true, save location to location history when teleport finishes
	bool						mSaveToLocationHistory;
};

#endif
