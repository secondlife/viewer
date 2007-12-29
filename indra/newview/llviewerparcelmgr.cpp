/** 
 * @file llviewerparcelmgr.cpp
 * @brief Viewer-side representation of owned land
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

#include "llviewerparcelmgr.h"

// Library includes
#include "audioengine.h"
#include "indra_constants.h"
#include "llcachename.h"
#include "llgl.h"
#include "llmediaengine.h"
#include "llparcel.h"
#include "llsecondlifeurls.h"
#include "message.h"

// Viewer includes
#include "llagent.h"
#include "llfloatergroupinfo.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfloaterbuyland.h"
#include "llfloatergroups.h"
//#include "llfloaterhtml.h"
#include "llfloatersellland.h"
#include "llfloatertools.h"
#include "llnotify.h"
#include "llresmgr.h"
#include "llstatusbar.h"
#include "llui.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
//#include "llwebbrowserctrl.h"
#include "llworld.h"
#include "lloverlaybar.h"
#include "roles_constants.h"
#include "llweb.h"

const F32 PARCEL_COLLISION_DRAW_SECS = 1.f;


// Globals
LLViewerParcelMgr *gParcelMgr = NULL;

U8* LLViewerParcelMgr::sPackedOverlay = NULL;

LLUUID gCurrentMovieID = LLUUID::null;

static LLParcelSelection* get_null_parcel_selection();
template<> 
	const LLHandle<LLParcelSelection>::NullFunc 
		LLHandle<LLParcelSelection>::sNullFunc = get_null_parcel_selection;


// Local functions
void optionally_start_music(const LLString& music_url);
void callback_start_music(S32 option, void* data);
void optionally_prepare_video(const LLParcel *parcelp);
void callback_prepare_video(S32 option, void* data);
void prepare_video(const LLParcel *parcelp);
void start_video(const LLParcel *parcelp);
void stop_video();
void callback_god_force_owner(S32 option, void* user_data);

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
	mWestSouth(),
	mEastNorth(),
	mSelectedDwell(0.f),
	mAgentParcelSequenceID(-1),
	mHoverWestSouth(),
	mHoverEastNorth(),
	mRenderCollision(FALSE),
	mRenderSelection(TRUE),
	mCollisionBanned(0),
	mCollisionTimer()
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

	mBlockedImageID.set(gViewerArt.getString("noentrylines.tga"));
	mBlockedImage = gImageList.getImage(mBlockedImageID, TRUE, TRUE);

	mPassImageID.set(gViewerArt.getString("noentrypasslines.tga"));
	mPassImage = gImageList.getImage(mPassImageID, TRUE, TRUE);

	S32 overlay_size = mParcelsPerEdge * mParcelsPerEdge / PARCEL_OVERLAY_CHUNKS;
	sPackedOverlay = new U8[overlay_size];

	mAgentParcelOverlay = new U8[mParcelsPerEdge * mParcelsPerEdge];
	S32 i;
	for (i = 0; i < mParcelsPerEdge * mParcelsPerEdge; i++)
	{
		mAgentParcelOverlay[i] = 0;
	}
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
	if (!gWorldp) return NULL;

	return gWorldp->getRegionFromPosGlobal( mWestSouth );
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
	F32 dwell = 0.f;

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
	if (!gWorldp)
	{
		return;
	}

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
	if (!gWorldp)
	{
		return NULL;
	}

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

	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal(mWestSouth);
	LLViewerRegion *region_other = gWorldp->getRegionFromPosGlobal( east_north_region_check );

	if(!region)
	{
		// just in case they somehow selected no land.
		mSelected = FALSE;
		return NULL;
	}

	if (region != region_other)
	{
		LLNotifyBox::showXml("CantSelectLandFromMultipleRegions");
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

	// clear the list of segments to prevent flashing
	resetSegments(mHighlightSegments);

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

		mSelectedDwell = 0.f;

		notifyObservers();

		// invalidate parcel selection so that existing users of this selection can clean up
		mCurrentParcelSelection->setParcel(NULL);
		mFloatingParcelSelection->setParcel(NULL);
		// create new parcel selection
		mCurrentParcelSelection = new LLParcelSelection(mCurrentParcel);
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
BOOL LLViewerParcelMgr::agentCanBuild() const
{
	if (mAgentParcel)
	{
		return (gAgent.isGodlike()
				|| (mAgentParcel->allowModifyBy(
						gAgent.getID(),
						gAgent.getGroupID())));
	}
	else
	{
		return gAgent.isGodlike();
	}
}

BOOL LLViewerParcelMgr::agentCanTakeDamage() const
{
	return mAgentParcel->getAllowDamage();
}

BOOL LLViewerParcelMgr::agentCanFly() const
{
	return TRUE;
}

F32 LLViewerParcelMgr::agentDrawDistance() const
{
	return 512.f;
}

BOOL LLViewerParcelMgr::isOwnedAt(const LLVector3d& pos_global) const
{
	if (!gWorldp) return FALSE;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;

	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;

	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );

	return overlay->isOwned( pos_region );
}

BOOL LLViewerParcelMgr::isOwnedSelfAt(const LLVector3d& pos_global) const
{
	if (!gWorldp) return FALSE;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;

	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;

	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );

	return overlay->isOwnedSelf( pos_region );
}

BOOL LLViewerParcelMgr::isOwnedOtherAt(const LLVector3d& pos_global) const
{
	if (!gWorldp) return FALSE;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal( pos_global );
	if (!region) return FALSE;

	LLViewerParcelOverlay* overlay = region->getParcelOverlay();
	if (!overlay) return FALSE;

	LLVector3 pos_region = region->getPosRegionFromGlobal( pos_global );

	return overlay->isOwnedOther( pos_region );
}

BOOL LLViewerParcelMgr::isSoundLocal(const LLVector3d& pos_global) const
{
	if (!gWorldp) return FALSE;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal( pos_global );
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
		if (gParcelMgr->getAgentParcel()->getSoundLocal())
		{
			// Not in same parcel, and agent parcel only has local sound
			return FALSE;
		}
		else if (gParcelMgr->isSoundLocal(pos_global))
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
	if (!gWorldp) return FALSE;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal(pos_global);
	if (region != gAgent.getRegion())
	{
		// Can't be in the agent parcel if you're not in the same region.
		return FALSE;
	}

	LLVector3 pos_region = gAgent.getRegion()->getPosRegionFromGlobal(pos_global);
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
	if (mSelected && mRenderSelection)
	{
		// Rendering is done in agent-coordinates, so need to supply
		// an appropriate offset to the render code.
		if (!gWorldp) return;
		LLViewerRegion* regionp = gWorldp->getRegionFromPosGlobal(mWestSouth);
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

	if (mRenderCollision)
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		BOOL use_pass = mCollisionParcel->getParcelFlag(PF_USE_PASS_LIST);
		renderCollisionSegments(mCollisionSegments, use_pass, regionp);
	}
}


void LLViewerParcelMgr::sendParcelAccessListRequest(U32 flags)
{
	if (!mSelected)
	{
		return;
	}

	if(!gWorldp) return;
	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal( mWestSouth );
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

	if(!gWorldp) return;
	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal( mWestSouth );
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
		gViewerWindow->alertXml("CannotSetLandOwnerNothingSelected");
		return;
	}

	llinfos << "Claiming " << mWestSouth << " to " << mEastNorth << llendl;

	// BUG: Only works for the region containing mWestSouthBottom
	LLVector3d east_north_region_check( mEastNorth );
	east_north_region_check.mdV[VX] -= 0.5;
	east_north_region_check.mdV[VY] -= 0.5;

	if(!gWorldp) return;
	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		// TODO: Add a force owner version of this alert.
		gViewerWindow->alertXml("CannotContentifyNoRegion");
		return;
	}

	// BUG: Make work for cross-region selections
	LLViewerRegion *region2 = gWorldp->getRegionFromPosGlobal( east_north_region_check );
	if (region != region2)
	{
		gViewerWindow->alertXml("CannotSetLandOwnerMultipleRegions");
		return;
	}

	llinfos << "Region " << region->getOriginGlobal() << llendl;

	LLGodForceOwnerData* data = new LLGodForceOwnerData(owner_id, mCurrentParcel->getLocalID(), region->getHost());
	if(mCurrentParcel->getAuctionID())
	{
		gViewerWindow->alertXml("ForceOwnerAuctionWarning",
			callback_god_force_owner,
			(void*)data);
	}
	else
	{
		callback_god_force_owner(0, (void*)data);
	}
}

void callback_god_force_owner(S32 option, void* user_data)
{
	LLGodForceOwnerData* data = (LLGodForceOwnerData*)user_data;
	if(data && (0 == option))
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ParcelGodForceOwner");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("OwnerID", data->mOwnerID);
		msg->addS32( "LocalID", data->mLocalID);
		msg->sendReliable(data->mHost);
	}
	delete data;
}

void LLViewerParcelMgr::sendParcelGodForceToContent()
{
	if (!mSelected)
	{
		gViewerWindow->alertXml("CannotContentifyNothingSelected");
		return;
	}
	if(!gWorldp) return;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		gViewerWindow->alertXml("CannotContentifyNoRegion");
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
        gViewerWindow->alertXml("CannotReleaseLandNothingSelected");
		return;
	}

	if(!gWorldp) return;
	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		gViewerWindow->alertXml("CannotReleaseLandNoRegion");
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
		gViewerWindow->alertXml("CannotBuyLandNothingSelected");
		return NULL;
	}

	if(!gWorldp) return NULL;
	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		gViewerWindow->alertXml("CannotBuyLandNoRegion");
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

		LLViewerRegion *region2 = gWorldp->getRegionFromPosGlobal( east_north_region_check );

		if (region != region2)
		{
			gViewerWindow->alertXml("CantBuyLandAcrossMultipleRegions");
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

void LLViewerParcelMgr::deleteParcelBuy(ParcelBuyInfo*& info)
{
	delete info;
	info = NULL;
}

void LLViewerParcelMgr::sendParcelDeed(const LLUUID& group_id)
{
	if (!mSelected || !mCurrentParcel)
	{
		gViewerWindow->alertXml("CannotDeedLandNothingSelected");
		return;
	}
	if(group_id.isNull())
	{
		gViewerWindow->alertXml("CannotDeedLandNoGroup");
		return;
	}
	if(!gWorldp) return;
	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal( mWestSouth );
	if (!region)
	{
		gViewerWindow->alertXml("CannotDeedLandNoRegion");
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
	region = gWorldp->getRegionFromPosGlobal(global_center);

	LLVector3 west_south_bottom_region = region->getPosRegionFromGlobal( mWestSouth );
	LLVector3 east_north_top_region = region->getPosRegionFromGlobal( mEastNorth );

	LLString name("My Land");
	char buffer[MAX_STRING];
	S32 pos_x = (S32)floor((west_south_bottom_region.mV[VX] + east_north_top_region.mV[VX]) / 2.0f);
	S32 pos_y = (S32)floor((west_south_bottom_region.mV[VY] + east_north_top_region.mV[VY]) / 2.0f);
	sprintf(buffer, "%s in %s (%d, %d)",
			name.c_str(),
			region->getName().c_str(),
			pos_x, pos_y);
	name.assign(buffer);

	const char* desc = "Claimed land";
	create_landmark(name.c_str(), desc, global_center);
}
*/

const LLString& LLViewerParcelMgr::getAgentParcelName() const
{
	return mAgentParcel->getName();
}


void LLViewerParcelMgr::sendParcelPropertiesUpdate(LLParcel* parcel)
{
	if (!parcel) return;
	if(!gWorldp) return;
	LLViewerRegion *region = gWorldp->getRegionFromPosGlobal( mWestSouth );
	if (!region) return;

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ParcelPropertiesUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_LocalID, parcel->getLocalID() );

	U32 flags = 0x0;
	// request new properties update from simulator
	flags |= 0x01;
	msg->addU32("Flags", flags);

	parcel->packMessage(msg);

	msg->sendReliable( region->getHost() );
}


void LLViewerParcelMgr::requestHoverParcelProperties(const LLVector3d& pos)
{
	if(!gWorldp) return;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal( pos );
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
	if (gNoRender)
	{
		return;
	}

	// Extract the packed overlay information
	S32 packed_overlay_size = msg->getSizeFast(_PREHASH_ParcelData, _PREHASH_Data);

	if (packed_overlay_size == 0)
	{
		llwarns << "Overlay size 0" << llendl;
		return;
	}

	S32 parcels_per_edge = gParcelMgr->mParcelsPerEdge;
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
	LLViewerRegion *region = gWorldp->getRegion(host);
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
		gParcelMgr->mRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = gParcelMgr->mCurrentParcel;
	}
	else if (sequence_id == HOVERED_PARCEL_SEQ_ID)
	{
		gParcelMgr->mHoverRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = gParcelMgr->mHoverParcel;
	}
	else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_NOT_ON_LIST_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
	{
		gParcelMgr->mHoverRequestResult = PARCEL_RESULT_SUCCESS;
		parcel = gParcelMgr->mCollisionParcel;
	}
	else if (sequence_id == 0 || sequence_id > gParcelMgr->mAgentParcelSequenceID)
	{
		// new agent parcel
		gParcelMgr->mAgentParcelSequenceID = sequence_id;
		parcel = gParcelMgr->mAgentParcel;
	}
	else
	{
		llinfos << "out of order agent parcel sequence id " << sequence_id
			<< " last good " << gParcelMgr->mAgentParcelSequenceID
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
	msg->getBOOLFast(_PREHASH_AgeVerificationBlock, _PREHASH_RegionDenyAgeUnverified, region_deny_age_unverified_override );

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

		if (parcel == gParcelMgr->mAgentParcel)
		{
			S32 bitmap_size =	gParcelMgr->mParcelsPerEdge
								* gParcelMgr->mParcelsPerEdge
								/ 8;
			U8* bitmap = new U8[ bitmap_size ];
			msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);

			gParcelMgr->writeAgentParcelFromBitmap(bitmap);
			delete[] bitmap;
		}
	}

	// Handle updating selections, if necessary.
	if (sequence_id == SELECTED_PARCEL_SEQ_ID)
	{
		// Update selected counts
		gParcelMgr->mCurrentParcelSelection->mSelectedSelfCount = self_count;
		gParcelMgr->mCurrentParcelSelection->mSelectedOtherCount = other_count;
		gParcelMgr->mCurrentParcelSelection->mSelectedPublicCount = public_count;

		gParcelMgr->mCurrentParcelSelection->mSelectedMultipleOwners =
							(request_result == PARCEL_RESULT_MULTIPLE);

		// Select the whole parcel
		if(!gWorldp) return;
		LLViewerRegion* region = gWorldp->getRegion( msg->getSender() );
		if (region)
		{
			if (!snap_selection)
			{
				// don't muck with the westsouth and eastnorth.
				// just highlight it
				LLVector3 west_south = region->getPosRegionFromGlobal(gParcelMgr->mWestSouth);
				LLVector3 east_north = region->getPosRegionFromGlobal(gParcelMgr->mEastNorth);

				gParcelMgr->resetSegments(gParcelMgr->mHighlightSegments);
				gParcelMgr->writeHighlightSegments(
								west_south.mV[VX],
								west_south.mV[VY],
								east_north.mV[VX],
								east_north.mV[VY] );
				gParcelMgr->mCurrentParcelSelection->mWholeParcelSelected = FALSE;
			}
			else if (0 == local_id)
			{
				// this is public land, just highlight the selection
				gParcelMgr->mWestSouth = region->getPosGlobalFromRegion( aabb_min );
				gParcelMgr->mEastNorth = region->getPosGlobalFromRegion( aabb_max );

				gParcelMgr->resetSegments(gParcelMgr->mHighlightSegments);
				gParcelMgr->writeHighlightSegments(
								aabb_min.mV[VX],
								aabb_min.mV[VY],
								aabb_max.mV[VX],
								aabb_max.mV[VY] );
				gParcelMgr->mCurrentParcelSelection->mWholeParcelSelected = TRUE;
			}
			else
			{
				gParcelMgr->mWestSouth = region->getPosGlobalFromRegion( aabb_min );
				gParcelMgr->mEastNorth = region->getPosGlobalFromRegion( aabb_max );

				// Owned land, highlight the boundaries
				S32 bitmap_size =	gParcelMgr->mParcelsPerEdge
									* gParcelMgr->mParcelsPerEdge
									/ 8;
				U8* bitmap = new U8[ bitmap_size ];
				msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);

				gParcelMgr->resetSegments(gParcelMgr->mHighlightSegments);
				gParcelMgr->writeSegmentsFromBitmap( bitmap, gParcelMgr->mHighlightSegments );

				delete[] bitmap;
				bitmap = NULL;

				gParcelMgr->mCurrentParcelSelection->mWholeParcelSelected = TRUE;
			}

			// Request access list information for this land
			gParcelMgr->sendParcelAccessListRequest(AL_ACCESS | AL_BAN);

			// Request dwell for this land, if it's not public land.
			gParcelMgr->mSelectedDwell = 0.f;
			if (0 != local_id)
			{
				gParcelMgr->sendParcelDwellRequest();
			}

			gParcelMgr->mSelected = TRUE;
			gParcelMgr->notifyObservers();
		}
	}
	else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID ||
			 sequence_id == COLLISION_NOT_ON_LIST_PARCEL_SEQ_ID  ||
			 sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
	{
		// We're about to collide with this parcel
		gParcelMgr->mRenderCollision = TRUE;
		gParcelMgr->mCollisionTimer.reset();

		// Differentiate this parcel if we are banned from it.
		if (sequence_id == COLLISION_BANNED_PARCEL_SEQ_ID)
		{
			gParcelMgr->mCollisionBanned = BA_BANNED;
		}
		else if (sequence_id == COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID)
		{
			gParcelMgr->mCollisionBanned = BA_NOT_IN_GROUP;
		}
		else 
		{
			gParcelMgr->mCollisionBanned = BA_NOT_ON_LIST;

		}

		S32 bitmap_size =	gParcelMgr->mParcelsPerEdge
							* gParcelMgr->mParcelsPerEdge
							/ 8;
		U8* bitmap = new U8[ bitmap_size ];
		msg->getBinaryDataFast(_PREHASH_ParcelData, _PREHASH_Bitmap, bitmap, bitmap_size);

		gParcelMgr->resetSegments(gParcelMgr->mCollisionSegments);
		gParcelMgr->writeSegmentsFromBitmap( bitmap, gParcelMgr->mCollisionSegments );

		delete[] bitmap;
		bitmap = NULL;

	}
	else if (sequence_id == HOVERED_PARCEL_SEQ_ID)
	{
		LLViewerRegion *region = gWorldp->getRegion( msg->getSender() );
		if (region)
		{
			gParcelMgr->mHoverWestSouth = region->getPosGlobalFromRegion( aabb_min );
			gParcelMgr->mHoverEastNorth = region->getPosGlobalFromRegion( aabb_max );
		}
		else
		{
			gParcelMgr->mHoverWestSouth.clearVec();
			gParcelMgr->mHoverEastNorth.clearVec();
		}
	}
	else
	{
		// It's the agent parcel
		BOOL new_parcel = parcel ? FALSE : TRUE;
		if (parcel)
		{
			S32 parcelid = parcel->getLocalID();
			U64 regionid = gAgent.getRegion()->getHandle();
			if (parcelid != gParcelMgr->mMediaParcelId || regionid != gParcelMgr->mMediaRegionId)
			{
				gParcelMgr->mMediaParcelId = parcelid;
				gParcelMgr->mMediaRegionId = regionid;
				new_parcel = TRUE;
			}
		}
		// look for music.
		if (gAudiop)
		{
			if (parcel)
			{
				LLString music_url_raw = parcel->getMusicURL();

				// Trim off whitespace from front and back
				LLString music_url = music_url_raw;
				LLString::trim(music_url);

				// On entering a new parcel, stop the last stream if the
				// new parcel has a different music url.  (Empty URL counts
				// as different.)
				const char* stream_url = gAudiop->getInternetStreamURL();

				if (music_url.empty() || music_url != stream_url)
				{
					// URL is different from one currently playing.
					gAudiop->stopInternetStream();

					// If there is a new music URL and it's valid, play it.
					if (music_url.size() > 12)
					{
						if (music_url.substr(0,7) == "http://")
						{
							optionally_start_music(music_url);
						}
					}
					else if (gAudiop->getInternetStreamURL()[0])
					{
						llinfos << "Stopping parcel music" << llendl;
						gAudiop->startInternetStream(NULL);
					}
				}
			}
			else
			{
				// Public land has no music
				gAudiop->stopInternetStream();
			}
		}//if gAudiop

		// now check for video
		if (LLMediaEngine::getInstance ()->isAvailable ())
		{
			// we have a player
			if (parcel)
			{
				// we're in a parcel
				std::string mediaUrl = std::string ( parcel->getMediaURL () );
				LLString::trim(mediaUrl);

				// clean spaces and whatnot 
				mediaUrl = LLWeb::escapeURL(mediaUrl);

				
				// something changed
				LLMediaEngine* me = LLMediaEngine::getInstance();
				if (  ( me->getUrl () != mediaUrl )
					|| ( me->getImageUUID () != parcel->getMediaID () ) 
					|| ( me->isAutoScaled () != parcel->getMediaAutoScale () ) )
				{
					BOOL video_was_playing = FALSE;
					LLMediaBase* renderer = me->getMediaRenderer();
					if (renderer && (renderer->isPlaying() || renderer->isLooping()))
					{
						video_was_playing = TRUE;
					}

					stop_video();

					if ( !mediaUrl.empty () )
					{
						// Someone has "changed the channel", changing the URL of a video
						// you were already watching.  Do we want to automatically start playing? JC
						if (!new_parcel
							&& gSavedSettings.getBOOL("AudioStreamingVideo")
							&& video_was_playing)
						{
							start_video(parcel);
						}
						else
						{
							// "Prepare" the media engine, but don't auto-play. JC
							optionally_prepare_video(parcel);
						}
					}
				}
			}
			else
			{
				stop_video();
			}
		}
		else
		{
			// no audio player, do a first use dialog if their is media here
			if (parcel)
			{
				std::string mediaUrl = std::string ( parcel->getMediaURL () );
				if (!mediaUrl.empty ())
				{
					if (gSavedSettings.getWarning("QuickTimeInstalled"))
					{
						gSavedSettings.setWarning("QuickTimeInstalled", FALSE);

						LLNotifyBox::showXml("NoQuickTime" );
					};
				}
			}
		}

	};
}

void optionally_start_music(const LLString& music_url)
{
	if (gSavedSettings.getWarning("FirstStreamingMusic"))
	{
		std::string* newstring = new std::string(music_url);
		gViewerWindow->alertXml("ParcelCanPlayMusic",
			callback_start_music,
			(void*)newstring);

	}
	else if (gSavedSettings.getBOOL("AudioStreamingMusic"))
	{
		// Make the user click the start button on the overlay bar. JC
		//		llinfos << "Starting parcel music " << music_url << llendl;

		// now only play music when you enter a new parcel if the control is in PLAY state
		// changed as part of SL-4878
		if ( gOverlayBar && gOverlayBar->musicPlaying() )
		{
			LLOverlayBar::musicPlay(NULL);
		}
	}
}


void callback_start_music(S32 option, void* data)
{
	std::string* music_url = (std::string*)data;

	if (0 == option)
	{
		gSavedSettings.setBOOL("AudioStreamingMusic", TRUE);
		llinfos << "Starting first parcel music " << music_url << llendl;
		LLOverlayBar::musicPlay(NULL);
	}
	else
	{
		gSavedSettings.setBOOL("AudioStreamingMusic", FALSE);
	}

	gSavedSettings.setWarning("FirstStreamingMusic", FALSE);

	delete music_url;
	music_url = NULL;
}

void prepare_video(const LLParcel *parcel)
{
	std::string mediaUrl;
	if (parcel->getParcelFlag(PF_URL_RAW_HTML))
	{
		mediaUrl = std::string("data:");
		mediaUrl.append(parcel->getMediaURL());
	}
	else
	{
		mediaUrl = std::string ( parcel->getMediaURL () );
	}

	// clean spaces and whatnot 
	mediaUrl = LLWeb::escapeURL(mediaUrl);
	
	LLMediaEngine::getInstance ()->setUrl ( mediaUrl );
	LLMediaEngine::getInstance ()->setImageUUID ( parcel->getMediaID () );
	LLMediaEngine::getInstance ()->setAutoScaled ( parcel->getMediaAutoScale () ? TRUE : FALSE );  // (U8 instead of BOOL for future expansion)
}

void start_video(const LLParcel *parcel)
{
	prepare_video(parcel);
	std::string path( "" );
	LLMediaEngine::getInstance ()->convertImageAndLoadUrl ( true, false, path);
}

void stop_video()
{
	// set up remote control so stop is selected
	LLMediaEngine::getInstance ()->stop ();
	if (gOverlayBar)
	{
		gOverlayBar->refresh ();
	}

	if (LLMediaEngine::getInstance ()->isLoaded())
	{
		LLMediaEngine::getInstance ()->unload ();

		gImageList.updateMovieImage(LLUUID::null, FALSE);
		gCurrentMovieID.setNull();
	}

	LLMediaEngine::getInstance ()->setUrl ( "" );
	LLMediaEngine::getInstance ()->setImageUUID ( LLUUID::null );
	
}

void optionally_prepare_video(const LLParcel *parcelp)
{
	if (gSavedSettings.getWarning("FirstStreamingVideo"))
	{
		gViewerWindow->alertXml("ParcelCanPlayMedia",
			callback_prepare_video,
			(void*)parcelp);
	}
	else
	{
		llinfos << "Entering parcel " << parcelp->getLocalID() << " with video " <<  parcelp->getMediaURL() << llendl;
		prepare_video(parcelp);
	}
}


void callback_prepare_video(S32 option, void* data)
{
	const LLParcel *parcelp = (const LLParcel *)data;

	if (0 == option)
	{
		gSavedSettings.setBOOL("AudioStreamingVideo", TRUE);
		llinfos << "Starting parcel video " <<  parcelp->getMediaURL() << " on parcel " << parcelp->getLocalID() << llendl;
		gMessageSystem->setHandlerFunc("ParcelMediaCommandMessage", LLMediaEngine::process_parcel_media);
		gMessageSystem->setHandlerFunc ( "ParcelMediaUpdate", LLMediaEngine::process_parcel_media_update );
		prepare_video(parcelp);
	}
	else
	{
		gMessageSystem->setHandlerFunc("ParcelMediaCommandMessage", null_message_callback);
		gMessageSystem->setHandlerFunc ( "ParcelMediaUpdate", null_message_callback );
		gSavedSettings.setBOOL("AudioStreamingVideo", FALSE);
	}

	gSavedSettings.setWarning("FirstStreamingVideo", FALSE);
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

	LLParcel* parcel = gParcelMgr->mCurrentParcel;
	if (!parcel) return;

	if (parcel_id != parcel->getLocalID())
	{
		llwarns << "processParcelAccessListReply for parcel " << parcel_id
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

	gParcelMgr->notifyObservers();
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

	if (local_id == gParcelMgr->mCurrentParcel->getLocalID())
	{
		gParcelMgr->mSelectedDwell = dwell;
		gParcelMgr->notifyObservers();
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

	if(!gWorldp) return;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal( mWestSouth );
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
	char group_name[MAX_STRING];		/* Flawfinder: ignore */
	gCacheName->getGroupName(mCurrentParcel->getGroupID(), group_name);
	LLString::format_map_t args;
	args["[AREA]"] = llformat("%d", mCurrentParcel->getArea());
	args["[GROUP_NAME]"] = group_name;
	if(mCurrentParcel->getContributeWithDeed())
	{
		char first_name[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
		first_name[0] = '\0';
		char last_name[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
		last_name[0] = '\0';		
		gCacheName->getName(mCurrentParcel->getOwnerID(), first_name, last_name);
		args["[FIRST_NAME]"] = first_name;
		args["[LAST_NAME]"] = last_name;
		gViewerWindow->alertXml("DeedLandToGroupWithContribution",args, deedAlertCB, NULL);
	}
	else
	{
		gViewerWindow->alertXml("DeedLandToGroup",args, deedAlertCB, NULL);
	}
}

// static
void LLViewerParcelMgr::deedAlertCB(S32 option, void*)
{
	if (option == 0)
	{
		LLParcel* parcel = gParcelMgr->getParcelSelection()->getParcel();
		LLUUID group_id;
		if(parcel)
		{
			group_id = parcel->getGroupID();
		}
		gParcelMgr->sendParcelDeed(group_id);
	}
}


void LLViewerParcelMgr::startReleaseLand()
{
	if (!mSelected)
	{
		gViewerWindow->alertXml("CannotReleaseLandNothingSelected");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_NO_DATA)
	{
		gViewerWindow->alertXml("CannotReleaseLandWatingForServer");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_MULTIPLE)
	{
		gViewerWindow->alertXml("CannotReleaseLandSelected");
		return;
	}

	if (!isParcelOwnedByAgent(mCurrentParcel, GP_LAND_RELEASE)
		&& !(gAgent.canManageEstate()))
	{
		gViewerWindow->alertXml("CannotReleaseLandDontOwn");
		return;
	}

	LLVector3d parcel_center = (mWestSouth + mEastNorth) / 2.0;
	if(!gWorldp) return;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		gViewerWindow->alertXml("CannotReleaseLandRegionNotFound");
		return;
	}
/*
	if ((region->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
		&& !gAgent.isGodlike())
	{
		LLStringBase<char>::format_map_t args;
		args["[REGION]"] = region->getName();
		gViewerWindow->alertXml("CannotReleaseLandNoTransfer", args);
		return;
	}
*/

	if (!mCurrentParcelSelection->mWholeParcelSelected)
	{
		gViewerWindow->alertXml("CannotReleaseLandPartialSelection");
		return;
	}

	// Compute claim price
	LLStringBase<char>::format_map_t args;
	args["[AREA]"] = llformat("%d",mCurrentParcel->getArea());
	gViewerWindow->alertXml("ReleaseLandWarning", args,
				releaseAlertCB, this);
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
	
	bool isForSale = parcel->getForSale()
			&& ((parcel->getSalePrice() > 0) || (authorizeBuyer.notNull()));
			
	bool isEmpowered
		= forGroup ? gAgent.hasPowerInActiveGroup(GP_LAND_DEED) == TRUE : true;
		
	bool isOwner
		= parcelOwner == (forGroup ? gAgent.getGroupID() : gAgent.getID());
	
	bool isAuthorized
		= (authorizeBuyer.isNull() || (gAgent.getID() == authorizeBuyer));
	
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
		gViewerWindow->alertXml("CannotDivideLandNothingSelected");
		return;
	}

	if (mCurrentParcelSelection->mWholeParcelSelected)
	{
		gViewerWindow->alertXml("CannotDivideLandPartialSelection");
		return;
	}

	gViewerWindow->alertXml("LandDivideWarning", 
		callbackDivideLand,
		this);
}

// static
void LLViewerParcelMgr::callbackDivideLand(S32 option, void* data)
{
	LLViewerParcelMgr* self = (LLViewerParcelMgr*)data;

	LLVector3d parcel_center = (self->mWestSouth + self->mEastNorth) / 2.0;
	if(!gWorldp) return;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		gViewerWindow->alertXml("CannotDivideLandNoRegion");
		return;
	}

	if (0 == option)
	{
		LLVector3 west_south = region->getPosRegionFromGlobal(self->mWestSouth);
		LLVector3 east_north = region->getPosRegionFromGlobal(self->mEastNorth);

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
}


void LLViewerParcelMgr::startJoinLand()
{
	if (!mSelected)
	{
		gViewerWindow->alertXml("CannotJoinLandNothingSelected");
		return;
	}

	if (mCurrentParcelSelection->mWholeParcelSelected)
	{
		gViewerWindow->alertXml("CannotJoinLandEntireParcelSelected");
		return;
	}

	if (!mCurrentParcelSelection->mSelectedMultipleOwners)
	{
		gViewerWindow->alertXml("CannotJoinLandSelection");
		return;
	}

	gViewerWindow->alertXml("JoinLandWarning",
		callbackJoinLand,
		this);
}

// static
void LLViewerParcelMgr::callbackJoinLand(S32 option, void* data)
{
	LLViewerParcelMgr* self = (LLViewerParcelMgr*)data;

	LLVector3d parcel_center = (self->mWestSouth + self->mEastNorth) / 2.0;
	if(!gWorldp) return;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		gViewerWindow->alertXml("CannotJoinLandNoRegion");
		return;
	}

	if (0 == option)
	{
		LLVector3 west_south = region->getPosRegionFromGlobal(self->mWestSouth);
		LLVector3 east_north = region->getPosRegionFromGlobal(self->mEastNorth);

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
}


void LLViewerParcelMgr::startDeedLandToGroup()
{
	if (!mSelected || !mCurrentParcel)
	{
		gViewerWindow->alertXml("CannotDeedLandNothingSelected");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_NO_DATA)
	{
		gViewerWindow->alertXml("CannotDeedLandWaitingForServer");
		return;
	}

	if (mRequestResult == PARCEL_RESULT_MULTIPLE)
	{
		gViewerWindow->alertXml("CannotDeedLandMultipleSelected");
		return;
	}

	LLVector3d parcel_center = (mWestSouth + mEastNorth) / 2.0;
	if(!gWorldp) return;
	LLViewerRegion* region = gWorldp->getRegionFromPosGlobal(parcel_center);
	if (!region)
	{
		gViewerWindow->alertXml("CannotDeedLandNoRegion");
		return;
	}

	/*
	if(!gAgent.isGodlike())
	{
		if((region->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
			&& (mCurrentParcel->getOwnerID() != region->getOwner()))
		{
			LLStringBase<char>::format_map_t args;
			args["[REGION]"] = region->getName();
			gViewerWindow->alertXml("CannotDeedLandNoTransfer", args);
			return;
		}
	}
	*/

	deedLandToGroup();
}
void LLViewerParcelMgr::reclaimParcel()
{
	LLParcel* parcel = gParcelMgr->getParcelSelection()->getParcel();
	LLViewerRegion* regionp = gParcelMgr->getSelectionRegion();
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
void LLViewerParcelMgr::releaseAlertCB(S32 option, void *)
{
	if (option == 0)
	{
		// Send the release message, not a force
		gParcelMgr->sendParcelRelease();
	}
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

//
// LLParcelSelection
//
LLParcelSelection::LLParcelSelection() : 
	mParcel(NULL),
	mSelectedMultipleOwners(FALSE),
	mWholeParcelSelected(FALSE),
	mSelectedSelfCount(0),
	mSelectedOtherCount(0),
	mSelectedPublicCount(0)
{
}

LLParcelSelection::LLParcelSelection(LLParcel* parcel)  : 
	mParcel(parcel),
	mSelectedMultipleOwners(FALSE),
	mWholeParcelSelected(FALSE),
	mSelectedSelfCount(0),
	mSelectedOtherCount(0),
	mSelectedPublicCount(0)
{
}

LLParcelSelection::~LLParcelSelection()
{
}

BOOL LLParcelSelection::getMultipleOwners() const
{
	return mSelectedMultipleOwners;
}


BOOL LLParcelSelection::getWholeParcelSelected() const
{
	return mWholeParcelSelected;
}


S32 LLParcelSelection::getClaimableArea() const
{
	const S32 UNIT_AREA = S32( PARCEL_GRID_STEP_METERS * PARCEL_GRID_STEP_METERS );
	return mSelectedPublicCount * UNIT_AREA;
}

bool LLParcelSelection::hasOthersSelected() const
{
	return mSelectedOtherCount != 0;
}

static LLPointer<LLParcelSelection> sNullSelection;

LLParcelSelection* get_null_parcel_selection()
{
	if (sNullSelection.isNull())
	{
		sNullSelection = new LLParcelSelection;
	}
	
	return sNullSelection;
}

void LLViewerParcelMgr::cleanupGlobals()
{
	delete gParcelMgr;
	gParcelMgr = NULL;
	sNullSelection = NULL;
}
