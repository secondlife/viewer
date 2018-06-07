/**
* @file llrigginginfo.cpp
* @brief  Functions for tracking rigged box extents
*
* $LicenseInfo:firstyear=2018&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2018, Linden Research, Inc.
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

#include "llmath.h"
#include "llrigginginfo.h"

//#if LL_WINDOWS
//#pragma optimize("", off)
//#endif

//-----------------------------------------------------------------------------
// LLJointRiggingInfo
//-----------------------------------------------------------------------------
LLJointRiggingInfo::LLJointRiggingInfo()
{
    mRiggedExtents[0].clear();
    mRiggedExtents[1].clear();
    mIsRiggedTo = false;
}

bool LLJointRiggingInfo::isRiggedTo() const
{
    return mIsRiggedTo;
}

void LLJointRiggingInfo::setIsRiggedTo(bool val)
{
    mIsRiggedTo = val;
}
    
LLVector4a *LLJointRiggingInfo::getRiggedExtents()
{
    return mRiggedExtents;
}

const LLVector4a *LLJointRiggingInfo::getRiggedExtents() const
{
    return mRiggedExtents;
}

// Combine two rigging info states.
// - isRiggedTo if either of the source infos are rigged to
// - box is union of the two sources
void LLJointRiggingInfo::merge(const LLJointRiggingInfo& other)
{
    if (other.mIsRiggedTo)
    {
        if (mIsRiggedTo)
        {
            // Combine existing boxes
            update_min_max(mRiggedExtents[0], mRiggedExtents[1], other.mRiggedExtents[0]);
            update_min_max(mRiggedExtents[0], mRiggedExtents[1], other.mRiggedExtents[1]);
        }
        else
        {
            // Initialize box
            mIsRiggedTo = true;
            mRiggedExtents[0] = other.mRiggedExtents[0];
            mRiggedExtents[1] = other.mRiggedExtents[1];
        }
    }
}

LLJointRiggingInfoTab::LLJointRiggingInfoTab():
    mRigInfoPtr(NULL),
    mSize(0)
{
}

LLJointRiggingInfoTab::~LLJointRiggingInfoTab()
{
    clear();
}

// This doesn't preserve data if the size changes. In practice
// this doesn't matter because the size is always either
// LL_CHARACTER_MAX_ANIMATED_JOINTS or 0.
void LLJointRiggingInfoTab::resize(S32 size)
{
    if (size != mSize)
    {
        clear();
        if (size > 0)
        {
            mRigInfoPtr = new LLJointRiggingInfo[size];
            mSize = size;
        }
    }
}

void LLJointRiggingInfoTab::clear()
{
    if (mRigInfoPtr)
    {
        delete[](mRigInfoPtr);
        mRigInfoPtr = NULL;
        mSize = 0;
    }
}

void showDetails(const LLJointRiggingInfoTab& src, const std::string& str)
{
	S32 count_rigged = 0;
	S32 count_box = 0;
    LLVector4a zero_vec;
    zero_vec.clear();
    for (S32 i=0; i<src.size(); i++)
    {
        if (src[i].isRiggedTo())
        {
            count_rigged++;
            if ((!src[i].getRiggedExtents()[0].equals3(zero_vec)) ||
                (!src[i].getRiggedExtents()[1].equals3(zero_vec)))
            {
                count_box++;
            }
       }
    }
    LL_DEBUGS("RigSpammish") << "details: " << str << " has " << count_rigged << " rigged joints, of which " << count_box << " are non-empty" << LL_ENDL;
}

void LLJointRiggingInfoTab::merge(const LLJointRiggingInfoTab& src)
{
    //showDetails(*this, "input this");
    // Size should be either LL_CHARACTER_MAX_ANIMATED_JOINTS, or 0 if
    // no data. Not necessarily the same for both inputs.
    if (src.size() > size())
    {
        resize(src.size());
    }
    S32 min_size = llmin(size(), src.size());
    for (S32 i=0; i<min_size; i++)
    {
        (*this)[i].merge(src[i]);
    }
    //showDetails(src, "input src");
    //showDetails(*this, "output this");

}
