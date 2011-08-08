/** 
 * @file LLAccountingQuotaManager.cpp
 * @ Handles the setting and accessing for costs associated with mesh 
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

#include "llviewerprecompiledheaders.h"
#include "llaccountingcostmanager.h"
#include "llagent.h"
#include "llcurl.h"
#include "llhttpclient.h"

//===============================================================================
LLAccountingCostManager::LLAccountingCostManager()
{	
}
//===============================================================================
class LLAccountingCostResponder : public LLCurl::Responder
{
public:
	LLAccountingCostResponder( const LLSD& objectIDs )
	: mObjectIDs( objectIDs )
	{
	}
		
	void clearPendingRequests ( void )
	{
		for ( LLSD::array_iterator iter = mObjectIDs.beginArray(); iter != mObjectIDs.endArray(); ++iter )
		{
			LLAccountingCostManager::getInstance()->removePendingObject( iter->asUUID() );
		}
	}
	
	void error( U32 statusNum, const std::string& reason )
	{
		llwarns	<< "Transport error "<<reason<<llendl;	
		clearPendingRequests();
	}
	
	void result( const LLSD& content )
	{
		//Check for error
		if ( !content.isMap() || content.has("error") )
		{
			llwarns	<< "Error on fetched data"<< llendl;
			clearPendingRequests();
			return;
		}
		
		bool containsSelection = content.has("selected");
		if ( containsSelection )
		{
			S32 dataCount = content["selected"].size();
				
			for(S32 i = 0; i < dataCount; i++)
			{
				
				F32 physicsCost		= 0.0f;
				F32 networkCost		= 0.0f;
				F32 simulationCost	= 0.0f;
					
				//LLTransactionID transactionID;
					
				//transactionID	= content["selected"][i]["local_id"].asUUID();
				physicsCost		= content["selected"][i]["physics"].asReal();
				networkCost		= content["selected"][i]["streaming"].asReal();
				simulationCost	= content["selected"][i]["simulation"].asReal();
					
				SelectionCost selectionCost( /*transactionID,*/ physicsCost, networkCost, simulationCost );
					
				//How do you want to handle the updating of the invoking object/ui element?
				
			}
		}
	}
	
private:
	//List of posted objects
	LLSD mObjectIDs;
};
//===============================================================================
void LLAccountingCostManager::fetchCosts( eSelectionType selectionType, const std::string& url )
{
	// Invoking system must have already determined capability availability
	if ( !url.empty() )
	{
		LLSD objectList;
		U32  objectIndex = 0;
		
		IDIt IDIter = mObjectList.begin();
		IDIt IDIterEnd = mObjectList.end();
		
		for ( ; IDIter != IDIterEnd; ++IDIter )
		{
			// Check to see if a request for this object has already been made.
			if ( mPendingObjectQuota.find( *IDIter ) ==	mPendingObjectQuota.end() )
			{
				mObjectList.insert( *IDIter );	
				objectList[objectIndex++] = *IDIter;
			}
		}
	
		mObjectList.clear();
		
		//Post results
		if ( objectList.size() > 0 )
		{
			std::string keystr;
			if ( selectionType == Roots ) 
			{ 
				keystr="selected_roots"; 
			}
			else
			if ( selectionType == Prims ) 
			{ 
				keystr="prim_roots"; 
			}
			else 
			{
				llinfos<<"Invalid selection type "<<llendl;
				mObjectList.clear();
				mPendingObjectQuota.clear();
				return;
			}
			
			LLSD dataToPost = LLSD::emptyMap();		
			dataToPost[keystr.c_str()] = objectList;

			LLHTTPClient::post( url, dataToPost, new LLAccountingCostResponder( objectList ));
		}
	}
	else
	{
		//url was empty - warn & continue
		llwarns<<"Supplied url is empty "<<llendl;
		mObjectList.clear();
		mPendingObjectQuota.clear();
	}
}
//===============================================================================
void LLAccountingCostManager::addObject( const LLUUID& objectID )
{
	mObjectList.insert( objectID );
}
//===============================================================================
void LLAccountingCostManager::removePendingObject( const LLUUID& objectID )
{
	mPendingObjectQuota.erase( objectID );
}
//===============================================================================
