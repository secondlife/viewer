#pragma once

/**
 * @file gltfscenemanager.h
 * @brief Builds menus out of items.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llsingleton.h"
#include "llviewerobject.h"

namespace LL
{

    class GLTFSceneManager : public LLSimpleton<GLTFSceneManager>
    {
    public:
        ~GLTFSceneManager();
        // load GLTF file from disk
        void load(); // open filepicker to choose asset
        void load(const std::string& filename); // load asset from filename
        void renderOpaque();

        LLDrawable* lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
            BOOL pick_transparent,
            BOOL pick_rigged,
            BOOL pick_unselectable,
            BOOL pick_reflection_probe,
            S32* face_hit,                   // return the face hit
            LLVector4a* intersection,         // return the intersection point
            LLVector2* tex_coord,            // return the texture coordinates of the intersection point
            LLVector4a* normal,               // return the surface normal at the intersection point
            LLVector4a* tangent);			// return the surface tangent at the intersection point

        std::vector<LLPointer<LLViewerObject>> mObjects;
    };
}


