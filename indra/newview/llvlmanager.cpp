/** 
 * @file llvlmanager.cpp
 * @brief LLVLManager class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llvlmanager.h"

#include "indra_constants.h"
#include "bitpack.h"
#include "patch_code.h"
#include "patch_dct.h"
#include "llviewerregion.h"
#include "llframetimer.h"
#include "llsurface.h"

LLVLManager gVLManager;

LLVLManager::~LLVLManager()
{
	S32 i;
	for (i = 0; i < mPacketData.count(); i++)
	{
		delete mPacketData[i];
	}
	mPacketData.reset();
}

void LLVLManager::addLayerData(LLVLData *vl_datap, const S32 mesg_size)
{
	if (LAND_LAYER_CODE == vl_datap->mType)
	{
		mLandBits += mesg_size * 8;
	}
	else if (WIND_LAYER_CODE == vl_datap->mType)
	{
		mWindBits += mesg_size * 8;
	}
	else if (CLOUD_LAYER_CODE == vl_datap->mType)
	{
		mCloudBits += mesg_size * 8;
	}
	else
	{
		llerrs << "Unknown layer type!" << (S32)vl_datap->mType << llendl;
	}

	mPacketData.put(vl_datap);
}

void LLVLManager::unpackData(const S32 num_packets)
{
	static LLFrameTimer decode_timer;
	
	S32 i;
	for (i = 0; i < mPacketData.count(); i++)
	{
		LLVLData *datap = mPacketData[i];

		LLBitPack bit_pack(datap->mData, datap->mSize);
		LLGroupHeader goph;

		decode_patch_group_header(bit_pack, &goph);
		if (LAND_LAYER_CODE == datap->mType)
		{
			datap->mRegionp->getLand().decompressDCTPatch(bit_pack, &goph, FALSE);
		}
		else if (WIND_LAYER_CODE == datap->mType)
		{
			datap->mRegionp->mWind.decompress(bit_pack, &goph);

		}
		else if (CLOUD_LAYER_CODE == datap->mType)
		{

		}
	}

	for (i = 0; i < mPacketData.count(); i++)
	{
		delete mPacketData[i];
	}
	mPacketData.reset();

}

void LLVLManager::resetBitCounts()
{
	mLandBits = mWindBits = mCloudBits = 0;
}

S32 LLVLManager::getLandBits() const
{
	return mLandBits;
}

S32 LLVLManager::getWindBits() const
{
	return mWindBits;
}

S32 LLVLManager::getCloudBits() const
{
	return mCloudBits;
}

S32 LLVLManager::getTotalBytes() const
{
	return mLandBits + mWindBits + mCloudBits;
}

void LLVLManager::cleanupData(LLViewerRegion *regionp)
{
	S32 cur = 0;
	while (cur < mPacketData.count())
	{
		if (mPacketData[cur]->mRegionp == regionp)
		{
			delete mPacketData[cur];
			mPacketData.remove(cur);
		}
		else
		{
			cur++;
		}
	}
}

LLVLData::LLVLData(LLViewerRegion *regionp, const S8 type, U8 *data, const S32 size)
{
	mType = type;
	mData = data;
	mRegionp = regionp;
	mSize = size;
}

LLVLData::~LLVLData()
{
	delete [] mData;
	mData = NULL;
	mRegionp = NULL;
}
