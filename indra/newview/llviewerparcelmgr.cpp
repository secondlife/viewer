/** 
 * @file llviewerparcelmgr.cpp
 * @brief Viewer-side representation of owned land
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

#include "llviewerparcelmgr.h"

// Library includes
#include "llaudioengine.h"
#include "indra_constants.h"
#include "llcachename.h"
#include "llgl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llsecondlifeurls.h"
#include "message.h"
#include "llfloaterreg.h"

// Viewer includes
#include "llagent.h"
#include "llagentaccess.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
//#include "llfirstuse.h"
#include "llfloaterbuyland.h"
#include "llfloatergroups.h"
#include "llpanelnearbymedia.h"
#include "llfloatersellland.h"
#include "llfloatertools.h"
#include "llparcelselection.h"
#include "llresmgr.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llslurl.h"
#include "llstatusbar.h"
#include "llui.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerparcelmedia.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "roles_constants.h"
#include "llweb.h"
#include "llvieweraudio.h"

const F32 PARCEL_COLLISION_DRAW_SECS = 1.f;


// Globals

U8* LLViewerParcelMgr::sPackedOverlay = NULL;

LLUUID gCurrentMovieID = LLUUID::null;

LLPointer<LLViewerTexture> sBlockedImage;
LLPointer<LLViewerTexture> sPassImage;

// Local functions
void callback_start_music(S32 option, void* data);
void optionally_prepare_video(const LLParcel *parcelp);
void callback_prepare_video(S32 option, void* data);
void prepare_video(const LLParcel *parcelp);
void start_video(const LLParcel *parcelp);
void stop_video();
bool callback_god_force_owner(const LLSD&, const LLSD&);

struct LLGodForceOwnerData
{
	LLUUID mOwnerID;
	S32 mLocalID;
	LLHost mHost;

	LLGodForceOwnerData(
		const LLUUID& owner_id,
		S32 local_parcel_id,
		const LLHost& host) :
		mOwnerID(owner_id),
		mLocalID(local_parcel_id),
		mHost(host) {}
};

//
// Methods
//
LLViewerParcelMgr::LLViewerParcelMgr()
:	mSelected(FALSE),
	mRequestResult(0),
	mWestSouth(),
	mEastNorth(),
	mSelectedDwell(DWELL_NAN),
	mAgentParcelSequenceID(-1),
	mHoverRequestResult(0),
	mHoverWestSouth(),
	mHoverEastNorth(),
	mRenderCollision(FALSE),
	mRenderSelection(TRUE),
	mCollisionBanned(0),
	mCollisionTimer(),
	mMediaParcelId(0),
	mMediaRegionId(0)
{
	mCurrentParcel = new LLParcel();
	mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);
	mFloatingParcelSelection = new LLParcelSelection(mCurrentParcel);

	mAgentParcel = new LLParcel();
	mHoverParcel = new LLParcel();
	mCollisionParcel = new LLParcel();

	mParcelsPerEdge = S32(	REGION_WIDTH_METERS / PARCEL_GRID_STEP_METERS );
	mHighlightSegments = new U8[(mParcelsPerEdge+1)*(mParcelsPerEdge+1)];
	resetSegments(mHighlightSegments);

	mCollisionSegments = new U8[(mParcelsPerEdge+1)*(mParcelsPerEdge+1)];
	resetSegments(mCollisionSegments);

	// JC: Resolved a merge conflict here, eliminated
	// mBlockedImage->setAddressMode(LLTexUnit::TAM_WRAP);
	// because it is done in llviewertexturelist.cpp
	mBlockedImage = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryLines.png");
	mPassImage = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryPassLines.png");

	S32 overlay_size = mParcelsPerEdge * mParcelsPerEdge / PARCEL_OVERLAY_CHUNKS;
	sPackedOverlay = new U8[overlay_size];

	mAgentParcelOverlay = new U8[mParcelsPerEdge * mParcelsPerEdge];
	S32 i;
	for (i = 0; i < mParcelsPerEdge * mParcelsPerEdge; i++)
	{
		mAgentParcelOverlay[i] = 0;
	}

	mTeleportInProgress = TRUE; // the initial parcel update is treated like teleport
}


LLViewerParcelMgr::~LLViewerParcelMgr()
{
	mCurrentParcelSelection->setParcel(NULL);
	mCurrentParcelSelection = NULL;

	mFloatingParcelSelection->setParcel(NULL);
	mFloatingParcelSelection = NULL;

	delete mCurrentParcel;
	mCurrentParcel = NULL;

	delete mAgentParcel;
	mAgentParcel = NULL;

	delete mCollisionParcel;
	mCollisionParcel = NULL;

	delete mHoverParcel;
	mHoverParcel = NULL;

	delete[] mHighlightSegments;
	mHighlightSegments = NULL;

	delete[] mCollisionSegments;
	mCollisionSegments = NULL;

	delete[] sPackedOverlay;
	sPackedOverlay = NULL;

	delete[] mAgentParcelOverlay;
	mAgentParcelOverlay = NULL;

	sBlockedImage = NULL;
	sPassImage = NULL;
}

void LLViewerParcelMgr::dump()
{
	llinfos << "Parcel Manager Dump" << llendl;
	llinfos << "mSelected " << S32(mSelected) << llendl;
	llinfos << "Selected parcel: " << llendl;
	llinfos << mWestSouth << " to " << mEastNorth << llendl;
	mCurrentParcel->dump();
	llinfos << "banning " << mCurrentParcel->mBanList.size() << llendl;
	
	access_map_const_iterator cit = mCurrentParcel->mBanList.begin();
	access_map_const_iterator end = mCurrentParcel->mBanList.end();
	for ( ; cit != end; ++cit)
	{
		llinfos << "ban id " << (*cit).first << llendl;
	}
	llinfos << "Hover parcel:" << llendl;
	mHoverParcel->dump();
	llinfos << "Agent parcel:" << llendl;
	mAgentParcel->dump();
}


LLViewerRegion* LLViewerParcelMgr::getSelectionRegion()
{
	return LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
}


void LLViewerParcelMgr::getDisplayInfo(S32* area_out, S32* claim_out,
									   S32* rent_out,
									   BOOL* for_sale_out,
									   F32* dwell_out)
{
	S32 area = 0;
	S32 price = 0;
	S32 rent = 0;
	BOOL for_sale = FALSE;
	F32 dwell = DWELL_NAN;

	if (mSelected)
	{
		if (mCurrentParcelSelection->mSelectedMultipleOwners)
		{
			area = mCurrentParcelSelection->getClaimableArea();
		}
		else
		{
			area = getSelectedArea();
		}

		if (mCurrentParcel->getForSale())
		{
			price = mCurrentParcel->getSalePrice();
			for_sale = TRUE;
		}
		else
		{
			price = area * mCurrentParcel->getClaimPricePerMeter();
			for_sale = FALSE;
		}

		rent = mCurrentParcel->getTotalRent();

		dwell = mSelectedDwell;
	}

	*area_out = area;
	*claim_out = price;
	*rent_out = rent;
	*for_sale_out = for_sale;
	*dwell_out = dwell;
}

S32 LLViewerParcelMgr::getSelectedArea() const
{
	S32 rv = 0;
	if(mSelected && mCurrentParcel && mCurrentParcelSelection->mWholeParcelSelected)
	{
		rv = mCurrentParcel->getArea();
	}
	else if(mSelected)
	{
		F64 width = mEastNorth.mdV[VX] - mWestSouth.mdV[VX];
		F64 height = mEastNorth.mdV[VY] - mWestSouth.mdV[VY];
		F32 area = (F32)(width * height);
		rv = llround(area);
	}
	return rv;
}

void LLViewerParcelMgr::resetSegments(U8* segments)
{
	S32 i;
	S32 count = (mParcelsPerEdge+1)*(mParcelsPerEdge+1);
	for (i = 0; i < count; i++)
	{
		segments[i] = 0x0;
	}
}


void LLViewerParcelMgr::writeHighlightSegments(F32 west, F32 south, F32 east,
											   F32 north)
{
	S32 x, y;
	S32 min_x = llround( west / PARCEL_GRID_STEP_METERS );
	S32 max_x = llround( east / PARCEL_GRID_STEP_METERS );
	S32 min_y = llround( south / PARCEL_GRID_STEP_METERS );
	S32 max_y = llround( north / PARCEL_GRID_STEP_METERS );

	const S32 STRIDE = mParcelsPerEdge+1;

	// south edge
	y = min_y;
	for (x = min_x; x < max_x; x++)
	{
		// exclusive OR means that writing to this segment twice
		// will turn it off
		mHighlightSegments[x + y*STRIDE] ^= SOUTH_MASK;
	}

	// west edge
	x = min_x;
	for (y = min_y; y < max_y; y++)
	{
		mHighlightSegments[x + y*STRIDE] ^= WEST_MASK;
	}

	// north edge - draw the south border on the y+1'th cell,
	// which given C-style arrays, is item foo[max_y]
	y = max_y;
	for (x = min_x; x < max_x; x++)
	{
		mHighlightSegments[x + y*STRIDE] ^= SOUTH_MASK;
	}

	// east edge - draw west border on x+1'th cell
	x = max_x;
	for (y = min_y; y < max_y; y++)
	{
		mHighlightSegments[x + y*STRIDE] ^= WEST_MASK;
	}
}


void LLViewerParcelMgr::writeSegmentsFromBitmap(U8* bitmap, U8* segments)
{
	S32 x;
	S32 y;
	const S32 IN_STRIDE = mParcelsPerEdge;
	const S32 OUT_STRIDE = mParcelsPerEdge+1;

	for (y = 0; y < IN_STRIDE; y++)
	{
		x = 0;
		while( x < IN_STRIDE )
		{
			U8 byte = bitmap[ (x + y*IN_STRIDE) / 8 ];

			S32 bit;
			for (bit = 0; bit < 8; bit++)
			{
				if (byte & (1 << bit) )
				{
					S32 out = x+y*OUT_STRIDE;

					// This and one above it
					segments[out]            ^= SOUTH_MASK;
					segments[out+OUT_STRIDE] ^= SOUTH_MASK;

					// This and one to the right
					segments[out]   ^= WEST_MASK;
					segments[out+1] ^= WEST_MASK;
				}
				x++;
			}
		}
	}
}


void LLViewerParcelMgr::writeAgentParcelFromBitmap(U8* bitmap)
{
	S32 x;
	S32 y;
	const S32 IN_STRIDE = mParcelsPerEdge;

	for (y = 0; y < IN_STRIDE; y++)
	{
		x = 0;
		while( x < IN_STRIDE )
		{
			U8 byte = bitmap[ (x + y*IN_STRIDE) / 8 ];

			S32 bit;
			for (bit = 0; bit < 8; bit++)
			{
				if (byte & (1 << bit) )
				{
					mAgentParcelOverlay[x+y*IN_STRIDE] = 1;
				}
				else
				{
					mAgentParcelOverlay[x+y*IN_STRIDE] = 0;
				}
				x++;
			}
		}
	}
}


// Given a point, find the PARCEL_GRID_STEP x PARCEL_GRID_STEP block
// containing it and select that.
LLParcelSelectionHandle LLViewerParcelMgr::selectParcelAt(const LLVector3d& pos_global)
{
	LLVector3d southwest = pos_global;
	LLVector3d northeast = pos_global;

	southwest -= LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );
	southwest.mdV[VX] = llround( southwest.mdV[VX], (F64)PARCEL_GRID_STEP_METERS );
	southwest.mdV[VY] = llround( southwest.mdV[VY], (F64)PARCEL_GRID_STEP_METERS );

	northeast += LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );
	northeast.mdV[VX] = llround( northeast.mdV[VX], (F64)PARCEL_GRID_STEP_METERS );
	northeast.mdV[VY] = llround( northeast.mdV[VY], (F64)PARCEL_GRID_STEP_METERS );

	// Snap to parcel
	return selectLand( southwest, northeast, TRUE );
}


// Tries to select the parcel inside the rectangle
LLParcelSelectionHandle LLViewerParcelMgr::selectParcelInRectangle()
{
	return selectLand(mWestSouth, mEastNorth, TRUE);
}


void LLViewerParcelMgr::selectCollisionParcel()
{
	// BUG: Claim to be in the agent's region
	mWestSouth = gAgent.getRegion()->getOriginGlobal();
	mEastNorth = mWestSouth;
	mEastNorth += LLVector3d(PARCEL_GRID_STEP_METERS, PARCEL_GRID_STEP_METERS, 0.0);

	// BUG: must be in the sim you are in
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelPropertiesRequestByID);
	msg->nextBlockFast(_PREHASH_AgentID);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_SequenceID, SELECTED_PARCEL_SEQ_ID );
	msg->addS32Fast(_PREHASH_LocalID, mCollisionParcel->getLocalID() );
	gAgent.sendReliableMessage();

	mRequestResult = PARCEL_RESULT_NO_DATA;

	// Hack: Copy some data over temporarily
	mCurrentParcel->setName( mCollisionParcel->getName() );
	mCurrentParcel->setDesc( mCollisionParcel->getDesc() );
	mCurrentParcel->setPassPrice(mCollisionParcel->getPassPrice());
	mCurrentParcel->setPassHours(mCollisionParcel->getPassHours());

	// clear the list of segments to prevent flashing
	resetSegments(mHighlightSegments);

	mFloatingParcelSelection->setParcel(mCurrentParcel);
	mCurrentParcelSelection->setParcel(NULL);
	mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);

	mSelected = TRUE;
	mCurrentParcelSelection->mWholeParcelSelected = TRUE;
	notifyObservers();
	return;
}


// snap_selection = auto-select the hit parcel, if there is exactly one
LLParcelSelectionHandle LLViewerParcelMgr::selectLand(const LLVector3d &corner1, const LLVector3d &corner2,
								   BOOL snap_selection)
{
	sanitize_corners( corner1, corner2, mWestSouth, mEastNorth );

	// ...x isn't more than one meter away
	F32 delta_x = getSelectionWidth();
	if (delta_x * delta_x <= 1.f * 1.f)
	{
		mSelected = FALSE;
		notifyObservers();
		return NULL;
	}

	// ...y isn't more than one meter away
	F32 delta_y = getSelectionHeight();
	if (delta_y * delta_y <= 1.f * 1.f)
	{
		mSelected = FALSE;
		notifyObservers();
		return NULL;
	}

	// Can't select across region boundary
	// We need to pull in the upper right corner by a little bit to allow
	// selection up to the x = 256 or y = 256 edge.
	LLVector3d east_north_region_check( mEastNorth );
	east_north_region_check.mdV[VX] -= 0.5;
	east_north_region_check.mdV[VY] -= 0.5;

	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal(mWestSouth);
	LLViewerRegion *region_other = LLWorld::getInstance()->getRegionFromPosGlobal( east_north_region_check );

	if(!region)
	{
		// just in case they somehow selected no land.
		mSelected = FALSE;
		return NULL;
	}

	if (region != region_other)
	{
		LLNotificationsUtil::add("CantSelectLandFromMultipleRegions");
		mSelected = FALSE;
		notifyObservers();
		return NULL;
	}

	// Build region global copies of corners
	LLVector3 wsb_region = region->getPosRegionFromGlobal( mWestSouth );
	LLVector3 ent_region = region->getPosRegionFromGlobal( mEastNorth );

	// Send request message
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelPropertiesRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_SequenceID, SELECTED_PARCEL_SEQ_ID );
	msg->addF32Fast(_PREHASH_West,  wsb_region.mV[VX] );
	msg->addF32Fast(_PREHASH_South, wsb_region.mV[VY] );
	msg->addF32Fast(_PREHASH_East,  ent_region.mV[VX] );
	msg->addF32Fast(_PREHASH_North, ent_region.mV[VY] );
	msg->addBOOL("SnapSelection", snap_selection);
	msg->sendReliable( region->getHost() );

	mRequestResult = PARCEL_RESULT_NO_DATA;

	mFloatingParcelSelection->setParcel(mCurrentParcel);
	mCurrentParcelSelection->setParcel(NULL);
	mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);

	mSelected = TRUE;
	mCurrentParcelSelection->mWholeParcelSelected = snap_selection;
	notifyObservers();
	return mCurrentParcelSelection;
}

void LLViewerParcelMgr::deselectUnused()
{
	// no more outstanding references to this selection, other than our own
	if (mCurrentParcelSelection->getNumRefs() == 1 && mFloatingParcelSelection->getNumRefs() == 1)
	{
		deselectLand();
	}
}

void LLViewerParcelMgr::deselectLand()
{
	if (mSelected)
	{
		mSelected = FALSE;

		// Invalidate the selected parcel
		mCurrentParcel->setLocalID(-1);
		mCurrentParcel->mAccessList.clear();
		mCurrentParcel->mBanList.clear();
		//mCurrentParcel->mRenterList.reset();

		mSelectedDwell = DWELL_NAN;

		// invalidate parcel selection so that existing users of this selection can clean up
		mCurrentParcelSelection->setParcel(NULL);
		mFloatingParcelSelection->setParcel(NULL);
		// create new parcel selection
		mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);

		notifyObservers(); // Notify observers *after* changing the parcel selection
	}
}


void LLViewerParcelMgr::addObserver(LLParcelObserver* observer)
{
	mObservers.put(observer);
}


void LLViewerParcelMgr::removeObserver(LLParcelObserver* observer)
{
	mObservers.removeObj(observer);
}


// Call this method when it's time to update everyone on a new state.
// Copy the list because an observer could respond by removing itself
// from the list.
void LLViewerParcelMgr::notifyObservers()
{
	LLDynamicArray<LLParcelObserver*> observers;
	S32 count = mObservers.count();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		observers.put(mObservers.get(i));
	}
	for(i = 0; i < count; ++i)
	{
		observers.get(i)->changed();
	}
}


//
// ACCESSORS
//
BOOL LLViewerParcelMgr::selectionEmpty() const
{
	return !mSelected;
}


LLParcelSelectionHandle LLViewerParcelMgr::getParcelSelection() const
{
	return mCurrentParcelSelection;
}

LLParcelSelectionHandle LLViewerParcelMgr::getFloatingParcelSelection() const
{
	return mFloatingParcelSelection;
}

LLParcel *LLViewerParcelMgr::getAgentParcel() const
{
	return mAgentParcel;
}

// Return whether the agent can build on the land they are on
bool LLViewerParcelMgr::allowAgentBuild() const
{
	if (mAgentParcel)
	{
		return (gAgent.isGodlike() ||
				(mAgentParcel->allowModifyBy(gAgent.getID(), gAgent.getGroupID())) ||
				(isParcelOwnedByAgent(mAgentParcel, GP_LAND_ALLOW_CREATE)));
	}
	else
	{
		return gAgent.isGodlike();
	}
}

// Return whether anyone can build on the given parcel
bool LLViewerParcelMgr::allowAgentBuild(const LLParcel* parcel) const
{
	return parcel->getAllowModify();
}

bool LLViewerParcelMgr::allowAgentVoice() const
{
	return allowAgentVoice(gAgent.getRegion(), mAgentParcel);
}

bool LLViewerParcelMgr::allowAgentVoice(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return region && region->isVoiceEnabled()
		&& parcel	&& parcel->getParcelFlagAllowVoice();
}

bool LLViewerParcelMgr::allowAgentFly(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return region && !region->getBlockFly()
		&& parcel && parcel->getAllowFly();
}

// Can the agent be pushed around by LLPushObject?
bool LLViewerParcelMgr::allowAgentPush(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return region && !region->getRestrictPushObject()
		&& parcel && !parcel->getRestrictPushObject();
}

bool LLViewerParcelMgr::allowAgentScripts(const LLViewerRegion* region, const LLParcel* parcel) const
{
	// *NOTE: This code does not take into account group-owned parcels
	// and the flag to allow group-owned scripted objects to run.
	// This mirrors the traditional menu bar parcel icon code, but is not
	// technically correct.
	return region
		&& !region->getRegionFlag(REGION_FLAGS_SKIP_SCRIPTS)
		&& !region->getRegionFlag(REGION_FLAGS_ESTATE_SKIP_SCRIPTS)
		&& parcel
		&& parcel->getAllowOtherScripts();
}

bool LLViewerParcelMgr::allowAgentDamage(const LLViewerRegion* region, const LLParcel* parcel) const
{
	return (region && region->getAllowDamage())
		|| (parcel && parcel->getAllowDamage());
}

BOOL LLViewerParcelMgr::isOwnedAt(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;

	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;

	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );

	return overlay->isOwned( pos_region );
}

BOOL LLViewerParcelMgr::isOwnedSelfAt(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;

	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;

	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );

	return overlay->isOwnedSelf( pos_region );
}

BOOL LLViewerParcelMgr::isOwnedOtherAt(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;

	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;

	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );

	return overlay->isOwnedOther( pos_region );
}

BOOL LLViewerParcelMgr::isSoundLocal(const LLVector3d& pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;

	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;

	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );

	return overlay->isSoundLocal( pos_region );
}

BOOL LLViewerParcelMgr::canHearSound(const LLVector3d &pos_global) const
{
	BOOL in_agent_parcel = inAgentParcel(pos_global);

	if (in_agent_parcel)
	{
		// In same parcel as the agent
		return TRUE;
	}
	else
	{
		if (LLViewerParcelMgr::getInstance()->getAgentParcel()->getSoundLocal())
		{
			// Not in same parcel, and agent parcel only has local sound
			return FALSE;
		}
		else if (LLViewerParcelMgr::getInstance()->isSoundLocal(pos_global))
		{
			// Not in same parcel, and target parcel only has local sound
			return FALSE;
		}
		else
		{
			// Not in same parcel, but neither are local sound
			return TRUE;
		}
	}
}


BOOL LLViewerParcelMgr::inAgentParcel(const LLVector3d &pos_global) const
{
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(pos_global);
	LLViewerRegion* agent_region = gAgent.getRegion();
	if (!region || !agent_region)
		return FALSE;

	if (region != agent_region)
	{
		// Can't be in the agent parcel if you're not in the same region.
		return FALSE;
	}

	LLVector3 pos_region = agent_region->getPosRegionFromGlobal(pos_global);
	S32 row =    S32(pos_region.mV[VY] / PARCEL_GRID_STEP_METERS);
	S32 column = S32(pos_region.mV[VX] / PARCEL_GRID_STEP_METERS);

	if (mAgentParcelOverlay[row*mParcelsPerEdge + column])
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// Returns NULL when there is no valid data.
LLParcel* LLViewerParcelMgr::getHoverParcel() const
{
	if (mHoverRequestResult == PARCEL_RESULT_SUCCESS)
	{
		return mHoverParcel;
	}
	else
	{
		return NULL;
	}
}

// Returns NULL when there is no valid data.
LLParcel* LLViewerParcelMgr::getCollisionParcel() const
{
	if (mRenderCollision)
	{
		return mCollisionParcel;
	}
	else
	{
		return NULL;
	}
}

//
// UTILITIES
//

void LLViewerParcelMgr::render()
{
	if (mSelected && mRenderSelection && gSavedSettings.getBOOL("RenderParcelSelection"))
	{
		// Rendering is done in agent-coordinates, so need to supply
		// an appropriate offset to the render code.
		LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosGlobal(mWestSouth);
		if (!regionp) return;

		renderHighlightSegments(mHighlightSegments, regionp);
	}
}


void LLViewerParcelMgr::renderParcelCollision()
{
	// check for expiration
	if (mCollisionTimer.getElapsedTimeF32() > PARCEL_COLLISION_DRAW_SECS)
	{
		mRenderCollision = FALSE;
	}

	if (mRenderCollision && gSavedSettings.getBOOL("ShowBanLines"))
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		if (regionp)
		{
			BOOL use_pass = mCollisionParcel->getParcelFlag(PF_USE_PASS_LIST);
			renderCollisionSegments(mCollisionSegments, use_pass, regionp);
		}
	}
}


void LLViewerParcelMgr::sendParcelAccessListRequest(U32 flags)
{
	if (!mSelected)
	{
		return;
	}

	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;

	LLMessageSystem *msg = gMessageSystem;
	

	if (flags & AL_BAN) 
	{
		mCurrentParcel->mBanList.clear();
	}
	if (flags & AL_ACCESS) 
	{
		mCurrentParcel->mAccessList.clear();
	}		

	// Only the headers differ
	msg->newMessageFast(_PREHASH_ParcelAccessListRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addS32Fast(_PREHASH_SequenceID, 0);
	msg->addU32Fast(_PREHASH_Flags, flags);
	msg->addS32("LocalID", mCurrentParcel->getLocalID() );
	msg->sendReliable( region->getHost() );
}


void LLViewerParcelMgr::sendParcelDwellRequest()
{
	if (!mSelected)
	{
		return;
	}

	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;

	LLMessageSystem *msg = gMessageSystem;

	// Only the headers differ
	msg->newMessage("ParcelDwellRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addS32("LocalID", mCurrentParcel->getLocalID());
	msg->addUUID("ParcelID", LLUUID::null);	// filled in on simulator
	msg->sendReliable( region->getHost() );
}


void LLViewerParcelMgr::sendParcelGodForceOwner(const LLUUID& owner_id)
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotSetLandOwnerNothingSelected");
		return;
	}

	llinfos << "Claiming " << mWestSouth << " to " << mEastNorth << llendl;

	// BUG: Only works for the region containing mWestSouthBottom
	LLVector3d east_north_region_check( mEastNorth );
	east_north_region_check.mdV[VX] -= 0.5;
	east_north_region_check.mdV[VY] -= 0.5;

	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		// TODO: Add a force owner version of this alert.
		LLNotificationsUtil::add("CannotContentifyNoRegion");
		return;
	}

	// BUG: Make work for cross-region selections
	LLViewerRegion *region2 = LLWorld::getInstance()->getRegionFromPosGlobal( east_north_region_check );
	if (region != region2)
	{
		LLNotificationsUtil::add("CannotSetLandOwnerMultipleRegions");
		return;
	}

	llinfos << "Region " << region->getOriginGlobal() << llendl;

	LLSD payload;
	payload["owner_id"] = owner_id;
	payload["parcel_local_id"] = mCurrentParcel->getLocalID();
	payload["region_host"] = region->getHost().getIPandPort();
	LLNotification::Params params("ForceOwnerAuctionWarning");
	params.payload(payload).functor.function(callback_god_force_owner);

	if(mCurrentParcel->getAuctionID())
	{
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
}

bool callback_god_force_owner(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelGodForceOwner");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("OwnerID", notification["payload"]["owner_id"].asUUID());
		msg->addS32( "LocalID", notification["payload"]["parcel_local_id"].asInteger());
		msg->sendReliable(LLHost(notification["payload"]["region_host"].asString()));
	}
	return false;
}

void LLViewerParcelMgr::sendParcelGodForceToContent()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotContentifyNothingSelected");
		return;
	}
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotContentifyNoRegion");
		return;
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("ParcelGodMarkAsContent");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("ParcelData");
	msg->addS32("LocalID", mCurrentParcel->getLocalID());
	msg->sendReliable(region->getHost());
}

void LLViewerParcelMgr::sendParcelRelease()
{
	if (!mSelected)
	{
        LLNotificationsUtil::add("CannotReleaseLandNothingSelected");
		return;
	}

	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotReleaseLandNoRegion");
		return;
	}

	//U32 flags = PR_NONE;
	//if (god_force) flags |= PR_GOD_FORCE;

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("ParcelRelease");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID() );
	msg->nextBlock("Data");
	msg->addS32("LocalID", mCurrentParcel->getLocalID() );
	//msg->addU32("Flags", flags);
	msg->sendReliable( region->getHost() );

	// Blitz selection, since the parcel might be non-rectangular, and
	// we won't have appropriate parcel information.
	deselectLand();
}

class LLViewerParcelMgr::ParcelBuyInfo
{
public:
	LLUUID	mAgent;
	LLUUID	mSession;
	LLUUID	mGroup;
	BOOL	mIsGroupOwned;
	BOOL	mRemoveContribution;
	BOOL	mIsClaim;
	LLHost	mHost;
	
	// for parcel buys
	S32		mParcelID;
	S32		mPrice;
	S32		mArea;

	// for land claims
	F32		mWest;
	F32		mSouth;
	F32		mEast;
	F32		mNorth;
};

LLViewerParcelMgr::ParcelBuyInfo* LLViewerParcelMgr::setupParcelBuy(
	const LLUUID& agent_id,
	const LLUUID& session_id,
	const LLUUID& group_id,
	BOOL is_group_owned,
	BOOL is_claim,
	BOOL remove_contribution)
{
	if (!mSelected || !mCurrentParcel)
	{
		LLNotificationsUtil::add("CannotBuyLandNothingSelected");
		return NULL;
	}

	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotBuyLandNoRegion");
		return NULL;
	}
	
	if (is_claim)
	{
		llinfos << "Claiming " << mWestSouth << " to " << mEastNorth << llendl;
		llinfos << "Region " << region->getOriginGlobal() << llendl;

		// BUG: Only works for the region containing mWestSouthBottom
		LLVector3d east_north_region_check( mEastNorth );
		east_north_region_check.mdV[VX] -= 0.5;
		east_north_region_check.mdV[VY] -= 0.5;

		LLViewerRegion *region2 = LLWorld::getInstance()->getRegionFromPosGlobal( east_north_region_check );

		if (region != region2)
		{
			LLNotificationsUtil::add("CantBuyLandAcrossMultipleRegions");
			return NULL;
		}
	}
	
	
	ParcelBuyInfo* info = new ParcelBuyInfo;
	
	info->mAgent = agent_id;
	info->mSession = session_id;
	info->mGroup = group_id;
	info->mIsGroupOwned = is_group_owned;
	info->mIsClaim = is_claim;
	info->mRemoveContribution = remove_contribution;
	info->mHost = region->getHost();
	info->mPrice = mCurrentParcel->getSalePrice();
	info->mArea = mCurrentParcel->getArea();
	
	if (!is_claim)
	{
		info->mParcelID = mCurrentParcel->getLocalID();
	}
	else
	{
		// BUG: Make work for cross-region selections
		LLVector3 west_south_bottom_region = region->getPosRegionFromGlobal( mWestSouth );
		LLVector3 east_north_top_region = region->getPosRegionFromGlobal( mEastNorth );
		
		info->mWest		= west_south_bottom_region.mV[VX];
		info->mSouth	= west_south_bottom_region.mV[VY];
		info->mEast		= east_north_top_region.mV[VX];
		info->mNorth	= east_north_top_region.mV[VY];
	}
	
	return info;
}

void LLViewerParcelMgr::sendParcelBuy(ParcelBuyInfo* info)
{
	// send the message
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage(info->mIsClaim ? "ParcelClaim" : "ParcelBuy");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", info->mAgent);
	msg->addUUID("SessionID", info->mSession);
	msg->nextBlock("Data");
	msg->addUUID("GroupID", info->mGroup);
	msg->addBOOL("IsGroupOwned", info->mIsGroupOwned);
	if (!info->mIsClaim)
	{
		msg->addBOOL("RemoveContribution", info->mRemoveContribution);
		msg->addS32("LocalID", info->mParcelID);
	}
	msg->addBOOL("Final", TRUE);	// don't allow escrow buys
	if (info->mIsClaim)
	{
		msg->nextBlock("ParcelData");
		msg->addF32("West",  info->mWest);
		msg->addF32("South", info->mSouth);
		msg->addF32("East",  info->mEast);
		msg->addF32("North", info->mNorth);
	}
	else // ParcelBuy
	{
		msg->nextBlock("ParcelData");
		msg->addS32("Price",info->mPrice);
		msg->addS32("Area",info->mArea);
	}
	msg->sendReliable(info->mHost);
}

void LLViewerParcelMgr::deleteParcelBuy(ParcelBuyInfo* *info)
{
	// Must be here because ParcelBuyInfo is local to this .cpp file
	delete *info;
	*info = NULL;
}

void LLViewerParcelMgr::sendParcelDeed(const LLUUID& group_id)
{
	if (!mSelected || !mCurrentParcel)
	{
		LLNotificationsUtil::add("CannotDeedLandNothingSelected");
		return;
	}
	if(group_id.isNull())
	{
		LLNotificationsUtil::add("CannotDeedLandNoGroup");
		return;
	}
	LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		LLNotificationsUtil::add("CannotDeedLandNoRegion");
		return;
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("ParcelDeedToGroup");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID() );
	msg->nextBlock("Data");
	msg->addUUID("GroupID", group_id );
	msg->addS32("LocalID", mCurrentParcel->getLocalID() );
	//msg->addU32("JoinNeighbors", join);
	msg->sendReliable( region->getHost() );
}


/*
// *NOTE: We cannot easily make landmarks at global positions because
// global positions probably refer to a sim/local combination which
// can move over time. We could implement this by looking up the
// region global x,y, but it's easier to take it out for now.
void LLViewerParcelMgr::makeLandmarkAtSelection()
{
	// Don't create for parcels you don't own
	if (gAgent.getID() != mCurrentParcel->getOwnerID())
	{
		return;
	}

	LLVector3d global_center(mWestSouth);
	global_center += mEastNorth;
	global_center *= 0.5f;

	LLViewerRegion* region;
	region = LLWorld::getInstance()->getRegionFromPosGlobal(global_center);

	LLVector3 west_south_bottom_region = region->getPosRegionFromGlobal( mWestSouth );
	LLVector3 east_north_top_region = region->getPosRegionFromGlobal( mEastNorth );

	std::string name("My Land");
	std::string buffer;
	S32 pos_x = (S32)floor((west_south_bottom_region.mV[VX] + east_north_top_region.mV[VX]) / 2.0f);
	S32 pos_y = (S32)floor((west_south_bottom_region.mV[VY] + east_north_top_region.mV[VY]) / 2.0f);
	buffer = llformat("%s in %s (%d, %d)",
			name.c_str(),
			region->getName().c_str(),
			pos_x, pos_y);
	name.assign(buffer);

	create_landmark(name, "Claimed land", global_center);
}
*/

const std::string& LLViewerParcelMgr::getAgentParcelName() const
{
	return mAgentParcel->getName();
}


void LLViewerParcelMgr::sendParcelPropertiesUpdate(LLParcel* parcel, bool use_agent_region)
{
	if(!parcel) return;

	LLViewerRegion *region = use_agent_region ? gAgent.getRegion() : LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;
	//llinfos << "found region: " << region->getName() << llendl;

	LLSD body;
	std::string url = region->getCapability("ParcelPropertiesUpdate");
	if (!url.empty())
	{
		// request new properties update from simulator
		U32 message_flags = 0x01;
		body["flags"] = ll_sd_from_U32(message_flags);
		parcel->packMessage(body);
		llinfos << "Sending parcel properties update via capability to: "
			<< url << llendl;
		LLHTTPClient::post(url, body, new LLHTTPClient::Responder());
	}
	else
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ParcelPropertiesUpdate);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ParcelData);
		msg->addS32Fast(_PREHASH_LocalID, parcel->getLocalID() );

		U32 message_flags = 0x01;
		msg->addU32("Flags", message_flags);

		parcel->packMessage(msg);

		msg->sendReliable( region->getHost() );
	}
}


void LLViewerParcelMgr::setHoverParcel(const LLVector3d& pos)
{
	static U32 last_west, last_south;


	// only request parcel info when tooltip is shown
	if (!gSavedSettings.getBOOL("ShowLandHoverTip"))
	{
		return;
	}

	// only request parcel info if position has changed outside of the
	// last parcel grid step
	U32 west_parcel_step = (U32) floor( pos.mdV[VX] / PARCEL_GRID_STEP_METERS );
	U32 south_parcel_step = (U32) floor( pos.mdV[VY] / PARCEL_GRID_STEP_METERS );
	
	if ((west_parcel_step == last_west) && (south_parcel_step == last_south))
	{
		return;
	}
	else 
	{
		last_west = west_parcel_step;
		last_south = south_parcel_step;
	}

	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( pos );
	if (!region)
	{
		return;
	}


	// Send a rectangle around the point.
	// This means the parcel sent back is at least a rectangle around the point,
	// which is more efficient for public land.  Fewer requests are sent.  JC
	LLVector3 wsb_region = region->getPosRegionFromGlobal( pos );

	F32 west  = PARCEL_GRID_STEP_METERS * floor( wsb_region.mV[VX] / PARCEL_GRID_STEP_METERS );
	F32 south = PARCEL_GRID_STEP_METERS * floor( wsb_region.mV[VY] / PARCEL_GRID_STEP_METERS );

	F32 east  = west  + PARCEL_GRID_STEP_METERS;
	F32 north = south + PARCEL_GRID_STEP_METERS;

	// Send request message
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelPropertiesRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_SequenceID,	HOVERED_PARCEL_SEQ_ID );
	msg->addF32Fast(_PREHASH_West,			west );
	msg->addF32Fast(_PREHASH_South,			south );
	msg->addF32Fast(_PREHASH_East,			east );
	msg->addF32Fast(_PREHASH_North,			north );
	msg->addBOOL("SnapSelection",			FALSE );
	msg->sendReliable( region->getHost() );

	mHoverRequestResult = PARCEL_RESULT_NO_DATA;
}


// static
void LLViewerParcelMgr::processParcelOverlay(LLMessageSystem *msg, void **user)
{
	// Extract the packed overlay information
	S32 packed_overlay_size = msg->getSizeFast(_PREHASH_ParcelData, _PREHASH_Data);

	if (packed_overlay_size <= 0)
	{
		llwarns << "Overlay size " << packed_overlay_size << llendl;
		return;
	}

	S32 parcels_per_edge = LLViewerParcelMgr::getInstance()->mParcelsPerEdge;
	S32 expected_size = parcels_per_edge * parcels_per_edge / PARCEL_OVERLAY_CHUNKS;
	if (packed_overlay_size != expected_size)
	{
		llwarns << "Got parcel overlay size " << packed_overlay_size
			<< " expecting " << expected_size << llendl;
		return;
	}

	S32 sequence_id;
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SequenceID, sequence_id);
	msg->getBinaryDataFast(
			_PREHASH_ParcelData,
			_PREHASH_Data,
			sPackedOverlay,
			expected_size);

	LLHost host = msg->getSender();
	LLViewerRegion *region = LLWorld::getInstance()->getRegion(host);
	if (region)
	{
		region->mParcelOverlay->uncompressLandOverlay( sequence_id, sPackedOverlay );
	}
}

// static
void LLViewerParcelMgr::processParcelProperties(LLMessageSystem *msg, void **user)
{
	S32		request_result;
	S32		sequence_id;
	BOOL	snap_selection = FALSE;
	S32		self_count = 0;
	S32		other_count = 0;
	S32		public_count = 0;
	S32		local_id;
	LLUUID	owner_id;
	BOOL	is_group_owned;
	U32 auction_id = 0;
	S32		claim_price_per_meter = 0;
	S32		rent_price_per_meter = 0;
	S32		claim_date = 0;
	LLVector3	aabb_min;
	LLVector3	aabb_max;
	S32		area = 0;
	S32		sw_max_prims = 0;
	S32		sw_total_prims = 0;
	//LLUUID	buyer_id;
	U8 status = 0;
	S32		max_prims = 0;
	S32		total_prims = 0;
	S32		owner_prims = 0;
	S32		group_prims = 0;
	S32		other_prims = 0;
	S32		selected_prims = 0;
	F32		parcel_prim_bonus = 1.f;
	BOOL	region_push_override = false;
	BOOL	region_deny_anonymous_override = false;
	BOOL	region_deny_identified_override = false; // Deprecated
	BOOL	region_deny_transacted_override = false; // Deprecated
	BOOL	region_deny_age_unverified_override = false;

	S32		other_clean_time = 0;

	LLViewerParcelMgr& parcel_mgr = LLViewerParcelMgr::instance();

	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_RequestResult, request_result );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SequenceID, sequence_id );

	if (request_result == PARCEL_RESULT_NO_DATA)
	{
		// no valid parcel data
		llinfos << "no valid parcel data" << llendl;
		return;
	}

	// Decide where the data will go.
	LLParcel* parcel = NULL;
	if (sequence_id == SELECTED_PARCEL_SEQ_ID)
	{
		// ...selected parcels report this sequence id
		parcel_mgr.mRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = parcel_mgr.mCurrentParcel;
	}
	else if (sequence_id == HOVERED_PARCEL_SEQ_ID)
	{
		parcel_mgr.mHoverRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = parcel_mgr.mHoverParcel;
	}
	else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_NOT_ON_LIST_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
	{
		parcel_mgr.mHoverRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = parcel_mgr.mCollisionParcel;
	}
	else if (sequence_id == 0 || sequence_id > parcel_mgr.mAgentParcelSequenceID)
	{
		// new agent parcel
		parcel_mgr.mAgentParcelSequenceID = sequence_id;
		parcel = parcel_mgr.mAgentParcel;
	}
	else
	{
		llinfos << "out of order agent parcel sequence id " << sequence_id
			<< " last good " << parcel_mgr.mAgentParcelSequenceID
			<< llendl;
		return;
	}

	msg->getBOOL("ParcelData", "SnapSelection", snap_selection);
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SelfCount, self_count);
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_OtherCount, other_count);
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_PublicCount, public_count);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_LocalID,		local_id );
	msg->getUUIDFast(_PREHASH_ParcelData, _PREHASH_OwnerID,		owner_id);
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_IsGroupOwned, is_group_owned);
	msg->getU32Fast(_PREHASH_ParcelData, _PREHASH_AuctionID, auction_id);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_ClaimDate,	claim_date);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_ClaimPrice,	claim_price_per_meter);
	msg->getS32Fast( _PREHASH_ParcelData, _PREHASH_RentPrice,	rent_price_per_meter);
	msg->getVector3Fast(_PREHASH_ParcelData, _PREHASH_AABBMin, aabb_min);
	msg->getVector3Fast(_PREHASH_ParcelData, _PREHASH_AABBMax, aabb_max);
	msg->getS32Fast(	_PREHASH_ParcelData, _PREHASH_Area, area );
	//msg->getUUIDFast(	_PREHASH_ParcelData, _PREHASH_BuyerID, buyer_id);
	msg->getU8("ParcelData", "Status", status);
	msg->getS32("ParcelData", "SimWideMaxPrims", sw_max_prims );
	msg->getS32("ParcelData", "SimWideTotalPrims", sw_total_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_MaxPrims, max_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_TotalPrims, total_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_OwnerPrims, owner_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_GroupPrims, group_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_OtherPrims, other_prims );
	msg->getS32Fast(_PREHASH_ParcelData, _PREHASH_SelectedPrims, selected_prims );
	msg->getF32Fast(_PREHASH_ParcelData, _PREHASH_ParcelPrimBonus, parcel_prim_bonus );
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionPushOverride, region_push_override );
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionDenyAnonymous, region_deny_anonymous_override );
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionDenyIdentified, region_deny_identified_override ); // Deprecated
	msg->getBOOLFast(_PREHASH_ParcelData, _PREHASH_RegionDenyTransacted, region_deny_transacted_override ); // Deprecated
	if (msg->getNumberOfBlocksFast(_PREHASH_AgeVerificationBlock))
	{
		// this block was added later and may not be on older sims, so we have to test its existence first
		msg->getBOOLFast(_PREHASH_AgeVerificationBlock, _PREHASH_RegionDenyAgeUnverified, region_deny_age_unverified_override );
	}

	msg->getS32("ParcelData", "OtherCleanTime", other_clean_time );

	// Actually extract the data.
	if (parcel)
	{
		parcel->init(owner_id,
			FALSE, FALSE, FALSE,
			claim_date, claim_price_per_meter, rent_price_per_meter,
			area, other_prims, parcel_prim_bonus, is_group_owned);
		parcel->setLocalID(local_id);
		parcel->setAABBMin(aabb_min);
		parcel->setAABBMax(aabb_max);

		parcel->setAuctionID(auction_id);
		parcel->setOwnershipStatus((LLParcel::EOwnershipStatus)status);

		parcel->setSimWideMaxPrimCapacity(sw_max_prims);
		parcel->setSimWidePrimCount(sw_total_prims);
		parcel->setMaxPrimCapacity(max_prims);
		parcel->setOwnerPrimCount(owner_prims);
		parcel->setGroupPrimCount(group_prims);
		parcel->setOtherPrimCount(other_prims);
		parcel->setSelectedPrimCount(selected_prims);
		parcel->setParcelPrimBonus(parcel_prim_bonus);

		parcel->setCleanOtherTime(other_clean_time);
		parcel->setRegionPushOverride(region_push_override);
		parcel->setRegionDenyAnonymousOverride(region_deny_anonymous_override);
		parcel->setRegionDenyAgeUnverifiedOverride(region_deny_age_unverified_override);
		parcel->unpackMessage(msg);

		if (parcel == parcel_mgr.mAgentParcel)
		{
			S32 bitmap_size =	parcel_mgr.mParcelsPerEdge
								* parcel_mgr.mParcelsPerEdge
								/ 8;
			U8* bitmap = new U8[ bitmap_size ];
			msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);

			parcel_mgr.writeAgentParcelFromBitmap(bitmap);
			delete[] bitmap;

			// Let interesting parties know about agent parcel change.
			LLViewerParcelMgr* instance = LLViewerParcelMgr::getInstance();

			instance->mAgentParcelChangedSignal();

			if (instance->mTeleportInProgress)
			{
				instance->mTeleportInProgress = FALSE;
				instance->mTeleportFinishedSignal(gAgent.getPositionGlobal(), false);
			}
		}
	}

	// Handle updating selections, if necessary.
	if (sequence_id == SELECTED_PARCEL_SEQ_ID)
	{
		// Update selected counts
		parcel_mgr.mCurrentParcelSelection->mSelectedSelfCount = self_count;
		parcel_mgr.mCurrentParcelSelection->mSelectedOtherCount = other_count;
		parcel_mgr.mCurrentParcelSelection->mSelectedPublicCount = public_count;

		parcel_mgr.mCurrentParcelSelection->mSelectedMultipleOwners =
							(request_result == PARCEL_RESULT_MULTIPLE);

		// Select the whole parcel
		LLViewerRegion* region = LLWorld::getInstance()->getRegion( msg->getSender() );
		if (region)
		{
			if (!snap_selection)
			{
				// don't muck with the westsouth and eastnorth.
				// just highlight it
				LLVector3 west_south = region->getPosRegionFromGlobal(parcel_mgr.mWestSouth);
				LLVector3 east_north = region->getPosRegionFromGlobal(parcel_mgr.mEastNorth);

				parcel_mgr.resetSegments(parcel_mgr.mHighlightSegments);
				parcel_mgr.writeHighlightSegments(
								west_south.mV[VX],
								west_south.mV[VY],
								east_north.mV[VX],
								east_north.mV[VY] );
				parcel_mgr.mCurrentParcelSelection->mWholeParcelSelected = FALSE;
			}
			else if (0 == local_id)
			{
				// this is public land, just highlight the selection
				parcel_mgr.mWestSouth = region->getPosGlobalFromRegion( aabb_min );
				parcel_mgr.mEastNorth = region->getPosGlobalFromRegion( aabb_max );

				parcel_mgr.resetSegments(parcel_mgr.mHighlightSegments);
				parcel_mgr.writeHighlightSegments(
								aabb_min.mV[VX],
								aabb_min.mV[VY],
								aabb_max.mV[VX],
								aabb_max.mV[VY] );
				parcel_mgr.mCurrentParcelSelection->mWholeParcelSelected = TRUE;
			}
			else
			{
				parcel_mgr.mWestSouth = region->getPosGlobalFromRegion( aabb_min );
				parcel_mgr.mEastNorth = region->getPosGlobalFromRegion( aabb_max );

				// Owned land, highlight the boundaries
				S32 bitmap_size =	parcel_mgr.mParcelsPerEdge
									* parcel_mgr.mParcelsPerEdge
									/ 8;
				U8* bitmap = new U8[ bitmap_size ];
				msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);

				parcel_mgr.resetSegments(parcel_mgr.mHighlightSegments);
				parcel_mgr.writeSegmentsFromBitmap( bitmap, parcel_mgr.mHighlightSegments );

				delete[] bitmap;
				bitmap = NULL;

				parcel_mgr.mCurrentParcelSelection->mWholeParcelSelected = TRUE;
			}

			// Request access list information for this land
			parcel_mgr.sendParcelAccessListRequest(AL_ACCESS | AL_BAN);

			// Request dwell for this land, if it's not public land.
			parcel_mgr.mSelectedDwell = DWELL_NAN;
			if (0 != local_id)
			{
				parcel_mgr.sendParcelDwellRequest();
			}

			parcel_mgr.mSelected = TRUE;
			parcel_mgr.notifyObservers();
		}
	}
	else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_NOT_ON_LIST_PARCEL_SEQ_ID  ||
			 sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
	{
		// We're about to collide with this parcel
		parcel_mgr.mRenderCollision = TRUE;
		parcel_mgr.mCollisionTimer.reset();

		// Differentiate this parcel if we are banned from it.
		if (sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
		{
			parcel_mgr.mCollisionBanned = BA_BANNED;
		}
		else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID)
		{
			parcel_mgr.mCollisionBanned = BA_NOT_IN_GROUP;
		}
		else 
		{
			parcel_mgr.mCollisionBanned = BA_NOT_ON_LIST;

		}

		S32 bitmap_size =	parcel_mgr.mParcelsPerEdge
							* parcel_mgr.mParcelsPerEdge
							/ 8;
		U8* bitmap = new U8[ bitmap_size ];
		msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);

		parcel_mgr.resetSegments(parcel_mgr.mCollisionSegments);
		parcel_mgr.writeSegmentsFromBitmap( bitmap, parcel_mgr.mCollisionSegments );

		delete[] bitmap;
		bitmap = NULL;

	}
	else if (sequence_id == HOVERED_PARCEL_SEQ_ID)
	{
		LLViewerRegion *region = LLWorld::getInstance()->getRegion( msg->getSender() );
		if (region)
		{
			parcel_mgr.mHoverWestSouth = region->getPosGlobalFromRegion( aabb_min );
			parcel_mgr.mHoverEastNorth = region->getPosGlobalFromRegion( aabb_max );
		}
		else
		{
			parcel_mgr.mHoverWestSouth.clearVec();
			parcel_mgr.mHoverEastNorth.clearVec();
		}
	}
	else
	{
		// Check for video
		LLViewerParcelMedia::update(parcel);

		// Then check for music
		if (gAudiop)
		{
			if (parcel)
			{
				std::string music_url_raw = parcel->getMusicURL();

				// Trim off whitespace from front and back
				std::string music_url = music_url_raw;
				LLStringUtil::trim(music_url);

				// If there is a new music URL and it's valid, play it.
				if (music_url.size() > 12)
				{
					if (music_url.substr(0,7) == "http://")
					{
						optionally_start_music(music_url);
					}
					else
					{
						llinfos << "Stopping parcel music (invalid audio stream URL)" << llendl;
						// clears the URL 
						// null value causes fade out
						LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLStringUtil::null);
					}
				}
				else if (!gAudiop->getInternetStreamURL().empty())
				{
					llinfos << "Stopping parcel music (parcel stream URL is empty)" << llendl;
					// null value causes fade out
					LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLStringUtil::null);
				}
			}
			else
			{
				// Public land has no music
				LLViewerAudio::getInstance()->stopInternetStreamWithAutoFade();
			}
		}//if gAudiop
	};
}

void LLViewerParcelMgr::optionally_start_music(const std::string& music_url)
{
	if (gSavedSettings.getBOOL("AudioStreamingMusic"))
	{
		// only play music when you enter a new parcel if the UI control for this
		// was not *explicitly* stopped by the user. (part of SL-4878)
		LLPanelNearByMedia* nearby_media_panel = gStatusBar->getNearbyMediaPanel();
		if ((nearby_media_panel &&
		     nearby_media_panel->getParcelAudioAutoStart()) ||
		    // or they have expressed no opinion in the UI, but have autoplay on...
		    (!nearby_media_panel &&
		     gSavedSettings.getBOOL(LLViewerMedia::AUTO_PLAY_MEDIA_SETTING) &&
			 gSavedSettings.getBOOL("MediaTentativeAutoPlay")))
		{
			llinfos << "Starting parcel music " << music_url << llendl;
			LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(music_url);
		}
		else
		{
			LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLStringUtil::null);
		}
	}
}

// static
void LLViewerParcelMgr::processParcelAccessListReply(LLMessageSystem *msg, void **user)
{
	LLUUID agent_id;
	S32 sequence_id = 0;
	U32 message_flags = 0x0;
	S32 parcel_id = -1;

	msg->getUUIDFast(_PREHASH_Data, _PREHASH_AgentID, agent_id);
	msg->getS32Fast( _PREHASH_Data, _PREHASH_SequenceID, sequence_id );	//ignored
	msg->getU32Fast( _PREHASH_Data, _PREHASH_Flags, message_flags);
	msg->getS32Fast( _PREHASH_Data, _PREHASH_LocalID, parcel_id);

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->mCurrentParcel;
	if (!parcel) return;

	if (parcel_id != parcel->getLocalID())
	{
		LL_WARNS_ONCE("") << "processParcelAccessListReply for parcel " << parcel_id
			<< " which isn't the selected parcel " << parcel->getLocalID()<< llendl;
		return;
	}

	if (message_flags & AL_ACCESS)
	{
		parcel->unpackAccessEntries(msg, &(parcel->mAccessList) );
	}
	else if (message_flags & AL_BAN)
	{
		parcel->unpackAccessEntries(msg, &(parcel->mBanList) );
	}
	/*else if (message_flags & AL_RENTER)
	{
		parcel->unpackAccessEntries(msg, &(parcel->mRenterList) );
	}*/

	LLViewerParcelMgr::getInstance()->notifyObservers();
}


// static
void LLViewerParcelMgr::processParcelDwellReply(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);

	S32 local_id;
	msg->getS32("Data", "LocalID", local_id);

	LLUUID parcel_id;
	msg->getUUID("Data", "ParcelID", parcel_id);

	F32 dwell;
	msg->getF32("Data", "Dwell", dwell);

	if (local_id == LLViewerParcelMgr::getInstance()->mCurrentParcel->getLocalID())
	{
		LLViewerParcelMgr::getInstance()->mSelectedDwell = dwell;
		LLViewerParcelMgr::getInstance()->notifyObservers();
	}
}


void LLViewerParcelMgr::sendParcelAccessListUpdate(U32 which)
{

	LLUUID transactionUUID;
	transactionUUID.generate();

	if (!mSelected)
	{
		return;
	}

	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;

	LLMessageSystem* msg = gMessageSystem;

	LLParcel* parcel = mCurrentParcel;
	if (!parcel) return;

	if (which & AL_ACCESS)
	{	
		S32 count = parcel->mAccessList.size();
		S32 num_sections = (S32) ceil(count/PARCEL_MAX_ENTRIES_PER_PACKET);
		S32 sequence_id = 1;
		BOOL start_message = TRUE;
		BOOL initial = TRUE;

		access_map_const_iterator cit = parcel->mAccessList.begin();
		access_map_const_iterator end = parcel->mAccessList.end();
		while ( (cit != end) || initial ) 
		{	
			if (start_message) 
			{
				msg->newMessageFast(_PREHASH_ParcelAccessListUpdate);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
				msg->nextBlockFast(_PREHASH_Data);
				msg->addU32Fast(_PREHASH_Flags, AL_ACCESS);
				msg->addS32(_PREHASH_LocalID, parcel->getLocalID() );
				msg->addUUIDFast(_PREHASH_TransactionID, transactionUUID);
				msg->addS32Fast(_PREHASH_SequenceID, sequence_id);
				msg->addS32Fast(_PREHASH_Sections, num_sections);
				start_message = FALSE;

				if (initial && (cit == end))
				{
					// pack an empty block if there will be no data
					msg->nextBlockFast(_PREHASH_List);
					msg->addUUIDFast(_PREHASH_ID,  LLUUID::null );
					msg->addS32Fast(_PREHASH_Time, 0 );
					msg->addU32Fast(_PREHASH_Flags,	0 );
				}

				initial = FALSE;
				sequence_id++;

			}
			
			while ( (cit != end) && (msg->getCurrentSendTotal() < MTUBYTES)) 
			{

				const LLAccessEntry& entry = (*cit).second;
				
				msg->nextBlockFast(_PREHASH_List);
				msg->addUUIDFast(_PREHASH_ID,  entry.mID );
				msg->addS32Fast(_PREHASH_Time, entry.mTime );
				msg->addU32Fast(_PREHASH_Flags,	entry.mFlags );
				++cit;
			}

			start_message = TRUE;
			msg->sendReliable( region->getHost() );
		}
	}

	if (which & AL_BAN)
	{	
		S32 count = parcel->mBanList.size();
		S32 num_sections = (S32) ceil(count/PARCEL_MAX_ENTRIES_PER_PACKET);
		S32 sequence_id = 1;
		BOOL start_message = TRUE;
		BOOL initial = TRUE;

		access_map_const_iterator cit = parcel->mBanList.begin();
		access_map_const_iterator end = parcel->mBanList.end();
		while ( (cit != end) || initial ) 
		{
			if (start_message) 
			{
				msg->newMessageFast(_PREHASH_ParcelAccessListUpdate);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
				msg->nextBlockFast(_PREHASH_Data);
				msg->addU32Fast(_PREHASH_Flags, AL_BAN);
				msg->addS32(_PREHASH_LocalID, parcel->getLocalID() );
				msg->addUUIDFast(_PREHASH_TransactionID, transactionUUID);
				msg->addS32Fast(_PREHASH_SequenceID, sequence_id);
				msg->addS32Fast(_PREHASH_Sections, num_sections);
				start_message = FALSE;

				if (initial && (cit == end))
				{
					// pack an empty block if there will be no data
					msg->nextBlockFast(_PREHASH_List);
					msg->addUUIDFast(_PREHASH_ID,  LLUUID::null );
					msg->addS32Fast(_PREHASH_Time, 0 );
					msg->addU32Fast(_PREHASH_Flags,	0 );
				}

				initial = FALSE;
				sequence_id++;

			}
			
			while ( (cit != end) && (msg->getCurrentSendTotal() < MTUBYTES)) 
			{
				const LLAccessEntry& entry = (*cit).second;
				
				msg->nextBlockFast(_PREHASH_List);
				msg->addUUIDFast(_PREHASH_ID,  entry.mID );
				msg->addS32Fast(_PREHASH_Time, entry.mTime );
				msg->addU32Fast(_PREHASH_Flags,	entry.mFlags );
				++cit;
			}

			start_message = TRUE;
			msg->sendReliable( region->getHost() );
		}
	}
}

void LLViewerParcelMgr::deedLandToGroup()
{
	std::string group_name;
	gCacheName->getGroupName(mCurrentParcel->getGroupID(), group_name);
	LLSD args;
	args["AREA"] = llformat("%d", mCurrentParcel->getArea());
	args["GROUP_NAME"] = group_name;
	if(mCurrentParcel->getContributeWithDeed())
	{
		args["NAME"] = LLSLURL("agent", mCurrentParcel->getOwnerID(), "completename").getSLURLString();
		LLNotificationsUtil::add("DeedLandToGroupWithContribution",args, LLSD(), deedAlertCB);
	}
	else
	{
		LLNotificationsUtil::add("DeedLandToGroup",args, LLSD(), deedAlertCB);
	}
}

// static
bool LLViewerParcelMgr::deedAlertCB(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel();
		LLUUID group_id;
		if(parcel)
		{
			group_id = parcel->getGroupID();
		}
		LLViewerParcelMgr::getInstance()->sendParcelDeed(group_id);
	}
	return false;
}


void LLViewerParcelMgr::startReleaseLand()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotReleaseLandNothingSelected");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_NO_DATA)
	{
		LLNotificationsUtil::add("CannotReleaseLandWatingForServer");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_MULTIPLE)
	{
		LLNotificationsUtil::add("CannotReleaseLandSelected");
		return;
	}

	if (!isParcelOwnedByAgent(mCurrentParcel, GP_LAND_RELEASE)
		&& !(gAgent.canManageEstate()))
	{
		LLNotificationsUtil::add("CannotReleaseLandDontOwn");
		return;
	}

	LLVector3d parcel_center = (mWestSouth + mEastNorth) / 2.0;
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotReleaseLandRegionNotFound");
		return;
	}
/*
	if (region->getRegionFlag(REGION_FLAGS_BLOCK_LAND_RESELL)
		&& !gAgent.isGodlike())
	{
		LLSD args;
		args["REGION"] = region->getName();
		LLNotificationsUtil::add("CannotReleaseLandNoTransfer", args);
		return;
	}
*/

	if (!mCurrentParcelSelection->mWholeParcelSelected)
	{
		LLNotificationsUtil::add("CannotReleaseLandPartialSelection");
		return;
	}

	// Compute claim price
	LLSD args;
	args["AREA"] = llformat("%d",mCurrentParcel->getArea());
	LLNotificationsUtil::add("ReleaseLandWarning", args, LLSD(), releaseAlertCB);
}

bool LLViewerParcelMgr::canAgentBuyParcel(LLParcel* parcel, bool forGroup) const
{
	if (!parcel)
	{
		return false;
	}
	
	if (mSelected  &&  parcel == mCurrentParcel)
	{
		if (mRequestResult == PARCEL_RESULT_NO_DATA)
		{
			return false;
		}
	}
	
	const LLUUID& parcelOwner = parcel->getOwnerID();
	const LLUUID& authorizeBuyer = parcel->getAuthorizedBuyerID();

	if (parcel->isPublic())
	{
		return true;	// change this if want to make it gods only
	}
	
	LLVector3 parcel_coord = parcel->getCenterpoint();
	LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosAgent(parcel_coord);
	if (regionp)
	{
		U8 sim_access = regionp->getSimAccess();
		const LLAgentAccess& agent_access = gAgent.getAgentAccess();
		// if the region is PG, we're happy already, so do nothing
		// but if we're set to avoid either mature or adult, get us outta here
		if ((sim_access == SIM_ACCESS_MATURE) &&
			!agent_access.canAccessMature())
		{
			return false;
		}
		else if ((sim_access == SIM_ACCESS_ADULT) &&
				 !agent_access.canAccessAdult())
		{
			return false;
		}
	}	
	
	bool isForSale = parcel->getForSale()
			&& ((parcel->getSalePrice() > 0) || (authorizeBuyer.notNull()));
			
	bool isEmpowered
		= forGroup ? gAgent.hasPowerInActiveGroup(GP_LAND_DEED) == TRUE : true;
		
	bool isOwner
		= parcelOwner == (forGroup ? gAgent.getGroupID() : gAgent.getID());
	
	bool isAuthorized
			= (authorizeBuyer.isNull()
				|| (gAgent.getID() == authorizeBuyer)
				|| (gAgent.hasPowerInGroup(authorizeBuyer,GP_LAND_DEED)
					&& gAgent.hasPowerInGroup(authorizeBuyer,GP_LAND_SET_SALE_INFO)));
	
	return isForSale && !isOwner && isAuthorized  && isEmpowered;
}


void LLViewerParcelMgr::startBuyLand(BOOL is_for_group)
{
	LLFloaterBuyLand::buyLand(getSelectionRegion(), mCurrentParcelSelection, is_for_group == TRUE);
}

void LLViewerParcelMgr::startSellLand()
{
	LLFloaterSellLand::sellLand(getSelectionRegion(), mCurrentParcelSelection);
}

void LLViewerParcelMgr::startDivideLand()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotDivideLandNothingSelected");
		return;
	}

	if (mCurrentParcelSelection->mWholeParcelSelected)
	{
		LLNotificationsUtil::add("CannotDivideLandPartialSelection");
		return;
	}

	LLSD payload;
	payload["west_south_border"] = ll_sd_from_vector3d(mWestSouth);
	payload["east_north_border"] = ll_sd_from_vector3d(mEastNorth);

	LLNotificationsUtil::add("LandDivideWarning", LLSD(), payload, callbackDivideLand);
}

// static
bool LLViewerParcelMgr::callbackDivideLand(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLVector3d west_south_d = ll_vector3d_from_sd(notification["payload"]["west_south_border"]);
	LLVector3d east_north_d = ll_vector3d_from_sd(notification["payload"]["east_north_border"]);
	LLVector3d parcel_center = (west_south_d + east_north_d) / 2.0;

	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotDivideLandNoRegion");
		return false;
	}

	if (0 == option)
	{
		LLVector3 west_south = region->getPosRegionFromGlobal(west_south_d);
		LLVector3 east_north = region->getPosRegionFromGlobal(east_north_d);

		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelDivide");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("ParcelData");
		msg->addF32("West", west_south.mV[VX]);
		msg->addF32("South", west_south.mV[VY]);
		msg->addF32("East", east_north.mV[VX]);
		msg->addF32("North", east_north.mV[VY]);
		msg->sendReliable(region->getHost());
	}
	return false;
}


void LLViewerParcelMgr::startJoinLand()
{
	if (!mSelected)
	{
		LLNotificationsUtil::add("CannotJoinLandNothingSelected");
		return;
	}

	if (mCurrentParcelSelection->mWholeParcelSelected)
	{
		LLNotificationsUtil::add("CannotJoinLandEntireParcelSelected");
		return;
	}

	if (!mCurrentParcelSelection->mSelectedMultipleOwners)
	{
		LLNotificationsUtil::add("CannotJoinLandSelection");
		return;
	}

	LLSD payload;
	payload["west_south_border"] = ll_sd_from_vector3d(mWestSouth);
	payload["east_north_border"] = ll_sd_from_vector3d(mEastNorth);

	LLNotificationsUtil::add("JoinLandWarning", LLSD(), payload, callbackJoinLand);
}

// static
bool LLViewerParcelMgr::callbackJoinLand(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLVector3d west_south_d = ll_vector3d_from_sd(notification["payload"]["west_south_border"]);
	LLVector3d east_north_d = ll_vector3d_from_sd(notification["payload"]["east_north_border"]);
	LLVector3d parcel_center = (west_south_d + east_north_d) / 2.0;

	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotJoinLandNoRegion");
		return false;
	}

	if (0 == option)
	{
		LLVector3 west_south = region->getPosRegionFromGlobal(west_south_d);
		LLVector3 east_north = region->getPosRegionFromGlobal(east_north_d);

		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelJoin");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("ParcelData");
		msg->addF32("West", west_south.mV[VX]);
		msg->addF32("South", west_south.mV[VY]);
		msg->addF32("East", east_north.mV[VX]);
		msg->addF32("North", east_north.mV[VY]);
		msg->sendReliable(region->getHost());
	}
	return false;
}


void LLViewerParcelMgr::startDeedLandToGroup()
{
	if (!mSelected || !mCurrentParcel)
	{
		LLNotificationsUtil::add("CannotDeedLandNothingSelected");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_NO_DATA)
	{
		LLNotificationsUtil::add("CannotDeedLandWaitingForServer");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_MULTIPLE)
	{
		LLNotificationsUtil::add("CannotDeedLandMultipleSelected");
		return;
	}

	LLVector3d parcel_center = (mWestSouth + mEastNorth) / 2.0;
	LLViewerRegion* region = LLWorld::getInstance()->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		LLNotificationsUtil::add("CannotDeedLandNoRegion");
		return;
	}

	/*
	if(!gAgent.isGodlike())
	{
		if(region->getRegionFlag(REGION_FLAGS_BLOCK_LAND_RESELL)
			&& (mCurrentParcel->getOwnerID() != region->getOwner()))
		{
			LLSD args;
			args["REGION"] = region->getName();
			LLNotificationsUtil::add("CannotDeedLandNoTransfer", args);
			return;
		}
	}
	*/

	deedLandToGroup();
}
void LLViewerParcelMgr::reclaimParcel()
{
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel();
	LLViewerRegion* regionp = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if(parcel && parcel->getOwnerID().notNull()
	   && (parcel->getOwnerID() != gAgent.getID())
	   && regionp && (regionp->getOwner() == gAgent.getID()))
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelReclaim");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addS32("LocalID", parcel->getLocalID());
		msg->sendReliable(regionp->getHost());
	}
}

// static
bool LLViewerParcelMgr::releaseAlertCB(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		// Send the release message, not a force
		LLViewerParcelMgr::getInstance()->sendParcelRelease();
	}
	return false;
}

void LLViewerParcelMgr::buyPass()
{
	LLParcel* parcel = getParcelSelection()->getParcel();
	if (!parcel) return;

	LLViewerRegion* region = getSelectionRegion();
	if (!region) return;

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ParcelBuyPass);
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_LocalID, parcel->getLocalID() );
	msg->sendReliable( region->getHost() );
}

//Tells whether we are allowed to buy a pass or not
BOOL LLViewerParcelMgr::isCollisionBanned()	
{ 
	if ((mCollisionBanned == BA_ALLOWED) || (mCollisionBanned == BA_NOT_ON_LIST) || (mCollisionBanned == BA_NOT_IN_GROUP))
		return FALSE;
	else 
		return TRUE;
}

// This implementation should mirror LLSimParcelMgr::isParcelOwnedBy
// static
BOOL LLViewerParcelMgr::isParcelOwnedByAgent(const LLParcel* parcelp, U64 group_proxy_power)
{
	if (!parcelp)
	{
		return FALSE;
	}

	// Gods can always assume ownership.
	if (gAgent.isGodlike())
	{
		return TRUE;
	}

	// The owner of a parcel automatically gets all powersr.
	if (parcelp->getOwnerID() == gAgent.getID())
	{
		return TRUE;
	}

	// Only gods can assume 'ownership' of public land.
	if (parcelp->isPublic())
	{
		return FALSE;
	}

	// Return whether or not the agent has group_proxy_power powers in the
	// parcel's group.
	return gAgent.hasPowerInGroup(parcelp->getOwnerID(), group_proxy_power);
}

// This implementation should mirror llSimParcelMgr::isParcelModifiableBy
// static
BOOL LLViewerParcelMgr::isParcelModifiableByAgent(const LLParcel* parcelp, U64 group_proxy_power)
{
	// If the agent can assume ownership, it is probably modifiable.
	BOOL rv = FALSE;
	if (parcelp)
	{
		// *NOTE: This should only work for leased parcels, but group owned
		// parcels cannot be OS_LEASED yet. Phoenix 2003-12-15.
		rv = isParcelOwnedByAgent(parcelp, group_proxy_power);

		// ... except for the case that the parcel is not OS_LEASED for agent-owned parcels.
		if( (gAgent.getID() == parcelp->getOwnerID())
			&& !gAgent.isGodlike()
			&& (parcelp->getOwnershipStatus() != LLParcel::OS_LEASED) )
		{
			rv = FALSE;
		}
	}
	return rv;
}

void sanitize_corners(const LLVector3d &corner1,
										const LLVector3d &corner2,
										LLVector3d &west_south_bottom,
										LLVector3d &east_north_top)
{
	west_south_bottom.mdV[VX] = llmin( corner1.mdV[VX], corner2.mdV[VX] );
	west_south_bottom.mdV[VY] = llmin( corner1.mdV[VY], corner2.mdV[VY] );
	west_south_bottom.mdV[VZ] = llmin( corner1.mdV[VZ], corner2.mdV[VZ] );

	east_north_top.mdV[VX] = llmax( corner1.mdV[VX], corner2.mdV[VX] );
	east_north_top.mdV[VY] = llmax( corner1.mdV[VY], corner2.mdV[VY] );
	east_north_top.mdV[VZ] = llmax( corner1.mdV[VZ], corner2.mdV[VZ] );
}


void LLViewerParcelMgr::cleanupGlobals()
{
	LLParcelSelection::sNullSelection = NULL;
}

LLViewerTexture* LLViewerParcelMgr::getBlockedImage() const
{
	return sBlockedImage;
}

LLViewerTexture* LLViewerParcelMgr::getPassImage() const
{
	return sPassImage;
}

boost::signals2::connection LLViewerParcelMgr::addAgentParcelChangedCallback(parcel_changed_callback_t cb)
{
	return mAgentParcelChangedSignal.connect(cb);
}
/*
 * Set finish teleport callback. You can use it to observe all  teleport events.
 * NOTE:
 * After local( in one region) teleports we
 *  cannot rely on gAgent.getPositionGlobal(),
 *  so the new position gets passed explicitly.
 *  Use args of this callback to get global position of avatar after teleport event.
 */
boost::signals2::connection LLViewerParcelMgr::setTeleportFinishedCallback(teleport_finished_callback_t cb)
{
	return mTeleportFinishedSignal.connect(cb);
}

boost::signals2::connection LLViewerParcelMgr::setTeleportFailedCallback(parcel_changed_callback_t cb)
{
	return mTeleportFailedSignal.connect(cb);
}

/* Ok, we're notified that teleport has been finished.
 * We should now propagate the notification via mTeleportFinishedSignal
 * to all interested parties.
 */
void LLViewerParcelMgr::onTeleportFinished(bool local, const LLVector3d& new_pos)
{
	// Treat only teleports within the same parcel as local (EXT-3139).
	if (local && LLViewerParcelMgr::getInstance()->inAgentParcel(new_pos))
	{
		// Local teleport. We already have the agent parcel data.
		// Emit the signal immediately.
		getInstance()->mTeleportFinishedSignal(new_pos, local);
	}
	else
	{
		// Non-local teleport (inter-region or between different parcels of the same region).
		// The agent parcel data has not been updated yet.
		// Let's wait for the update and then emit the signal.
		mTeleportInProgress = TRUE;
	}
}

void LLViewerParcelMgr::onTeleportFailed()
{
	mTeleportFailedSignal();
}
