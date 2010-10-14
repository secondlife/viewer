/** 
 * @file lllayoutstack.h
 * @author Richard Nelson
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

#ifndef LL_LLLAYOUTSTACK_H
#define LL_LLLAYOUTSTACK_H

#include "llpanel.h"

class LLPanel;

class LLLayoutPanel;

class LLLayoutStack : public LLView, public LLInstanceTracker<LLLayoutStack>
{
public:
	struct LayoutStackRegistry : public LLChildRegistry<LayoutStackRegistry>
	{};

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<std::string>	orientation;
		Optional<S32>			border_size;
		Optional<bool>			animate,
								clip;

		Params();
	};

	typedef LayoutStackRegistry child_registry_t;

	typedef enum e_layout_orientation
	{
		HORIZONTAL,
		VERTICAL
	} ELayoutOrientation;

	virtual ~LLLayoutStack();

	/*virtual*/ void draw();
	/*virtual*/ void removeChild(LLView*);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ bool addChild(LLView* child, S32 tab_group = 0);

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	S32 getMinWidth() const { return mMinWidth; }
	S32 getMinHeight() const { return mMinHeight; }
	
	typedef enum e_animate
	{
		NO_ANIMATE,
		ANIMATE
	} EAnimate;

	void addPanel(LLLayoutPanel* panel, EAnimate animate = NO_ANIMATE);
	void removePanel(LLPanel* panel);
	void collapsePanel(LLPanel* panel, BOOL collapsed = TRUE);
	S32 getNumPanels() { return mPanels.size(); }
	/**
	 * Moves panel_to_move before target_panel inside layout stack (both panels should already be there).
	 * If move_to_front is true target_panel is ignored and panel_to_move is moved to the beginning of mPanels
	 */
	void movePanel(LLPanel* panel_to_move, LLPanel* target_panel, bool move_to_front = false);

	void updatePanelAutoResize(const std::string& panel_name, BOOL auto_resize);
	void setPanelUserResize(const std::string& panel_name, BOOL user_resize);
	
	/**
	 * Gets minimal dimension along layout_stack axis of the specified by name panel.
	 *
	 * @returns true if specified by panel_name internal panel exists, false otherwise.
	 */
	bool getPanelMinSize(const std::string& panel_name, S32* min_dimp);

	/**
	 * Gets maximal dimension along layout_stack axis of the specified by name panel.
	 *
	 * @returns true if specified by panel_name internal panel exists, false otherwise.
	 */
	bool getPanelMaxSize(const std::string& panel_name, S32* max_dim);
	
	void updateLayout(BOOL force_resize = FALSE);
	
	S32 getPanelSpacing() const { return mPanelSpacing; }
	BOOL getAnimate () const { return mAnimate; }
	void setAnimate (BOOL animate) { mAnimate = animate; }
	
	static void updateClass();

protected:
	LLLayoutStack(const Params&);
	friend class LLUICtrlFactory;

private:
	void createResizeBars();
	void calcMinExtents();
	S32 getDefaultHeight(S32 cur_height);
	S32 getDefaultWidth(S32 cur_width);

	const ELayoutOrientation mOrientation;

	typedef std::vector<LLLayoutPanel*> e_panel_list_t;
	e_panel_list_t mPanels;

	LLLayoutPanel* findEmbeddedPanel(LLPanel* panelp) const;
	LLLayoutPanel* findEmbeddedPanelByName(const std::string& name) const;

	S32 mMinWidth;  // calculated by calcMinExtents
	S32 mMinHeight;  // calculated by calcMinExtents
	S32 mPanelSpacing;

	// true if we already applied animation this frame
	bool mAnimatedThisFrame;
	bool mAnimate;
	bool mClip;
}; // end class LLLayoutStack

class LLLayoutPanel : public LLPanel
{
friend class LLLayoutStack;
friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<S32>			min_dim,
								max_dim;
		Optional<bool>			user_resize,
								auto_resize;

		Params()
		:	min_dim("min_dim", 0),
			max_dim("max_dim", 0),
			user_resize("user_resize", true),
			auto_resize("auto_resize", true)
		{
			addSynonym(min_dim, "min_width");
			addSynonym(min_dim, "min_height");
			addSynonym(max_dim, "max_width");
			addSynonym(max_dim, "max_height");
		}
	};

	~LLLayoutPanel();

	void initFromParams(const Params& p);
protected:
	LLLayoutPanel(const Params& p)	;

	
	F32 getCollapseFactor(LLLayoutStack::ELayoutOrientation orientation);

	S32 mMinDim;
	S32 mMaxDim;
	BOOL mAutoResize;
	BOOL mUserResize;
	BOOL mCollapsed;
	class LLResizeBar* mResizeBar;
	F32 mVisibleAmt;
	F32 mCollapseAmt;
};


#endif
