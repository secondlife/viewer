/** 
 * @file llresourcedata.h
 * @brief Tracking object for uploads.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLRESOURCEDATA_H
#define LLRESOURCEDATA_H

#include "llassetstorage.h"
#include "llinventorytype.h"

struct LLResourceData
{
	LLAssetInfo mAssetInfo;
	LLAssetType::EType mPreferredLocation;
	LLInventoryType::EType mInventoryType;
	U32 mNextOwnerPerm;
	void *mUserData;
};

#endif
