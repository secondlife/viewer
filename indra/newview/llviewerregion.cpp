/** 
 * @file llviewerregion.cpp
 * @brief Implementation of the LLViewerRegion class.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewerregion.h"

#include "indra_constants.h"
#include "llmath.h"
#include "llhttpclient.h"
#include "llregionflags.h"
#include "llregionhandle.h"
#include "llsurface.h"
#include "message.h"
//#include "vmath.h"
#include "v3math.h"
#include "v4math.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llcaphttpsender.h"
#include "lldir.h"
#include "lleventpoll.h"
#include "llfloatergodtools.h"
#include "llfloaterreporter.h"
#include "llfloaterregioninfo.h"
#include "llhttpnode.h"
#include "llnetmap.h"
#include "llstartup.h"
#include "llviewerobjectlist.h"
#include "llviewerparceloverlay.h"
#include "llvlmanager.h"
#include "llvlcomposition.h"
#include "llvocache.h"
#include "llvoclouds.h"
#include "llworld.h"
#include "viewer.h"

// Viewer object cache version, change if object update
// format changes. JC
const U32 INDRA_OBJECT_CACHE_VERSION = 14;


extern BOOL gNoRender;

const F32 WATER_TEXTURE_SCALE = 8.f;			//  Number of times to repeat the water texture across a region
const S16 MAX_MAP_DIST = 10;




LLViewerRegion::LLViewerRegion(const U64 &handle,
							   const LLHost &host,
							   const U32 grids_per_region_edge, 
							   const U32 grids_per_patch_edge, 
							   const F32 region_width_meters)
:	mCenterGlobal(),
	mHandle(handle),
	mHost( host ),
	mTimeDilation(1.0f),
	mName(""),
	mZoning(""),
	mOwnerID(),
	mIsEstateManager(FALSE),
	mCompositionp(NULL),
	mRegionFlags( REGION_FLAGS_DEFAULT ),
	mSimAccess( SIM_ACCESS_MIN ),
	mBillableFactor(1.0),
	mMaxTasks(MAX_TASKS_PER_REGION),
	mCacheLoaded(FALSE),
	mCacheMap(),
	mCacheEntriesCount(0),
	mCacheID(),
	mEventPoll(NULL)
{
	mWidth = region_width_meters;

	mOriginGlobal = from_region_handle(handle); 

	mLandp = new LLSurface('l', NULL);
	if (!gNoRender)
	{
		// Create the composition layer for the surface
		mCompositionp = new LLVLComposition(mLandp, grids_per_region_edge, region_width_meters/grids_per_region_edge);
		mCompositionp->setSurface(mLandp);

		// Create the surfaces
		mLandp->setRegion(this);
		mLandp->create(grids_per_region_edge,
						grids_per_patch_edge,
						mOriginGlobal,
						mWidth);
	}

	if (!gNoRender)
	{
		mParcelOverlay = new LLViewerParcelOverlay(this, region_width_meters);
	}
	else
	{
		mParcelOverlay = NULL;
	}

	setOriginGlobal(from_region_handle(handle));
	calculateCenterGlobal();

	// Create the object lists
	initStats();

	mCacheStart.append(mCacheEnd);
	
}


void LLViewerRegion::initStats()
{
	mLastNetUpdate.reset();
	mPacketsIn = 0;
	mBitsIn = 0;
	mLastBitsIn = 0;
	mLastPacketsIn = 0;
	mPacketsOut = 0;
	mLastPacketsOut = 0;
	mPacketsLost = 0;
	mLastPacketsLost = 0;
	mPingDelay = 0;
	mAlive = FALSE;					// can become false if circuit disconnects
}



LLViewerRegion::~LLViewerRegion() 
{
	gVLManager.cleanupData(this);
	// Can't do this on destruction, because the neighbor pointers might be invalid.
	// This should be reference counted...
	disconnectAllNeighbors();
	mCloudLayer.destroy();
	gWorldPointer->mPartSim.cleanupRegion(this);

	gObjectList.killObjects(this);

	delete mCompositionp;
	delete mParcelOverlay;
	delete mLandp;
	delete mEventPoll;
	LLHTTPSender::clearSender(mHost);
	
	saveCache();
}


void LLViewerRegion::loadCache()
{
	if (mCacheLoaded)
	{
		return;
	}

	// Presume success.  If it fails, we don't want to try again.
	mCacheLoaded = TRUE;

	LLVOCacheEntry *entry;

	char filename[256];		/* Flawfinder: ignore */
	snprintf(filename, sizeof(filename), "%s%sobjects_%d_%d.slc", 		/* Flawfinder: ignore */
		gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"").c_str(), 
		gDirUtilp->getDirDelimiter().c_str(),
		U32(mHandle>>32)/REGION_WIDTH_UNITS, 
		U32(mHandle)/REGION_WIDTH_UNITS );

	FILE* fp = LLFile::fopen(filename, "rb");		/* Flawfinder: ignore */
	if (!fp)
	{
		// might not have a file, which is normal
		return;
	}

	U32 zero;
	fread(&zero, 1, sizeof(U32), fp);
	if (zero)
	{
		// a non-zero value here means bad things!
		// skip reading the cached values
		llinfos << "Cache file invalid" << llendl;
		fclose(fp);
		return;
	}

	U32 version;
	fread(&version, 1, sizeof(U32), fp);
	if (version != INDRA_OBJECT_CACHE_VERSION)
	{
		// a version mismatch here means we've changed the binary format!
		// skip reading the cached values
		llinfos << "Cache version changed, discarding" << llendl;
		fclose(fp);
		return;
	}

	LLUUID cache_id;
	fread(&cache_id.mData, UUID_BYTES, sizeof(U8), fp);
	if (mCacheID != cache_id)
	{
		llinfos << "Cache ID doesn't match for this region, discarding"
			<< llendl;
		fclose(fp);
		return;
	}

	S32 num_entries;
	fread(&num_entries, 1, sizeof(S32), fp);
	S32 i;
	for (i = 0; i < num_entries; i++)
	{
		entry = new LLVOCacheEntry(fp);
		if (!entry->getLocalID())
		{
			llwarns << "Aborting cache file load for " << filename << ", cache file corruption!" << llendl;
			delete entry;
			entry = NULL;
			break;
		}
		mCacheEnd.insert(*entry);
		mCacheMap[entry->getLocalID()] = entry;
		mCacheEntriesCount++;
	}

	fclose(fp);
}


void LLViewerRegion::saveCache()
{
	if (!mCacheLoaded)
	{
		return;
	}

	S32 num_entries = mCacheEntriesCount;
	if (0 == num_entries)
	{
		return;
	}

	char filename[256];		/* Flawfinder: ignore */
	snprintf(filename, sizeof(filename), "%s%sobjects_%d_%d.slc", 		/* Flawfinder: ignore */
		gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"").c_str(), 
		gDirUtilp->getDirDelimiter().c_str(),
		U32(mHandle>>32)/REGION_WIDTH_UNITS, 
		U32(mHandle)/REGION_WIDTH_UNITS );

	FILE* fp = LLFile::fopen(filename, "wb");		/* Flawfinder: ignore */
	if (!fp)
	{
		llwarns << "Unable to write cache file " << filename << llendl;
		return;
	}

	// write out zero to indicate a version cache file
	U32 zero = 0;
	fwrite(&zero, 1, sizeof(U32), fp);

	// write out version number
	U32 version = INDRA_OBJECT_CACHE_VERSION;
	fwrite(&version, 1, sizeof(U32), fp);

	// write the cache id for this sim
	fwrite(&mCacheID.mData, UUID_BYTES, sizeof(U8), fp);

	fwrite(&num_entries, 1, sizeof(S32), fp);

	LLVOCacheEntry *entry;

	for (entry = mCacheStart.getNext(); entry && (entry != &mCacheEnd); entry = entry->getNext())
	{
		entry->writeToFile(fp);
	}

	mCacheMap.removeAllData();
	mCacheEnd.unlink();
	mCacheEnd.init();
	mCacheStart.deleteAll();
	mCacheStart.init();

	fclose(fp);
}

void LLViewerRegion::sendMessage()
{
	gMessageSystem->sendMessage(mHost);
}

void LLViewerRegion::sendReliableMessage()
{
	gMessageSystem->sendReliable(mHost);
}

void LLViewerRegion::setFlags(BOOL b, U32 flags)
{
	if (b)
	{
		mRegionFlags |=  flags;
	}
	else
	{
		mRegionFlags &= ~flags;
	}
}

void LLViewerRegion::setWaterHeight(F32 water_level)
{
	mLandp->setWaterHeight(water_level);
}

F32 LLViewerRegion::getWaterHeight() const
{
	return mLandp->getWaterHeight();
}

void LLViewerRegion::setRegionFlags(U32 flags)
{
	mRegionFlags = flags;
}


void LLViewerRegion::setOriginGlobal(const LLVector3d &origin_global) 
{ 
	mOriginGlobal = origin_global; 
	mLandp->setOriginGlobal(origin_global);
	mWind.setOriginGlobal(origin_global);
	mCloudLayer.setOriginGlobal(origin_global);
	calculateCenterGlobal();
}


void LLViewerRegion::setTimeDilation(F32 time_dilation)
{
	mTimeDilation = time_dilation;
}


LLVector3 LLViewerRegion::getOriginAgent() const
{
	return gAgent.getPosAgentFromGlobal(mOriginGlobal);
}


LLVector3 LLViewerRegion::getCenterAgent() const
{
	return gAgent.getPosAgentFromGlobal(mCenterGlobal);
}

void LLViewerRegion::setRegionNameAndZone(const char* name_and_zone)
{
	LLString name_zone(name_and_zone);
	std::string::size_type pipe_pos = name_zone.find('|');
	S32 length   = name_zone.size();
	if (pipe_pos != std::string::npos)
	{
		mName   = name_zone.substr(0, pipe_pos);
		mZoning = name_zone.substr(pipe_pos+1, length-(pipe_pos+1));
	}
	else
	{
		mName   = name_zone;
		mZoning = "";
	}

	LLString::stripNonprintable(mName);
	LLString::stripNonprintable(mZoning);
}

BOOL LLViewerRegion::canManageEstate() const
{
	return gAgent.isGodlike()
		|| isEstateManager()
		|| gAgent.getID() == getOwner();
}

const char* LLViewerRegion::getSimAccessString() const
{
	return accessToString(mSimAccess);
}


// static
std::string LLViewerRegion::regionFlagsToString(U32 flags)
{
	std::string result;

	if (flags & REGION_FLAGS_SANDBOX)
	{
		result += "Sandbox";
	}

	if (flags & REGION_FLAGS_ALLOW_DAMAGE)
	{
		result += " Not Safe";
	}

	return result;
}

char* SIM_ACCESS_STR[] = { "Free Trial",
						   "PG",
						   "Mature",
						   "Offline",
						   "Unknown" };
							
// static
const char* LLViewerRegion::accessToString(U8 access)		/* Flawfinder: ignore */
{
	switch(access)		/* Flawfinder: ignore */
	{
	case SIM_ACCESS_TRIAL:
		return SIM_ACCESS_STR[0];

	case SIM_ACCESS_PG:
		return SIM_ACCESS_STR[1];

	case SIM_ACCESS_MATURE:
		return SIM_ACCESS_STR[2];

	case SIM_ACCESS_DOWN:
		return SIM_ACCESS_STR[3];

	case SIM_ACCESS_MIN:
	default:
		return SIM_ACCESS_STR[4];
	}
}

// static
U8 LLViewerRegion::stringToAccess(const char* access_str)
{
	U8 access = SIM_ACCESS_MIN;
	if (0 == strcmp(access_str, SIM_ACCESS_STR[0]))
	{
		access = SIM_ACCESS_TRIAL;
	}
	else if (0 == strcmp(access_str, SIM_ACCESS_STR[1]))
	{
		access = SIM_ACCESS_PG;
	}
	else if (0 == strcmp(access_str, SIM_ACCESS_STR[2]))
	{
		access = SIM_ACCESS_MATURE;
	}
	return access;		/* Flawfinder: ignore */
}

// static
const char* LLViewerRegion::accessToShortString(U8 access)		/* Flawfinder: ignore */
{
	switch(access)		/* Flawfinder: ignore */
	{
	case SIM_ACCESS_PG:
		return "PG";

	case SIM_ACCESS_TRIAL:
		return "TR";

	case SIM_ACCESS_MATURE:
		return "M";

	case SIM_ACCESS_MIN:
	default:
		return "U";
	}
}

// static
void LLViewerRegion::processRegionInfo(LLMessageSystem* msg, void**)
{
	// send it to 'observers'
	LLFloaterGodTools::processRegionInfo(msg);
	LLFloaterRegionInfo::processRegionInfo(msg);
	LLFloaterReporter::processRegionInfo(msg);
}



S32 LLViewerRegion::renderPropertyLines()
{
	if (mParcelOverlay)
	{
		return mParcelOverlay->renderPropertyLines();
	}
	else
	{
		return 0;
	}
}

// This gets called when the height field changes.
void LLViewerRegion::dirtyHeights()
{
	// Property lines need to be reconstructed when the land changes.
	if (mParcelOverlay)
	{
		mParcelOverlay->setDirty();
	}
}

BOOL LLViewerRegion::idleUpdate(F32 max_update_time)
{
	// did_update returns TRUE if we did at least one significant update
	BOOL did_update = mLandp->idleUpdate(max_update_time);
	
	if (mParcelOverlay)
	{
		// Hopefully not a significant time sink...
		mParcelOverlay->idleUpdate();
	}

	return did_update;
}


// As above, but forcibly do the update.
void LLViewerRegion::forceUpdate()
{
	mLandp->idleUpdate(0.f);

	if (mParcelOverlay)
	{
		mParcelOverlay->idleUpdate(true);
	}
}

void LLViewerRegion::connectNeighbor(LLViewerRegion *neighborp, U32 direction)
{
	mLandp->connectNeighbor(neighborp->mLandp, direction);
	mCloudLayer.connectNeighbor(&(neighborp->mCloudLayer), direction);
}


void LLViewerRegion::disconnectAllNeighbors()
{
	mLandp->disconnectAllNeighbors();
	mCloudLayer.disconnectAllNeighbors();
}


F32 LLViewerRegion::getCompositionXY(const S32 x, const S32 y) const
{
	if (x >= 256)
	{
		if (y >= 256)
		{
			LLVector3d center = getCenterGlobal() + LLVector3d(256.f, 256.f, 0.f);
			LLViewerRegion *regionp = gWorldPointer->getRegionFromPosGlobal(center);
			if (regionp)
			{
				// OK, we need to do some hackery here - different simulators no longer use
				// the same composition values, necessarily.
				// If we're attempting to blend, then we want to make the fractional part of
				// this region match the fractional of the adjacent.  For now, just minimize
				// the delta.
				F32 our_comp = getComposition()->getValueScaled(255, 255);
				F32 adj_comp = regionp->getComposition()->getValueScaled(x - 256.f, y - 256.f);
				while (llabs(our_comp - adj_comp) >= 1.f)
				{
					if (our_comp > adj_comp)
					{
						adj_comp += 1.f;
					}
					else
					{
						adj_comp -= 1.f;
					}
				}
				return adj_comp;
			}
		}
		else
		{
			LLVector3d center = getCenterGlobal() + LLVector3d(256.f, 0, 0.f);
			LLViewerRegion *regionp = gWorldPointer->getRegionFromPosGlobal(center);
			if (regionp)
			{
				// OK, we need to do some hackery here - different simulators no longer use
				// the same composition values, necessarily.
				// If we're attempting to blend, then we want to make the fractional part of
				// this region match the fractional of the adjacent.  For now, just minimize
				// the delta.
				F32 our_comp = getComposition()->getValueScaled(255.f, (F32)y);
				F32 adj_comp = regionp->getComposition()->getValueScaled(x - 256.f, (F32)y);
				while (llabs(our_comp - adj_comp) >= 1.f)
				{
					if (our_comp > adj_comp)
					{
						adj_comp += 1.f;
					}
					else
					{
						adj_comp -= 1.f;
					}
				}
				return adj_comp;
			}
		}
	}
	else if (y >= 256)
	{
		LLVector3d center = getCenterGlobal() + LLVector3d(0.f, 256.f, 0.f);
		LLViewerRegion *regionp = gWorldPointer->getRegionFromPosGlobal(center);
		if (regionp)
		{
			// OK, we need to do some hackery here - different simulators no longer use
			// the same composition values, necessarily.
			// If we're attempting to blend, then we want to make the fractional part of
			// this region match the fractional of the adjacent.  For now, just minimize
			// the delta.
			F32 our_comp = getComposition()->getValueScaled((F32)x, 255.f);
			F32 adj_comp = regionp->getComposition()->getValueScaled((F32)x, y - 256.f);
			while (llabs(our_comp - adj_comp) >= 1.f)
			{
				if (our_comp > adj_comp)
				{
					adj_comp += 1.f;
				}
				else
				{
					adj_comp -= 1.f;
				}
			}
			return adj_comp;
		}
	}

	return getComposition()->getValueScaled((F32)x, (F32)y);
}

void LLViewerRegion::calculateCenterGlobal() 
{
	mCenterGlobal = mOriginGlobal;
	mCenterGlobal.mdV[VX] += 0.5 * mWidth;
	mCenterGlobal.mdV[VY] += 0.5 * mWidth;
	if (mLandp)
	{
		mCenterGlobal.mdV[VZ] = 0.5*mLandp->getMinZ() + mLandp->getMaxZ();
	}
	else
	{
		mCenterGlobal.mdV[VZ] = F64( getWaterHeight() );
	}
}

void LLViewerRegion::calculateCameraDistance()
{
	mCameraDistanceSquared = (F32)(gAgent.getCameraPositionGlobal() - getCenterGlobal()).magVecSquared();
}

// ---------------- Friends ----------------

std::ostream& operator<<(std::ostream &s, const LLViewerRegion &region)
{
	s << "{ ";
	s << region.mHost;
	s << " mOriginGlobal = " << region.getOriginGlobal()<< "\n";
	s << "}";
	return s;
}


// ---------------- Protected Member Functions ----------------

void LLViewerRegion::updateNetStats()
{
	F32 dt = mLastNetUpdate.getElapsedTimeAndResetF32();

	LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(mHost);
	if (!cdp)
	{
		mAlive = FALSE;
		return;
	}

	mAlive = TRUE;
	mDeltaTime = dt;

	mLastPacketsIn =	mPacketsIn;
	mLastBitsIn =		mBitsIn;
	mLastPacketsOut =	mPacketsOut;
	mLastPacketsLost =	mPacketsLost;

	mPacketsIn =				cdp->getPacketsIn();
	mBitsIn =					8 * cdp->getBytesIn();
	mPacketsOut =				cdp->getPacketsOut();
	mPacketsLost =				cdp->getPacketsLost();
	mPingDelay =				cdp->getPingDelay();

	mBitStat.addValue(mBitsIn - mLastBitsIn);
	mPacketsStat.addValue(mPacketsIn - mLastPacketsIn);
	mPacketsLostStat.addValue(mPacketsLost);
}


U32 LLViewerRegion::getPacketsLost() const
{
	LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(mHost);
	if (!cdp)
	{
		llinfos << "LLViewerRegion::getPacketsLost couldn't find circuit for " << mHost << llendl;
		return 0;
	}
	else
	{
		return cdp->getPacketsLost();
	}
}

BOOL LLViewerRegion::pointInRegionGlobal(const LLVector3d &point_global) const
{
	LLVector3 pos_region = getPosRegionFromGlobal(point_global);

	if (pos_region.mV[VX] < 0)
	{
		return FALSE;
	}
	if (pos_region.mV[VX] >= mWidth)
	{
		return FALSE;
	}
	if (pos_region.mV[VY] < 0)
	{
		return FALSE;
	}
	if (pos_region.mV[VY] >= mWidth)
	{
		return FALSE;
	}
	return TRUE;
}

LLVector3 LLViewerRegion::getPosRegionFromGlobal(const LLVector3d &point_global) const
{
	LLVector3 pos_region;
	pos_region.setVec(point_global - mOriginGlobal);
	return pos_region;
}

LLVector3d LLViewerRegion::getPosGlobalFromRegion(const LLVector3 &pos_region) const
{
	LLVector3d pos_region_d;
	pos_region_d.setVec(pos_region);
	return pos_region_d + mOriginGlobal;
}

LLVector3 LLViewerRegion::getPosAgentFromRegion(const LLVector3 &pos_region) const
{
	LLVector3d pos_global = getPosGlobalFromRegion(pos_region);

	return gAgent.getPosAgentFromGlobal(pos_global);
}

LLVector3 LLViewerRegion::getPosRegionFromAgent(const LLVector3 &pos_agent) const
{
	return pos_agent - getOriginAgent();
}

F32 LLViewerRegion::getLandHeightRegion(const LLVector3& region_pos)
{
	return mLandp->resolveHeightRegion( region_pos );
}

BOOL LLViewerRegion::isOwnedSelf(const LLVector3& pos)
{
	return mParcelOverlay->isOwnedSelf(pos);
}

// Owned by a group you belong to?  (officer or member)
BOOL LLViewerRegion::isOwnedGroup(const LLVector3& pos)
{
	return mParcelOverlay->isOwnedGroup(pos);
}

void LLViewerRegion::updateCoarseLocations(LLMessageSystem* msg)
{
	//llinfos << "CoarseLocationUpdate" << llendl;
	mMapAvatars.reset();

	U8 x_pos = 0;
	U8 y_pos = 0;
	U8 z_pos = 0;

	U32 pos = 0x0;

	S16 agent_index;
	S16 target_index;
	msg->getS16Fast(_PREHASH_Index, _PREHASH_You, agent_index);
	msg->getS16Fast(_PREHASH_Index, _PREHASH_Prey, target_index);

	S32 count = msg->getNumberOfBlocksFast(_PREHASH_Location);
	for(S32 i = 0; i < count; i++)
	{
		msg->getU8Fast(_PREHASH_Location, _PREHASH_X, x_pos, i);
		msg->getU8Fast(_PREHASH_Location, _PREHASH_Y, y_pos, i);
		msg->getU8Fast(_PREHASH_Location, _PREHASH_Z, z_pos, i);

		//llinfos << "  object X: " << (S32)x_pos << " Y: " << (S32)y_pos
		//		<< " Z: " << (S32)(z_pos * 4)
		//		<< llendl;

		// treat the target specially for the map, and don't add you
		// or the target
		if(i == target_index)
		{
			LLVector3d global_pos(mOriginGlobal);
			global_pos.mdV[VX] += (F64)(x_pos);
			global_pos.mdV[VY] += (F64)(y_pos);
			global_pos.mdV[VZ] += (F64)(z_pos) * 4.0;
			LLAvatarTracker::instance().setTrackedCoarseLocation(global_pos);
		}
		else if( i != agent_index)
		{
			pos = 0x0;
			pos |= x_pos;
			pos <<= 8;
			pos |= y_pos;
			pos <<= 8;
			pos |= z_pos;
			mMapAvatars.put(pos);
		}
	}
}

LLString LLViewerRegion::getInfoString()
{
	char tmp_buf[256];		/* Flawfinder: ignore */
	LLString info;
	
	info = "Region: ";
	getHost().getString(tmp_buf, 256);
	info += tmp_buf;
	info += ":";
	info += getName();
	info += "\n";

	U32 x, y;
	from_region_handle(getHandle(), &x, &y);
	snprintf(tmp_buf, sizeof(tmp_buf), "%d:%d", x, y);		/* Flawfinder: ignore */
	info += "Handle:";
	info += tmp_buf;
	info += "\n";
	return info;
}


void LLViewerRegion::cacheFullUpdate(LLViewerObject* objectp, LLDataPackerBinaryBuffer &dp)
{
	U32 local_id = objectp->getLocalID();
	U32 crc = objectp->getCRC();

	LLVOCacheEntry *entry = mCacheMap.getIfThere(local_id);

	if (entry)
	{
		// we've seen this object before
		if (entry->getCRC() == crc)
		{
			// Record a hit
			entry->recordDupe();
		}
		else
		{
			// Update the cache entry
			mCacheMap.removeData(local_id);
			delete entry;
			entry = new LLVOCacheEntry(local_id, crc, dp);
			mCacheEnd.insert(*entry);
			mCacheMap[local_id] = entry;
		}
	}
	else
	{
		// we haven't seen this object before

		// Create new entry and add to map
		if (mCacheEntriesCount > MAX_OBJECT_CACHE_ENTRIES)
		{
			entry = mCacheStart.getNext();
			mCacheMap.removeData(entry->getLocalID());
			delete entry;
			mCacheEntriesCount--;
		}
		entry = new LLVOCacheEntry(local_id, crc, dp);

		mCacheEnd.insert(*entry);
		mCacheMap[local_id] = entry;
		mCacheEntriesCount++;
	}
	return ;
}

// Get data packer for this object, if we have cached data
// AND the CRC matches. JC
LLDataPacker *LLViewerRegion::getDP(U32 local_id, U32 crc)
{
	llassert(mCacheLoaded);

	LLVOCacheEntry *entry = mCacheMap.getIfThere(local_id);

	if (entry)
	{
		// we've seen this object before
		if (entry->getCRC() == crc)
		{
			// Record a hit
			entry->recordHit();
			return entry->getDP(crc);
		}
		else
		{
			// llinfos << "CRC miss for " << local_id << llendl;
			mCacheMissCRC.put(local_id);
		}
	}
	else
	{
		// llinfos << "Cache miss for " << local_id << llendl;
		mCacheMissFull.put(local_id);
	}
	return NULL;
}

void LLViewerRegion::addCacheMissFull(const U32 local_id)
{
	mCacheMissFull.put(local_id);
}

void LLViewerRegion::requestCacheMisses()
{
	S32 full_count = mCacheMissFull.count();
	S32 crc_count = mCacheMissCRC.count();
	if (full_count == 0 && crc_count == 0) return;

	LLMessageSystem* msg = gMessageSystem;
	BOOL start_new_message = TRUE;
	S32 blocks = 0;
	S32 i;

	const U8 CACHE_MISS_TYPE_FULL = 0;
	const U8 CACHE_MISS_TYPE_CRC  = 1;

	// Send full cache miss updates.  For these, we KNOW we don't
	// have a viewer object.
	for (i = 0; i < full_count; i++)
	{
		if (start_new_message)
		{
			msg->newMessageFast(_PREHASH_RequestMultipleObjects);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			start_new_message = FALSE;
		}

		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU8Fast(_PREHASH_CacheMissType, CACHE_MISS_TYPE_FULL);
		msg->addU32Fast(_PREHASH_ID, mCacheMissFull[i]);
		blocks++;

		if (blocks >= 255)
		{
			sendReliableMessage();
			start_new_message = TRUE;
			blocks = 0;
		}
	}

	// Send CRC miss updates.  For these, we _might_ have a viewer object,
	// but probably not.
	for (i = 0; i < crc_count; i++)
	{
		if (start_new_message)
		{
			msg->newMessageFast(_PREHASH_RequestMultipleObjects);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			start_new_message = FALSE;
		}

		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU8Fast(_PREHASH_CacheMissType, CACHE_MISS_TYPE_CRC);
		msg->addU32Fast(_PREHASH_ID, mCacheMissCRC[i]);
		blocks++;

		if (blocks >= 255)
		{
			sendReliableMessage();
			start_new_message = TRUE;
			blocks = 0;
		}
	}

	// finish any pending message
	if (!start_new_message)
	{
		sendReliableMessage();
	}
	mCacheMissFull.reset();
	mCacheMissCRC.reset();

	// llinfos << "KILLDEBUG Sent cache miss full " << full_count << " crc " << crc_count << llendl;
}

void LLViewerRegion::dumpCache()
{
	const S32 BINS = 4;
	S32 hit_bin[BINS];
	S32 change_bin[BINS];

	S32 i;
	for (i = 0; i < BINS; ++i)
	{
		hit_bin[i] = 0;
		change_bin[i] = 0;
	}

	LLVOCacheEntry *entry;

	for (entry = mCacheStart.getNext(); entry && (entry != &mCacheEnd); entry = entry->getNext())
	{
		S32 hits = entry->getHitCount();
		S32 changes = entry->getCRCChangeCount();

		hits = llclamp(hits, 0, BINS-1);
		changes = llclamp(changes, 0, BINS-1);

		hit_bin[hits]++;
		change_bin[changes]++;
	}

	llinfos << "Count " << mCacheEntriesCount << llendl;
	for (i = 0; i < BINS; i++)
	{
		llinfos << "Hits " << i << " " << hit_bin[i] << llendl;
	}
	for (i = 0; i < BINS; i++)
	{
		llinfos << "Changes " << i << " " << change_bin[i] << llendl;
	}
}

void LLViewerRegion::unpackRegionHandshake()
{
	LLMessageSystem *msg = gMessageSystem;

	const S32 SIM_NAME_BUF = 256;
	U32 region_flags;
	U8 sim_access;
	char sim_name[SIM_NAME_BUF];		/* Flawfinder: ignore */
	LLUUID sim_owner;
	BOOL is_estate_manager;
	F32 water_height;
	F32 billable_factor;
	LLUUID cache_id;

	msg->getU32		("RegionInfo", "RegionFlags", region_flags);
	msg->getU8		("RegionInfo", "SimAccess", sim_access);
	msg->getString	("RegionInfo", "SimName", SIM_NAME_BUF, sim_name);
	msg->getUUID	("RegionInfo", "SimOwner", sim_owner);
	msg->getBOOL	("RegionInfo", "IsEstateManager", is_estate_manager);
	msg->getF32		("RegionInfo", "WaterHeight", water_height);
	msg->getF32		("RegionInfo", "BillableFactor", billable_factor);
	msg->getUUID	("RegionInfo", "CacheID", cache_id );

	setRegionFlags(region_flags);
	setSimAccess(sim_access);
	setRegionNameAndZone(sim_name);
	setOwner(sim_owner);
	setIsEstateManager(is_estate_manager);
	setWaterHeight(water_height);
	setBillableFactor(billable_factor);
	setCacheID(cache_id);

	LLUUID region_id;
	msg->getUUID("RegionInfo2", "RegionID", region_id);
	setRegionID(region_id);

	LLVLComposition *compp = getComposition();
	if (compp)
	{
		LLUUID tmp_id;

		msg->getUUID("RegionInfo", "TerrainDetail0", tmp_id);
		compp->setDetailTextureID(0, tmp_id);
		msg->getUUID("RegionInfo", "TerrainDetail1", tmp_id);
		compp->setDetailTextureID(1, tmp_id);
		msg->getUUID("RegionInfo", "TerrainDetail2", tmp_id);
		compp->setDetailTextureID(2, tmp_id);
		msg->getUUID("RegionInfo", "TerrainDetail3", tmp_id);
		compp->setDetailTextureID(3, tmp_id);

		F32 tmp_f32;
		msg->getF32("RegionInfo", "TerrainStartHeight00", tmp_f32);
		compp->setStartHeight(0, tmp_f32);
		msg->getF32("RegionInfo", "TerrainStartHeight01", tmp_f32);
		compp->setStartHeight(1, tmp_f32);
		msg->getF32("RegionInfo", "TerrainStartHeight10", tmp_f32);
		compp->setStartHeight(2, tmp_f32);
		msg->getF32("RegionInfo", "TerrainStartHeight11", tmp_f32);
		compp->setStartHeight(3, tmp_f32);

		msg->getF32("RegionInfo", "TerrainHeightRange00", tmp_f32);
		compp->setHeightRange(0, tmp_f32);
		msg->getF32("RegionInfo", "TerrainHeightRange01", tmp_f32);
		compp->setHeightRange(1, tmp_f32);
		msg->getF32("RegionInfo", "TerrainHeightRange10", tmp_f32);
		compp->setHeightRange(2, tmp_f32);
		msg->getF32("RegionInfo", "TerrainHeightRange11", tmp_f32);
		compp->setHeightRange(3, tmp_f32);

		// If this is an UPDATE (params already ready, we need to regenerate
		// all of our terrain stuff, by
		if (compp->getParamsReady())
		{
			getLand().dirtyAllPatches();
		}
		else
		{
			compp->setParamsReady();
		}
	}


	// Now that we have the name, we can load the cache file
	// off disk.
	loadCache();

	// After loading cache, signal that simulator can start
	// sending data.
	// TODO: Send all upstream viewer->sim handshake info here.
	LLHost host = msg->getSender();
	msg->newMessage("RegionHandshakeReply");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("RegionInfo");
	msg->addU32("Flags", 0x0 );
	msg->sendReliable(host);
}



class BaseCapabilitiesComplete : public LLHTTPClient::Responder
{
public:
    BaseCapabilitiesComplete(LLViewerRegion* region)
		: mRegion(region)
    { }

    void error(U32 statusNum, const std::string& reason)
    {
		llinfos << "BaseCapabilitiesComplete::error "
			<< statusNum << ": " << reason << llendl;
		
		if (STATE_SEED_GRANTED_WAIT == gStartupState)
		{
			gStartupState = STATE_SEED_CAP_GRANTED;
		}
    }

    void result(const LLSD& content)
    {
		LLSD::map_const_iterator iter;
		for(iter = content.beginMap(); iter != content.endMap(); ++iter)
		{
			mRegion->setCapability(iter->first, iter->second);
			llinfos << "BaseCapabilitiesComplete::result got capability for " 
				<< iter->first << llendl;
		}
		
		if (STATE_SEED_GRANTED_WAIT == gStartupState)
		{
			gStartupState = STATE_SEED_CAP_GRANTED;
		}
	}

    static boost::intrusive_ptr<BaseCapabilitiesComplete> build(
								LLViewerRegion* region)
    {
		return boost::intrusive_ptr<BaseCapabilitiesComplete>(
							 new BaseCapabilitiesComplete(region));
    }

private:
	LLViewerRegion* mRegion;
};


void LLViewerRegion::setSeedCapability(const std::string& url)
{
  if (getCapability("Seed") == url)
    {
      llwarns << "Ignoring duplicate seed capability" << llendl;
      return;
    }
	delete mEventPoll;
	mEventPoll = NULL;
	
	mCapabilities.clear();
	setCapability("Seed", url);

	LLSD capabilityNames = LLSD::emptyArray();
	capabilityNames.append("MapLayer");
	capabilityNames.append("MapLayerGod");
	capabilityNames.append("NewFileAgentInventory");
	capabilityNames.append("EventQueueGet");
	capabilityNames.append("UpdateGestureAgentInventory");
	capabilityNames.append("UpdateNotecardAgentInventory");
	capabilityNames.append("UpdateScriptAgentInventory");
	capabilityNames.append("UpdateGestureTaskInventory");
	capabilityNames.append("UpdateNotecardTaskInventory");
	capabilityNames.append("UpdateScriptTaskInventory");
	capabilityNames.append("SendPostcard");
	capabilityNames.append("ViewerStartAuction");
	capabilityNames.append("ParcelGodReserveForNewbie");
	capabilityNames.append("SendUserReport");
	capabilityNames.append("SendUserReportWithScreenshot");
	capabilityNames.append("RequestTextureDownload");
	capabilityNames.append("UntrustedSimulatorMessage");
	capabilityNames.append("ParcelVoiceInfoRequest");
	capabilityNames.append("ChatSessionRequest");

	llinfos << "posting to seed " << url << llendl;
	
	LLHTTPClient::post(url, capabilityNames, BaseCapabilitiesComplete::build(this));
}

void LLViewerRegion::setCapability(const std::string& name, const std::string& url)
{
	if(name == "EventQueueGet")
	{
		delete mEventPoll;
		mEventPoll = NULL;
		mEventPoll = new LLEventPoll(url, getHost());
	}
	else if(name == "UntrustedSimulatorMessage")
	{
		LLHTTPSender::setSender(mHost, new LLCapHTTPSender(url));
	}
	else
	{
		mCapabilities[name] = url;
	}
}

std::string LLViewerRegion::getCapability(const std::string& name) const
{
	CapabilityMap::const_iterator iter = mCapabilities.find(name);
	if(iter == mCapabilities.end())
	{
		return "";
	}
	return iter->second;
}

void LLViewerRegion::logActiveCapabilities() const
{
	int count = 0;
	CapabilityMap::const_iterator iter;
	for (iter = mCapabilities.begin(); iter != mCapabilities.end(); iter++, count++)
	{
		if (!iter->second.empty())
		{
			llinfos << iter->first << " URL is " << iter->second << llendl;
		}
	}
	llinfos << "Dumped " << count << " entries." << llendl;
}

