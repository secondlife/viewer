/**
 * @file   llgltfmateriallist.h
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


#pragma once

#include "llgltfmaterial.h"
#include "llpointer.h"

#include <unordered_map>

class LLGLTFMaterialList
{
public:
    LLGLTFMaterialList() {}

    typedef std::unordered_map<LLUUID, LLPointer<LLGLTFMaterial > > List;
    List mList;

    LLGLTFMaterial* getMaterial(const LLUUID& id);

    void addMaterial(const LLUUID& id, LLGLTFMaterial* material);
    void removeMaterial(const LLUUID& id);

    static void registerCallbacks();

};

extern LLGLTFMaterialList gGLTFMaterialList;


