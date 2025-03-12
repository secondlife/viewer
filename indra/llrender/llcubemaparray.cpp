/**
 * @file llcubemaparray.cpp
 * @brief LLCubeMap class implementation
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "llcubemaparray.h"

#include "v4coloru.h"
#include "v3math.h"
#include "v3dmath.h"
#include "m3math.h"
#include "m4math.h"

#include "llrender.h"
#include "llglslshader.h"

#include "llglheaders.h"

//#pragma optimize("", off)

using namespace LLImageGLMemory;

// MUST match order of OpenGL face-layers
GLenum LLCubeMapArray::sTargets[6] =
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

LLVector3 LLCubeMapArray::sLookVecs[6] =
{
        LLVector3(1, 0, 0),
        LLVector3(-1, 0, 0),
        LLVector3(0, 1, 0),
        LLVector3(0, -1, 0),
        LLVector3(0, 0, 1),
        LLVector3(0, 0, -1)
};

LLVector3 LLCubeMapArray::sUpVecs[6] =
{
    LLVector3(0, -1, 0),
    LLVector3(0, -1, 0),
    LLVector3(0, 0, 1),
    LLVector3(0, 0, -1),
    LLVector3(0, -1, 0),
    LLVector3(0, -1, 0)
};

LLVector3 LLCubeMapArray::sClipToCubeLookVecs[6] =
{
        LLVector3(0, 0, -1), //GOOD
        LLVector3(0, 0, 1), //GOOD

        LLVector3(1, 0, 0), // GOOD
        LLVector3(1, 0, 0), // GOOD

        LLVector3(1, 0, 0),
        LLVector3(-1, 0, 0),
};

LLVector3 LLCubeMapArray::sClipToCubeUpVecs[6] =
{
    LLVector3(-1, 0, 0), //GOOD
    LLVector3(1, 0, 0), //GOOD

    LLVector3(0, 1, 0), // GOOD
    LLVector3(0, -1, 0), // GOOD

    LLVector3(0, 0, -1),
    LLVector3(0, 0, 1)
};

LLCubeMapArray::LLCubeMapArray()
    : mTextureStage(0)
{

}

LLCubeMapArray::~LLCubeMapArray()
{
}

void LLCubeMapArray::allocate(U32 resolution, U32 components, U32 count, bool use_mips, bool hdr)
{
    U32 texname = 0;
    mWidth = resolution;
    mCount = count;

    LLImageGL::generateTextures(1, &texname);

    mImage = new LLImageGL(resolution, resolution, components, use_mips);
    mImage->setTexName(texname);
    mImage->setTarget(sTargets[0], LLTexUnit::TT_CUBE_MAP_ARRAY);

    mImage->setUseMipMaps(use_mips);
    mImage->setHasMipMaps(use_mips);

    bind(0);
    free_cur_tex_image();

    U32 format = components == 4 ? GL_RGBA16F : GL_R11F_G11F_B10F;
    if (!hdr)
    {
        format = components == 4 ? GL_RGBA8 : GL_RGB8;
    }
    U32 mip = 0;
    U32 mip_resolution = resolution;
    while (mip_resolution >= 1)
    {
        glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, format, mip_resolution, mip_resolution, count * 6, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        if (!use_mips)
        {
            break;
        }
        mip_resolution /= 2;
        ++mip;
    }

    alloc_tex_image(resolution, resolution, format, count * 6);

    mImage->setAddressMode(LLTexUnit::TAM_CLAMP);

    if (use_mips)
    {
        mImage->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
        //glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);  // <=== latest AMD drivers do not appreciate this method of allocating mipmaps
    }
    else
    {
        mImage->setFilteringOption(LLTexUnit::TFO_BILINEAR);
    }

    unbind();
}

void LLCubeMapArray::bind(S32 stage)
{
    mTextureStage = stage;
    gGL.getTexUnit(stage)->bindManual(LLTexUnit::TT_CUBE_MAP_ARRAY, getGLName(), mImage->getUseMipMaps());
}

void LLCubeMapArray::unbind()
{
    gGL.getTexUnit(mTextureStage)->unbind(LLTexUnit::TT_CUBE_MAP_ARRAY);
    mTextureStage = -1;
}

GLuint LLCubeMapArray::getGLName()
{
    return mImage->getTexName();
}

void LLCubeMapArray::destroyGL()
{
    mImage = NULL;
}
