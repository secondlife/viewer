/** 
 * @file llnavmeshstation.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "llnavmeshstation.h"
#include "llcurl.h"
#include "llpathinglib.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llsdutil.h"
//===============================================================================
LLNavMeshStation::LLNavMeshStation()
{
}
//===============================================================================
class LLNavMeshUploadResponder : public LLCurl::Responder
{
public:
	LLNavMeshUploadResponder( const LLHandle<LLNavMeshObserver>& observer_handle )
	:  mObserverHandle( observer_handle )
	{
		LLNavMeshObserver* pObserver = mObserverHandle.get();		
	}

	void clearPendingRequests ( void )
	{		
	}
	
	void error( U32 statusNum, const std::string& reason )
	{
		statusNum;
		llwarns	<< "Transport error "<<reason<<llendl;			
	}
	
	void result( const LLSD& content )
	{		
		llinfos<<"Content received"<<llendl;
		//TODO# some sanity checking
		if ( content.has("error") )
		{
			llwarns	<< "Error on fetched data"<< llendl;
		}
		else 
		{
			LLNavMeshObserver* pObserver = mObserverHandle.get();
			if ( pObserver )
			{
				llinfos<<"Do something immensely important w/content"<<llendl;
				//pObserver->execute();
			}
		}	
	}
	
private:
	//Observer handle
	LLHandle<LLNavMeshObserver> mObserverHandle;
};
//===============================================================================
class LLNavMeshDownloadResponder : public LLCurl::Responder
{
public:
	LLNavMeshDownloadResponder( const LLHandle<LLNavMeshDownloadObserver>& observer_handle )
	:  mObserverHandle( observer_handle )
	{
		LLNavMeshDownloadObserver* pObserver = mObserverHandle.get();		
	}

	void clearPendingRequests ( void )
	{		
	}
	
	void error( U32 statusNum, const std::string& reason )
	{
		statusNum;
		llwarns	<< "Transport error "<<reason<<llendl;			
	}
	
	void result( const LLSD& content )
	{		
		llinfos<<"Content received"<<llendl;
		//TODO# some sanity checking
		if ( content.has("error") )
		{
			llwarns	<< "Error on fetched data"<< llendl;
			llinfos<<"LLsd buffer on error"<<ll_pretty_print_sd(content)<<llendl;
		}
		else 
		{
			LLNavMeshDownloadObserver* pObserver = mObserverHandle.get();
			if ( pObserver )
			{
				
				if ( content.has("navmesh_data") )
				{
					LLSD::Binary& stuff = content["navmesh_data"].asBinary();
					LLPathingLib::getInstance()->extractNavMeshSrcFromLLSD( stuff );
				}
				else
				{
					llwarns<<"no mesh data "<<llendl;
				}
			}
		}	
}	
	
private:
	//Observer handle
	LLHandle<LLNavMeshDownloadObserver> mObserverHandle;
};

//===============================================================================
bool LLNavMeshStation::postNavMeshToServer( LLSD& data, const LLHandle<LLNavMeshObserver>& observerHandle ) 
{	
	mCurlRequest = new LLCurlRequest();

	if ( mNavMeshUploadURL.empty() )
	{
		llinfos << "Unable to upload navmesh because of missing URL" << llendl;
	}
	else
	{
		LLCurlRequest::headers_t headers;
		mCurlRequest->post( mNavMeshUploadURL, headers, data,
						    new LLNavMeshUploadResponder(/*this, data,*/ observerHandle ) );
		do
		{
			mCurlRequest->process();
			//sleep for 10ms to prevent eating a whole core
			apr_sleep(10000);
		} while ( mCurlRequest->getQueued() > 0 );
	}

	delete mCurlRequest;

	mCurlRequest = NULL;

	return true;
}
//===============================================================================
void LLNavMeshStation::downloadNavMeshSrc( const LLHandle<LLNavMeshDownloadObserver>& observerHandle ) 
{	
	if ( mNavMeshDownloadURL.empty() )
	{
		llinfos << "Unable to upload navmesh because of missing URL" << llendl;
	}
	else
	{		
		LLSD data;
		data["agent_id"]  = gAgent.getID();
		data["region_id"] = gAgent.getRegion()->getRegionID();
		LLHTTPClient::post(mNavMeshDownloadURL, data, new LLNavMeshDownloadResponder( observerHandle ) );
	}
}
//===============================================================================