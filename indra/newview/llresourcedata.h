/** 
 * @file llresourcedata.h
 * @brief Tracking object for uploads.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LLRESOURCEDATA_H
#define LLRESOURCEDATA_H

#include "llassetstorage.h"
#include "llinventorytype.h"

struct LLResourceData
{
    LLAssetInfo mAssetInfo;
    LLFolderType::EType mPreferredLocation;
    LLInventoryType::EType mInventoryType;
    U32 mNextOwnerPerm;
    S32 mExpectedUploadCost;
    void *mUserData;
    static const S8 INVALID_LOCATION = -2;
};

#endif
