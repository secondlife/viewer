/** 
 * @file llinspect.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llinspect.h"

#include "lltooltip.h"
#include "llcontrol.h"  // LLCachedControl
#include "llui.h"       // LLUI::getInstance()->mSettingsGroups
#include "llviewermenu.h"

LLInspect::LLInspect(const LLSD& key)
:   LLFloater(key),
    mCloseTimer(),
    mOpenTimer()
{
}

LLInspect::~LLInspect()
{
}

// virtual
void LLInspect::draw()
{
    static LLCachedControl<F32> FADE_TIME(*LLUI::getInstance()->mSettingGroups["config"], "InspectorFadeTime", 1.f);
    static LLCachedControl<F32> STAY_TIME(*LLUI::getInstance()->mSettingGroups["config"], "InspectorShowTime", 1.f);
    if (mOpenTimer.getStarted())
    {
        LLFloater::draw();
        if (mOpenTimer.getElapsedTimeF32() > STAY_TIME)
        {
            mOpenTimer.stop();
            mCloseTimer.start();
        }

    }
    else if (mCloseTimer.getStarted())
    {
        F32 alpha = clamp_rescale(mCloseTimer.getElapsedTimeF32(), 0.f, FADE_TIME(), 1.f, 0.f);
        LLViewDrawContext context(alpha);
        LLFloater::draw();
        if (mCloseTimer.getElapsedTimeF32() > FADE_TIME)
        {
            closeFloater(false);
        }
    }
    else
    {
        LLFloater::draw();
    }
}

// virtual
void LLInspect::onOpen(const LLSD& data)
{
    LLFloater::onOpen(data);
    
    mCloseTimer.stop();
    mOpenTimer.start();
}

// virtual
void LLInspect::onFocusLost()
{
    LLFloater::onFocusLost();
    
    // Start closing when we lose focus
    mCloseTimer.start();
    mOpenTimer.stop();
}

// virtual
BOOL LLInspect::handleHover(S32 x, S32 y, MASK mask)
{
    mOpenTimer.pause();
    return LLView::handleHover(x, y, mask);
}

BOOL LLInspect::handleToolTip(S32 x, S32 y, MASK mask)
{
    BOOL handled = FALSE;


    //delegate handling of tooltip to the hovered child
    LLView* child_handler = childFromPoint(x,y);
    if (child_handler && !child_handler->getToolTip().empty())// show tooltip if a view has non-empty tooltip message
    {
        //build LLInspector params to get correct tooltip setting, etc. background image
        LLInspector::Params params;
        params.fillFrom(LLUICtrlFactory::instance().getDefaultParams<LLInspector>());
        params.message = child_handler->getToolTip();
        //set up delay if there is no visible tooltip at this moment
        params.delay_time =  LLToolTipMgr::instance().toolTipVisible() ? 0.f : LLUI::getInstance()->mSettingGroups["config"]->getF32( "ToolTipDelay" );
        LLToolTipMgr::instance().show(params);
        handled = TRUE;
    }
    return handled;
}
// virtual
void LLInspect::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mOpenTimer.unpause();
}

bool LLInspect::childHasVisiblePopupMenu()
{
    // Child text-box may spawn a pop-up menu, if mouse is over the menu, Inspector 
    // will hide(which is not expected).
    // This is an attempt to find out if child control has spawned a menu.

    LLView* child_menu = gMenuHolder->getVisibleMenu();
    if(child_menu)
    {
        LLRect floater_rc = calcScreenRect();
        LLRect menu_screen_rc = child_menu->calcScreenRect();
        S32 mx, my;
        LLUI::getInstance()->getMousePositionScreen(&mx, &my);

        // This works wrong if we spawn a menu near Inspector and menu overlaps Inspector.
        if(floater_rc.overlaps(menu_screen_rc) && menu_screen_rc.pointInRect(mx, my))
        {
            return true;
        }
    }
    return false;
}

void LLInspect::repositionInspector(const LLSD& data)
{
    // Position the inspector relative to the mouse cursor
    // Similar to how tooltips are positioned
    // See LLToolTipMgr::createToolTip
    if (data.has("pos"))
    {
        LLUI::getInstance()->positionViewNearMouse(this, data["pos"]["x"].asInteger(), data["pos"]["y"].asInteger());
    }
    else
    {
        LLUI::getInstance()->positionViewNearMouse(this);
    }
    applyRectControl();
}
