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

#include "llview.h"

class LLPanel;

class LLLayoutStack : public LLView, public LLInstanceTracker<LLLayoutStack>
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string>	orientation;
		Optional<S32>			border_size;
		Optional<bool>			animate,
								clip;

		Params();
	};

	typedef enum e_layout_orientation
	{
		HORIZONTAL,
		VERTICAL
	} ELayoutOrientation;

	virtual ~LLLayoutStack();

	/*virtual*/ void draw();
	/*virtual*/ void removeChild(LLView*);
	/*virtual*/ BOOL postBuild();

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	S32 getMinWidth() const { return mMinWidth; }
	S32 getMinHeight() const { return mMinHeight; }
	
	typedef enum e_animate
	{
		NO_ANIMATE,
		ANIMATE
	} EAnimate;

	void addPanel(LLPanel* panel, S32 min_width, S32 min_height, S32 max_width, S32 max_height, BOOL auto_resize, BOOL user_resize, EAnimate animate = NO_ANIMATE, S32 index = S32_MAX);
	void removePanel(LLPanel* panel);
	void collapsePanel(LLPanel* panel, BOOL collapsed = TRUE);
	S32 getNumPanels() { return mPanels.size(); }

	void updatePanelAutoResize(const std::string& panel_name, BOOL auto_resize);
	void setPanelUserResize(const std::string& panel_name, BOOL user_resize);
	
	/**
	 * Gets minimal width and/or height of the specified by name panel.
	 *
	 * If it is necessary to get only the one dimension pass NULL for another one.
	 * @returns true if specified by panel_name internal panel exists, false otherwise.
	 */
	bool getPanelMinSize(const std::string& panel_name, S32* min_widthp, S32* min_heightp);

	/**
	 * Gets maximal width and/or height of the specified by name panel.
	 *
	 * If it is necessary to get only the one dimension pass NULL for another one.
	 * @returns true if specified by panel_name internal panel exists, false otherwise.
	 */
	bool getPanelMaxSize(const std::string& panel_name, S32* max_width, S32* max_height);
	
	void updateLayout(BOOL force_resize = FALSE);
	
	S32 getPanelSpacing() const { return mPanelSpacing; }
	BOOL getAnimate () const { return mAnimate; }
	void setAnimate (BOOL animate) { mAnimate = animate; }
	
	static void updateClass();

protected:
	LLLayoutStack(const Params&);
	friend class LLUICtrlFactory;

private:
	struct LayoutPanel;

	void calcMinExtents();
	S32 getDefaultHeight(S32 cur_height);
	S32 getDefaultWidth(S32 cur_width);

	const ELayoutOrientation mOrientation;

	typedef std::vector<LayoutPanel*> e_panel_list_t;
	e_panel_list_t mPanels;

	LayoutPanel* findEmbeddedPanel(LLPanel* panelp) const;
	LayoutPanel* findEmbeddedPanelByName(const std::string& name) const;

	S32 mMinWidth;  // calculated by calcMinExtents
	S32 mMinHeight;  // calculated by calcMinExtents
	S32 mPanelSpacing;

	// true if we already applied animation this frame
	bool mAnimatedThisFrame;
	bool mAnimate;
	bool mClip;
}; // end class LLLayoutStack

#endif
