/** 
 * @file lllayoutstack.cpp
 * @brief LLLayout class - dynamic stacking of UI elements
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

// Opaque view with a background and a border.  Can contain LLUICtrls.

#include "linden_common.h"

#include "lllayoutstack.h"

#include "lllocalcliprect.h"
#include "llpanel.h"
#include "llcriticaldamp.h"
#include "lliconctrl.h"
#include "boost/foreach.hpp"

static const F32 MIN_FRACTIONAL_SIZE = 0.00001f;
static const F32 MAX_FRACTIONAL_SIZE = 1.f;

static LLDefaultChildRegistry::Register<LLLayoutStack> register_layout_stack("layout_stack");
static LLLayoutStack::LayoutStackRegistry::Register<LLLayoutPanel> register_layout_panel("layout_panel");

//
// LLLayoutPanel
//
LLLayoutPanel::Params::Params()	
:	expanded_min_dim("expanded_min_dim", 0),
	min_dim("min_dim", -1),
	user_resize("user_resize", false),
	auto_resize("auto_resize", true)
{
	addSynonym(min_dim, "min_width");
	addSynonym(min_dim, "min_height");
}

LLLayoutPanel::LLLayoutPanel(const Params& p)	
:	LLPanel(p),
	mExpandedMinDim(p.expanded_min_dim.isProvided() ? p.expanded_min_dim : p.min_dim),
 	mMinDim(p.min_dim), 
 	mAutoResize(p.auto_resize),
 	mUserResize(p.user_resize),
	mCollapsed(FALSE),
	mCollapseAmt(0.f),
	mVisibleAmt(1.f), // default to fully visible
	mResizeBar(NULL),
	mFractionalSize(0.f),
	mTargetDim(0),
	mIgnoreReshape(false),
	mOrientation(LLLayoutStack::HORIZONTAL)
{
	// panels initialized as hidden should not start out partially visible
	if (!getVisible())
	{
		mVisibleAmt = 0.f;
	}
}

void LLLayoutPanel::initFromParams(const Params& p)
{
	LLPanel::initFromParams(p);
	setFollowsNone();
}


LLLayoutPanel::~LLLayoutPanel()
{
	// probably not necessary, but...
	delete mResizeBar;
	mResizeBar = NULL;
}

F32 LLLayoutPanel::getAutoResizeFactor() const
{
	return mVisibleAmt * (1.f - mCollapseAmt);
}
 
F32 LLLayoutPanel::getVisibleAmount() const
{
	return mVisibleAmt;
}

S32 LLLayoutPanel::getLayoutDim() const
{
	return ll_round((F32)((mOrientation == LLLayoutStack::HORIZONTAL)
					? getRect().getWidth()
					: getRect().getHeight()));
}

S32 LLLayoutPanel::getTargetDim() const
{
	return mTargetDim;
}

void LLLayoutPanel::setTargetDim(S32 value)
{
	LLRect new_rect(getRect());
	if (mOrientation == LLLayoutStack::HORIZONTAL)
	{
		new_rect.mRight = new_rect.mLeft + value;
	}
	else
	{
		new_rect.mTop = new_rect.mBottom + value;
	}
	setShape(new_rect, true);
}

S32 LLLayoutPanel::getVisibleDim() const
{
	F32 min_dim = getRelevantMinDim();
	return ll_round(mVisibleAmt
					* (min_dim
						+ (((F32)mTargetDim - min_dim) * (1.f - mCollapseAmt))));
}
 
void LLLayoutPanel::setOrientation( LLView::EOrientation orientation )
{
	mOrientation = orientation;
	S32 layout_dim = ll_round((F32)((mOrientation == LLLayoutStack::HORIZONTAL)
		? getRect().getWidth()
		: getRect().getHeight()));

	if (mAutoResize == FALSE 
		&& mUserResize == TRUE 
		&& mMinDim == -1 )
	{
		setMinDim(layout_dim);
	}
	mTargetDim = llmax(layout_dim, getMinDim());
}
 
void LLLayoutPanel::setVisible( BOOL visible )
{
	if (visible != getVisible())
	{
		LLLayoutStack* stackp = dynamic_cast<LLLayoutStack*>(getParent());
		if (stackp)
		{
			stackp->mNeedsLayout = true;
		}
	}
	LLPanel::setVisible(visible);
}

void LLLayoutPanel::reshape( S32 width, S32 height, BOOL called_from_parent /*= TRUE*/ )
{
	if (width == getRect().getWidth() && height == getRect().getHeight()) return;

	if (!mIgnoreReshape && mAutoResize == false)
	{
		mTargetDim = (mOrientation == LLLayoutStack::HORIZONTAL) ? width : height;
		LLLayoutStack* stackp = dynamic_cast<LLLayoutStack*>(getParent());
		if (stackp)
		{
			stackp->mNeedsLayout = true;
		}
	}
	LLPanel::reshape(width, height, called_from_parent);
}

void LLLayoutPanel::handleReshape(const LLRect& new_rect, bool by_user)
{
	LLLayoutStack* stackp = dynamic_cast<LLLayoutStack*>(getParent());
	if (stackp)
	{
		if (by_user)
		{	// tell layout stack to account for new shape
			
			// make sure that panels have already been auto resized
			stackp->updateLayout();
			// now apply requested size to panel
			stackp->updatePanelRect(this, new_rect);
		}
		stackp->mNeedsLayout = true;
	}
	LLPanel::handleReshape(new_rect, by_user);
}

//
// LLLayoutStack
//

LLLayoutStack::Params::Params()
:	orientation("orientation"),
	animate("animate", true),
	clip("clip", true),
	open_time_constant("open_time_constant", 0.02f),
	close_time_constant("close_time_constant", 0.03f),
	resize_bar_overlap("resize_bar_overlap", 1),
	border_size("border_size", LLCachedControl<S32>(*LLUI::getInstance()->mSettingGroups["config"], "UIResizeBarHeight", 0)),
	show_drag_handle("show_drag_handle", false),
	drag_handle_first_indent("drag_handle_first_indent", 0),
	drag_handle_second_indent("drag_handle_second_indent", 0),
	drag_handle_thickness("drag_handle_thickness", 5),
	drag_handle_shift("drag_handle_shift", 2)
{
	addSynonym(border_size, "drag_handle_gap");
}

LLLayoutStack::LLLayoutStack(const LLLayoutStack::Params& p) 
:	LLView(p),
	mPanelSpacing(p.border_size),
	mOrientation(p.orientation),
	mAnimate(p.animate),
	mAnimatedThisFrame(false),
	mNeedsLayout(true),
	mClip(p.clip),
	mOpenTimeConstant(p.open_time_constant),
	mCloseTimeConstant(p.close_time_constant),
	mResizeBarOverlap(p.resize_bar_overlap),
	mShowDragHandle(p.show_drag_handle),
	mDragHandleFirstIndent(p.drag_handle_first_indent),
	mDragHandleSecondIndent(p.drag_handle_second_indent),
	mDragHandleThickness(p.drag_handle_thickness),
	mDragHandleShift(p.drag_handle_shift)
{
}

LLLayoutStack::~LLLayoutStack()
{
	e_panel_list_t panels = mPanels; // copy list of panel pointers
	mPanels.clear(); // clear so that removeChild() calls don't cause trouble
	std::for_each(panels.begin(), panels.end(), DeletePointer());
}

void LLLayoutStack::draw()
{
	updateLayout();

	// always clip to stack itself
	LLLocalClipRect clip(getLocalRect());
	for (LLLayoutPanel* panelp : mPanels)
	{
		if ((!panelp->getVisible() || panelp->mCollapsed)
			&& (panelp->mVisibleAmt < 0.001f || !mAnimate))
		{
			// essentially invisible
			continue;
		}
		// clip to layout rectangle, not bounding rectangle
		LLRect clip_rect = panelp->getRect();
		// scale clipping rectangle by visible amount
		if (mOrientation == HORIZONTAL)
		{
			clip_rect.mRight = clip_rect.mLeft + panelp->getVisibleDim();
		}
		else
		{
			clip_rect.mBottom = clip_rect.mTop - panelp->getVisibleDim();
		}

		{LLLocalClipRect clip(clip_rect, mClip);
			// only force drawing invisible children if visible amount is non-zero
			drawChild(panelp, 0, 0, !clip_rect.isEmpty());
		}
		if (panelp->getResizeBar()->getVisible())
		{
			drawChild(panelp->getResizeBar());
		}
	}
}

void LLLayoutStack::removeChild(LLView* view)
{
	LLLayoutPanel* embedded_panelp = findEmbeddedPanel(dynamic_cast<LLPanel*>(view));

	if (embedded_panelp)
	{
		mPanels.erase(std::find(mPanels.begin(), mPanels.end(), embedded_panelp));
		delete embedded_panelp;
		updateFractionalSizes();
		mNeedsLayout = true;
	}

	LLView::removeChild(view);
}

BOOL LLLayoutStack::postBuild()
{
	updateLayout();
	return TRUE;
}

bool LLLayoutStack::addChild(LLView* child, S32 tab_group)
{
	LLLayoutPanel* panelp = dynamic_cast<LLLayoutPanel*>(child);
	if (panelp)
	{
		panelp->setOrientation(mOrientation);
		mPanels.push_back(panelp);
		createResizeBar(panelp);
		mNeedsLayout = true;
	}
	BOOL result = LLView::addChild(child, tab_group);

	updateFractionalSizes();
	return result;
}

void LLLayoutStack::addPanel(LLLayoutPanel* panel, EAnimate animate)
{
	addChild(panel);

	// panel starts off invisible (collapsed)
	if (animate == ANIMATE)
	{
		panel->mVisibleAmt = 0.f;
		panel->setVisible(TRUE);
	}
}

void LLLayoutStack::collapsePanel(LLPanel* panel, BOOL collapsed)
{
	LLLayoutPanel* panel_container = findEmbeddedPanel(panel);
	if (!panel_container) return;

	panel_container->mCollapsed = collapsed;
	mNeedsLayout = true;
}

static LLTrace::BlockTimerStatHandle FTM_UPDATE_LAYOUT("Update LayoutStacks");

class LLImagePanel : public LLPanel
{
public:
	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<bool> horizontal;
		Params() : horizontal("horizontal", false) {}
	};
	LLImagePanel(const Params& p) : LLPanel(p), mHorizontal(p.horizontal) {}
	virtual ~LLImagePanel() {}

	void draw()
	{
		const LLRect& parent_rect = getParent()->getRect();
		const LLRect& rect = getRect();
		LLRect clip_rect( -rect.mLeft, parent_rect.getHeight() - rect.mBottom - 2
			, parent_rect.getWidth() - rect.mLeft - (mHorizontal ? 2 : 0), -rect.mBottom);
		LLLocalClipRect clip(clip_rect);
		LLPanel::draw();
	}

private:
	bool mHorizontal;
};

void LLLayoutStack::updateLayout()
{	
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_LAYOUT);

	if (!mNeedsLayout) return;

	bool continue_animating = animatePanels();
	F32 total_visible_fraction = 0.f;
	S32 space_to_distribute = (mOrientation == HORIZONTAL)
							? getRect().getWidth()
							: getRect().getHeight();

	// first, assign minimum dimensions
	LLLayoutPanel* panelp = NULL;
	BOOST_FOREACH(panelp, mPanels)
	{
		if (panelp->mAutoResize)
		{
			panelp->mTargetDim = panelp->getRelevantMinDim();
		}
		space_to_distribute -= panelp->getVisibleDim() + ll_round((F32)mPanelSpacing * panelp->getVisibleAmount());
		total_visible_fraction += panelp->mFractionalSize * panelp->getAutoResizeFactor();
	}

	llassert(total_visible_fraction < 1.05f);

	// don't need spacing after last panel
	space_to_distribute += panelp ? ll_round((F32)mPanelSpacing * panelp->getVisibleAmount()) : 0;

	S32 remaining_space = space_to_distribute;
	F32 fraction_distributed = 0.f;
	if (space_to_distribute > 0 && total_visible_fraction > 0.f)
	{	// give space proportionally to visible auto resize panels
		BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
		{
			if (panelp->mAutoResize)
			{
				F32 fraction_to_distribute = (panelp->mFractionalSize * panelp->getAutoResizeFactor()) / (total_visible_fraction);
				S32 delta = ll_round((F32)space_to_distribute * fraction_to_distribute);
				fraction_distributed += fraction_to_distribute;
				panelp->mTargetDim += delta;
				remaining_space -= delta;
			}
		}
	}

	// distribute any left over pixels to non-collapsed, visible panels
	BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		if (remaining_space == 0) break;

		if (panelp->mAutoResize 
			&& !panelp->mCollapsed 
			&& panelp->getVisible())
		{
			S32 space_for_panel = remaining_space > 0 ? 1 : -1;
			panelp->mTargetDim += space_for_panel;
			remaining_space -= space_for_panel;
		}
	}

	F32 cur_pos = (mOrientation == HORIZONTAL) ? 0.f : (F32)getRect().getHeight();

	BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		F32 panel_dim = llmax(panelp->getExpandedMinDim(), panelp->mTargetDim);

		LLRect panel_rect;
		if (mOrientation == HORIZONTAL)
		{
			panel_rect.setLeftTopAndSize(ll_round(cur_pos),
										getRect().getHeight(),
										ll_round(panel_dim),
										getRect().getHeight());
		}
		else
		{
			panel_rect.setLeftTopAndSize(0,
										ll_round(cur_pos),
										getRect().getWidth(),
										ll_round(panel_dim));
		}

		LLRect resize_bar_rect(panel_rect);
		F32 panel_spacing = (F32)mPanelSpacing * panelp->getVisibleAmount();
		F32 panel_visible_dim = panelp->getVisibleDim();
		S32 panel_spacing_round = (S32)(ll_round(panel_spacing));

		if (mOrientation == HORIZONTAL)
		{
			cur_pos += panel_visible_dim + panel_spacing;

			if (mShowDragHandle && panel_spacing_round > mDragHandleThickness)
			{
				resize_bar_rect.mLeft = panel_rect.mRight + mDragHandleShift;
				resize_bar_rect.mRight = resize_bar_rect.mLeft + mDragHandleThickness;
			}
			else
			{
				resize_bar_rect.mLeft = panel_rect.mRight - mResizeBarOverlap;
				resize_bar_rect.mRight = panel_rect.mRight + panel_spacing_round + mResizeBarOverlap;
			}

			if (mShowDragHandle)
			{
				resize_bar_rect.mBottom += mDragHandleSecondIndent;
				resize_bar_rect.mTop -= mDragHandleFirstIndent;
			}

		}
		else //VERTICAL
		{
			cur_pos -= panel_visible_dim + panel_spacing;

			if (mShowDragHandle && panel_spacing_round > mDragHandleThickness)
			{
				resize_bar_rect.mTop = panel_rect.mBottom - mDragHandleShift;
				resize_bar_rect.mBottom = resize_bar_rect.mTop - mDragHandleThickness;
			}
			else
			{
				resize_bar_rect.mTop = panel_rect.mBottom + mResizeBarOverlap;
				resize_bar_rect.mBottom = panel_rect.mBottom - panel_spacing_round - mResizeBarOverlap;
			}

			if (mShowDragHandle)
			{
				resize_bar_rect.mLeft += mDragHandleFirstIndent;
				resize_bar_rect.mRight -= mDragHandleSecondIndent;
			}
		}

		panelp->setIgnoreReshape(true);
		panelp->setShape(panel_rect);
		panelp->setIgnoreReshape(false);
		panelp->mResizeBar->setShape(resize_bar_rect);
	}

	updateResizeBarLimits();

	// clear animation flag at end, since panel resizes will set it
	// and leave it set if there is any animation in progress
	mNeedsLayout = continue_animating;
} // end LLLayoutStack::updateLayout

LLLayoutPanel* LLLayoutStack::findEmbeddedPanel(LLPanel* panelp) const
{
	if (!panelp) return NULL;

	BOOST_FOREACH(LLLayoutPanel* p, mPanels)
	{
		if (p == panelp)
		{
			return p;
		}
	}
	return NULL;
}

LLLayoutPanel* LLLayoutStack::findEmbeddedPanelByName(const std::string& name) const
{
	LLLayoutPanel* result = NULL;

	BOOST_FOREACH(LLLayoutPanel* p, mPanels)
	{
		if (p->getName() == name)
		{
			result = p;
			break;
		}
	}

	return result;
}

void LLLayoutStack::createResizeBar(LLLayoutPanel* panelp)
{
	BOOST_FOREACH(LLLayoutPanel* lp, mPanels)
	{
		if (lp->mResizeBar == NULL)
		{
			LLResizeBar::Params resize_params;
			resize_params.name("resize");
			resize_params.resizing_view(lp);
			resize_params.min_size(lp->getRelevantMinDim());
			resize_params.side((mOrientation == HORIZONTAL) ? LLResizeBar::RIGHT : LLResizeBar::BOTTOM);
			resize_params.snapping_enabled(false);
			LLResizeBar* resize_bar = LLUICtrlFactory::create<LLResizeBar>(resize_params);
			lp->mResizeBar = resize_bar;

			if (mShowDragHandle)
			{
				LLPanel::Params resize_bar_bg_panel_p;
				resize_bar_bg_panel_p.name = "resize_handle_bg_panel";
				resize_bar_bg_panel_p.rect = lp->mResizeBar->getLocalRect();
				resize_bar_bg_panel_p.follows.flags = FOLLOWS_ALL;
				resize_bar_bg_panel_p.tab_stop = false;
				resize_bar_bg_panel_p.background_visible = true;
				resize_bar_bg_panel_p.bg_alpha_color = LLUIColorTable::instance().getColor("ResizebarBody");
				resize_bar_bg_panel_p.has_border = true;
				resize_bar_bg_panel_p.border.border_thickness = 1;
				resize_bar_bg_panel_p.border.highlight_light_color = LLUIColorTable::instance().getColor("ResizebarBorderLight");
				resize_bar_bg_panel_p.border.shadow_dark_color = LLUIColorTable::instance().getColor("ResizebarBorderDark");

				LLPanel* resize_bar_bg_panel = LLUICtrlFactory::create<LLPanel>(resize_bar_bg_panel_p);

				LLIconCtrl::Params icon_p;
				icon_p.name = "resize_handle_image";
				icon_p.rect = lp->mResizeBar->getLocalRect();
				icon_p.follows.flags = FOLLOWS_ALL;
				icon_p.image = LLUI::getUIImage(mOrientation == HORIZONTAL ? "Vertical Drag Handle" : "Horizontal Drag Handle");
				resize_bar_bg_panel->addChild(LLUICtrlFactory::create<LLIconCtrl>(icon_p));

				lp->mResizeBar->addChild(resize_bar_bg_panel);
			}

			/*if (mShowDragHandle)
			{
				LLViewBorder::Params border_params;
				border_params.border_thickness = 1;
				border_params.highlight_light_color = LLUIColorTable::instance().getColor("ResizebarBorderLight");
				border_params.shadow_dark_color = LLUIColorTable::instance().getColor("ResizebarBorderDark");

				addBorder(border_params);
				setBorderVisible(TRUE);

				LLImagePanel::Params image_panel;
				mDragHandleImage = LLUI::getUIImage(LLResizeBar::RIGHT == mSide ? "Vertical Drag Handle" : "Horizontal Drag Handle");
				image_panel.bg_alpha_image = mDragHandleImage;
				image_panel.background_visible = true;
				image_panel.horizontal = (LLResizeBar::BOTTOM == mSide);
				mImagePanel = LLUICtrlFactory::create<LLImagePanel>(image_panel);
				setImagePanel(mImagePanel);
			}*/

			//if (mShowDragHandle)
			//{
			//	setBackgroundVisible(TRUE);
			//	setTransparentColor(LLUIColorTable::instance().getColor("ResizebarBody"));
			//}

			/*if (mShowDragHandle)
			{
			S32 image_width = mDragHandleImage->getTextureWidth();
			S32 image_height = mDragHandleImage->getTextureHeight();
			const LLRect& panel_rect = getRect();
			S32 image_left = (panel_rect.getWidth() - image_width) / 2 - 1;
			S32 image_bottom = (panel_rect.getHeight() - image_height) / 2;
			mImagePanel->setRect(LLRect(image_left, image_bottom + image_height, image_left + image_width, image_bottom));
			}*/
			LLView::addChild(resize_bar, 0);
		}
	}
	// bring all resize bars to the front so that they are clickable even over the panels
	// with a bit of overlap
	for (e_panel_list_t::iterator panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLResizeBar* resize_barp = (*panel_it)->mResizeBar;
		sendChildToFront(resize_barp);
	}
}

// update layout stack animations, etc. once per frame
// NOTE: we use this to size world view based on animating UI, *before* we draw the UI
// we might still need to call updateLayout during UI draw phase, in case UI elements
// are resizing themselves dynamically
//static 
void LLLayoutStack::updateClass()
{
	for (auto& layout : instance_snapshot())
	{
		layout.updateLayout();
		layout.mAnimatedThisFrame = false;
	}
}

void LLLayoutStack::updateFractionalSizes()
{
	F32 total_resizable_dim = 0.f;

	BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		if (panelp->mAutoResize)
		{
			total_resizable_dim += llmax(MIN_FRACTIONAL_SIZE, (F32)(panelp->getLayoutDim() - panelp->getRelevantMinDim()));
		}
	}

	BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		if (panelp->mAutoResize)
		{
			F32 panel_resizable_dim = llmax(MIN_FRACTIONAL_SIZE, (F32)(panelp->getLayoutDim() - panelp->getRelevantMinDim()));
			panelp->mFractionalSize = panel_resizable_dim > 0.f 
				? llclamp(panel_resizable_dim / total_resizable_dim, MIN_FRACTIONAL_SIZE, MAX_FRACTIONAL_SIZE)
				: MIN_FRACTIONAL_SIZE;
			llassert(!llisnan(panelp->mFractionalSize));
		}
	}

	normalizeFractionalSizes();
}


void LLLayoutStack::normalizeFractionalSizes()
{
	S32 num_auto_resize_panels = 0;
	F32 total_fractional_size = 0.f;
	
	BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		if (panelp->mAutoResize)
		{
			total_fractional_size += panelp->mFractionalSize;
			num_auto_resize_panels++;
		}
	}

	if (total_fractional_size == 0.f)
	{ // equal distribution
		BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
		{
			if (panelp->mAutoResize)
			{
				panelp->mFractionalSize = MAX_FRACTIONAL_SIZE / (F32)num_auto_resize_panels;
			}
		}
	}
	else
	{ // renormalize
		BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
		{
			if (panelp->mAutoResize)
			{
				panelp->mFractionalSize /= total_fractional_size;
			}
		}
	}
}

bool LLLayoutStack::animatePanels()
{
	bool continue_animating = false;
	
	//
	// animate visibility
	//
	BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		if (panelp->getVisible())
		{
			if (mAnimate && panelp->mVisibleAmt < 1.f)
			{
				if (!mAnimatedThisFrame)
				{
					panelp->mVisibleAmt = lerp(panelp->mVisibleAmt, 1.f, LLSmoothInterpolation::getInterpolant(mOpenTimeConstant));
					if (panelp->mVisibleAmt > 0.99f)
					{
						panelp->mVisibleAmt = 1.f;
					}
				}
				
				mAnimatedThisFrame = true;
				continue_animating = true;
			}
			else
			{
				if (panelp->mVisibleAmt != 1.f)
				{
					panelp->mVisibleAmt = 1.f;
					mAnimatedThisFrame = true;
				}
			}
		}
		else // not visible
		{
			if (mAnimate && panelp->mVisibleAmt > 0.f)
			{
				if (!mAnimatedThisFrame)
				{
					panelp->mVisibleAmt = lerp(panelp->mVisibleAmt, 0.f, LLSmoothInterpolation::getInterpolant(mCloseTimeConstant));
					if (panelp->mVisibleAmt < 0.001f)
					{
						panelp->mVisibleAmt = 0.f;
					}
				}

				continue_animating = true;
				mAnimatedThisFrame = true;
			}
			else
			{
				if (panelp->mVisibleAmt != 0.f)
				{
					panelp->mVisibleAmt = 0.f;
					mAnimatedThisFrame = true;
				}
			}
		}

		F32 collapse_state = panelp->mCollapsed ? 1.f : 0.f;
		if (panelp->mCollapseAmt != collapse_state)
		{
			if (mAnimate)
			{
				if (!mAnimatedThisFrame)
				{
					panelp->mCollapseAmt = lerp(panelp->mCollapseAmt, collapse_state, LLSmoothInterpolation::getInterpolant(mCloseTimeConstant));
				}
			
				if (llabs(panelp->mCollapseAmt - collapse_state) < 0.001f)
				{
					panelp->mCollapseAmt = collapse_state;
				}

				mAnimatedThisFrame = true;
				continue_animating = true;
			}
			else
			{
				panelp->mCollapseAmt = collapse_state;
				mAnimatedThisFrame = true;
			}
		}
	}

	if (mAnimatedThisFrame) mNeedsLayout = true;
	return continue_animating;
}

void LLLayoutStack::updatePanelRect( LLLayoutPanel* resized_panel, const LLRect& new_rect )
{
	S32 new_dim = (mOrientation == HORIZONTAL)
					? new_rect.getWidth()
					: new_rect.getHeight();
	S32 delta_panel_dim = new_dim - resized_panel->getVisibleDim();
	if (delta_panel_dim == 0) return;

	F32 total_visible_fraction = 0.f;
	F32 delta_auto_resize_headroom = 0.f;
	F32 old_auto_resize_headroom = 0.f;

	LLLayoutPanel* other_resize_panel = NULL;
	LLLayoutPanel* following_panel = NULL;

	BOOST_REVERSE_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		if (panelp->mAutoResize)
		{
			old_auto_resize_headroom += (F32)(panelp->mTargetDim - panelp->getRelevantMinDim());
			if (panelp->getVisible() && !panelp->mCollapsed)
			{
				total_visible_fraction += panelp->mFractionalSize;
			}
		}

		if (panelp == resized_panel)
		{
			other_resize_panel = following_panel;
		}

		if (panelp->getVisible() && !panelp->mCollapsed)
		{
			following_panel = panelp;
		}
	}

	if (resized_panel->mAutoResize)
	{
		if (!other_resize_panel || !other_resize_panel->mAutoResize)
		{
			delta_auto_resize_headroom += delta_panel_dim;	
		}
	}
	else 
	{
		if (!other_resize_panel || other_resize_panel->mAutoResize)
		{
			delta_auto_resize_headroom -= delta_panel_dim;
		}
	}

	F32 fraction_given_up = 0.f;
	F32 fraction_remaining = 1.f;
	F32 new_auto_resize_headroom = old_auto_resize_headroom + delta_auto_resize_headroom;

	enum
	{
		BEFORE_RESIZED_PANEL,
		RESIZED_PANEL,
		NEXT_PANEL,
		AFTER_RESIZED_PANEL
	} which_panel = BEFORE_RESIZED_PANEL;

	BOOST_FOREACH(LLLayoutPanel* panelp, mPanels)
	{
		if (!panelp->getVisible() || panelp->mCollapsed) 
		{
			if (panelp->mAutoResize) 
			{
				fraction_remaining -= panelp->mFractionalSize;
			}
			continue;
		}

		if (panelp == resized_panel)
		{
			which_panel = RESIZED_PANEL;
		}

		switch(which_panel)
		{
		case BEFORE_RESIZED_PANEL:
			if (panelp->mAutoResize)
			{	// freeze current size as fraction of overall auto_resize space
				F32 fractional_adjustment_factor = new_auto_resize_headroom == 0.f
													? 1.f
													: old_auto_resize_headroom / new_auto_resize_headroom;
				F32 new_fractional_size = llclamp(panelp->mFractionalSize * fractional_adjustment_factor,
													MIN_FRACTIONAL_SIZE,
													MAX_FRACTIONAL_SIZE);
				fraction_given_up -= new_fractional_size - panelp->mFractionalSize;
				fraction_remaining -= panelp->mFractionalSize;
				panelp->mFractionalSize = new_fractional_size;
				llassert(!llisnan(panelp->mFractionalSize));
			}
			else
			{
				// leave non auto-resize panels alone
			}
			break;
		case RESIZED_PANEL:
			if (panelp->mAutoResize)
			{	// freeze new size as fraction
				F32 new_fractional_size = (new_auto_resize_headroom == 0.f)
					? MAX_FRACTIONAL_SIZE
					: llclamp(total_visible_fraction * (F32)(new_dim - panelp->getRelevantMinDim()) / new_auto_resize_headroom, MIN_FRACTIONAL_SIZE, MAX_FRACTIONAL_SIZE);
				fraction_given_up -= new_fractional_size - panelp->mFractionalSize;
				fraction_remaining -= panelp->mFractionalSize;
				panelp->mFractionalSize = new_fractional_size;
				llassert(!llisnan(panelp->mFractionalSize));
			}
			else
			{	// freeze new size as original size
				panelp->mTargetDim = new_dim;
			}
			which_panel = NEXT_PANEL;
			break;
		case NEXT_PANEL:
			if (panelp->mAutoResize)
			{
				fraction_remaining -= panelp->mFractionalSize;
				if (resized_panel->mAutoResize)
				{
					panelp->mFractionalSize = llclamp(panelp->mFractionalSize + fraction_given_up, MIN_FRACTIONAL_SIZE, MAX_FRACTIONAL_SIZE);
					fraction_given_up = 0.f;
				}
				else
				{
					if (new_auto_resize_headroom < 1.f)
					{
						new_auto_resize_headroom = 1.f;
					}

					F32 new_fractional_size = llclamp(total_visible_fraction * (F32)(panelp->mTargetDim - panelp->getRelevantMinDim() + delta_auto_resize_headroom) 
														/ new_auto_resize_headroom,
													MIN_FRACTIONAL_SIZE,
													MAX_FRACTIONAL_SIZE);
					fraction_given_up -= new_fractional_size - panelp->mFractionalSize;
					panelp->mFractionalSize = new_fractional_size;
				}
			}
			else
			{
				panelp->mTargetDim -= delta_panel_dim;
			}
			which_panel = AFTER_RESIZED_PANEL;
			break;
		case AFTER_RESIZED_PANEL:
			if (panelp->mAutoResize && fraction_given_up != 0.f)
			{
				panelp->mFractionalSize = llclamp(panelp->mFractionalSize + (panelp->mFractionalSize / fraction_remaining) * fraction_given_up,
												MIN_FRACTIONAL_SIZE,
												MAX_FRACTIONAL_SIZE);
			}
		default:
			break;
		}
	}
	updateLayout();
	//normalizeFractionalSizes();
}

void LLLayoutStack::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	mNeedsLayout = true;
	LLView::reshape(width, height, called_from_parent);
}

void LLLayoutStack::updateResizeBarLimits()
{
	LLLayoutPanel* previous_visible_panelp = NULL;
	BOOST_REVERSE_FOREACH(LLLayoutPanel* visible_panelp, mPanels)
	{
		if (!visible_panelp->getVisible() || visible_panelp->mCollapsed)
		{
			visible_panelp->mResizeBar->setVisible(FALSE);
			continue;
		}

		// toggle resize bars based on panel visibility, resizability, etc
		if (previous_visible_panelp
			&& (visible_panelp->mUserResize || previous_visible_panelp->mUserResize)				// one of the pair is user resizable
			&& (visible_panelp->mAutoResize || visible_panelp->mUserResize)							// current panel is resizable
			&& (previous_visible_panelp->mAutoResize || previous_visible_panelp->mUserResize))		// previous panel is resizable
		{
			visible_panelp->mResizeBar->setVisible(TRUE);
			S32 previous_panel_headroom = previous_visible_panelp->getVisibleDim() - previous_visible_panelp->getRelevantMinDim();
			visible_panelp->mResizeBar->setResizeLimits(visible_panelp->getRelevantMinDim(), 
														visible_panelp->getVisibleDim() + previous_panel_headroom);
		}
		else
		{
			visible_panelp->mResizeBar->setVisible(FALSE);
		}

		previous_visible_panelp = visible_panelp;
	}
}

