/** 
 * @file llinspect.h
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

#ifndef LLINSPECT_H
#define LLINSPECT_H

#include "llfloater.h"
#include "llframetimer.h"

/// Base class for all inspectors (super-tooltips showing a miniature
/// properties view).
class LLInspect : public LLFloater
{
public:
    LLInspect(const LLSD& key);
    virtual ~LLInspect();
    
    /// Inspectors have a custom fade-in/fade-out animation
    /*virtual*/ void draw();
    
    /*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);
    /*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
    
    /// Start open animation
    /*virtual*/ void onOpen(const LLSD& avatar_id);
    
    /// Inspectors close themselves when they lose focus
    /*virtual*/ void onFocusLost();

    void repositionInspector(const LLSD& data);
    
protected:

    virtual bool childHasVisiblePopupMenu();

    LLFrameTimer        mCloseTimer;
    LLFrameTimer        mOpenTimer;
};

#endif

