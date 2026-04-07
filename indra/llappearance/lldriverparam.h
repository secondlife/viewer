/**
 * @file lldriverparam.h
 * @brief A visual parameter that drives (controls) other visual parameters.
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

#pragma once

#include "llviewervisualparam.h"
#include "llwearabletype.h"
#include <deque>

class LLAvatarAppearance;
class LLDriverParam;
class LLWearable;

//-----------------------------------------------------------------------------

struct LLDrivenEntryInfo
{
    LLDrivenEntryInfo( S32 id, F32 min1, F32 max1, F32 max2, F32 min2 )
        : mDrivenID( id ), mMin1( min1 ), mMax1( max1 ), mMax2( max2 ), mMin2( min2 ) {}
    S32                 mDrivenID;
    F32                 mMin1;
    F32                 mMax1;
    F32                 mMax2;
    F32                 mMin2;
};

struct LLDrivenEntry
{
    LLDrivenEntry( LLViewerVisualParam* param, LLDrivenEntryInfo *info )
        : mParam( param ), mInfo( info ) {}
    LLViewerVisualParam* mParam;
    LLDrivenEntryInfo*   mInfo;
};

//-----------------------------------------------------------------------------

class LLDriverParamInfo : public LLViewerVisualParamInfo
{
    friend class LLDriverParam;
public:
    LLDriverParamInfo();
    ~LLDriverParamInfo() override = default;

    bool parseXml(LLXmlTreeNode* node) override;

    void toStream(std::ostream &out) override;

protected:
    using entry_info_list_t = std::deque<LLDrivenEntryInfo>;
    entry_info_list_t mDrivenInfoList;
    LLDriverParam* mDriverParam; // backpointer
};

//-----------------------------------------------------------------------------

class alignas(16) LLDriverParam : public LLViewerVisualParam
{
    LL_ALIGN_NEW
private:
    // Hide the default constructor.  Force construction with LLAvatarAppearance.
    LLDriverParam() = default;
public:
    explicit LLDriverParam(LLAvatarAppearance* appearance, LLWearable* wearable = NULL);
    ~LLDriverParam();

    // Special: These functions are overridden by child classes
    LLDriverParamInfo* getInfo() const { return static_cast<LLDriverParamInfo*>(mInfo); }
    //   This sets mInfo and calls initialization functions
    bool                    setInfo(LLDriverParamInfo* info);

    LLAvatarAppearance* getAvatarAppearance() { return mAvatarAppearance; }
    const LLAvatarAppearance* getAvatarAppearance() const { return mAvatarAppearance; }

    void                    updateCrossDrivenParams(LLWearableType::EType driven_type);

    LLViewerVisualParam* cloneParam(LLWearable* wearable) const override;

    // LLVisualParam Virtual functions
    void                apply(ESex sex) override {} // apply is called separately for each driven param.
    void                setWeight(F32 weight) override;
    void                setAnimationTarget(F32 target_value) override;
    void                stopAnimating() override;
    bool                linkDrivenParams(visual_param_mapper mapper, bool only_cross_params) override;
    void                resetDrivenParams() override;

    // LLViewerVisualParam Virtual functions
    F32                 getTotalDistortion() override;
    const LLVector4a& getAvgDistortion() override;
    F32                 getMaxDistortion() override;
    LLVector4a          getVertexDistortion(S32 index, LLPolyMesh* poly_mesh) override;
    const LLVector4a* getFirstDistortion(U32* index, LLPolyMesh** poly_mesh) override;
    const LLVector4a* getNextDistortion(U32* index, LLPolyMesh** poly_mesh) override;

    S32                             getDrivenParamsCount() const;
    const LLViewerVisualParam* getDrivenParam(S32 index) const;

    using entry_list_t = std::vector<LLDrivenEntry>;
    entry_list_t& getDrivenList() { return mDriven; }
    void                            setDrivenList(entry_list_t& driven_list) { mDriven = driven_list; }

protected:
    LLDriverParam(const LLDriverParam& pOther);
    F32 getDrivenWeight(const LLDrivenEntry* driven, F32 input_weight);
    void setDrivenWeight(LLDrivenEntry* driven, F32 driven_weight);


    LL_ALIGN_16(LLVector4a  mDefaultVec); // temp holder
    entry_list_t mDriven;
    LLViewerVisualParam* mCurrentDistortionParam{ nullptr };
    // Backlink only; don't make this an LLPointer.
    LLAvatarAppearance* mAvatarAppearance{ nullptr };
    LLWearable* mWearablep{ nullptr };
};

