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

// MUST match order of OpenGL face-layers
GLenum LLCubeMapArray::sTargets[6] =
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
};

LLCubeMapArray::LLCubeMapArray()
	: mTextureStage(0)
{
	
}

LLCubeMapArray::~LLCubeMapArray()
{
}

void LLCubeMapArray::allocate(U32 resolution, U32 components, U32 count)
{
    U32 texname = 0;

    LLImageGL::generateTextures(1, &texname);

    mImage = new LLImageGL(resolution, resolution, components, TRUE);
    mImage->setTexName(texname);
    mImage->setTarget(sTargets[0], LLTexUnit::TT_CUBE_MAP_ARRAY);

    mImage->setUseMipMaps(TRUE);
    mImage->setHasMipMaps(TRUE);

    bind(0);

    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0, GL_RGB, resolution, resolution, count*6, 0,
        GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    mImage->setAddressMode(LLTexUnit::TAM_CLAMP);

    mImage->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY_ARB);

    unbind();
}

void LLCubeMapArray::bind(S32 stage)
{
    mTextureStage = stage;
    gGL.getTexUnit(stage)->bindManual(LLTexUnit::TT_CUBE_MAP_ARRAY, getGLName(), TRUE);
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
