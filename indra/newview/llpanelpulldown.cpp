/**
* @file llpanelpulldown.cpp
* @brief A panel that serves as a basis for multiple toolbar pulldown panels
*
* $LicenseInfo:firstyear=2020&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2020, Linden Research, Inc.
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

#include "llpanelpulldown.h"

const F32 AUTO_CLOSE_FADE_TIME_START_SEC = 2.0f;
const F32 AUTO_CLOSE_FADE_TIME_END_SEC = 3.0f;

///----------------------------------------------------------------------------
/// Class LLPanelPresetsCameraPulldown
///----------------------------------------------------------------------------

// Default constructor
LLPanelPulldown::LLPanelPulldown()
{
    mHoverTimer.stop();
}

/*virtual*/
void LLPanelPulldown::onMouseEnter(S32 x, S32 y, MASK mask)
{
    mHoverTimer.stop();
    LLPanel::onMouseEnter(x, y, mask);
}

/*virtual*/
void LLPanelPulldown::onTopLost()
{
    setVisible(FALSE);
}

/*virtual*/
BOOL LLPanelPulldown::handleMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleMouseDown(x, y, mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPulldown::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleRightMouseDown(x, y, mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPulldown::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    LLPanel::handleDoubleClick(x, y, mask);
    return TRUE;
}

BOOL LLPanelPulldown::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    LLPanel::handleScrollWheel(x, y, clicks);
    return TRUE; //If we got here, then we are in Pulldown's rect, consume the event.
}

/*virtual*/
void LLPanelPulldown::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mHoverTimer.start();
    LLPanel::onMouseLeave(x, y, mask);
}

/*virtual*/
void LLPanelPulldown::onVisibilityChange(BOOL new_visibility)
{
    if (new_visibility)
    {
        mHoverTimer.start(); // timer will be stopped when mouse hovers over panel
    }
    else
    {
        mHoverTimer.stop();
    }
}

//virtual
void LLPanelPulldown::draw()
{
    F32 alpha = mHoverTimer.getStarted()
        ? clamp_rescale(mHoverTimer.getElapsedTimeF32(), AUTO_CLOSE_FADE_TIME_START_SEC, AUTO_CLOSE_FADE_TIME_END_SEC, 1.f, 0.f)
        : 1.0f;
    LLViewDrawContext context(alpha);

    LLPanel::draw();

    if (alpha == 0.f)
    {
        setVisible(FALSE);
    }
}
