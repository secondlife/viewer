/** 
 * @file llvlmanager.cpp
 * @brief LLVLManager class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
#include "llagent.h"
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
			datap->mRegionp->mCloudLayer.decompress(bit_pack, &goph);
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
