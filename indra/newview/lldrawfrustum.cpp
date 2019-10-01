/** 
* @file lldrawfrustum.cpp
* @brief Implementation of lldrawfrustum
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

#include "llviewerprecompiledheaders.h"

#include "lldrawfrustum.h"

#include "llviewercontrol.h"

LLDrawFrustum::LLDrawFrustum()
    : mContextConeOpacity(0.f)
{
    mContextConeInAlpha = gSavedSettings.getF32("ContextConeInAlpha"); // 0.0f
    mContextConeOutAlpha = gSavedSettings.getF32("ContextConeOutAlpha"); // 1.f
    mContextConeFadeTime = gSavedSettings.getF32("ContextConeFadeTime"); // 0.08f
}

LLDrawFrustum::LLDrawFrustum(LLView *origin)
    : mContextConeOpacity(0.f)
{
    mContextConeInAlpha = gSavedSettings.getF32("ContextConeInAlpha");
    mContextConeOutAlpha = gSavedSettings.getF32("ContextConeOutAlpha");
    mContextConeFadeTime = gSavedSettings.getF32("ContextConeFadeTime");
    setFrustumOrigin(origin);
}

void LLDrawFrustum::setFrustumOrigin(LLView *origin)
{
    if (origin)
    {
        mFrustumOrigin = origin->getHandle();
    }
}

void LLDrawFrustum::drawFrustum(const LLRect &derived_local_rect, const LLView *root_view, const LLView *drag_handle, bool has_focus)
{
    if (mFrustumOrigin.get())
    {
        LLView * frustumOrigin = mFrustumOrigin.get();
        LLRect origin_rect;
        frustumOrigin->localRectToOtherView(frustumOrigin->getLocalRect(), &origin_rect, root_view);
        // draw context cone connecting derived floater (ex: color picker) with view (ex: color swatch) in parent floater
        if (has_focus && frustumOrigin->isInVisibleChain() && mContextConeOpacity > 0.001f)
        {
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            LLGLEnable(GL_CULL_FACE);
            gGL.begin(LLRender::QUADS);
            {
                gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
                gGL.vertex2i(origin_rect.mLeft, origin_rect.mTop);
                gGL.vertex2i(origin_rect.mRight, origin_rect.mTop);
                gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
                gGL.vertex2i(derived_local_rect.mRight, derived_local_rect.mTop);
                gGL.vertex2i(derived_local_rect.mLeft, derived_local_rect.mTop);

                gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
                gGL.vertex2i(derived_local_rect.mLeft, derived_local_rect.mTop);
                gGL.vertex2i(derived_local_rect.mLeft, derived_local_rect.mBottom);
                gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
                gGL.vertex2i(origin_rect.mLeft, origin_rect.mBottom);
                gGL.vertex2i(origin_rect.mLeft, origin_rect.mTop);

                gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
                gGL.vertex2i(derived_local_rect.mRight, derived_local_rect.mBottom);
                gGL.vertex2i(derived_local_rect.mRight, derived_local_rect.mTop);
                gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
                gGL.vertex2i(origin_rect.mRight, origin_rect.mTop);
                gGL.vertex2i(origin_rect.mRight, origin_rect.mBottom);

                gGL.color4f(0.f, 0.f, 0.f, mContextConeOutAlpha * mContextConeOpacity);
                gGL.vertex2i(derived_local_rect.mLeft, derived_local_rect.mBottom);
                gGL.vertex2i(derived_local_rect.mRight, derived_local_rect.mBottom);
                gGL.color4f(0.f, 0.f, 0.f, mContextConeInAlpha * mContextConeOpacity);
                gGL.vertex2i(origin_rect.mRight, origin_rect.mBottom);
                gGL.vertex2i(origin_rect.mLeft, origin_rect.mBottom);
            }
            gGL.end();
        }

        if (gFocusMgr.childHasMouseCapture(drag_handle))
        {
            mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLCriticalDamp::getInterpolant(mContextConeFadeTime));
        }
        else
        {
            mContextConeOpacity = lerp(mContextConeOpacity, 0.f, LLCriticalDamp::getInterpolant(mContextConeFadeTime));
        }
    }
}

