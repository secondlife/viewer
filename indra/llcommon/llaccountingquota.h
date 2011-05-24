/** 
 * @file llaccountingquota.h
 * @
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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



