/**
 * @file llprocessparams.cpp
 * @brief Implementation of LLProcessParams class.
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

#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llstl.h"
#include "llmath.h"
#include "llquantize.h"

// appearance includes
#include "lltexturemanagerbridge.h"
#include "llwearabletype.h"
#include "llavatarappearancedefines.h"

// project includes
#include "llappappearanceutility.h"
#include "llbakingavatar.h"
#include "llbakingtexlayer.h"
#include "llbakingwearable.h"
#include "llbakingwearablesdata.h"
#include "llprocessparams.h"

#define APPEARANCE_PARAM_VERSION 11000

const U32 FULL_RIG_JOINT_COUNT = 20;

using namespace LLAvatarAppearanceDefines;

// Create a bridge to the viewer texture manager.
class LLNullTextureManagerBridge : public LLTextureManagerBridge
{
public:
    /*virtual*/ LLPointer<LLGLTexture> getLocalTexture(bool usemipmaps = TRUE, bool generate_gl_tex = TRUE)
    {
        return NULL;
    }

    /*virtual*/ LLPointer<LLGLTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps, bool generate_gl_tex = TRUE)
    {
        return NULL;
    }

    /*virtual*/ LLGLTexture* getFetchedTexture(const LLUUID &image_id)
    {
        return NULL;
    }
};

LLSD dump_joint_offsets_for_avatar(LLBakingAvatar& avatar)
{
    LLSD joints = LLSD::emptyArray();

    for (U32 joint_index = 0;; joint_index++)
    {
        LLJoint* pJoint = avatar.getCharacterJoint(joint_index);
        if (!pJoint)
        {
            break;
        }
        LLVector3 pos;
        LLUUID mesh_id;
        if (pJoint->hasAttachmentPosOverride(pos, mesh_id))
        {
            LLSD info;
            info["name"] = pJoint->getName();
            info["pos"] = ll_sd_from_vector3(pos);
            info["mesh_id"] = mesh_id;
            joints.append(info);
        }
    }

    return joints;
}

void LLProcessParams::process(std::ostream& output)
{
    if (!mInputData.has("wearables")) throw LLAppException(RV_UNABLE_TO_PARSE, " Missing wearables");

    // Create a null-texture manager bridge.
    gTextureManagerBridgep = new LLNullTextureManagerBridge();

    // Construct the avatar.
    LLBakingWearablesData wearable_data;
    LLBakingAvatar avatar(&wearable_data, mApp->bakeTextureSize());
    avatar.initInstance();
    wearable_data.setAvatarAppearance(&avatar);

    //Process the params data for joint information
    //bool bodySizeSet = processInputDataForJointInfo( avatar );

    // Extract and parse wearables.
    wearable_data.setWearableOutfit(mInputData["wearables"]);

    // Set appearance parameter to flag server-side baking.
    avatar.setVisualParamWeight(APPEARANCE_PARAM_VERSION, 1.0f);

    avatar.setSex( (avatar.getVisualParamWeight( "male" ) > 0.5f) ? SEX_MALE : SEX_FEMALE );

    avatar.updateVisualParams();

    // Extract texture ids, texture hashes, and avatar scale.
    LLSD texture_ids = LLSD::emptyMap();

    //Process the params data for joint information
    bool bodySizeSet = processInputDataForJointInfo( avatar );

    //if we didn't compute a body size from a mesh attachment that included
    //joint offsets we need to do it now
    if (!bodySizeSet )
    {
        avatar.computeBodySize();
    }

    for (U32 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
    {
        EBakedTextureIndex bake_type = (EBakedTextureIndex) baked_index;
        LLBakingTexLayerSet* layer_set = dynamic_cast<LLBakingTexLayerSet*>(avatar.getAvatarLayerSet(bake_type));
        const std::string& slot_name = LLAvatarAppearance::getDictionary()->getTexture(
                            LLAvatarAppearance::getDictionary()->bakedToLocalTextureIndex(bake_type) )->mDefaultImageName;
        texture_ids[slot_name] = layer_set->computeTextureIDs();
    }

    // Extract visual params
    U32 count = 0;
    LLSD params = LLSD::emptyMap();
    LLSD debug_params = LLSD::emptyMap();
    for (LLViewerVisualParam* param = (LLViewerVisualParam*)avatar.getFirstVisualParam();
         param;
         param = (LLViewerVisualParam*)avatar.getNextVisualParam())
    {
        if (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ||
            param->getGroup() == VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE) // do not transmit params of group VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
        {
            const F32 param_value = param->getWeight();
            const U8 new_weight = F32_to_U8(param_value, param->getMinWeight(), param->getMaxWeight());
            LLSD body;
            body["name"] = param->getName();
            body["value"] = param_value;
            body["weight"] = new_weight;
            debug_params[LLSD(param->getID()).asString()] = body;
            params.append(new_weight);
            count++;
        }
    }

    // Output
    LLSD result;
    result["success"] = true;
    result["params"] = params;
    result["debug_params"] = debug_params;
    result["slot_textures"] = texture_ids;
    result["avatar_scale"] = ll_sd_from_vector3(avatar.mBodySize + avatar.mAvatarOffset);
    result["dump_joint_offsets"] = dump_joint_offsets_for_avatar(avatar);
    output << LLSDOStreamer<LLSDXMLFormatter>(result);
}

bool LLProcessParams::processInputDataForJointInfo( LLBakingAvatar& avatar )
{
    bool returnResult = false;

    if ( !mInputData.has("skindata") || mInputData["skindata"].isUndefined() )
    {
        LL_DEBUGS() << "Skipping missing skindata" << LL_ENDL;
        return returnResult;
    }

    const LLSD& skindata = mInputData["skindata"];

    //float pelvisOffset = 0.0f;
    std::vector<std::string> jointNames;
    std::vector<LLVector3>   jointOffsets;

    LLSD::map_const_iterator skinIter = skindata.beginMap();
    LLSD::map_const_iterator skinIterEnd   = skindata.endMap();

    for ( ; skinIter!=skinIterEnd; ++skinIter )
    {
        const LLSD& skin = skinIter->second;
        const char *uuid_str = skinIter->first.c_str();
        const LLUUID mesh_id(uuid_str);

        if ( !skin.has("joint_names") || !skin.has("joint_offset") )
        {
            throw LLAppException(RV_INVALID_SKIN_BLOCK);
        }

        // Build list of joints
        {
            LLSD::array_const_iterator iter(skin["joint_names"].beginArray());
            LLSD::array_const_iterator iter_end(skin["joint_names"].endArray());
            for ( ; iter != iter_end; ++iter )
            {
                jointNames.push_back( iter->asString() );
            }
        }
        // Extract joint offsets
        {
            LLSD::array_const_iterator iter(skin["joint_offset"].beginArray());
            LLSD::array_const_iterator iter_end(skin["joint_offset"].endArray());
            for ( ; iter != iter_end; ++iter )
            {
                jointOffsets.push_back( ll_vector3_from_sd(*iter) );
            }
        }

        // get at the *optional* pelvis offset
        //if ( skin.has("pelvis_offset") )
        //{
        //  pelvisOffset = skin["pelvis_offset"].asReal();
        //}
        // Now apply the extracted joint data to the avatar
        U32 jointCount = jointNames.size();

        if ( jointOffsets.size() < jointCount )
        {
            throw LLAppException(RV_INVALID_SKIN_BLOCK);
        }

        bool fullRig = ( jointCount>= FULL_RIG_JOINT_COUNT ) ? true : false;
        //bool pelvisGotSet = false;

        returnResult = fullRig;

        if ( fullRig )
        {
            for ( U32 i=0; i<jointCount; ++i )
            {
                LLJoint* pJoint = avatar.getJoint( jointNames[i] );
                if ( pJoint )
                {
                    LL_DEBUGS() << "Apply joint : " << jointNames[i] << " "
                             << jointOffsets[i][0] << " "
                             << jointOffsets[i][1] << " "
                             << jointOffsets[i][2] << LL_ENDL;
                    // FIXME replace with new attachment override code.
                    //pJoint->storeCurrentXform( jointOffsets[i] );

                    bool active_override_changed = false;
                    pJoint->addAttachmentPosOverride(jointOffsets[i], mesh_id, "", active_override_changed);
                }
            }

            avatar.computeBodySize();
            avatar.mRoot->touch();
            avatar.mRoot->updateWorldMatrixChildren();

        }

        jointNames.clear();
        jointOffsets.clear();
    }
    return returnResult;
}
