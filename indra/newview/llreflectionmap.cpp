/**
 * @file llreflectionmap.cpp
 * @brief LLReflectionMap class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llreflectionmap.h"
#include "pipeline.h"
#include "llviewerwindow.h"

LLReflectionMap::LLReflectionMap()
{
}

void LLReflectionMap::update(const LLVector3& origin, U32 resolution)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(LLPipeline::sRenderDeferred);

    // make sure resolution is < gPipeline.mDeferredScreen.getWidth()

    while (resolution > gPipeline.mDeferredScreen.getWidth() ||
        resolution > gPipeline.mDeferredScreen.getHeight())
    {
        resolution /= 2;
    }

    if (resolution == 0)
    {
        return;
    }

    mOrigin.load3(origin.mV);

    mCubeMap = new LLCubeMap(false);
    mCubeMap->initReflectionMap(resolution);

    gViewerWindow->cubeSnapshot(origin, mCubeMap);

    mCubeMap->generateMipMaps();
}

