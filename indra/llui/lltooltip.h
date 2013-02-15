/** 
 * @file lltooltip.h
 * @brief LLToolTipMgr class definition and related classes
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLTOOLTIP_H
#define LL_LLTOOLTIP_H

// Library includes
#include "llsingleton.h"
#include "llinitparam.h"
#include "llpanel.h"
#include "llstyle.h"

//
// Classes
//
class LLToolTipView : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Params();
	};
	LLToolTipView(const LLToolTipView::Params&);
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleScrollWheel( S32 x, S32 y, S32 clicks );

	void drawStickyRect();

	/*virtual*/ void draw();
};

class LLToolTip : public LLPanel
{
public:

	struct StyledText : public LLInitParam::Block<StyledText>
	{
		Mandatory<std::string>		text;
		Optional<LLStyle::Params>	style;
	};

	struct Params : public LLInitParam::Block<Params, LLPanel::Params> 
	{
		typedef boost::function<void(void)> click_callback_t;

		Optional<std::string>		message;
		Multiple<StyledText>		styled_message;

		Optional<LLCoordGL>			pos;
		Optional<F32>				delay_time,
									visible_time_over,  // time for which tooltip is visible while mouse on it
									visible_time_near,	// time for which tooltip is visible while mouse near it
									visible_time_far;	// time for which tooltip is visible while mouse moved away
		Optional<LLRect>			sticky_rect;
		Optional<const LLFontGL*>	font;
		Optional<LLUIImage*>		image;
		Optional<LLUIColor>			text_color;
		Optional<bool>				time_based_media,
									web_based_media,
									media_playing;
		Optional<click_callback_t>	click_callback,
									click_playmedia_callback,
									click_homepage_callback;
		Optional<S32>				max_width,
									padding;
		Optional<bool>				wrap;

		Params();
	};
	/*virtual*/ void draw();
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
	/*virtual*/ void setVisible(BOOL visible);

	bool isFading();
	F32 getVisibleTime();
	bool hasClickCallback();

	LLToolTip(const Params& p);
	void initFromParams(const LLToolTip::Params& params);

	void getToolTipMessage(std::string & message);

private:
	class LLTextBox*	mTextBox;
	class LLButton*     mInfoButton;
	class LLButton*     mPlayMediaButton;
	class LLButton*     mHomePageButton;

	LLFrameTimer	mFadeTimer;
	LLFrameTimer	mVisibleTimer;
	bool			mHasClickCallback;
	S32				mPadding;	// pixels
};

// used for the inspector tooltips which need different background images etc.
class LLInspector : public LLToolTip
{
public:
	struct Params : public LLInitParam::Block<Params, LLToolTip::Params> 
	{};
};

class LLToolTipMgr : public LLSingleton<LLToolTipMgr>
{
	LOG_CLASS(LLToolTipMgr);
public:
	LLToolTipMgr();
	void show(const LLToolTip::Params& params);
	void show(const std::string& message);

	void unblockToolTips();
	void blockToolTips();

	void hideToolTips();
	bool toolTipVisible();
	LLRect getToolTipRect();
	LLRect getMouseNearRect();
	void updateToolTipVisibility();

	void getToolTipMessage(std::string & message);

private:
	void createToolTip(const LLToolTip::Params& params);

	bool				mToolTipsBlocked;
	class LLToolTip*	mToolTip;

	// tooltip creation is deferred until the UI is drawn every frame
	// so the last tooltip to be created in a given frame will win
	LLToolTip::Params	mLastToolTipParams;	// description of last tooltip we showed
	LLToolTip::Params	mNextToolTipParams; // description of next tooltip we want to show
	bool				mNeedsToolTip;		// do we want to show a tooltip

	LLRect				mMouseNearRect;
};

//
// Globals
//

extern LLToolTipView *gToolTipView;

#endif
