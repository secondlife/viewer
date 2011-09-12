/** 
 * @file llcontainerview.h
 * @brief Container for all statistics info.
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

#ifndef LL_LLCONTAINERVIEW_H
#define LL_LLCONTAINERVIEW_H

#include "stdtypes.h"
#include "lltextbox.h"
#include "llstatbar.h"
#include "llview.h"

class LLScrollContainer;

struct ContainerViewRegistry : public LLChildRegistry<ContainerViewRegistry>
{};

class LLContainerView : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string> label;
		Optional<bool> show_label;
		Optional<bool> display_children;
		Params()
			: label("label"),
			  show_label("show_label", FALSE),
			  display_children("display_children", TRUE)
		{
			changeDefault(mouse_opaque, false);
		}
	};

	// my valid children are stored in this registry
 	typedef ContainerViewRegistry child_registry_t;
	
protected:
	LLContainerView(const Params& p);
	friend class LLUICtrlFactory;
public:
	~LLContainerView();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ bool addChild(LLView* view, S32 tab_group = 0);
	
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);

	/*virtual*/ void draw();
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ LLRect getRequiredRect();	// Return the height of this object, given the set options.

	void setLabel(const std::string& label);
	void showLabel(BOOL show) { mShowLabel = show; }
	void setDisplayChildren(const BOOL displayChildren);
	BOOL getDisplayChildren() { return mDisplayChildren; }
	void setScrollContainer(LLScrollContainer* scroll) {mScrollContainer = scroll;}

 private:
	LLScrollContainer* mScrollContainer;
	void arrange(S32 width, S32 height, BOOL called_from_parent = TRUE);
	BOOL mShowLabel;

protected:
	BOOL mDisplayChildren;
	std::string mLabel;
public:
	BOOL mCollapsible;

};
#endif // LL_CONTAINERVIEW_
