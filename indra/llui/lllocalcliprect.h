/** 
* @file lllocalcliprect.h
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
#ifndef LLLOCALCLIPRECT_H
#define LLLOCALCLIPRECT_H

#include "llgl.h"
#include "llrect.h"     // can't forward declare, it's templated
#include <stack>

// Clip rendering to a specific rectangle using GL scissor
// Just create one of these on the stack:
// {
//     LLLocalClipRect(rect);
//     draw();
// }
class LLScreenClipRect
{
public:
    LLScreenClipRect(const LLRect& rect, BOOL enabled = TRUE);
    virtual ~LLScreenClipRect();

private:
    static void pushClipRect(const LLRect& rect);
    static void popClipRect();
    static void updateScissorRegion();

private:
    LLGLState       mScissorState;
    BOOL            mEnabled;

    static std::stack<LLRect> sClipRectStack;
};

class LLLocalClipRect : public LLScreenClipRect
{
public:
    LLLocalClipRect(const LLRect& rect, BOOL enabled = TRUE);
    ~LLLocalClipRect();
};

#endif
