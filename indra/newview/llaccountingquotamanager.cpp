/** 
 * @file LLAccountingQuotaManager.cpp
 * @ Handles the setting and accessing for costs associated with mesh 
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llaccountingquotamanager.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"

//===============================================================================
LLAccountingQuotaManager::LLAccountingQuotaManager()
{	
}
//===============================================================================
class LLAccountingQuotaResponder : public LLCurl::Responder
{
public:
	LLAccountingQuotaResponder( const LLSD& objectIDs )
	: mObjectIDs( objectIDs )
	{
	}
		
	void clearPendingRequests ( void )
	{
		for ( LLSD::array_iterator iter = mObjectIDs.beginArray(); iter != mObjectIDs.endArray(); ++iter )
		{
			LLAccountingQuotaManager::getInstance()->removePendingObjectQuota( iter->asUUID() );
		}
	}
	
	void error( U32 statusNum, const std::string& reason )
	{
		llwarns	<< "Transport error "<<reason<<llendl;	
		//prep#do we really want to remove all because of one failure - verify
		clearPendingRequests();
	}
	
	void result( const LLSD& content )
	{
		if ( !content.isMap() || content.has("error") )
		{
			llwarns	<< "Error on fetched data"<< llendl;
			//prep#do we really want to remove all because of one failure - verify
			clearPendingRequests();
			return;
		}
		
		//Differentiate what the incoming caps could be from the data
		//bool VOContent  = content.has("Objects");
		bool containsParcel    = content.has("parcel");
		bool containsSelection = content.has("selected");
		//bool VORegion   = content.has("region");
				
		//Loop over the stored object ids checking against the incoming data
		for ( LLSD::array_iterator iter = mObjectIDs.beginArray(); iter != mObjectIDs.endArray(); ++iter )
		{
			LLUUID objectID = iter->asUUID();
						
			LLAccountingQuotaManager::getInstance()->removePendingObjectQuota( objectID );
				
			if ( containsParcel )
			{
					//Typically should be one
					S32 dataCount = content["parcel"].size();
					for(S32 i = 0; i < dataCount; i++)
					{
						//prep#todo verify that this is safe, otherwise just add a bool
						S32 parcelId = 0;
						S32 parcelOwner = 0;
						if ( content["parcel"][i].has("parcel_id") )
						{
							parcelId = content["parcel"][i]["parcel_id"].asInteger();
						}
						if ( content["parcel"][i].has("parcel_owner") )
						{
							parcelOwner = content["parcel"][i]["parcel_owner"].asInteger();
						}
											
						F32 ownerRenderCost		= 0;
						F32 ownerPhysicsCost	= 0;
						F32 ownerNetworkCost	= 0;
						F32 ownerSimulationCost = 0;
						
						F32 groupRenderCost		= 0;
						F32 groupPhysicsCost	= 0;
						F32 groupNetworkCost	= 0;
						F32 groupSimulationCost = 0;
						
						F32 otherRenderCost		= 0;
						F32 otherPhysicsCost	= 0;
						F32 otherNetworkCost	= 0;
						F32 otherSimulationCost = 0;
						
						F32 totalRenderCost		= 0;
						F32 totalPhysicsCost	= 0;
						F32 totalNetworkCost	= 0;
						F32 totalSimulationCost = 0;
						
						if ( content["parcel"][i].has("owner") )
						{
							ownerRenderCost		= content["parcel"][i]["owner"]["render"].asReal();
							ownerPhysicsCost	= content["parcel"][i]["owner"]["physics"].asReal();
							ownerNetworkCost	= content["parcel"][i]["owner"]["network"].asReal();
							ownerSimulationCost = content["parcel"][i]["owner"]["simulation"].asReal();
							
						}
						if ( content["parcel"][i].has("group") )
						{
							groupRenderCost		= content["parcel"][i]["group"]["render"].asReal();
							groupPhysicsCost	= content["parcel"][i]["group"]["physics"].asReal();
							groupNetworkCost	= content["parcel"][i]["group"]["network"].asReal();
							groupSimulationCost = content["parcel"][i]["group"]["simulation"].asReal();
							
						}
						if ( content["parcel"][i].has("other") )
						{
							otherRenderCost		= content["parcel"][i]["other"]["render"].asReal();
							otherPhysicsCost	= content["parcel"][i]["other"]["physics"].asReal();
							otherNetworkCost	= content["parcel"][i]["other"]["network"].asReal();
							otherSimulationCost = content["parcel"][i]["other"]["simulation"].asReal();
						}
						
						if ( content["parcel"][i].has("total") )
						{
							totalRenderCost		= content["parcel"][i]["total"]["render"].asReal();
							totalPhysicsCost	= content["parcel"][i]["total"]["physics"].asReal();
							totalNetworkCost	= content["parcel"][i]["total"]["network"].asReal();
							totalSimulationCost = content["parcel"][i]["total"]["simulation"].asReal();
							
						}
						
						ParcelQuota parcelQuota( ownerRenderCost, ownerPhysicsCost, ownerNetworkCost, ownerSimulationCost,
												 groupRenderCost, groupPhysicsCost, groupNetworkCost, groupSimulationCost,
												 otherRenderCost, otherPhysicsCost, otherNetworkCost, otherSimulationCost,
												 totalRenderCost, totalPhysicsCost, totalNetworkCost, totalSimulationCost );
						//Update the Parcel						
						LLParcel* pParcel = LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel();
						if ( pParcel )
						{
							pParcel->updateQuota( objectID, parcelQuota ); 
						}
					}					
				}
			else 
			if ( containsSelection )
			{
				S32 dataCount = content["selected"].size();
				for(S32 i = 0; i < dataCount; i++)
				{
					
					F32 renderCost		= 0;
					F32 physicsCost		= 0;
					F32 networkCost		= 0;
					F32 simulationCost	= 0;
					
					S32 localId = 0;
					
					localId			= content["selected"][i]["local_id"].asInteger();
					renderCost		= content["selected"][i]["render"].asReal();
					physicsCost		= content["selected"][i]["physics"].asReal();
					networkCost		= content["selected"][i]["network"].asReal();
					simulationCost	= content["selected"][i]["simulation"].asReal();
					
					SelectionQuota selectionQuota( localId, renderCost, physicsCost, networkCost, simulationCost );
					
					//Update the objects					
					//gObjectList.updateQuota( localId, selectionQuota ); 
					
				}
			}
			else
			{
				//Nothing in string 
				LLAccountingQuotaManager::getInstance()->removePendingObjectQuota( objectID );
			}
		}
	}
	
private:
	//List of posted objects
	LLSD mObjectIDs;
};
//===============================================================================
void LLAccountingQuotaManager::fetchQuotas( const std::string& url )
{
	// Invoking system must have already determined capability availability
	if ( !url.empty() )
	{
		LLSD objectList;
		U32  objectIndex = 0;
		IDIt IDIter = mUpdateObjectQuota.begin();
		IDIt IDIterEnd = mUpdateObjectQuota.end();
		
		for ( ; IDIter != IDIterEnd; ++IDIter )
		{
			// Check to see if a request for this object has already been made.
			if ( mPendingObjectQuota.find( *IDIter ) ==	mPendingObjectQuota.end() )
			{
				mPendingObjectQuota.insert( *IDIter );	
				objectList[objectIndex++] = *IDIter;
			}
		}
	
		mUpdateObjectQuota.clear();
		
		//Post results
		if ( objectList.size() > 0 )
		{
			LLSD dataToPost = LLSD::emptyMap();			
			dataToPost["object_ids"] = objectList;
			LLHTTPClient::post( url, dataToPost, new LLAccountingQuotaResponder( objectList ));
		}
	}
	else
	{
		//url was empty - warn & continue
		llwarns<<"Supplied url is empty "<<llendl;
		mUpdateObjectQuota.clear();
		mPendingObjectQuota.clear();
	}
}
//===============================================================================
void LLAccountingQuotaManager::updateObjectCost( const LLUUID& objectID )
{
	mUpdateObjectQuota.insert( objectID );
}
//===============================================================================
void LLAccountingQuotaManager::removePendingObjectQuota( const LLUUID& objectID )
{
	mPendingObjectQuota.erase( objectID );
}
//===============================================================================
