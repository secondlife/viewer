/** 
 * @file lltooltip.h
 * @brief LLToolTipMgr class definition and related classes
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
