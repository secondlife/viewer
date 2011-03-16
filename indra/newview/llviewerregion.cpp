/** 
 * @file llviewerregion.cpp
 * @brief Implementation of the LLViewerRegion class.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llviewerregion.h"

// linden libraries
#include "indra_constants.h"
#include "llavatarnamecache.h"		// name lookup cap url
#include "llfloaterreg.h"
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
#include "llagentcamera.h"
#include "llcallingcard.h"
#include "llcaphttpsender.h"
#include "llcommandhandler.h"
#include "lldir.h"
#include "lleventpoll.h"
#include "llfloatergodtools.h"
#include "llfloaterreporter.h"
#include "llfloaterregioninfo.h"
#include "llhttpnode.h"
#include "llsdutil.h"
#include "llstartup.h"
#include "lltrans.h"
#include "llurldispatcher.h"
#include "llviewerobjectlist.h"
#include "llviewerparceloverlay.h"
#include "llviewerstatsrecorder.h"
#include "llvlmanager.h"
#include "llvlcomposition.h"
#include "llvocache.h"
#include "llvoclouds.h"
#include "llworld.h"
#include "llspatialpartition.h"
#include "stringize.h"
#include "llviewercontrol.h"

#ifdef LL_WINDOWS
	#pragma warning(disable:4355)
#endif

extern BOOL gNoRender;

const F32 WATER_TEXTURE_SCALE = 8.f;			//  Number of times to repeat the water texture across a region
const S16 MAX_MAP_DIST = 10;

// support for secondlife:///app/region/{REGION} SLapps
// N.B. this is defined to work exactly like the classic secondlife://{REGION}
// However, the later syntax cannot support spaces in the region name because
// spaces (and %20 chars) are illegal in the hostname of an http URL. Some
// browsers let you get away with this, but some do not (such as Qt's Webkit).
// Hence we introduced the newer secondlife:///app/region alternative.
class LLRegionHandler : public LLCommandHandler
{
public:
	// requests will be throttled from a non-trusted browser
	LLRegionHandler() : LLCommandHandler("region", UNTRUSTED_THROTTLE) {}

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		// make sure that we at least have a region name
		int num_params = params.size();
		if (num_params < 1)
		{
			return false;
		}

		// build a secondlife://{PLACE} SLurl from this SLapp
		std::string url = "secondlife://";
		for (int i = 0; i < num_params; i++)
		{
			if (i > 0)
			{
				url += "/";
			}
			url += params[i].asString();
		}

		// Process the SLapp as if it was a secondlife://{PLACE} SLurl
		LLURLDispatcher::dispatch(url, web, true);
		return true;
	}
};
LLRegionHandler gRegionHandler;

class BaseCapabilitiesComplete : public LLHTTPClient::Responder
{
	LOG_CLASS(BaseCapabilitiesComplete);
public:
    BaseCapabilitiesComplete(LLViewerRegion* region)
		: mRegion(region)
    { }
	virtual ~BaseCapabilitiesComplete()
	{
		if(mRegion)
		{
			mRegion->setHttpResponderPtrNULL() ;
		}
	}

	void setRegion(LLViewerRegion* region)
	{
		mRegion = region ;
	}

    void error(U32 statusNum, const std::string& reason)
    {
		LL_WARNS2("AppInit", "Capabilities") << statusNum << ": " << reason << LL_ENDL;
		
		if (STATE_SEED_GRANTED_WAIT == LLStartUp::getStartupState())
		{
			LLStartUp::setStartupState( STATE_SEED_CAP_GRANTED );
		}
    }

    void result(const LLSD& content)
    {
		if(!mRegion || LLHTTPClient::ResponderPtr(this) != mRegion->getHttpResponderPtr()) //region is removed or responder is not created.
		{
			return ;
		}

		LLSD::map_const_iterator iter;
		for(iter = content.beginMap(); iter != content.endMap(); ++iter)
		{
			mRegion->setCapability(iter->first, iter->second);
			LL_DEBUGS2("AppInit", "Capabilities") << "got capability for " 
				<< iter->first << LL_ENDL;

			/* HACK we're waiting for the ServerReleaseNotes */
			if (iter->first == "ServerReleaseNotes" && mRegion->getReleaseNotesRequested())
			{
				mRegion->showReleaseNotes();
			}
		}

		mRegion->setCapabilitiesReceived(true);

		if (STATE_SEED_GRANTED_WAIT == LLStartUp::getStartupState())
		{
			LLStartUp::setStartupState( STATE_SEED_CAP_GRANTED );
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
	mMaxTasks(DEFAULT_MAX_REGION_WIDE_PRIM_COUNT),
	mClassID(0),
	mCPURatio(0),
	mColoName("unknown"),
	mProductSKU("unknown"),
	mProductName("unknown"),
	mHttpUrl(""),
	mCacheLoaded(FALSE),
	mCacheDirty(FALSE),
	mCacheID(),
	mEventPoll(NULL),
	mReleaseNotesRequested(FALSE),
    // I'd prefer to set the LLCapabilityListener name to match the region
    // name -- it's disappointing that's not available at construction time.
    // We could instead store an LLCapabilityListener*, making
    // setRegionNameAndZone() replace the instance. Would that pose
    // consistency problems? Can we even request a capability before calling
    // setRegionNameAndZone()?
    // For testability -- the new Michael Feathers paradigm --
    // LLCapabilityListener binds all the globals it expects to need at
    // construction time.
    mCapabilityListener(host.getString(), gMessageSystem, *this,
                        gAgent.getID(), gAgent.getSessionID()),
	mCapabilitiesReceived(false)
{
	mWidth = region_width_meters;
	mOriginGlobal = from_region_handle(handle); 
	updateRenderMatrix();

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

	//create object partitions
	//MUST MATCH declaration of eObjectPartitions
	mObjectPartition.push_back(new LLHUDPartition());		//PARTITION_HUD
	mObjectPartition.push_back(new LLTerrainPartition());	//PARTITION_TERRAIN
	mObjectPartition.push_back(new LLVoidWaterPartition());	//PARTITION_VOIDWATER
	mObjectPartition.push_back(new LLWaterPartition());		//PARTITION_WATER
	mObjectPartition.push_back(new LLTreePartition());		//PARTITION_TREE
	mObjectPartition.push_back(new LLParticlePartition());	//PARTITION_PARTICLE
	mObjectPartition.push_back(new LLCloudPartition());		//PARTITION_CLOUD
	mObjectPartition.push_back(new LLGrassPartition());		//PARTITION_GRASS
	mObjectPartition.push_back(new LLVolumePartition());	//PARTITION_VOLUME
	mObjectPartition.push_back(new LLBridgePartition());	//PARTITION_BRIDGE
	mObjectPartition.push_back(new LLHUDParticlePartition());//PARTITION_HUD_PARTICLE
	mObjectPartition.push_back(NULL);						//PARTITION_NONE
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
	mAlive = false;					// can become false if circuit disconnects
}

LLViewerRegion::~LLViewerRegion() 
{
	if(mHttpResponderPtr)
	{
		(static_cast<BaseCapabilitiesComplete*>(mHttpResponderPtr.get()))->setRegion(NULL) ;
	}

	gVLManager.cleanupData(this);
	// Can't do this on destruction, because the neighbor pointers might be invalid.
	// This should be reference counted...
	disconnectAllNeighbors();
	mCloudLayer.destroy();
	LLViewerPartSim::getInstance()->cleanupRegion(this);

	gObjectList.killObjects(this);

	delete mCompositionp;
	delete mParcelOverlay;
	delete mLandp;
	delete mEventPoll;
	LLHTTPSender::clearSender(mHost);
	
	saveObjectCache();

	std::for_each(mObjectPartition.begin(), mObjectPartition.end(), DeletePointer());
}

/*virtual*/ 
const LLHost&	LLViewerRegion::getHost() const				
{ 
	return mHost; 
}

void LLViewerRegion::loadObjectCache()
{
	if (mCacheLoaded)
	{
		return;
	}

	// Presume success.  If it fails, we don't want to try again.
	mCacheLoaded = TRUE;

	if(LLVOCache::hasInstance())
	{
		LLVOCache::getInstance()->readFromCache(mHandle, mCacheID, mCacheMap) ;
	}
}


void LLViewerRegion::saveObjectCache()
{
	if (!mCacheLoaded)
	{
		return;
	}

	if (mCacheMap.empty())
	{
		return;
	}

	if(LLVOCache::hasInstance())
	{
		LLVOCache::getInstance()->writeToCache(mHandle, mCacheID, mCacheMap, mCacheDirty) ;
		mCacheDirty = FALSE;
	}

	for(LLVOCacheEntry::vocache_entry_map_t::iterator iter = mCacheMap.begin(); iter != mCacheMap.end(); ++iter)
	{
		delete iter->second;
	}
	mCacheMap.clear();
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

BOOL LLViewerRegion::isVoiceEnabled() const
{
	return (getRegionFlags() & REGION_FLAGS_ALLOW_VOICE);
}

void LLViewerRegion::setRegionFlags(U32 flags)
{
	mRegionFlags = flags;
}


void LLViewerRegion::setOriginGlobal(const LLVector3d &origin_global) 
{ 
	mOriginGlobal = origin_global; 
	updateRenderMatrix();
	mLandp->setOriginGlobal(origin_global);
	mWind.setOriginGlobal(origin_global);
	mCloudLayer.setOriginGlobal(origin_global);
	calculateCenterGlobal();
}

void LLViewerRegion::updateRenderMatrix()
{
	mRenderMatrix.setTranslation(getOriginAgent());
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

void LLViewerRegion::setRegionNameAndZone	(const std::string& name_zone)
{
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

	LLStringUtil::stripNonprintable(mName);
	LLStringUtil::stripNonprintable(mZoning);
}

BOOL LLViewerRegion::canManageEstate() const
{
	return gAgent.isGodlike()
		|| isEstateManager()
		|| gAgent.getID() == getOwner();
}

const std::string LLViewerRegion::getSimAccessString() const
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

// static
std::string LLViewerRegion::accessToString(U8 sim_access)
{
	switch(sim_access)
	{
	case SIM_ACCESS_PG:
		return LLTrans::getString("SIM_ACCESS_PG");

	case SIM_ACCESS_MATURE:
		return LLTrans::getString("SIM_ACCESS_MATURE");

	case SIM_ACCESS_ADULT:
		return LLTrans::getString("SIM_ACCESS_ADULT");

	case SIM_ACCESS_DOWN:
		return LLTrans::getString("SIM_ACCESS_DOWN");

	case SIM_ACCESS_MIN:
	default:
		return LLTrans::getString("SIM_ACCESS_MIN");
	}
}

// static
std::string LLViewerRegion::getAccessIcon(U8 sim_access)
{
	switch(sim_access)
	{
	case SIM_ACCESS_MATURE:
		return "Parcel_M_Dark";

	case SIM_ACCESS_ADULT:
		return "Parcel_R_Light";

	case SIM_ACCESS_PG:
		return "Parcel_PG_Light";

	case SIM_ACCESS_MIN:
	default:
		return "";
	}
}

// static
std::string LLViewerRegion::accessToShortString(U8 sim_access)
{
	switch(sim_access)		/* Flawfinder: ignore */
	{
	case SIM_ACCESS_PG:
		return "PG";

	case SIM_ACCESS_MATURE:
		return "M";

	case SIM_ACCESS_ADULT:
		return "A";

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
	LLMemType mt_ivr(LLMemType::MTYPE_IDLE_UPDATE_VIEWER_REGION);
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
			LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(center);
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
			LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(center);
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
		LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(center);
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
	mCenterGlobal.mdV[VZ] = 0.5*mLandp->getMinZ() + mLandp->getMaxZ();
}

void LLViewerRegion::calculateCameraDistance()
{
	mCameraDistanceSquared = (F32)(gAgentCamera.getCameraPositionGlobal() - getCenterGlobal()).magVecSquared();
}

std::ostream& operator<<(std::ostream &s, const LLViewerRegion &region)
{
	s << "{ ";
	s << region.mHost;
	s << " mOriginGlobal = " << region.getOriginGlobal()<< "\n";
    std::string name(region.getName()), zone(region.getZoning());
    if (! name.empty())
    {
        s << " mName         = " << name << '\n';
    }
    if (! zone.empty())
    {
        s << " mZoning       = " << zone << '\n';
    }
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
		mAlive = false;
		return;
	}

	mAlive = true;
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

bool LLViewerRegion::isAlive()
{
	return mAlive;
}

BOOL LLViewerRegion::isOwnedSelf(const LLVector3& pos)
{
	if (mParcelOverlay)
	{
		return mParcelOverlay->isOwnedSelf(pos);
	} else {
		return FALSE;
	}
}

// Owned by a group you belong to?  (officer or member)
BOOL LLViewerRegion::isOwnedGroup(const LLVector3& pos)
{
	if (mParcelOverlay)
	{
		return mParcelOverlay->isOwnedGroup(pos);
	} else {
		return FALSE;
	}
}

// the new TCP coarse location handler node
class CoarseLocationUpdate : public LLHTTPNode
{
public:
	virtual void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const
	{
		LLHost host(input["sender"].asString());
		LLViewerRegion* region = LLWorld::getInstance()->getRegion(host);
		if( !region )
		{
			return;
		}

		S32 target_index = input["body"]["Index"][0]["Prey"].asInteger();
		S32 you_index    = input["body"]["Index"][0]["You" ].asInteger();

		LLDynamicArray<U32>* avatar_locs = &region->mMapAvatars;
		LLDynamicArray<LLUUID>* avatar_ids = &region->mMapAvatarIDs;
		avatar_locs->reset();
		avatar_ids->reset();

		//llinfos << "coarse locations agent[0] " << input["body"]["AgentData"][0]["AgentID"].asUUID() << llendl;
		//llinfos << "my agent id = " << gAgent.getID() << llendl;
		//llinfos << ll_pretty_print_sd(input) << llendl;

		LLSD 
			locs   = input["body"]["Location"],
			agents = input["body"]["AgentData"];
		LLSD::array_iterator 
			locs_it = locs.beginArray(), 
			agents_it = agents.beginArray();
		BOOL has_agent_data = input["body"].has("AgentData");

		for(int i=0; 
			locs_it != locs.endArray(); 
			i++, locs_it++)
		{
			U8 
				x = locs_it->get("X").asInteger(),
				y = locs_it->get("Y").asInteger(),
				z = locs_it->get("Z").asInteger();
			// treat the target specially for the map, and don't add you or the target
			if(i == target_index)
			{
				LLVector3d global_pos(region->getOriginGlobal());
				global_pos.mdV[VX] += (F64)x;
				global_pos.mdV[VY] += (F64)y;
				global_pos.mdV[VZ] += (F64)z * 4.0;
				LLAvatarTracker::instance().setTrackedCoarseLocation(global_pos);
			}
			else if( i != you_index)
			{
				U32 loc = x << 16 | y << 8 | z; loc = loc;
				U32 pos = 0x0;
				pos |= x;
				pos <<= 8;
				pos |= y;
				pos <<= 8;
				pos |= z;
				avatar_locs->put(pos);
				//llinfos << "next pos: " << x << "," << y << "," << z << ": " << pos << llendl;
				if(has_agent_data) // for backwards compatibility with old message format
				{
					LLUUID agent_id(agents_it->get("AgentID").asUUID());
					//llinfos << "next agent: " << agent_id.asString() << llendl;
					avatar_ids->put(agent_id);
				}
			}
			if (has_agent_data)
			{
				agents_it++;
			}
		}
	}
};

// build the coarse location HTTP node under the "/message" URL
LLHTTPRegistration<CoarseLocationUpdate>
   gHTTPRegistrationCoarseLocationUpdate(
	   "/message/CoarseLocationUpdate");


// the deprecated coarse location handler
void LLViewerRegion::updateCoarseLocations(LLMessageSystem* msg)
{
	//llinfos << "CoarseLocationUpdate" << llendl;
	mMapAvatars.reset();
	mMapAvatarIDs.reset(); // only matters in a rare case but it's good to be safe.

	U8 x_pos = 0;
	U8 y_pos = 0;
	U8 z_pos = 0;

	U32 pos = 0x0;

	S16 agent_index;
	S16 target_index;
	msg->getS16Fast(_PREHASH_Index, _PREHASH_You, agent_index);
	msg->getS16Fast(_PREHASH_Index, _PREHASH_Prey, target_index);

	BOOL has_agent_data = msg->has(_PREHASH_AgentData);
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_Location);
	for(S32 i = 0; i < count; i++)
	{
		msg->getU8Fast(_PREHASH_Location, _PREHASH_X, x_pos, i);
		msg->getU8Fast(_PREHASH_Location, _PREHASH_Y, y_pos, i);
		msg->getU8Fast(_PREHASH_Location, _PREHASH_Z, z_pos, i);
		LLUUID agent_id = LLUUID::null;
		if(has_agent_data)
		{
			msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id, i);
		}

		//llinfos << "  object X: " << (S32)x_pos << " Y: " << (S32)y_pos
		//		<< " Z: " << (S32)(z_pos * 4)
		//		<< llendl;

		// treat the target specially for the map
		if(i == target_index)
		{
			LLVector3d global_pos(mOriginGlobal);
			global_pos.mdV[VX] += (F64)(x_pos);
			global_pos.mdV[VY] += (F64)(y_pos);
			global_pos.mdV[VZ] += (F64)(z_pos) * 4.0;
			LLAvatarTracker::instance().setTrackedCoarseLocation(global_pos);
		}
		
		//don't add you
		if( i != agent_index)
		{
			pos = 0x0;
			pos |= x_pos;
			pos <<= 8;
			pos |= y_pos;
			pos <<= 8;
			pos |= z_pos;
			mMapAvatars.put(pos);
			if(has_agent_data)
			{
				mMapAvatarIDs.put(agent_id);
			}
		}
	}
}

void LLViewerRegion::getInfo(LLSD& info)
{
	info["Region"]["Host"] = getHost().getIPandPort();
	info["Region"]["Name"] = getName();
	U32 x, y;
	from_region_handle(getHandle(), &x, &y);
	info["Region"]["Handle"]["x"] = (LLSD::Integer)x;
	info["Region"]["Handle"]["y"] = (LLSD::Integer)y;
}

LLViewerRegion::eCacheUpdateResult LLViewerRegion::cacheFullUpdate(LLViewerObject* objectp, LLDataPackerBinaryBuffer &dp)
{
	U32 local_id = objectp->getLocalID();
	U32 crc = objectp->getCRC();

	LLVOCacheEntry* entry = get_if_there(mCacheMap, local_id, (LLVOCacheEntry*)NULL);

	if (entry)
	{
		// we've seen this object before
		if (entry->getCRC() == crc)
		{
			// Record a hit
			entry->recordDupe();
			return CACHE_UPDATE_DUPE;
		}

		// Update the cache entry
		mCacheMap.erase(local_id);
		delete entry;
		entry = new LLVOCacheEntry(local_id, crc, dp);
		mCacheMap[local_id] = entry;
		return CACHE_UPDATE_CHANGED;
	}

	// we haven't seen this object before

	// Create new entry and add to map
	eCacheUpdateResult result = CACHE_UPDATE_ADDED;
	if (mCacheMap.size() > MAX_OBJECT_CACHE_ENTRIES)
	{
		mCacheMap.erase(mCacheMap.begin());
		result = CACHE_UPDATE_REPLACED;
		
	}
	entry = new LLVOCacheEntry(local_id, crc, dp);

	mCacheMap[local_id] = entry;
	return result;
}

// Get data packer for this object, if we have cached data
// AND the CRC matches. JC
LLDataPacker *LLViewerRegion::getDP(U32 local_id, U32 crc, U8 &cache_miss_type)
{
	//llassert(mCacheLoaded);  This assert failes often, changing to early-out -- davep, 2010/10/18

	if (mCacheLoaded)
	{
		LLVOCacheEntry* entry = get_if_there(mCacheMap, local_id, (LLVOCacheEntry*)NULL);

		if (entry)
		{
			// we've seen this object before
			if (entry->getCRC() == crc)
			{
				// Record a hit
				entry->recordHit();
			cache_miss_type = CACHE_MISS_TYPE_NONE;
				return entry->getDP(crc);
			}
			else
			{
				// llinfos << "CRC miss for " << local_id << llendl;
			cache_miss_type = CACHE_MISS_TYPE_CRC;
				mCacheMissCRC.put(local_id);
			}
		}
		else
		{
			// llinfos << "Cache miss for " << local_id << llendl;
		cache_miss_type = CACHE_MISS_TYPE_FULL;
			mCacheMissFull.put(local_id);
		}
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

	mCacheDirty = TRUE ;
	// llinfos << "KILLDEBUG Sent cache miss full " << full_count << " crc " << crc_count << llendl;
	#if LL_RECORD_VIEWER_STATS
	LLViewerStatsRecorder::instance()->beginObjectUpdateEvents(this);
	LLViewerStatsRecorder::instance()->recordRequestCacheMissesEvent(full_count + crc_count);
	LLViewerStatsRecorder::instance()->endObjectUpdateEvents();
	#endif
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
	for(LLVOCacheEntry::vocache_entry_map_t::iterator iter = mCacheMap.begin(); iter != mCacheMap.end(); ++iter)
	{
		entry = iter->second ;

		S32 hits = entry->getHitCount();
		S32 changes = entry->getCRCChangeCount();

		hits = llclamp(hits, 0, BINS-1);
		changes = llclamp(changes, 0, BINS-1);

		hit_bin[hits]++;
		change_bin[changes]++;
	}

	llinfos << "Count " << mCacheMap.size() << llendl;
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

	U32 region_flags;
	U8 sim_access;
	std::string sim_name;
	LLUUID sim_owner;
	BOOL is_estate_manager;
	F32 water_height;
	F32 billable_factor;
	LLUUID cache_id;

	msg->getU32		("RegionInfo", "RegionFlags", region_flags);
	msg->getU8		("RegionInfo", "SimAccess", sim_access);
	msg->getString	("RegionInfo", "SimName", sim_name);
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
	
	// Retrieve the CR-53 (Homestead/Land SKU) information
	S32 classID = 0;
	S32 cpuRatio = 0;
	std::string coloName;
	std::string productSKU;
	std::string productName;

	// the only reasonable way to decide if we actually have any data is to
	// check to see if any of these fields have positive sizes
	if (msg->getSize("RegionInfo3", "ColoName") > 0 ||
	    msg->getSize("RegionInfo3", "ProductSKU") > 0 ||
	    msg->getSize("RegionInfo3", "ProductName") > 0)
	{
		msg->getS32     ("RegionInfo3", "CPUClassID",  classID);
		msg->getS32     ("RegionInfo3", "CPURatio",    cpuRatio);
		msg->getString  ("RegionInfo3", "ColoName",    coloName);
		msg->getString  ("RegionInfo3", "ProductSKU",  productSKU);
		msg->getString  ("RegionInfo3", "ProductName", productName);
		
		mClassID = classID;
		mCPURatio = cpuRatio;
		mColoName = coloName;
		mProductSKU = productSKU;
		mProductName = productName;
	}

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
	loadObjectCache();

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

void LLViewerRegion::setSeedCapability(const std::string& url)
{
	if (getCapability("Seed") == url)
    {
		// llwarns << "Ignoring duplicate seed capability" << llendl;
		return;
    }
	
	delete mEventPoll;
	mEventPoll = NULL;
	
	mCapabilities.clear();
	setCapability("Seed", url);

	LLSD capabilityNames = LLSD::emptyArray();
	
	capabilityNames.append("AttachmentResources");
	capabilityNames.append("AvatarPickerSearch");
	capabilityNames.append("ChatSessionRequest");
	capabilityNames.append("CopyInventoryFromNotecard");
	capabilityNames.append("DispatchRegionInfo");
	capabilityNames.append("EstateChangeInfo");
	capabilityNames.append("EventQueueGet");
	capabilityNames.append("ObjectMedia");
	capabilityNames.append("ObjectMediaNavigate");

	if (gSavedSettings.getBOOL("UseHTTPInventory"))
	{
		capabilityNames.append("FetchLib2");
		capabilityNames.append("FetchLibDescendents2");
		capabilityNames.append("FetchInventory2");
		capabilityNames.append("FetchInventoryDescendents2");
	}

	capabilityNames.append("GetDisplayNames");
	capabilityNames.append("GetTexture");
	capabilityNames.append("GetMesh");
	capabilityNames.append("GetObjectCost");
	capabilityNames.append("GetObjectPhysicsData");
	capabilityNames.append("GroupProposalBallot");
	capabilityNames.append("HomeLocation");
	capabilityNames.append("LandResources");
	capabilityNames.append("MapLayer");
	capabilityNames.append("MapLayerGod");
	capabilityNames.append("NewFileAgentInventory");
	capabilityNames.append("NewFileAgentInventoryVariablePrice");
	capabilityNames.append("ObjectAdd");
	capabilityNames.append("ParcelPropertiesUpdate");
	capabilityNames.append("ParcelMediaURLFilterList");
	capabilityNames.append("ParcelNavigateMedia");
	capabilityNames.append("ParcelVoiceInfoRequest");
	capabilityNames.append("ProductInfoRequest");
	capabilityNames.append("ProvisionVoiceAccountRequest");
	capabilityNames.append("RemoteParcelRequest");
	capabilityNames.append("RequestTextureDownload");
	capabilityNames.append("SearchStatRequest");
	capabilityNames.append("SearchStatTracking");
	capabilityNames.append("SendPostcard");
	capabilityNames.append("SendUserReport");
	capabilityNames.append("SendUserReportWithScreenshot");
	capabilityNames.append("ServerReleaseNotes");
	capabilityNames.append("SimConsole");
	capabilityNames.append("SimulatorFeatures");
	capabilityNames.append("SetDisplayName");
	capabilityNames.append("SimConsoleAsync");
	capabilityNames.append("StartGroupProposal");
	capabilityNames.append("TextureStats");
	capabilityNames.append("UntrustedSimulatorMessage");
	capabilityNames.append("UpdateAgentInformation");
	capabilityNames.append("UpdateAgentLanguage");
	capabilityNames.append("UpdateGestureAgentInventory");
	capabilityNames.append("UpdateNotecardAgentInventory");
	capabilityNames.append("UpdateScriptAgent");
	capabilityNames.append("UpdateGestureTaskInventory");
	capabilityNames.append("UpdateNotecardTaskInventory");
	capabilityNames.append("UpdateScriptTask");
	capabilityNames.append("UploadBakedTexture");
	capabilityNames.append("UploadObjectAsset");
	capabilityNames.append("ViewerMetrics");
	capabilityNames.append("ViewerStartAuction");
	capabilityNames.append("ViewerStats");
	// Please add new capabilities alphabetically to reduce
	// merge conflicts.

	llinfos << "posting to seed " << url << llendl;

	mHttpResponderPtr = BaseCapabilitiesComplete::build(this) ;
	LLHTTPClient::post(url, capabilityNames, mHttpResponderPtr);
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
		if(name == "GetTexture")
		{
			mHttpUrl = url ;
		}
	}
}

bool LLViewerRegion::isSpecialCapabilityName(const std::string &name)
{
	return name == "EventQueueGet" || name == "UntrustedSimulatorMessage";
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

bool LLViewerRegion::capabilitiesReceived() const
{
	return mCapabilitiesReceived;
}

void LLViewerRegion::setCapabilitiesReceived(bool received)
{
	mCapabilitiesReceived = received;
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

LLSpatialPartition* LLViewerRegion::getSpatialPartition(U32 type)
{
	if (type < mObjectPartition.size())
	{
		return mObjectPartition[type];
	}
	return NULL;
}

// the viewer can not yet distinquish between normal- and estate-owned objects
// so we collapse these two bits and enable the UI if either are set
const U32 ALLOW_RETURN_ENCROACHING_OBJECT = REGION_FLAGS_ALLOW_RETURN_ENCROACHING_OBJECT
											| REGION_FLAGS_ALLOW_RETURN_ENCROACHING_ESTATE_OBJECT;

bool LLViewerRegion::objectIsReturnable(const LLVector3& pos, const std::vector<LLBBox>& boxes) const
{
	return (mParcelOverlay != NULL)
		&& (mParcelOverlay->isOwnedSelf(pos)
			|| mParcelOverlay->isOwnedGroup(pos)
			|| ((mRegionFlags & ALLOW_RETURN_ENCROACHING_OBJECT)
				&& mParcelOverlay->encroachesOwned(boxes)) );
}

void LLViewerRegion::showReleaseNotes()
{
	std::string url = this->getCapability("ServerReleaseNotes");

	if (url.empty()) {
		// HACK haven't received the capability yet, we'll wait until
		// it arives.
		mReleaseNotesRequested = TRUE;
		return;
	}

	LLWeb::loadURL(url);
	mReleaseNotesRequested = FALSE;
}

std::string LLViewerRegion::getDescription() const
{
    return stringize(*this);
}
