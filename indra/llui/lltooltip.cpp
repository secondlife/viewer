/** 
 * @file lltooltip.cpp
 * @brief LLToolTipMgr class implementation and related classes
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

#include "linden_common.h"

// self include
#include "lltooltip.h"

// Library includes
#include "llpanel.h"
#include "lltextbox.h"
#include "lliconctrl.h"
#include "llui.h"			// positionViewNearMouse()
#include "llwindow.h"

//
// Constants
//
const F32 DELAY_BEFORE_SHOW_TIP = 0.35f;

//
// Local globals
//

LLToolTipView *gToolTipView = NULL;

//
// Member functions
//

LLToolTipView::LLToolTipView(const LLToolTipView::Params& p)
:	LLView(p)
{
}

void LLToolTipView::draw()
{
	if (LLUI::getWindow()->isCursorHidden() )
	{
		LLToolTipMgr::instance().hideToolTips();
	}

	// do the usual thing
	LLView::draw();
}

BOOL LLToolTipView::handleHover(S32 x, S32 y, MASK mask)
{
	static S32 last_x = x;
	static S32 last_y = y;

	LLToolTipMgr& tooltip_mgr = LLToolTipMgr::instance();

	// hide existing tooltips when mouse moves out of sticky rect
	if (tooltip_mgr.toolTipVisible() 
		&& !tooltip_mgr.getStickyRect().pointInRect(x, y))
	{
		tooltip_mgr.hideToolTips();
	}

	// allow new tooltips whenever mouse moves
	if (x != last_x && y != last_y)
	{
		tooltip_mgr.enableToolTips();
	}

	last_x = x;
	last_y = y;
	return LLView::handleHover(x, y, mask);
}

BOOL LLToolTipView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().hideToolTips();
	return LLView::handleMouseDown(x, y, mask);
}

BOOL LLToolTipView::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().hideToolTips();
	return LLView::handleMiddleMouseDown(x, y, mask);
}

BOOL LLToolTipView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().hideToolTips();
	return LLView::handleRightMouseDown(x, y, mask);
}


BOOL LLToolTipView::handleScrollWheel( S32 x, S32 y, S32 clicks )
{
	LLToolTipMgr::instance().hideToolTips();
	return FALSE;
}

void LLToolTipView::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().hideToolTips();
}


void LLToolTipView::drawStickyRect()
{
	gl_rect_2d(LLToolTipMgr::instance().getStickyRect(), LLColor4::white, false);
}
//
// LLToolTip
//
class LLToolTip : public LLPanel
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params> 
	{
		Mandatory<F32>				visible_time;
	
		Optional<LLToolTipParams::click_callback_t>	click_callback;
		Optional<LLUIImage*>						image;

		Params()
		{
			//use_bounding_rect = true;
		}
	};
	/*virtual*/ void draw();
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);

	/*virtual*/ void setValue(const LLSD& value);
	/*virtual*/ void setVisible(BOOL visible);

	bool isFading() { return mFadeTimer.getStarted(); }

	LLToolTip(const Params& p);

private:
	LLTextBox*		mTextBox;
	LLFrameTimer	mFadeTimer;
	F32				mVisibleTime;
	bool			mHasClickCallback;
};

static LLDefaultChildRegistry::Register<LLToolTip> r("tool_tip");

const S32 TOOLTIP_PADDING = 4;

LLToolTip::LLToolTip(const LLToolTip::Params& p)
:	LLPanel(p),
	mVisibleTime(p.visible_time),
	mHasClickCallback(p.click_callback.isProvided())
{
	LLTextBox::Params params;
	params.text = "tip_text";
	params.name = params.text;
	// bake textbox padding into initial rect
	params.rect = LLRect (TOOLTIP_PADDING, TOOLTIP_PADDING + 1, TOOLTIP_PADDING + 1, TOOLTIP_PADDING);
	params.follows.flags = FOLLOWS_ALL;
	params.h_pad = 4;
	params.v_pad = 2;
	params.mouse_opaque = false;
	params.text_color = LLUIColorTable::instance().getColor( "ToolTipTextColor" );
	params.bg_visible = false;
	params.font.style = "NORMAL";
	//params.border_drop_shadow_visible = true;
	mTextBox = LLUICtrlFactory::create<LLTextBox> (params);
	addChild(mTextBox);

	if (p.image.isProvided())
	{
		LLIconCtrl::Params icon_params;
		icon_params.name = "tooltip_icon";
		LLRect icon_rect;
		const S32 TOOLTIP_ICON_SIZE = 18;
		icon_rect.setOriginAndSize(TOOLTIP_PADDING, TOOLTIP_PADDING, TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE);
		icon_params.rect = icon_rect;
		icon_params.follows.flags = FOLLOWS_LEFT | FOLLOWS_BOTTOM;
		icon_params.image = p.image;
		icon_params.mouse_opaque = false;
		addChild(LLUICtrlFactory::create<LLIconCtrl>(icon_params));

		// move text over to fit image in
		mTextBox->translate(TOOLTIP_ICON_SIZE,0);
	}

	if (p.click_callback.isProvided())
	{
		setMouseUpCallback(boost::bind(p.click_callback()));
	}
}

void LLToolTip::setValue(const LLSD& value)
{
	mTextBox->setWrappedText(value.asString());
	mTextBox->reshapeToFitText();

	// reshape tooltip panel to fit text box
	LLRect tooltip_rect = calcBoundingRect();
	tooltip_rect.mTop += TOOLTIP_PADDING;
	tooltip_rect.mRight += TOOLTIP_PADDING;
	tooltip_rect.mBottom = 0;
	tooltip_rect.mLeft = 0;

	setRect(tooltip_rect);
}

void LLToolTip::setVisible(BOOL visible)
{
	// fade out tooltip over time
	if (!visible)
	{
		// don't actually change mVisible state, start fade out transition instead
		if (!mFadeTimer.getStarted())
		{
			mFadeTimer.start();
		}
	}
	else
	{
		mFadeTimer.stop();
		LLPanel::setVisible(TRUE);
	}
}

BOOL LLToolTip::handleHover(S32 x, S32 y, MASK mask)
{
	LLPanel::handleHover(x, y, mask);
	if (mHasClickCallback)
	{
		getWindow()->setCursor(UI_CURSOR_HAND);
	}
	return TRUE;
}

void LLToolTip::draw()
{
	F32 alpha = 1.f;

	if (LLUI::getMouseIdleTime() > mVisibleTime)
	{
		LLToolTipMgr::instance().hideToolTips();
	}

	if (mFadeTimer.getStarted())
	{
		F32 tool_tip_fade_time = LLUI::sSettingGroups["config"]->getF32("ToolTipFadeTime");
		alpha = clamp_rescale(mFadeTimer.getElapsedTimeF32(), 0.f, tool_tip_fade_time, 1.f, 0.f);
		if (alpha == 0.f)
		{
			// finished fading out, so hide ourselves
			mFadeTimer.stop();
			LLPanel::setVisible(false);
		}
	}

	// draw tooltip contents with appropriate alpha
	{
		LLViewDrawContext context(alpha);
		LLPanel::draw();
	}
}



//
// LLToolTipMgr
//
LLToolTipParams::LLToolTipParams()
:	pos("pos"),
	message("message"),
	delay_time("delay_time", LLUI::sSettingGroups["config"]->getF32( "ToolTipDelay" )),
	visible_time("visible_time", LLUI::sSettingGroups["config"]->getF32( "ToolTipVisibleTime" )),
	sticky_rect("sticky_rect"),
	width("width", 200),
	image("image")
{}

LLToolTipMgr::LLToolTipMgr()
:	mToolTip(NULL)
{
}

LLToolTip* LLToolTipMgr::createToolTip(const LLToolTipParams& params)
{
	S32 mouse_x;
	S32 mouse_y;
	LLUI::getMousePositionLocal(gToolTipView->getParent(), &mouse_x, &mouse_y);


	LLToolTip::Params tooltip_params;
	tooltip_params.name = "tooltip";
	tooltip_params.mouse_opaque = true;
	tooltip_params.rect = LLRect (0, 1, 1, 0);
	tooltip_params.bg_opaque_color = LLUIColorTable::instance().getColor( "ToolTipBgColor" );
	tooltip_params.background_visible = true;
	tooltip_params.visible_time = params.visible_time;
	if (params.image.isProvided())
	{
		tooltip_params.image = params.image;
	}
	if (params.click_callback.isProvided())
	{
		tooltip_params.click_callback = params.click_callback;
	}

	LLToolTip* tooltip = LLUICtrlFactory::create<LLToolTip> (tooltip_params);

	// make tooltip fixed width and tall enough to fit text
	tooltip->reshape(params.width, 2000);
	tooltip->setValue(params.message());
	gToolTipView->addChild(tooltip);

	if (params.pos.isProvided())
	{
		// try to spawn at requested position
		LLUI::positionViewNearMouse(tooltip, params.pos.x, params.pos.y);
	}
	else
	{
		// just spawn at mouse location
		LLUI::positionViewNearMouse(tooltip);
	}

	//...update "sticky" rect and tooltip position
	if (params.sticky_rect.isProvided())
	{
		mToolTipStickyRect = params.sticky_rect;
	}
	else
	{
		// otherwise just use one pixel rect around mouse cursor
		mToolTipStickyRect.setOriginAndSize(mouse_x, mouse_y, 1, 1);
	}
	
	if (params.click_callback.isProvided())
	{
		// keep tooltip up when we mouse over it
		mToolTipStickyRect.unionWith(tooltip->getRect());
	}

	return tooltip;
}


void LLToolTipMgr::show(const std::string& msg)
{
	show(LLToolTipParams().message(msg));
}

void LLToolTipMgr::show(const LLToolTipParams& params)
{
	if (!params.validateBlock()) 
	{
		llwarns << "Could not display tooltip!" << llendl;
		return;
	}
	
	bool tooltip_shown =	mToolTip 
							&& mToolTip->getVisible() 
							&& !mToolTip->isFading();

	// if tooltip contents change, hide existing tooltip
	if (tooltip_shown && mLastToolTipMessage != params.message())
	{
		hideToolTips();
	}

	if (!mToolTipsBlocked									// we haven't hit a key, moved the mouse, etc.
		&& LLUI::getMouseIdleTime() > params.delay_time		// the mouse has been still long enough
		&& !tooltip_shown)									// tooltip not visible
	{
		// create new tooltip at mouse cursor position
		delete mToolTip;
		mToolTip = createToolTip(params);

		// remember this tooltip so we know when it changes
		mLastToolTipMessage = params.message();
	}
}

// allow new tooltips to be created, e.g. after mouse has moved
void LLToolTipMgr::enableToolTips()
{
	mToolTipsBlocked = false;
}

void LLToolTipMgr::hideToolTips() 
{ 
	mToolTipsBlocked = true; 
	if (mToolTip)
	{
		mToolTip->setVisible(FALSE);
	}
}

bool LLToolTipMgr::toolTipVisible()
{
	return mToolTip ? mToolTip->getVisible() : false;
}

LLRect LLToolTipMgr::getToolTipRect()
{
	if (mToolTip && mToolTip->getVisible())
	{
		return mToolTip->getRect();
	}
	return LLRect();
}


LLRect LLToolTipMgr::getStickyRect() 
{ 
	if (!mToolTip) return LLRect();

	return mToolTip->isInVisibleChain() ? mToolTipStickyRect : LLRect(); 
}

// EOF
