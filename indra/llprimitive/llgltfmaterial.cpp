/**
 * @file llgltfmaterial.cpp
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
#include "llgltfmaterial.h"

bool LLGLTFMaterial::updateFromStrings(const std::vector<std::string>& strings) {
    // Lots of different ways that we could send updates.
    // For now, the strings are an ordered list of all the parameters, with none missing.
    if (strings.size() != 16) {
        LL_WARNS() << "Received " << strings.size() << " parameters in GLTF material update." << LL_ENDL;
        return false;
    }
    uint index = 0;
    mAlbedoId.set(strings[index++]);
    mNormalId.set(strings[index++]);
    mMetallicRoughnessId.set(strings[index++]);
    mEmissiveId.set(strings[index++]);
    // TODO: Probably need to tell someone to get the texture when changed? Maybe involving LLViewerTextureManager::getFetchedTexture()?
    mAlbedoColor.set(atof(strings[index].c_str()),
                     atof(strings[index+1].c_str()),
                     atof(strings[index+2].c_str()),
                     atof(strings[index+3].c_str()));
    index += 4;  // C++ doesn't define the order of evaluation of arguments to a funcall, so can't use index++;
    mEmissiveColor.set(atof(strings[index].c_str()),
                       atof(strings[index+1].c_str()),
                       atof(strings[index+2].c_str()));
    index += 3;
    mMetallicFactor = atof(strings[index++].c_str());
    mRoughnessFactor = atof(strings[index++].c_str());
    mAlphaCutoff = atof(strings[index++].c_str());
    mDoubleSided = atoi(strings[index++].c_str());
    setAlphaMode(strings[index++]);
    return true;
}

