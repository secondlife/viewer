/** 
 * @file lltoolselect.h
 * @brief LLToolSelect class header file
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

#ifndef LL_TOOLSELECT_H
#define LL_TOOLSELECT_H

#include "lltool.h"
#include "v3math.h"
#include "lluuid.h"
#include "llviewerwindow.h" // for LLPickInfo

class LLObjectSelection;

class LLToolSelect : public LLTool
{
public:
    LLToolSelect( LLToolComposite* composite );

    virtual BOOL        handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL        handleMouseUp(S32 x, S32 y, MASK mask);

    virtual void        stopEditing();

    static LLSafeHandle<LLObjectSelection>  handleObjectSelection(const LLPickInfo& pick, BOOL ignore_group, BOOL temp_select, BOOL select_root = FALSE);

    virtual void        onMouseCaptureLost();
    virtual void        handleDeselect();

protected:
    BOOL                mIgnoreGroup;
    LLUUID              mSelectObjectID;
    LLPickInfo          mPick;
};


#endif  // LL_TOOLSELECTION_H
