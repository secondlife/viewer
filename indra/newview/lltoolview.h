/** 
 * @file lltoolview.h
 * @brief UI container for tools.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLTOOLVIEW_H
#define LL_LLTOOLVIEW_H

// requires stdtypes.h
#include "llpanel.h"

// forward declares
class LLTool;
class LLButton;

class LLToolView;


// Utility class, container for the package of information we need about
// each tool.  The package can either point directly to a tool, or indirectly
// to another view of tools.
class LLToolContainer
{
public:
	LLToolContainer(LLToolView* parent);
	~LLToolContainer();

public:
	LLToolView*		mParent;		// toolview that owns this container
	LLButton*		mButton;
	LLPanel*		mPanel;
	LLTool*			mTool;			// if not NULL, this is a tool ref
};


// A view containing automatically arranged button icons representing
// tools.  The icons sit on top of panels containing options for each
// tool.
class LLToolView
:	public LLView
{
public:
	LLToolView(const std::string& name, const LLRect& rect);
	~LLToolView();

	virtual void	draw();			// handle juggling tool button highlights, panel visibility

	static void		onClickToolButton(void* container);

	void			addTool(const LLString& icon_off, const LLString& icon_on, LLPanel* panel, LLTool* tool, LLView* hoverView, const char* label);

	LLView*			getCurrentHoverView();

private:
	LLRect			getButtonRect(S32 button_index);	// return rect for button to add, zero-based index
	LLToolContainer	*findToolContainer(LLTool *tool);


private:
	typedef std::vector<LLToolContainer*> contain_list_t;
	contain_list_t 			mContainList;
	S32						mButtonCount;			// used to compute rectangles
};

#endif
