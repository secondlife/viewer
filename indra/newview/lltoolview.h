/** 
 * @file lltoolview.h
 * @brief UI container for tools.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOLVIEW_H
#define LL_LLTOOLVIEW_H

// requires stdtypes.h
#include "linked_lists.h"
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
	LLLinkedList
		<LLToolContainer>	mContainList;
	S32						mButtonCount;			// used to compute rectangles
};

#endif
