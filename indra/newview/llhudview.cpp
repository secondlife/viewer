/** 
 * @file llhudview.cpp
 * @brief 2D HUD overlay
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llhudview.h"

// library includes
#include "v4color.h"
#include "llcoord.h"

// viewer includes
#include "llagent.h"
#include "llcallingcard.h"
#include "llcolorscheme.h"
#include "llviewercontrol.h"
#include "llfloaterworldmap.h"
#include "llworldmapview.h"
#include "lltracker.h"
#include "llviewercamera.h"
#include "llui.h"

LLHUDView *gHUDView = NULL;

const S32 HUD_ARROW_SIZE = 32;

LLHUDView::LLHUDView(const std::string& name, const LLRect& rect)
:	LLView(name, rect, FALSE)
{ }

LLHUDView::~LLHUDView()
{ }

EWidgetType LLHUDView::getWidgetType() const
{
	return WIDGET_TYPE_HUD_VIEW;
}

LLString LLHUDView::getWidgetTag() const
{
	return LL_HUD_VIEW_TAG;
}

// virtual
void LLHUDView::draw()
{
	LLTracker::drawHUDArrow();
}


// public
const LLColor4& LLHUDView::colorFromType(S32 type)
{
	switch (type)
	{
	case 0:
		return LLColor4::green;
	default:
		return LLColor4::black;
	}
}


/*virtual*/
BOOL LLHUDView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (LLTracker::handleMouseDown(x, y))
	{
		return TRUE;
	}
	return LLView::handleMouseDown(x, y, mask);
}

