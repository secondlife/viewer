/** 
 * @file lltoolobjpicker.h
 * @brief LLToolObjPicker class header file
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

#ifndef LL_TOOLOBJPICKER_H
#define LL_TOOLOBJPICKER_H

#include "lltool.h"
#include "v3math.h"
#include "lluuid.h"

class LLPickInfo;

class LLToolObjPicker : public LLTool, public LLSingleton<LLToolObjPicker>
{
    LLSINGLETON(LLToolObjPicker);
public:

    virtual BOOL        handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL        handleMouseUp(S32 x, S32 y, MASK mask);
    virtual BOOL        handleHover(S32 x, S32 y, MASK mask);

    virtual void        handleSelect();
    virtual void        handleDeselect();

    virtual void        onMouseCaptureLost();

    virtual void        setExitCallback(void (*callback)(void *), void *callback_data);

    LLUUID              getObjectID() const { return mHitObjectID; }

    static void         pickCallback(const LLPickInfo& pick_info);

protected:
    BOOL                mPicked;
    LLUUID              mHitObjectID;
    void                (*mExitCallback)(void *callback_data);
    void                *mExitCallbackData;
};


#endif  
