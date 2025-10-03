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
    bool handleHover(S32 x, S32 y, MASK mask) override;
    bool handleMouseDown(S32 x, S32 y, MASK mask) override;
    bool handleMiddleMouseDown(S32 x, S32 y, MASK mask) override;
    bool handleRightMouseDown(S32 x, S32 y, MASK mask) override;
    bool handleScrollWheel( S32 x, S32 y, S32 clicks ) override;

    void drawStickyRect();

    void draw() override;
};

class LLToolTip : public LLPanel
{
public:

    struct StyledText : public LLInitParam::Block<StyledText>
    {
        Mandatory<std::string>      text;
        Optional<LLStyle::Params>   style;
    };

    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {
        typedef boost::function<void(void)> click_callback_t;
        typedef boost::function<LLToolTip*(LLToolTip::Params)> create_callback_t;

        Optional<std::string>       message;
        Multiple<StyledText>        styled_message;

        Optional<LLCoordGL>         pos;
        Optional<F32>               delay_time,
                                    visible_time_over,  // time for which tooltip is visible while mouse on it
                                    visible_time_near,  // time for which tooltip is visible while mouse near it
                                    visible_time_far;   // time for which tooltip is visible while mouse moved away
        Optional<LLRect>            sticky_rect;
        Optional<const LLFontGL*>   font;
        Optional<LLUIImage*>        image;
        Optional<LLUIColor>         text_color;
        Optional<bool>              time_based_media,
                                    web_based_media,
                                    media_playing;
        Optional<create_callback_t> create_callback;
        Optional<LLSD>              create_params;
        Optional<click_callback_t>  click_callback,
                                    click_playmedia_callback,
                                    click_homepage_callback;
        Optional<S32>               max_width,
                                    padding;
        Optional<bool>              wrap;

        Optional<bool> allow_paste_tooltip;

        Params();
    };
    void draw() override;
    bool handleHover(S32 x, S32 y, MASK mask) override;
    void onMouseLeave(S32 x, S32 y, MASK mask) override;
    void setVisible(bool visible) override;

    bool isFading() const;
    F32 getVisibleTime() const;
    bool hasClickCallback() const;

    LLToolTip(const Params& p);
    virtual void initFromParams(const LLToolTip::Params& params);

    void getToolTipMessage(std::string & message) const;
    bool isTooltipPastable() const { return mIsTooltipPastable; }

protected:
    void updateTextBox();
    void snapToChildren();

protected:
    class LLTextBox*    mTextBox;
    class LLButton*     mInfoButton;
    class LLButton*     mPlayMediaButton;
    class LLButton*     mHomePageButton;

    LLFrameTimer    mFadeTimer;
    LLFrameTimer    mVisibleTimer;
    bool            mHasClickCallback;
    S32             mPadding;   // pixels
    S32             mMaxWidth;

    bool mIsTooltipPastable;
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
    LLSINGLETON(LLToolTipMgr);
    LOG_CLASS(LLToolTipMgr);

public:
    void show(const LLToolTip::Params& params);
    void show(const std::string& message, bool allow_paste_tooltip = false);

    void unblockToolTips();
    void blockToolTips();

    void hideToolTips();
    bool toolTipVisible();
    LLRect getToolTipRect();
    LLRect getMouseNearRect();
    void updateToolTipVisibility();

    void getToolTipMessage(std::string & message);
    bool isTooltipPastable();

private:
    void createToolTip(const LLToolTip::Params& params);

    bool                mToolTipsBlocked;
    class LLToolTip*    mToolTip;

    // tooltip creation is deferred until the UI is drawn every frame
    // so the last tooltip to be created in a given frame will win
    LLToolTip::Params   mLastToolTipParams; // description of last tooltip we showed
    LLToolTip::Params   mNextToolTipParams; // description of next tooltip we want to show
    bool                mNeedsToolTip;      // do we want to show a tooltip

    LLRect              mMouseNearRect;
};

//
// Globals
//

extern LLToolTipView *gToolTipView;

#endif
