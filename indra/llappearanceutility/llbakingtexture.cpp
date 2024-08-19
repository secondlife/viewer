/**
 * @file llbakingtexture.cpp
 * @brief Implementation of LLBakingTexture class.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

// linden includes
#include "linden_common.h"

// project includes
#include "llbakingtexture.h"
#include "llgltexture.h"
#include "llimage.h"

static const bool USE_MIP_MAPS = TRUE;

LLBakingTexture::LLBakingTexture(const LLUUID& id, const LLImageRaw* raw)
    : LLGLTexture(raw, USE_MIP_MAPS),
      mID(id)
{
}

LLBakingTexture::LLBakingTexture(bool usemipmaps)
    : LLGLTexture(usemipmaps),
      mID(LLUUID::null)
{
}

LLBakingTexture::LLBakingTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps)
    : LLGLTexture(width, height, components, USE_MIP_MAPS),
      mID(LLUUID::null)
{
}

// virtual
LLBakingTexture::~LLBakingTexture()
{
}

void LLBakingTexture::setKnownDrawSize(S32 width, S32 height)
{
    //nothing here.
    LL_ERRS() << "Not implemented." << LL_ENDL;
}

bool LLBakingTexture::bindDefaultImage(S32 stage)
{
    LL_ERRS() << "Not implemented." << LL_ENDL;
    return false;
}

void LLBakingTexture::forceImmediateUpdate()
{
    LL_ERRS() << "Not implemented." << LL_ENDL;
}

void LLBakingTexture::updateBindStatsForTester()
{
    LL_ERRS() << "Not implemented." << LL_ENDL;
}

