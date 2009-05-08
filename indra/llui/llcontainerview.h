/** 
 * @file llcontainerview.h
 * @brief Container for all statistics info.
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

#ifndef LL_LLCONTAINERVIEW_H
#define LL_LLCONTAINERVIEW_H

#include "stdtypes.h"
#include "lltextbox.h"
#include "llstatbar.h"

class LLScrollContainer;

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
			mouse_opaque(false);
		}
	};
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
