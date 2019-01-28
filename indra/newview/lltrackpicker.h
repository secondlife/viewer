/** 
 * @file lltrackpicker.h
 * @author AndreyK Productengine
 * @brief LLTrackPicker class header file including related functions
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
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

#ifndef LL_TRACKPICKER_H
#define LL_TRACKPICKER_H

#include "llfloater.h"


//=========================================================================

class LLFloaterTrackPicker : public LLFloater
{
public:
    LLFloaterTrackPicker(LLView * owner, const LLSD &params = LLSD());
    virtual ~LLFloaterTrackPicker() override;

    virtual BOOL postBuild() override;
    virtual void onClose(bool app_quitting) override;
    void         showPicker(const LLSD &args);

    virtual void            draw() override;

    void         onButtonCancel();
    void         onButtonSelect();

private:
    void                    onFocusLost() override;

    F32              mContextConeOpacity;
    LLHandle<LLView> mOwnerHandle;
};

#endif  // LL_TRACKPICKER_H
