/** 
 * @file lllandmark.cpp
 * @brief Landmark asset class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lllandmark.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>  	// for sscanf() on linux

#include "message.h"
#include "llregionhandle.h"

std::pair<LLUUID, U64> LLLandmark::mLocalRegion;
LLLandmark::region_map_t LLLandmark::mRegions;
LLLandmark::region_callback_t LLLandmark::mRegionCallback;

LLLandmark::LLLandmark() :
	mGlobalPositionKnown(false)
{
}

LLLandmark::LLLandmark(const LLVector3d& pos) :
	mGlobalPositionKnown(true),
	mGlobalPos( pos )
{
}

bool LLLandmark::getGlobalPos(LLVector3d& pos)
{
	if(mGlobalPositionKnown)
	{
		pos = mGlobalPos;
	}
	else if(mRegionID.notNull())
	{
		F32 g_x = -1.0;
		F32 g_y = -1.0;
		if(mRegionID == mLocalRegion.first)
		{
			from_region_handle(mLocalRegion.second, &g_x, &g_y);
		}
		else
		{
			region_map_t::iterator it = mRegions.find(mRegionID);
			if(it != mRegions.end())
			{
				from_region_handle((*it).second.mRegionHandle, &g_x, &g_y);
			}
		}
		if((g_x > 0.f) && (g_y > 0.f))
		{
			pos.mdV[0] = g_x + mRegionPos.mV[0];
			pos.mdV[1] = g_y + mRegionPos.mV[1];
			pos.mdV[2] = mRegionPos.mV[2];
			setGlobalPos(pos);
		}
	}
	return mGlobalPositionKnown;
}

void LLLandmark::setGlobalPos(const LLVector3d& pos)
{
	mGlobalPos = pos;
	mGlobalPositionKnown = true;
}

bool LLLandmark::getRegionID(LLUUID& region_id)
{
	if(mRegionID.notNull())
	{
		region_id = mRegionID;
		return true;
	}
	return false;
}

LLVector3 LLLandmark::getRegionPos() const
{
	return mRegionPos;
}


// static
LLLandmark* LLLandmark::constructFromString(const char *buffer)
{
	const char* cur = buffer;
	S32 chars_read = 0;
	S32 count = 0;
	U32 version = 0;

	// read version 
	count = sscanf( cur, "Landmark version %u\n%n", &version, &chars_read );
	if(count != 1)
	{
		goto error;
	}

	if(version == 1)
	{
		LLVector3d pos;
		cur += chars_read;
		// read position
		count = sscanf( cur, "position %lf %lf %lf\n%n", pos.mdV+VX, pos.mdV+VY, pos.mdV+VZ, &chars_read );
		if( count != 3 )
		{
			goto error;
		}
		cur += chars_read;
		// llinfos << "Landmark read: " << pos << llendl;
		
		return new LLLandmark(pos);
	}
	else if(version == 2)
	{
		// *NOTE: Changing the buffer size will require changing the
		// scanf call below.
		char region_id_str[MAX_STRING];
		LLVector3 pos;
		cur += chars_read;
		count = sscanf(cur, "region_id %254s\n%n", region_id_str, &chars_read);
		if(count != 1) goto error;
		cur += chars_read;
		count = sscanf(cur, "local_pos %f %f %f\n%n", pos.mV+VX, pos.mV+VY, pos.mV+VZ, &chars_read);
		if(count != 3) goto error;
		cur += chars_read;
		LLLandmark* lm = new LLLandmark;
		lm->mRegionID.set(region_id_str);
		lm->mRegionPos = pos;
		return lm;
	}

 error:
	llinfos << "Bad Landmark Asset: bad _DATA_ block." << llendl;
	return NULL;
}


// static
void LLLandmark::registerCallbacks(LLMessageSystem* msg)
{
	msg->setHandlerFunc("RegionIDAndHandleReply", &processRegionIDAndHandle);
}

// static
void LLLandmark::requestRegionHandle(
	LLMessageSystem* msg,
	const LLHost& upstream_host,
	const LLUUID& region_id,
	LLRegionHandleCallback* callback)
{
	if(region_id.isNull())
	{
		// don't bother with checking - it's 0.
		lldebugs << "requestRegionHandle: null" << llendl;
		if(callback)
		{
			const U64 U64_ZERO = 0;
			callback->dataReady(region_id, U64_ZERO);
		}
	}
	else
	{
		if(region_id == mLocalRegion.first)
		{
			lldebugs << "requestRegionHandle: local" << llendl;
			if(callback)
			{
				callback->dataReady(region_id, mLocalRegion.second);
			}
		}
		else
		{
			region_map_t::iterator it = mRegions.find(region_id);
			if(it == mRegions.end())
			{
				lldebugs << "requestRegionHandle: upstream" << llendl;
				if(callback)
				{
					region_callback_t::value_type vt(region_id, callback);
					mRegionCallback.insert(vt);
				}
				lldebugs << "Landmark requesting information about: "
						 << region_id << llendl;
				msg->newMessage("RegionHandleRequest");
				msg->nextBlock("RequestBlock");
				msg->addUUID("RegionID", region_id);
				msg->sendReliable(upstream_host);
			}
			else if(callback)
			{
				// we have the answer locally - just call the callack.
				lldebugs << "requestRegionHandle: ready" << llendl;
				callback->dataReady(region_id, (*it).second.mRegionHandle);
			}
		}
	}

	// As good a place as any to expire old entries.
	expireOldEntries();
}

// static
void LLLandmark::setRegionHandle(const LLUUID& region_id, U64 region_handle)
{
	mLocalRegion.first = region_id;
	mLocalRegion.second = region_handle;
}


// static
void LLLandmark::processRegionIDAndHandle(LLMessageSystem* msg, void**)
{
	LLUUID region_id;
	msg->getUUID("ReplyBlock", "RegionID", region_id);
	mRegions.erase(region_id);
	CacheInfo info;
	const F32 CACHE_EXPIRY_SECONDS = 60.0f * 10.0f; // ten minutes
	info.mTimer.setTimerExpirySec(CACHE_EXPIRY_SECONDS);
	msg->getU64("ReplyBlock", "RegionHandle", info.mRegionHandle);
	region_map_t::value_type vt(region_id, info);
	mRegions.insert(vt);

#if LL_DEBUG
	U32 grid_x, grid_y;
	grid_from_region_handle(info.mRegionHandle, &grid_x, &grid_y);
	lldebugs << "Landmark got reply for region: " << region_id << " "
			 << grid_x << "," << grid_y << llendl;
#endif

	// make all the callbacks here.
	region_callback_t::iterator it;
	while((it = mRegionCallback.find(region_id)) != mRegionCallback.end())
	{
		(*it).second->dataReady(region_id, info.mRegionHandle);
		mRegionCallback.erase(it);
	}
}

// static
void LLLandmark::expireOldEntries()
{
	for(region_map_t::iterator it = mRegions.begin(); it != mRegions.end(); )
	{
		if((*it).second.mTimer.hasExpired())
		{
			mRegions.erase(it++);
		}
		else
		{
			++it;
		}
	}
}
