/**
 * @file llcubemap.cpp
 * @brief LLCubeMap class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "linden_common.h"

#include "llworkerthread.h"

#include "llcubemap.h"

#include "v4coloru.h"
#include "v3math.h"
#include "v3dmath.h"
#include "m3math.h"
#include "m4math.h"

#include "llrender.h"
#include "llglslshader.h"

#include "llglheaders.h"

namespace {
    const U16 RESOLUTION = 64;
}

bool LLCubeMap::sUseCubeMaps = true;

LLCubeMap::LLCubeMap(bool init_as_srgb)
    : mTextureStage(0),
      mMatrixStage(0),
      mIssRGB(init_as_srgb)
{
    mTargets[0] = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    mTargets[1] = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    mTargets[2] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    mTargets[3] = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    mTargets[4] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    mTargets[5] = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
}

LLCubeMap::~LLCubeMap()
{
}

void LLCubeMap::initGL()
{
    llassert(gGLManager.mInited);

    if (LLCubeMap::sUseCubeMaps)
    {
        // Not initialized, do stuff.
        if (mImages[0].isNull())
        {
            U32 texname = 0;

            LLImageGL::generateTextures(1, &texname);

            for (int i = 0; i < 6; i++)
            {
                mImages[i] = new LLImageGL(RESOLUTION, RESOLUTION, 4, false);
            #if USE_SRGB_DECODE
                if (mIssRGB) {
                    mImages[i]->setExplicitFormat(GL_SRGB8_ALPHA8, GL_RGBA);
                }
            #endif
                mImages[i]->setTarget(mTargets[i], LLTexUnit::TT_CUBE_MAP);
                mRawImages[i] = new LLImageRaw(RESOLUTION, RESOLUTION, 4);
                mImages[i]->createGLTexture(0, mRawImages[i], texname);

                gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, texname);
                mImages[i]->setAddressMode(LLTexUnit::TAM_CLAMP);
                stop_glerror();
            }
            gGL.getTexUnit(0)->disable();
        }
        disable();
    }
    else
    {
        LL_WARNS() << "Using cube map without extension!" << LL_ENDL;
    }
}

void LLCubeMap::initRawData(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
    bool flip_x[6] =    { false, true,  false, false, true,  false };
    bool flip_y[6] =    { true,  true,  true,  false, true,  true  };
    bool transpose[6] = { false, false, false, false, true,  true  };

    // Yes, I know that this is inefficient! - djs 08/08/02
    for (int i = 0; i < 6; i++)
    {
        LLImageDataSharedLock lockIn(rawimages[i]);
        LLImageDataLock lockOut(mRawImages[i]);

        const U8 *sd = rawimages[i]->getData();
        U8 *td = mRawImages[i]->getData();

        S32 offset = 0;
        S32 sx, sy, so;
        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                sx = x;
                sy = y;
                if (flip_y[i])
                {
                    sy = 63 - y;
                }
                if (flip_x[i])
                {
                    sx = 63 - x;
                }
                if (transpose[i])
                {
                    S32 temp = sx;
                    sx = sy;
                    sy = temp;
                }

                so = 64*sy + sx;
                so *= 4;
                *(td + offset++) = *(sd + so++);
                *(td + offset++) = *(sd + so++);
                *(td + offset++) = *(sd + so++);
                *(td + offset++) = *(sd + so++);
            }
        }
    }
}

void LLCubeMap::initGLData()
{
    LL_PROFILE_ZONE_SCOPED;
    for (int i = 0; i < 6; i++)
    {
        mImages[i]->setSubImage(mRawImages[i], 0, 0, RESOLUTION, RESOLUTION);
    }
}

void LLCubeMap::init(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
    if (!gGLManager.mIsDisabled)
    {
        initGL();
        initRawData(rawimages);
        initGLData();
    }
}

void LLCubeMap::initReflectionMap(U32 resolution, U32 components)
{
    U32 texname = 0;

    LLImageGL::generateTextures(1, &texname);

    mImages[0] = new LLImageGL(resolution, resolution, components, true);
    mImages[0]->setTexName(texname);
    mImages[0]->setTarget(mTargets[0], LLTexUnit::TT_CUBE_MAP);
    gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, texname);
    mImages[0]->setAddressMode(LLTexUnit::TAM_CLAMP);
}

void LLCubeMap::initEnvironmentMap(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
    llassert(rawimages.size() == 6);

    U32 texname = 0;

    LLImageGL::generateTextures(1, &texname);

    U32 resolution = rawimages[0]->getWidth();
    U32 components = rawimages[0]->getComponents();

    for (int i = 0; i < 6; i++)
    {
        llassert(rawimages[i]->getWidth() == resolution);
        llassert(rawimages[i]->getHeight() == resolution);
        llassert(rawimages[i]->getComponents() == components);

        mImages[i] = new LLImageGL(resolution, resolution, components, true);
        mImages[i]->setTarget(mTargets[i], LLTexUnit::TT_CUBE_MAP);
        mRawImages[i] = rawimages[i];
        mImages[i]->createGLTexture(0, mRawImages[i], texname);

        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, texname);
        mImages[i]->setAddressMode(LLTexUnit::TAM_CLAMP);
        stop_glerror();

        mImages[i]->setSubImage(mRawImages[i], 0, 0, resolution, resolution);
    }
    enableTexture(0);
    bind();
    mImages[0]->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    gGL.getTexUnit(0)->disable();
    disable();
}

void LLCubeMap::generateMipMaps()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    mImages[0]->setUseMipMaps(true);
    mImages[0]->setHasMipMaps(true);
    enableTexture(0);
    bind();
    mImages[0]->setFilteringOption(LLTexUnit::TFO_BILINEAR);
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("cmgmm - glGenerateMipmap");
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
    gGL.getTexUnit(0)->disable();
    disable();
}

GLuint LLCubeMap::getGLName()
{
    return mImages[0]->getTexName();
}

void LLCubeMap::bind()
{
    gGL.getTexUnit(mTextureStage)->bind(this);
}

void LLCubeMap::enable(S32 stage)
{
    enableTexture(stage);
}

void LLCubeMap::enableTexture(S32 stage)
{
    mTextureStage = stage;
    if (stage >= 0 && LLCubeMap::sUseCubeMaps)
    {
        gGL.getTexUnit(stage)->enable(LLTexUnit::TT_CUBE_MAP);
    }
}

void LLCubeMap::disable(void)
{
    disableTexture();
}

void LLCubeMap::disableTexture(void)
{
    if (mTextureStage >= 0 && LLCubeMap::sUseCubeMaps)
    {
        gGL.getTexUnit(mTextureStage)->disable();
        if (mTextureStage == 0)
        {
            gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
        }
    }
}

void LLCubeMap::setMatrix(S32 stage)
{
    mMatrixStage = stage;

    if (mMatrixStage < 0) return;

    //if (stage > 0)
    {
        gGL.getTexUnit(stage)->activate();
    }

    LLVector3 x(gGLModelView+0);
    LLVector3 y(gGLModelView+4);
    LLVector3 z(gGLModelView+8);

    LLMatrix3 mat3;
    mat3.setRows(x,y,z);
    LLMatrix4 trans(mat3);
    trans.transpose();

    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.pushMatrix();
    gGL.loadMatrix((F32 *)trans.mMatrix);
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    /*if (stage > 0)
    {
        gGL.getTexUnit(0)->activate();
    }*/
}

void LLCubeMap::restoreMatrix()
{
    if (mMatrixStage < 0) return;

    //if (mMatrixStage > 0)
    {
        gGL.getTexUnit(mMatrixStage)->activate();
    }
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    /*if (mMatrixStage > 0)
    {
        gGL.getTexUnit(0)->activate();
    }*/
}


void LLCubeMap::destroyGL()
{
    for (S32 i = 0; i < 6; i++)
    {
        mImages[i] = NULL;
    }
}
