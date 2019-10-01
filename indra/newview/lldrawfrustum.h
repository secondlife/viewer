/** 
* @file   lldrawfrustum.h
* @brief  Header file for lldrawfrustum
*
* $LicenseInfo:firstyear=2019&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2019, Linden Research, Inc.
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
#ifndef LL_LLDRAWFRUSTUM_H
#define LL_LLDRAWFRUSTUM_H

#include "llview.h" 

class LLDrawFrustum
{
public:
    LLDrawFrustum();
    LLDrawFrustum(LLView *origin);
protected:

    // Draw's a cone from origin to derived view or floater
    // @derived_local_rect derived flaoter's local rect
    // @droot_view usually it is a derived floater
    // @drag_handle floater's drag handle getDragHandle()
    void drawFrustum(const LLRect &derived_local_rect, const LLView *root_view, const LLView *drag_handle, bool has_focus);

    void setFrustumOrigin(LLView *origin);
private:

    LLHandle <LLView>   mFrustumOrigin;
    F32		            mContextConeOpacity;
    F32                 mContextConeInAlpha;
    F32                 mContextConeOutAlpha;
    F32                 mContextConeFadeTime;
};

#endif // LL_LLDRAWFRUSTUM_H

