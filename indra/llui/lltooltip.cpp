/** 
 * @file lltooltip.cpp
 * @brief LLToolTipMgr class implementation and related classes
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

#include "linden_common.h"

// self include
#include "lltooltip.h"

// Library includes
#include "lltextbox.h"
#include "lliconctrl.h"
#include "llbutton.h"
#include "llmenugl.h"       // hideMenus()
#include "llui.h"			// positionViewNearMouse()
#include "llwindow.h"
#include "lltrans.h"
//
// Constants
//

//
// Local globals
//

LLToolTipView *gToolTipView = NULL;

//
// Member functions
//

static LLDefaultChildRegistry::Register<LLToolTipView> register_tooltip_view("tooltip_view");

LLToolTipView::Params::Params()
{
	changeDefault(mouse_opaque, false);
}

LLToolTipView::LLToolTipView(const LLToolTipView::Params& p)
:	LLView(p)
{
}

void LLToolTipView::draw()
{
	LLToolTipMgr::instance().updateToolTipVisibility();

	// do the usual thing
	LLView::draw();
}

BOOL LLToolTipView::handleHover(S32 x, S32 y, MASK mask)
{
	static S32 last_x = x;
	static S32 last_y = y;

	LLToolTipMgr& tooltip_mgr = LLToolTipMgr::instance();

	if (x != last_x && y != last_y && !tooltip_mgr.getMouseNearRect().pointInRect(x, y))
	{
		// allow new tooltips because mouse moved outside of mouse near rect
		tooltip_mgr.unblockToolTips();
	}

	last_x = x;
	last_y = y;
	return LLView::handleHover(x, y, mask);
}

BOOL LLToolTipView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().blockToolTips();

	if (LLView::handleMouseDown(x, y, mask))
	{
		// If we are handling the mouse event menu holder 
		// won't get a chance to close menus so do this here 
		LLMenuGL::sMenuContainer->hideMenus();
		return TRUE;
	}

	return FALSE;
}

BOOL LLToolTipView::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().blockToolTips();
	return LLView::handleMiddleMouseDown(x, y, mask);
}

BOOL LLToolTipView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().blockToolTips();
	return LLView::handleRightMouseDown(x, y, mask);
}


BOOL LLToolTipView::handleScrollWheel( S32 x, S32 y, S32 clicks )
{
	LLToolTipMgr::instance().blockToolTips();
	return FALSE;
}

void LLToolTipView::drawStickyRect()
{
	gl_rect_2d(LLToolTipMgr::instance().getMouseNearRect(), LLColor4::white, false);
}

// defaults for floater param block pulled from widgets/floater.xml
static LLWidgetNameRegistry::StaticRegistrar sRegisterInspectorParams(&typeid(LLInspector::Params), "inspector");

//
// LLToolTip
//


static LLDefaultChildRegistry::Register<LLToolTip> register_tooltip("tool_tip");


LLToolTip::Params::Params()
:	max_width("max_width", 200),
	padding("padding", 4),
	wrap("wrap", true),
	pos("pos"),
	message("message"),
	delay_time("delay_time", LLUI::sSettingGroups["config"]->getF32( "ToolTipDelay" )),
	visible_time_over("visible_time_over", LLUI::sSettingGroups["config"]->getF32( "ToolTipVisibleTimeOver" )),
	visible_time_near("visible_time_near", LLUI::sSettingGroups["config"]->getF32( "ToolTipVisibleTimeNear" )),
	visible_time_far("visible_time_far", LLUI::sSettingGroups["config"]->getF32( "ToolTipVisibleTimeFar" )),
	sticky_rect("sticky_rect"),
	image("image"),
	text_color("text_color"),
	time_based_media("time_based_media", false),
	web_based_media("web_based_media", false),
	media_playing("media_playing", false)
{
	changeDefault(chrome, true);
}

LLToolTip::LLToolTip(const LLToolTip::Params& p)
:	LLPanel(p),
	mHasClickCallback(p.click_callback.isProvided()),
	mPadding(p.padding),
	mTextBox(NULL),
	mInfoButton(NULL),
	mPlayMediaButton(NULL),
	mHomePageButton(NULL)
{
	LLTextBox::Params params;
	params.name = params.initial_value().asString();
	// bake textbox padding into initial rect
	params.rect = LLRect (mPadding, mPadding + 1, mPadding + 1, mPadding);
	params.h_pad = 0;
	params.v_pad = 0;
	params.mouse_opaque = false;
	params.text_color = p.text_color;
	params.bg_visible = false;
	params.font = p.font;
	params.use_ellipses = true;
	params.wrap = p.wrap;
	params.font_valign = LLFontGL::VCENTER;
	params.parse_urls = false; // disallow hyperlinks in tooltips, as they want to spawn their own explanatory tooltips
	mTextBox = LLUICtrlFactory::create<LLTextBox> (params);
	addChild(mTextBox);
	
	S32 TOOLTIP_ICON_SIZE = 0;
	S32 TOOLTIP_PLAYBUTTON_SIZE = 0;
	if (p.image.isProvided())
	{
		LLButton::Params icon_params;
		icon_params.name = "tooltip_info";
		LLRect icon_rect;
		LLUIImage* imagep = p.image;
		TOOLTIP_ICON_SIZE = (imagep ? imagep->getWidth() : 16);
		icon_rect.setOriginAndSize(mPadding, mPadding, TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE);
		icon_params.rect = icon_rect;
		icon_params.image_unselected(imagep);
		icon_params.image_selected(imagep);

		icon_params.scale_image(true);
		icon_params.flash_color.control = "ButtonUnselectedFgColor";
		mInfoButton  = LLUICtrlFactory::create<LLButton>(icon_params);
		if (p.click_callback.isProvided())
		{
			mInfoButton->setCommitCallback(boost::bind(p.click_callback()));
		}
		addChild(mInfoButton);
		
		// move text over to fit image in
		mTextBox->translate(TOOLTIP_ICON_SIZE + mPadding, 0);
	}
	
	if (p.time_based_media)
	{
		LLButton::Params p_button;
		p_button.name(std::string("play_media"));
		p_button.label(""); // provide label but set to empty so name does not overwrite it -angela
		TOOLTIP_PLAYBUTTON_SIZE = 16;
		LLRect button_rect;
		button_rect.setOriginAndSize((mPadding +TOOLTIP_ICON_SIZE+ mPadding ), mPadding, TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE);
		p_button.rect = button_rect;
		p_button.image_selected.name("button_anim_pause.tga");
		p_button.image_unselected.name("button_anim_play.tga");
		p_button.scale_image(true);
		
		mPlayMediaButton = LLUICtrlFactory::create<LLButton>(p_button); 
		if(p.click_playmedia_callback.isProvided())
		{
			mPlayMediaButton->setCommitCallback(boost::bind(p.click_playmedia_callback()));
		}
		mPlayMediaButton->setToggleState(p.media_playing);
		addChild(mPlayMediaButton);
		
		// move text over to fit image in
		mTextBox->translate(TOOLTIP_PLAYBUTTON_SIZE + mPadding, 0);
	}
	
	if (p.web_based_media)
	{
		LLButton::Params p_w_button;
		p_w_button.name(std::string("home_page"));
		p_w_button.label(""); // provid label but set to empty so name does not overwrite it -angela
		TOOLTIP_PLAYBUTTON_SIZE = 16;
		LLRect button_rect;
		button_rect.setOriginAndSize((mPadding +TOOLTIP_ICON_SIZE+ mPadding ), mPadding, TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE);
		p_w_button.rect = button_rect;
		p_w_button.image_unselected.name("map_home.tga");
		p_w_button.scale_image(true);
		
		mHomePageButton = LLUICtrlFactory::create<LLButton>(p_w_button); 
		if(p.click_homepage_callback.isProvided())
		{
			mHomePageButton->setCommitCallback(boost::bind(p.click_homepage_callback()));
		}
		addChild(mHomePageButton);
		
		// move text over to fit image in
		mTextBox->translate(TOOLTIP_PLAYBUTTON_SIZE + mPadding, 0);
	}
	
	if (p.click_callback.isProvided())
	{
		setMouseUpCallback(boost::bind(p.click_callback()));
	}
}

void LLToolTip::initFromParams(const LLToolTip::Params& p)
{
	LLPanel::initFromParams(p);

	// do this *after* we've had our size set in LLPanel::initFromParams();
	const S32 REALLY_LARGE_HEIGHT = 10000;
	mTextBox->reshape(p.max_width, REALLY_LARGE_HEIGHT);

	if (p.styled_message.isProvided())
	{
		for (LLInitParam::ParamIterator<LLToolTip::StyledText>::const_iterator text_it = p.styled_message.begin();
			text_it != p.styled_message.end();
			++text_it)
		{
			mTextBox->appendText(text_it->text(), false, text_it->style);
		}
	}
	else
	{
		mTextBox->setText(p.message());
	}

	S32 text_width = llmin(p.max_width(), mTextBox->getTextPixelWidth());
	S32 text_height = mTextBox->getTextPixelHeight();
	mTextBox->reshape(text_width, text_height);
	if (mInfoButton)
	{
		LLRect text_rect = mTextBox->getRect();
		LLRect icon_rect = mInfoButton->getRect();
		mTextBox->translate(0, icon_rect.getCenterY() - text_rect.getCenterY());
	}

	// reshape tooltip panel to fit text box
	LLRect tooltip_rect = calcBoundingRect();
	tooltip_rect.mTop += mPadding;
	tooltip_rect.mRight += mPadding;
	tooltip_rect.mBottom = 0;
	tooltip_rect.mLeft = 0;

	mTextBox->reshape(mTextBox->getRect().getWidth(), llmax(mTextBox->getRect().getHeight(), tooltip_rect.getHeight() - 2 * mPadding));

	setShape(tooltip_rect);
}

void LLToolTip::setVisible(BOOL visible)
{
	// fade out tooltip over time
	if (visible)
	{
		mVisibleTimer.start();
		mFadeTimer.stop();
		LLPanel::setVisible(TRUE);
	}
	else
	{
		mVisibleTimer.stop();
		// don't actually change mVisible state, start fade out transition instead
		if (!mFadeTimer.getStarted())
		{
			mFadeTimer.start();
		}
	}
}

BOOL LLToolTip::handleHover(S32 x, S32 y, MASK mask)
{
	//mInfoButton->setFlashing(true);
	if(mInfoButton)
		mInfoButton->setHighlight(true);
	
	LLPanel::handleHover(x, y, mask);
	if (mHasClickCallback)
	{
		getWindow()->setCursor(UI_CURSOR_HAND);
	}
	return TRUE;
}

void LLToolTip::onMouseLeave(S32 x, S32 y, MASK mask)
{
	//mInfoButton->setFlashing(true);
	if(mInfoButton)
		mInfoButton->setHighlight(false);
	LLUICtrl::onMouseLeave(x, y, mask);
}

void LLToolTip::draw()
{
	F32 alpha = 1.f;

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

bool LLToolTip::isFading() 
{ 
	return mFadeTimer.getStarted(); 
}

F32 LLToolTip::getVisibleTime() 
{ 
	return mVisibleTimer.getStarted() ? mVisibleTimer.getElapsedTimeF32() : 0.f; 
}

bool LLToolTip::hasClickCallback() 
{
	return mHasClickCallback; 
}

void LLToolTip::getToolTipMessage(std::string & message)
{
	if (mTextBox)
	{
		message = mTextBox->getText();
	}
}



//
// LLToolTipMgr
//

LLToolTipMgr::LLToolTipMgr()
:       mToolTipsBlocked(false),
	mToolTip(NULL),
	mNeedsToolTip(false)
{}

void LLToolTipMgr::createToolTip(const LLToolTip::Params& params)
{
	// block all other tooltips until tooltips re-enabled (e.g. mouse moved)
	blockToolTips(); 

	delete mToolTip;

	LLToolTip::Params tooltip_params(params);
	// block mouse events if there is a click handler registered (specifically, hover)
	if (params.click_callback.isProvided() && !params.mouse_opaque.isProvided())
	{
		// set mouse_opaque to true if it wasn't already set to something else
		// this prevents mouse down from going "through" the tooltip and ultimately
		// causing the tooltip to disappear
		tooltip_params.mouse_opaque = true;
	}
	tooltip_params.rect = LLRect (0, 1, 1, 0);

	mToolTip = LLUICtrlFactory::create<LLToolTip> (tooltip_params);

	gToolTipView->addChild(mToolTip);

	if (params.pos.isProvided())
	{
		LLCoordGL pos = params.pos;
		// try to spawn at requested position
		LLUI::positionViewNearMouse(mToolTip, pos.mX, pos.mY);
	}
	else
	{
		// just spawn at mouse location
		LLUI::positionViewNearMouse(mToolTip);
	}

	//...update "sticky" rect and tooltip position
	if (params.sticky_rect.isProvided())
	{
		mMouseNearRect = params.sticky_rect;
	}
	else
	{
		S32 mouse_x;
		S32 mouse_y;
		LLUI::getMousePositionLocal(gToolTipView->getParent(), &mouse_x, &mouse_y);

		// allow mouse a little bit of slop before changing tooltips
		mMouseNearRect.setCenterAndSize(mouse_x, mouse_y, 3, 3);
	}

	// allow mouse to move all the way to the tooltip without changing tooltips
	// (tooltip can still time out)
	if (mToolTip->hasClickCallback())
	{
		// keep tooltip up when we mouse over it
		mMouseNearRect.unionWith(mToolTip->getRect());
	}
}


void LLToolTipMgr::show(const std::string& msg)
{
	show(LLToolTip::Params().message(msg));
}

void LLToolTipMgr::show(const LLToolTip::Params& params)
{
	// fill in default tooltip params from tool_tip.xml
	LLToolTip::Params params_with_defaults(params);
	params_with_defaults.fillFrom(LLUICtrlFactory::instance().getDefaultParams<LLToolTip>());
	if (!params_with_defaults.validateBlock()) 
	{
		llwarns << "Could not display tooltip!" << llendl;
		return;
	}
	
	S32 mouse_x;
	S32 mouse_y;
	LLUI::getMousePositionLocal(gToolTipView, &mouse_x, &mouse_y);

	// are we ready to show the tooltip?
	if (!mToolTipsBlocked									// we haven't hit a key, moved the mouse, etc.
		&& LLUI::getMouseIdleTime() > params_with_defaults.delay_time)	// the mouse has been still long enough
	{
		bool tooltip_changed = mLastToolTipParams.message() != params_with_defaults.message()
								|| mLastToolTipParams.pos() != params_with_defaults.pos()
								|| mLastToolTipParams.time_based_media() != params_with_defaults.time_based_media()
								|| mLastToolTipParams.web_based_media() != params_with_defaults.web_based_media();

		bool tooltip_shown = mToolTip 
							 && mToolTip->getVisible() 
							 && !mToolTip->isFading();

		mNeedsToolTip = tooltip_changed || !tooltip_shown;
		// store description of tooltip for later creation
		mNextToolTipParams = params_with_defaults;
	}
}

// allow new tooltips to be created, e.g. after mouse has moved
void LLToolTipMgr::unblockToolTips()
{
	mToolTipsBlocked = false;
}

// disallow new tooltips until unblockTooltips called
void LLToolTipMgr::blockToolTips()
{
	hideToolTips();
	mToolTipsBlocked = true;
}

void LLToolTipMgr::hideToolTips() 
{ 
	if (mToolTip)
	{
		mToolTip->setVisible(FALSE);
	}
}

bool LLToolTipMgr::toolTipVisible()
{
	return mToolTip ? mToolTip->isInVisibleChain() : false;
}

LLRect LLToolTipMgr::getToolTipRect()
{
	if (mToolTip && mToolTip->getVisible())
	{
		return mToolTip->getRect();
	}
	return LLRect();
}


LLRect LLToolTipMgr::getMouseNearRect() 
{ 
	return toolTipVisible() ? mMouseNearRect : LLRect(); 
}

// every frame, determine if current tooltip should be hidden
void LLToolTipMgr::updateToolTipVisibility()
{
	// create new tooltip if we have one ready to go
	if (mNeedsToolTip)
	{
		mNeedsToolTip = false;
		createToolTip(mNextToolTipParams);
		mLastToolTipParams = mNextToolTipParams;
		
		return;
	}

	// hide tooltips when mouse cursor is hidden
	if (LLUI::getWindow()->isCursorHidden())
	{
		blockToolTips();
		return;
	}

	// hide existing tooltips if they have timed out
	S32 mouse_x, mouse_y;
	LLUI::getMousePositionLocal(gToolTipView, &mouse_x, &mouse_y);

	F32 tooltip_timeout = 0.f;
	if (toolTipVisible())
	{
		// mouse far away from tooltip
		tooltip_timeout = mLastToolTipParams.visible_time_far;
		// mouse near rect will only include the tooltip if the 
		// tooltip is clickable
		if (mMouseNearRect.pointInRect(mouse_x, mouse_y))
		{
			// mouse "close" to tooltip
			tooltip_timeout = mLastToolTipParams.visible_time_near;

			// if tooltip is clickable (has large mMouseNearRect)
			// than having cursor over tooltip keeps it up indefinitely
			if (mToolTip->parentPointInView(mouse_x, mouse_y))
			{
				// mouse over tooltip itself, don't time out
				tooltip_timeout = mLastToolTipParams.visible_time_over;
			}
		}

		if (mToolTip->getVisibleTime() > tooltip_timeout)
		{
			hideToolTips();
			unblockToolTips();
		}
	}
}


// Return the current tooltip text
void LLToolTipMgr::getToolTipMessage(std::string & message)
{
	if (toolTipVisible())
	{
		mToolTip->getToolTipMessage(message);
	}
}


// EOF
