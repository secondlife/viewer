/** 
 * @file llaccountingquota.h
 * @
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_ACCOUNTINGQUOTA_H
#define LL_ACCOUNTINGQUOTA_H

struct ParcelQuota
{
	ParcelQuota( F32 ownerRenderCost,	 F32 ownerPhysicsCost,	  F32 ownerNetworkCost,	   F32 ownerSimulationCost,
				 F32 groupRenderCost,	 F32 groupPhysicsCost,	  F32 groupNetworkCost,	   F32 groupSimulationCost,
				 F32 otherRenderCost,	 F32 otherPhysicsCost,	  F32 otherNetworkCost,	   F32 otherSimulationCost,
				 F32 tempRenderCost,	 F32 tempPhysicsCost,	  F32 tempNetworkCost,	   F32 tempSimulationCost,
				 F32 selectedRenderCost, F32 selectedPhysicsCost, F32 selectedNetworkCost, F32 selectedSimulationCost,
				 F32 parcelCapacity )
	: mOwnerRenderCost( ownerRenderCost ), mOwnerPhysicsCost( ownerPhysicsCost ) 
	, mOwnerNetworkCost( ownerNetworkCost ), mOwnerSimulationCost( ownerSimulationCost )
	, mGroupRenderCost( groupRenderCost ), mGroupPhysicsCost( groupPhysicsCost )
	, mGroupNetworkCost( groupNetworkCost ), mGroupSimulationCost( groupSimulationCost )
	, mOtherRenderCost( otherRenderCost ), mOtherPhysicsCost( otherPhysicsCost )
	, mOtherNetworkCost( otherNetworkCost ), mOtherSimulationCost( otherSimulationCost )
	, mTempRenderCost( tempRenderCost ), mTempPhysicsCost( tempPhysicsCost ) 
	, mTempNetworkCost( tempNetworkCost ), mTempSimulationCost( tempSimulationCost )
	, mSelectedRenderCost( tempRenderCost ), mSelectedPhysicsCost( tempPhysicsCost ) 
	, mSelectedNetworkCost( tempNetworkCost ), mSelectedSimulationCost( selectedSimulationCost )
	, mParcelCapacity( parcelCapacity )
	{
	}

	ParcelQuota(){}			
	F32 mOwnerRenderCost, mOwnerPhysicsCost, mOwnerNetworkCost, mOwnerSimulationCost;
	F32 mGroupRenderCost, mGroupPhysicsCost, mGroupNetworkCost, mGroupSimulationCost;
	F32 mOtherRenderCost, mOtherPhysicsCost, mOtherNetworkCost, mOtherSimulationCost;
	F32 mTempRenderCost,  mTempPhysicsCost,  mTempNetworkCost,  mTempSimulationCost;
	F32 mSelectedRenderCost, mSelectedPhysicsCost, mSelectedNetworkCost, mSelectedSimulationCost;
	F32 mParcelCapacity;
};

struct SelectionQuota
{
	SelectionQuota( LLUUID localId, F32 renderCost, F32 physicsCost, F32 networkCost, F32 simulationCost )
	: mLocalId( localId)
	, mRenderCost( renderCost )
	, mPhysicsCost( physicsCost )
	, mNetworkCost( networkCost )
	, mSimulationCost( simulationCost )
	{
	}
	SelectionQuota() {}
	
	F32 mRenderCost, mPhysicsCost, mNetworkCost, mSimulationCost;	
	LLUUID mLocalId;
};

#endif



