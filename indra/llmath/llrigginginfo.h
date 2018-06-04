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

#ifndef	LL_LLRIGGINGINFO_H
#define	LL_LLRIGGINGINFO_H

// Extents are in joint space
// isRiggedTo is based on the state of all currently associated rigged meshes
class LLJointRiggingInfo
{
public:
    LLJointRiggingInfo();
    bool isRiggedTo() const;
    void setIsRiggedTo(bool val);
    LLVector4a *getRiggedExtents();
    const LLVector4a *getRiggedExtents() const;
    void merge(const LLJointRiggingInfo& other);
private:
	LL_ALIGN_16(LLVector4a mRiggedExtents[2]);
    bool mIsRiggedTo;
};

// For storing all the rigging info associated with a given avatar or
// object, keyed by joint_num.
typedef std::vector<LLJointRiggingInfo> joint_rig_info_tab;

void mergeRigInfoTab(joint_rig_info_tab& dst, const joint_rig_info_tab& src);

#endif
