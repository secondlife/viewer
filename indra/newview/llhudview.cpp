/** 
 * @file llhudview.cpp
 * @brief 2D HUD overlay
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llhudview.h"

// library includes
#include "v4color.h"
#include "llcoord.h"

// viewer includes
#include "llcallingcard.h"
#include "llviewercontrol.h"
#include "llfloaterworldmap.h"
#include "llworldmapview.h"
#include "lltracker.h"
#include "llviewercamera.h"
#include "llui.h"
#include "lluictrlfactory.h"

LLHUDView *gHUDView = NULL;

const S32 HUD_ARROW_SIZE = 32;



LLHUDView::LLHUDView(const LLRect& r)
{
	buildFromFile( "panel_hud.xml");
	setShape(r, true);
}

LLHUDView::~LLHUDView()
{
}

// virtual
void LLHUDView::draw()
{
	LLTracker::drawHUDArrow();
	LLView::draw();
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
