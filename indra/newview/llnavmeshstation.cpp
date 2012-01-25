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
#include "llfloaterpathfindingconsole.h"

//===============================================================================
LLNavMeshStation::LLNavMeshStation()
{
}
//===============================================================================
class LLNavMeshDownloadResponder : public LLCurl::Responder
{
public:
	LLNavMeshDownloadResponder( const LLHandle<LLNavMeshDownloadObserver>& observer_handle, int dir )
	:  mObserverHandle( observer_handle )
	,  mDir( dir )
	{
	}

	void error( U32 statusNum, const std::string& reason )
	{
		//statusNum;
		llwarns	<< "Transport error "<<reason<<llendl;			
	}
	
	void result( const LLSD& content )
	{		
		llinfos<<"Content received"<<llendl;
		if ( content.has("error") )
		{
			llwarns	<< "Error on fetched data"<< llendl;
			//llinfos<<"LLsd buffer on error"<<ll_pretty_print_sd(content)<<llendl;
		}
		else 
		{
			LLNavMeshDownloadObserver* pObserver = mObserverHandle.get();
			if ( pObserver )
			{
				
				if ( content.has("navmesh_data") )
				{
					const LLSD::Binary& stuff = content["navmesh_data"].asBinary();
					LLPathingLib::getInstance()->extractNavMeshSrcFromLLSD( stuff, mDir );
					pObserver->getPathfindingConsole()->setHasNavMeshReceived();
				}
				else
				{
					llwarns<<"no mesh data "<<llendl;
					pObserver->getPathfindingConsole()->setHasNoNavMesh();
				}
			}
		}	
}	
	
private:
	//Observer handle
	LLHandle<LLNavMeshDownloadObserver> mObserverHandle;
	int mDir;
};
//===============================================================================
void LLNavMeshStation::downloadNavMeshSrc( const LLHandle<LLNavMeshDownloadObserver>& observerHandle, int dir ) 
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
		LLHTTPClient::post(mNavMeshDownloadURL, data, new LLNavMeshDownloadResponder( observerHandle, dir ) );
	}
}
//===============================================================================
