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
	ParcelQuota( F32 ownerRenderCost, F32 ownerPhysicsCost, F32 ownerNetworkCost, F32 ownerSimulationCost,
				F32 groupRenderCost, F32 groupPhysicsCost, F32 groupNetworkCost, F32 groupSimulationCost,
				F32 otherRenderCost, F32 otherPhysicsCost, F32 otherNetworkCost, F32 otherSimulationCost,
				F32 totalRenderCost, F32 totalPhysicsCost, F32 totalNetworkCost, F32 totalSimulationCost)
	: mOwnerRenderCost( ownerRenderCost ), mOwnerPhysicsCost( ownerPhysicsCost ) 
	, mOwnerNetworkCost( ownerNetworkCost ), mOwnerSimulationCost( ownerSimulationCost )
	, mGroupRenderCost( groupRenderCost ), mGroupPhysicsCost( groupPhysicsCost )
	, mGroupNetworkCost( groupNetworkCost ), mGroupSimulationCost( groupSimulationCost )
	, mOtherRenderCost( otherRenderCost ), mOtherPhysicsCost( otherPhysicsCost )
	, mOtherNetworkCost( otherNetworkCost ), mOtherSimulationCost( otherSimulationCost )
	, mTotalRenderCost( totalRenderCost ), mTotalPhysicsCost( totalPhysicsCost ) 
	, mTotalNetworkCost( totalNetworkCost ), mTotalSimulationCost( totalSimulationCost )
	{
	}
	ParcelQuota(){}			
	F32 mOwnerRenderCost, mOwnerPhysicsCost, mOwnerNetworkCost, mOwnerSimulationCost;
	F32 mGroupRenderCost, mGroupPhysicsCost, mGroupNetworkCost, mGroupSimulationCost;
	F32 mOtherRenderCost, mOtherPhysicsCost, mOtherNetworkCost, mOtherSimulationCost;
	F32 mTotalRenderCost, mTotalPhysicsCost, mTotalNetworkCost, mTotalSimulationCost;
};

struct SelectionQuota
{
	SelectionQuota( S32 localId, F32 renderCost, F32 physicsCost, F32 networkCost, F32 simulationCost )
	: mLocalId( localId)
	, mRenderCost( renderCost )
	, mPhysicsCost( physicsCost )
	, mNetworkCost( networkCost )
	, mSimulationCost( simulationCost )
	{
	}
	SelectionQuota() {}
	
	F32 mRenderCost, mPhysicsCost, mNetworkCost, mSimulationCost;	
	S32 mLocalId;
};

#endif



