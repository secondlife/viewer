/**
 * @file lldynamictexture.cpp
 * @brief Implementation of LLViewerDynamicTexture class
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

#include "llviewerprecompiledheaders.h"

#include "lldynamictexture.h"

// Linden library includes
#include "llglheaders.h"
#include "llwindow.h"           // getPosition()

// Viewer includes
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llvertexbuffer.h"
#include "llviewerdisplay.h"
#include "llrender.h"
#include "pipeline.h"
#include "llglslshader.h"

// static
LLViewerDynamicTexture::instance_list_t LLViewerDynamicTexture::sInstances[ LLViewerDynamicTexture::ORDER_COUNT ];
S32 LLViewerDynamicTexture::sNumRenders = 0;

//-----------------------------------------------------------------------------
// LLViewerDynamicTexture()
//-----------------------------------------------------------------------------
LLViewerDynamicTexture::LLViewerDynamicTexture(S32 width, S32 height, S32 components, EOrder order, bool clamp) :
    LLViewerTexture(width, height, components, false),
    mClamp(clamp)
{
    llassert((1 <= components) && (components <= 4));

    generateGLTexture();

    llassert( 0 <= order && order < ORDER_COUNT );
    LLViewerDynamicTexture::sInstances[ order ].insert(this);
}

//-----------------------------------------------------------------------------
// LLViewerDynamicTexture()
//-----------------------------------------------------------------------------
LLViewerDynamicTexture::~LLViewerDynamicTexture()
{
    for( S32 order = 0; order < ORDER_COUNT; order++ )
    {
        LLViewerDynamicTexture::sInstances[order].erase(this);  // will fail in all but one case.
    }
}

//virtual
S8 LLViewerDynamicTexture::getType() const
{
    return LLViewerTexture::DYNAMIC_TEXTURE ;
}

//-----------------------------------------------------------------------------
// generateGLTexture()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::generateGLTexture()
{
    LLViewerTexture::generateGLTexture() ;
    generateGLTexture(-1, 0, 0, false);
}

void LLViewerDynamicTexture::generateGLTexture(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, bool swap_bytes)
{
    if (mComponents < 1 || mComponents > 4)
    {
        LL_ERRS() << "Bad number of components in dynamic texture: " << mComponents << LL_ENDL;
    }

    LLPointer<LLImageRaw> raw_image = new LLImageRaw(getFullWidth(), getFullHeight(), mComponents);
    if (internal_format >= 0)
    {
        setExplicitFormat(internal_format, primary_format, type_format, swap_bytes);
    }
    createGLTexture(0, raw_image, 0, true, LLGLTexture::DYNAMIC_TEX);
    setAddressMode((mClamp) ? LLTexUnit::TAM_CLAMP : LLTexUnit::TAM_WRAP);
    mGLTexturep->setGLTextureCreated(false);
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
bool LLViewerDynamicTexture::render()
{
    return false;
}

//-----------------------------------------------------------------------------
// preRender()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::preRender(bool clear_depth)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

     //use the bottom left corner
    mOrigin.set(0, 0);

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    // Set up camera
    LLViewerCamera* camera = LLViewerCamera::getInstance();
    mCamera.setOrigin(*camera);
    mCamera.setAxes(*camera);
    mCamera.setAspect(camera->getAspect());
    mCamera.setView(camera->getView());
    mCamera.setNear(camera->getNear());

    glViewport(mOrigin.mX, mOrigin.mY, getFullWidth(), getFullHeight());
    if (clear_depth)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

//-----------------------------------------------------------------------------
// postRender()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::postRender(bool success)
{
    {
        if (success)
        {
            if(mGLTexturep.isNull())
            {
                generateGLTexture() ;
            }
            else if(!mGLTexturep->getHasGLTexture())
            {
                generateGLTexture() ;
            }
            else if(mGLTexturep->getDiscardLevel() != 0)//do not know how it happens, but regenerate one if it does.
            {
                generateGLTexture() ;
            }

            success = mGLTexturep->setSubImageFromFrameBuffer(0, 0, mOrigin.mX, mOrigin.mY, getFullWidth(), getFullHeight());
        }
    }

    // restore viewport
    gViewerWindow->setup2DViewport();

    // restore camera
    LLViewerCamera* camera = LLViewerCamera::getInstance();
    camera->setOrigin(mCamera);
    camera->setAxes(mCamera);
    camera->setAspect(mCamera.getAspect());
    camera->setViewNoBroadcast(mCamera.getView());
    camera->setNear(mCamera.getNear());
}

//-----------------------------------------------------------------------------
// static
// updateDynamicTextures()
// Calls update on each dynamic texture.  Calls each group in order: "first," then "middle," then "last."
//-----------------------------------------------------------------------------
bool LLViewerDynamicTexture::updateAllInstances()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

    sNumRenders = 0;
    if (gGLManager.mIsDisabled)
    {
        return true;
    }

    LLRenderTarget& preview_target = gPipeline.mAuxillaryRT.deferredScreen;
    LLRenderTarget& bake_target = gPipeline.mBakeMap;
    if (!preview_target.isComplete() || !bake_target.isComplete())
    {
        llassert(false);
        return false;
    }
    llassert(preview_target.getWidth() >= LLPipeline::MAX_PREVIEW_WIDTH);
    llassert(preview_target.getHeight() >= LLPipeline::MAX_PREVIEW_WIDTH);
    llassert(bake_target.getWidth() >= (U32) LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH);
    llassert(bake_target.getHeight() >= (U32) LLAvatarAppearanceDefines::SCRATCH_TEX_HEIGHT);

    preview_target.bindTarget();
    preview_target.clear();

    LLGLSLShader::unbind();
    LLVertexBuffer::unbind();

    bool result = false;
    bool ret = false ;
    auto update_func = [&](LLViewerDynamicTexture* dynamicTexture, LLRenderTarget& renderTarget, S32 width, S32 height)
        {
            if (dynamicTexture->needsRender())
            {
                llassert(dynamicTexture->getFullWidth() <= width);
                llassert(dynamicTexture->getFullHeight() <= height);

                glClear(GL_DEPTH_BUFFER_BIT);

                gGL.color4f(1.f, 1.f, 1.f, 1.f);
                dynamicTexture->setBoundTarget(&renderTarget);
                dynamicTexture->preRender();    // Must be called outside of startRender()
                result = false;
                if (dynamicTexture->render())
                {
                    ret = true;
                    result = true;
                    sNumRenders++;
                }
                gGL.flush();
                LLVertexBuffer::unbind();
                dynamicTexture->setBoundTarget(nullptr);
                dynamicTexture->postRender(result);
            }
        };

    // ORDER_FIRST is unused, ORDER_MIDDLE is various ui preview
    for(S32 order = 0; order < ORDER_LAST; ++order)
    {
        for (LLViewerDynamicTexture* dynamicTexture : LLViewerDynamicTexture::sInstances[order])
        {
            update_func(dynamicTexture, preview_target, LLPipeline::MAX_PREVIEW_WIDTH, LLPipeline::MAX_PREVIEW_WIDTH);
        }
    }
    preview_target.flush();

    // ORDER_LAST is baked skin preview, ORDER_RESET resets appearance parameters and does not render.
    bake_target.bindTarget();
    bake_target.clear();

    result = false;
    ret = false;
    for (S32 order = ORDER_LAST; order < ORDER_COUNT; ++order)
    {
        for (LLViewerDynamicTexture* dynamicTexture : LLViewerDynamicTexture::sInstances[order])
        {
            update_func(dynamicTexture, bake_target, LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH, LLAvatarAppearanceDefines::SCRATCH_TEX_HEIGHT);
        }
    }
    bake_target.flush();

    gGL.flush();

    return ret;
}

//-----------------------------------------------------------------------------
// static
// destroyGL()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::destroyGL()
{
    for( S32 order = 0; order < ORDER_COUNT; order++ )
    {
        for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
             iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
        {
            LLViewerDynamicTexture *dynamicTexture = *iter;
            dynamicTexture->destroyGLTexture() ;
        }
    }
}

//-----------------------------------------------------------------------------
// static
// restoreGL()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::restoreGL()
{
    if (gGLManager.mIsDisabled)
    {
        return ;
    }

    for( S32 order = 0; order < ORDER_COUNT; order++ )
    {
        for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
             iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
        {
            LLViewerDynamicTexture *dynamicTexture = *iter;
            dynamicTexture->restoreGLTexture() ;
        }
    }
}
