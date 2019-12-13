/** 
 * @file llstatusbar.h
 * @brief LLStatusBar class definition
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

#ifndef LL_LLSTATUSBAR_H
#define LL_LLSTATUSBAR_H

#include "llpanel.h"

// "Constants" loaded from settings.xml at start time
extern S32 STATUS_BAR_HEIGHT;

class LLButton;
class LLLineEditor;
class LLMessageSystem;
class LLTextBox;
class LLTextEditor;
class LLUICtrl;
class LLUUID;
class LLFrameTimer;
class LLStatGraph;
class LLPanelPresetsCameraPulldown;
class LLPanelPresetsPulldown;
class LLPanelVolumePulldown;
class LLPanelNearByMedia;
class LLIconCtrl;
class LLSearchEditor;

namespace ll
{
	namespace statusbar
	{
		struct SearchData;
	}
}
class LLStatusBar
:	public LLPanel
{
public:
	LLStatusBar(const LLRect& rect );
	/*virtual*/ ~LLStatusBar();
	
	/*virtual*/ void draw();

	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL postBuild();

	// MANIPULATORS
	void		setBalance(S32 balance);
	void		debitBalance(S32 debit);
	void		creditBalance(S32 credit);

	// Request the latest currency balance from the server
	static void sendMoneyBalanceRequest();

	void		setHealth(S32 percent);

	void setLandCredit(S32 credit);
	void setLandCommitted(S32 committed);

	void		refresh();
	void setVisibleForMouselook(bool visible);
		// some elements should hide in mouselook

	// ACCESSORS
	S32			getBalance() const;
	S32			getHealth() const;

	BOOL isUserTiered() const;
	S32 getSquareMetersCredit() const;
	S32 getSquareMetersCommitted() const;
	S32 getSquareMetersLeft() const;

	LLPanelNearByMedia* getNearbyMediaPanel() { return mPanelNearByMedia; }

private:
	
	void onClickBuyCurrency();
	void onVolumeChanged(const LLSD& newvalue);

	void onMouseEnterPresetsCamera();
	void onMouseEnterPresets();
	void onMouseEnterVolume();
	void onMouseEnterNearbyMedia();
	void onClickScreen(S32 x, S32 y);

	static void onClickMediaToggle(void* data);
	static void onClickBalance(void* data);

	LLSearchEditor *mFilterEdit;
	LLPanel *mSearchPanel;
	void onUpdateFilterTerm();

	std::unique_ptr< ll::statusbar::SearchData > mSearchData;
	void collectSearchableItems();
	void updateMenuSearchVisibility( const LLSD& data );
	void updateMenuSearchPosition(); // depends onto balance position
	void updateBalancePanelPosition();

private:
	LLTextBox	*mTextTime;

	LLStatGraph *mSGBandwidth;
	LLStatGraph *mSGPacketLoss;

	LLIconCtrl	*mIconPresetsCamera;
	LLIconCtrl	*mIconPresetsGraphic;
	LLButton	*mBtnVolume;
	LLTextBox	*mBoxBalance;
	LLButton	*mMediaToggle;
	LLFrameTimer	mClockUpdateTimer;

	S32				mBalance;
	S32				mHealth;
	S32				mSquareMetersCredit;
	S32				mSquareMetersCommitted;
	LLFrameTimer*	mBalanceTimer;
	LLFrameTimer*	mHealthTimer;
	LLPanelPresetsCameraPulldown* mPanelPresetsCameraPulldown;
	LLPanelPresetsPulldown* mPanelPresetsPulldown;
	LLPanelVolumePulldown* mPanelVolumePulldown;
	LLPanelNearByMedia*	mPanelNearByMedia;
};

// *HACK: Status bar owns your cached money balance. JC
BOOL can_afford_transaction(S32 cost);

extern LLStatusBar *gStatusBar;

#endif
