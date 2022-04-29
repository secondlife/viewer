/**
 * @file llenvironmentmap.cpp
 * @brief LLEnvironmentMap class implementation
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

#include "llenvironmentmap.h"
#include "pipeline.h"
#include "llviewerwindow.h"

LLEnvironmentMap::LLEnvironmentMap()
{
    mOrigin.setVec(0, 0, 0);
}

void LLEnvironmentMap::update(const LLVector3& origin, U32 resolution)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    mOrigin = origin;

    // allocate images
    std::vector<LLPointer<LLImageRaw> > rawimages;
    rawimages.reserve(6);

    for (int i = 0; i < 6; ++i)
    {
        rawimages.push_back(new LLImageRaw(resolution, resolution, 3));
    }

    // ============== modified copy/paste of LLFloater360Capture::capture360Images() follows ==============

    // these are the 6 directions we will point the camera, see LLCubeMap::mTargets
    LLVector3 look_dirs[6] = {
        LLVector3(-1, 0, 0),
        LLVector3(1, 0, 0),
        LLVector3(0, -1, 0),
        LLVector3(0, 1, 0),
        LLVector3(0, 0, -1),
        LLVector3(0, 0, 1)
    };

    LLVector3 look_upvecs[6] = { 
        LLVector3(0, -1, 0), 
        LLVector3(0, -1, 0), 
        LLVector3(0, 0, -1), 
        LLVector3(0, 0, 1), 
        LLVector3(0, -1, 0), 
        LLVector3(0, -1, 0) 
    };

    // save current view/camera settings so we can restore them afterwards
    S32 old_occlusion = LLPipeline::sUseOcclusion;

    // set new parameters specific to the 360 requirements
    LLPipeline::sUseOcclusion = 0;
    LLViewerCamera* camera = LLViewerCamera::getInstance();
    LLVector3 old_origin = camera->getOrigin();
    F32 old_fov = camera->getView();
    F32 old_aspect = camera->getAspect();
    F32 old_yaw = camera->getYaw();

    // camera constants for the square, cube map capture image
    camera->setAspect(1.0); // must set aspect ratio first to avoid undesirable clamping of vertical FoV
    camera->setView(F_PI_BY_TWO);
    camera->yaw(0.0);
    camera->setOrigin(mOrigin);

    // for each of the 6 directions we shoot...
    for (int i = 0; i < 6; i++)
    {
        // set up camera to look in each direction
        camera->lookDir(look_dirs[i], look_upvecs[i]);

        // call the (very) simplified snapshot code that simply deals
        // with a single image, no sub-images etc. but is very fast
        gViewerWindow->simpleSnapshot(rawimages[i],
            resolution, resolution, 1);
    }

    // restore original view/camera/avatar settings settings
    camera->setAspect(old_aspect);
    camera->setView(old_fov);
    camera->yaw(old_yaw);
    camera->setOrigin(old_origin);

    LLPipeline::sUseOcclusion = old_occlusion;

    // ====================================================
    
    mCubeMap = new LLCubeMap(false);
    mCubeMap->initEnvironmentMap(rawimages);
}

