/** 
 * @file lllayoutstack.h
 * @author Richard Nelson
 * @brief LLLayout class - dynamic stacking of UI elements
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Reshasearch, Inc.
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

#ifndef LL_LLLAYOUTSTACK_H
#define LL_LLLAYOUTSTACK_H

#include "llpanel.h"
#include "llresizebar.h"


class LLLayoutPanel;


class LLLayoutStack : public LLView, public LLInstanceTracker<LLLayoutStack>
{
public:

	struct LayoutStackRegistry : public LLChildRegistry<LayoutStackRegistry>
	{
		LLSINGLETON_EMPTY_CTOR(LayoutStackRegistry);
	};

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<EOrientation>	orientation;
		Optional<S32>			border_size;
		Optional<bool>			animate,
								clip;
		Optional<F32>			open_time_constant,
								close_time_constant;
		Optional<S32>			resize_bar_overlap;
		Optional<bool>			show_drag_handle;
		Optional<S32>			drag_handle_first_indent;
		Optional<S32>			drag_handle_second_indent;
		Optional<S32>			drag_handle_thickness;
		Optional<S32>			drag_handle_shift;

        Optional<LLUIColor>     drag_handle_color;

		Params();
	};

	typedef LayoutStackRegistry child_registry_t;

	virtual ~LLLayoutStack();

	/*virtual*/ void draw();
    /*virtual*/ void deleteAllChildren();
	/*virtual*/ void removeChild(LLView*);
	/*virtual*/ bool postBuild();
	/*virtual*/ bool addChild(LLView* child, S32 tab_group = 0);
	/*virtual*/ void reshape(S32 width, S32 height, bool called_from_parent = true);


	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	typedef enum e_animate
	{
		NO_ANIMATE,
		ANIMATE
	} EAnimate;

	void addPanel(LLLayoutPanel* panel, EAnimate animate = NO_ANIMATE);
	void collapsePanel(LLPanel* panel, bool collapsed = true);
	S32 getNumPanels() { return mPanels.size(); }

	void updateLayout();

	S32 getPanelSpacing() const { return mPanelSpacing; }
    void setPanelSpacing(S32 val);
	
	static void updateClass();

protected:
	LLLayoutStack(const Params&);
	friend class LLUICtrlFactory;
	friend class LLLayoutPanel;

private:
	void updateResizeBarLimits();
	bool animatePanels();
	void createResizeBar(LLLayoutPanel* panel);

	const EOrientation mOrientation;

	typedef std::vector<LLLayoutPanel*> e_panel_list_t;
	e_panel_list_t mPanels;

	LLLayoutPanel* findEmbeddedPanel(LLPanel* panelp) const;
	LLLayoutPanel* findEmbeddedPanelByName(const std::string& name) const;
	void updateFractionalSizes();
	void normalizeFractionalSizes();
	void updatePanelRect( LLLayoutPanel* param1, const LLRect& new_rect );

	S32 mPanelSpacing;

	// true if we already applied animation this frame
	bool mAnimatedThisFrame;
	bool mAnimate;
	bool mClip;
	F32  mOpenTimeConstant;
	F32  mCloseTimeConstant;
	bool mNeedsLayout;
	S32  mResizeBarOverlap;
	bool mShowDragHandle;
	S32  mDragHandleFirstIndent;
	S32  mDragHandleSecondIndent;
	S32  mDragHandleThickness;
	S32  mDragHandleShift;
    LLUIColor mDragHandleColor;
}; // end class LLLayoutStack


class LLLayoutPanel : public LLPanel
{
friend class LLLayoutStack;
friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<S32>			expanded_min_dim,
								min_dim;
		Optional<bool>			user_resize,
								auto_resize;

		Params();
	};

	~LLLayoutPanel();

	void initFromParams(const Params& p);

	void handleReshape(const LLRect& new_rect, bool by_user);

	void reshape(S32 width, S32 height, bool called_from_parent = true);
	

	void setVisible(bool visible);

	S32 getLayoutDim() const;
	S32 getTargetDim() const;
	void setTargetDim(S32 value);
	S32 getMinDim() const { return llmax(0, mMinDim); }
	void setMinDim(S32 value) { mMinDim = value; }

	S32 getExpandedMinDim() const { return mExpandedMinDim >= 0 ? mExpandedMinDim : getMinDim(); }
	void setExpandedMinDim(S32 value) { mExpandedMinDim = value; }
	
	S32 getRelevantMinDim() const
	{
		S32 min_dim = mMinDim;
		
		if (!mCollapsed)
		{
			min_dim = getExpandedMinDim();
		}
		
		return min_dim;
	}

	F32 getAutoResizeFactor() const;
	F32 getVisibleAmount() const;
	S32 getVisibleDim() const;
	LLResizeBar* getResizeBar() { return mResizeBar; }

	bool isCollapsed() const { return mCollapsed;}

	void setOrientation(LLView::EOrientation orientation);
	void storeOriginalDim();

	void setIgnoreReshape(bool ignore) { mIgnoreReshape = ignore; }

protected:
	LLLayoutPanel(const Params& p);
	
	const bool	mAutoResize;
	const bool	mUserResize;

	S32		mExpandedMinDim;
	S32		mMinDim;
	bool	mCollapsed;
	F32		mVisibleAmt;
	F32		mCollapseAmt;
	F32		mFractionalSize;
	S32		mTargetDim;
	bool	mIgnoreReshape;
	LLView::EOrientation mOrientation;
	class LLResizeBar* mResizeBar;
};


#endif
