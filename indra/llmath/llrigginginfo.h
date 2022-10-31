/**
* @file llrigginginfo.h
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

// Stores information related to associated rigged mesh vertices
// This lives in llmath because llvolume lives in llmath.

#ifndef LL_LLRIGGINGINFO_H
#define LL_LLRIGGINGINFO_H

#include "llvector4a.h"

// Extents are in joint space
// isRiggedTo is based on the state of all currently associated rigged meshes
class alignas(16) LLJointRiggingInfo
{
    LL_ALIGN_NEW
public:
    LLJointRiggingInfo();
    bool isRiggedTo() const;
    void setIsRiggedTo(bool val);
    LLVector4a *getRiggedExtents();
    const LLVector4a *getRiggedExtents() const;
    void merge(const LLJointRiggingInfo& other);

private:
    LLVector4a mRiggedExtents[2];
    bool mIsRiggedTo;
};

// For storing all the rigging info associated with a given avatar or
// object, keyed by joint_num.
// Using direct memory management instead of std::vector<> to avoid alignment issues.
class LLJointRiggingInfoTab
{
public:
    LLJointRiggingInfoTab();
    ~LLJointRiggingInfoTab();
    void resize(S32 size);
    void clear();
    S32 size() const { return mSize; }
    void merge(const LLJointRiggingInfoTab& src);
    LLJointRiggingInfo& operator[](S32 i) { return mRigInfoPtr[i]; }
    const LLJointRiggingInfo& operator[](S32 i) const { return mRigInfoPtr[i]; };
    bool needsUpdate() { return mNeedsUpdate; }
    void setNeedsUpdate(bool val) { mNeedsUpdate = val; }
private:
    // Not implemented
    LLJointRiggingInfoTab& operator=(const LLJointRiggingInfoTab& src);
    LLJointRiggingInfoTab(const LLJointRiggingInfoTab& src);

    LLJointRiggingInfo *mRigInfoPtr;
    S32 mSize;
    bool mNeedsUpdate;
};

#endif
