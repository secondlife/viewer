/**
 * @file   llavatarrenderinfoaccountant.cpp
 * @author Dave Simmons
 * @date   2013-02-28
 * @brief  
 * 
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

// Precompiled header
#include "llviewerprecompiledheaders.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llcharacter.h"
#include "httprequest.h"
#include "httphandler.h"
#include "httpresponse.h"
#include "llcorehttputil.h"
#include "llappcorehttp.h"
#include "lltimer.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llworld.h"
// associated header
#include "llavatarrenderinfoaccountant.h"


static	const std::string KEY_AGENTS = "agents";			// map
static 	const std::string KEY_WEIGHT = "weight";			// integer

static	const std::string KEY_IDENTIFIER = "identifier";
static	const std::string KEY_MESSAGE = "message";
static	const std::string KEY_ERROR = "error";


static const F32 SECS_BETWEEN_REGION_SCANS   =  5.f;		// Scan the region list every 5 seconds
static const F32 SECS_BETWEEN_REGION_REQUEST = 15.0;		// Look for new avs every 15 seconds
static const F32 SECS_BETWEEN_REGION_REPORTS = 60.0;		// Update each region every 60 seconds
	

// The policy class for HTTP traffic; this is the right value for all capability requests.
static LLCore::HttpRequest::policy_t http_policy(LLAppCoreHttp::AP_REPORTING);

// Priority for HTTP requests.  Use 0U.
static LLCore::HttpRequest::priority_t http_priority(0U);

LLAvatarRenderInfoAccountant::LLAvatarRenderInfoAccountant()
	: mHttpRequest(new LLCore::HttpRequest)
	, mHttpHeaders(new LLCore::HttpHeaders)
	, mHttpOptions(new LLCore::HttpOptions)
{
	mHttpOptions->setTransferTimeout(SECS_BETWEEN_REGION_SCANS);

	mHttpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_LLSD_XML);
	mHttpHeaders->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_LLSD_XML);
}

LLAvatarRenderInfoAccountant::~LLAvatarRenderInfoAccountant()
{
	mHttpOptions->release();
	mHttpHeaders->release();
	// delete mHttpRequest; ???
}

// HTTP responder class for GET request for avatar render weight information
class LLAvatarRenderInfoGetHandler : public LLCore::HttpHandler
{
private:
	LOG_CLASS(LLAvatarRenderInfoGetHandler);
	
public:
	LLAvatarRenderInfoGetHandler() : LLCore::HttpHandler()
	{
	}

	void onCompleted(LLCore::HttpHandle handle,
					 LLCore::HttpResponse* response)
		{
            LLCore::HttpStatus status = response->getStatus();
            if (status)
            {
				LLSD avatar_render_info;
				if (LLCoreHttpUtil::responseToLLSD(response, false /* quiet logging */,
												   avatar_render_info))
				{
					if (avatar_render_info.isMap())
					{
						if (avatar_render_info.has(KEY_AGENTS))
						{
							const LLSD & agents = avatar_render_info[KEY_AGENTS];
							if (agents.isMap())
							{
								for (LLSD::map_const_iterator agent_iter = agents.beginMap();
									 agent_iter != agents.endMap();
									 agent_iter++
									 )
								{
									LLUUID target_agent_id = LLUUID(agent_iter->first);
									LLViewerObject* avatarp = gObjectList.findObject(target_agent_id);
									if (avatarp && avatarp->isAvatar())
									{
										const LLSD & agent_info_map = agent_iter->second;
										if (agent_info_map.isMap())
										{
											LL_DEBUGS("AvatarRenderInfo") << " Agent " << target_agent_id 
																		  << ": " << agent_info_map << LL_ENDL;

											if (agent_info_map.has(KEY_WEIGHT))
											{
												((LLVOAvatar *) avatarp)->setReportedVisualComplexity(agent_info_map[KEY_WEIGHT].asInteger());
											}
										}
										else
										{
											LL_WARNS("AvatarRenderInfo") << "agent entry invalid"
																		 << " agent " << target_agent_id
																		 << " map " << agent_info_map
																		 << LL_ENDL;
										}
									}
									else
									{
										LL_DEBUGS("AvatarRenderInfo") << "Unknown agent " << target_agent_id << LL_ENDL;
									}
								} // for agent_iter
							}
							else
							{
								LL_WARNS("AvatarRenderInfo") << "malformed get response agents avatar_render_info is not map" << LL_ENDL;
							}
						}	// has "agents"
						else if (avatar_render_info.has(KEY_ERROR))
						{
							const LLSD & error = avatar_render_info[KEY_ERROR];
							LL_WARNS("AvatarRenderInfo") << "Avatar render info GET error: "
														 << error[KEY_IDENTIFIER]
														 << ": " << error[KEY_MESSAGE] 
														 << LL_ENDL;
						}
						else
						{
							LL_WARNS("AvatarRenderInfo") << "no agent key in get response" << LL_ENDL;
						}
					}
					else
					{
						LL_WARNS("AvatarRenderInfo") << "malformed get response is not map" << LL_ENDL;
					}
                }
				else
				{
					LL_WARNS("AvatarRenderInfo") << "malformed get response parse failure" << LL_ENDL;
				}
            }
            else
            {
                // Something went wrong.  Translate the status to
                // a meaningful message.
                LL_WARNS("AvatarRenderInfo") << "GET failed Status:  "
											 << status.toTerseString()
											 << ", Reason:  " << status.toString()
											 << LL_ENDL;
            }           

			delete this;    // release the handler object
		}
};


// HTTP responder class for POST request for avatar render weight information
class LLAvatarRenderInfoPostHandler : public LLCore::HttpHandler
{
  private:
	LOG_CLASS(LLAvatarRenderInfoPostHandler);

  public:
	LLAvatarRenderInfoPostHandler() : LLCore::HttpHandler()
	{
	}

	void onCompleted(LLCore::HttpHandle handle,
					 LLCore::HttpResponse* response)
		{
			LLCore::HttpStatus status = response->getStatus();
			if (status)
			{
				LL_DEBUGS("AvatarRenderInfo") << "post succeeded" << LL_ENDL;
			}
			else
			{
				// Something went wrong.  Translate the status to
				// a meaningful message.
				LL_WARNS("AvatarRenderInfo") << "POST failed Status:  "
											 << status.toTerseString()
											 << ", Reason:  " << status.toString()
											 << LL_ENDL;
			}           

			delete this;    // release the handler object
		}
};


// Send request for one region, no timer checks
// called when the 
void LLAvatarRenderInfoAccountant::sendRenderInfoToRegion(LLViewerRegion * regionp)
{
	if ( regionp->getRenderInfoReportTimer().hasExpired() ) // Time to make request
	{
		U32 num_avs = 0;
	
		std::string url = regionp->getCapability("AvatarRenderInfo");
		if (!url.empty())
		{
			// Build the render info to POST to the region
			LLSD agents = LLSD::emptyMap();
				
			std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			while( iter != LLCharacter::sInstances.end() )
			{
				LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*iter);
				if (avatar &&
					avatar->getRezzedStatus() >= 2 &&					// Mostly rezzed (maybe without baked textures downloaded)
					!avatar->isDead() &&								// Not dead yet
					avatar->getObjectHost() == regionp->getHost())		// Ensure it's on the same region
				{
					avatar->calculateUpdateRenderCost();			// Make sure the numbers are up-to-date

					LLSD info = LLSD::emptyMap();
					if (avatar->getVisualComplexity() > 0)
					{
						info[KEY_WEIGHT] = avatar->getVisualComplexity();
						agents[avatar->getID().asString()] = info;

						LL_DEBUGS("AvatarRenderInfo") << "Sending avatar render info for " << avatar->getID()
													  << ": " << info << LL_ENDL;
						num_avs++;
					}
				}
				iter++;
			}

			if (num_avs > 0)
			{
				LLSD report = LLSD::emptyMap();
				report[KEY_AGENTS] = agents;

				LLCore::HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);
				LLAvatarRenderInfoPostHandler* handler = new LLAvatarRenderInfoPostHandler;
			
				handle = LLCoreHttpUtil::requestPostWithLLSD(mHttpRequest,
															 http_policy,
															 http_priority,
															 url,
															 report,
															 mHttpOptions,
															 mHttpHeaders,
															 handler);
				if (LLCORE_HTTP_HANDLE_INVALID == handle)
				{
					LLCore::HttpStatus status(mHttpRequest->getStatus());
					LL_WARNS("AvatarRenderInfo") << "HTTP POST request failed"
												 << " Status: " << status.toTerseString()
												 << " Reason: '" << status.toString() << "'"
												 << LL_ENDL;
					delete handler;
				}
				else
				{
					LL_INFOS("AvatarRenderInfo") << "Sent render costs for " << num_avs
												 << " avatars to region " << regionp->getName()
												 << LL_ENDL;


				}			
			}
			else
			{
				LL_DEBUGS("AvatarRenderInfo") << "no agent info to send" << LL_ENDL;
			}
		}
		else
		{
			LL_WARNS("AvatarRenderInfo") << "AvatarRenderInfo cap is empty" << LL_ENDL;
		}

		// Reset this regions timer, moving to longer intervals if there are lots of avatars around
		regionp->getRenderInfoReportTimer().resetWithExpiry(SECS_BETWEEN_REGION_REPORTS + (2.f * num_avs));
	}
}




// static 
// Send request for one region, no timer checks
void LLAvatarRenderInfoAccountant::getRenderInfoFromRegion(LLViewerRegion * regionp)
{
	if (regionp->getRenderInfoRequestTimer().hasExpired())
	{
		std::string url = regionp->getCapability("AvatarRenderInfo");
		if (!url.empty())
		{
			
			LLAvatarRenderInfoGetHandler* handler = new LLAvatarRenderInfoGetHandler;
			// First send a request to get the latest data
			LLCore::HttpHandle handle = mHttpRequest->requestGet(http_policy,
																 http_priority,
																 url,
																 NULL,
																 NULL,
																 handler);
			if (LLCORE_HTTP_HANDLE_INVALID != handle)
			{
				LL_INFOS("AvatarRenderInfo") << "Requested avatar render info for region "
											 << regionp->getName() 
											 << LL_ENDL;
			}
			else
			{
				LL_WARNS("AvatarRenderInfo") << "Failed to launch HTTP GET request.  Try again."
											 << LL_ENDL;
				delete handler;
			}
		}
		else
		{
			LL_WARNS("AvatarRenderInfo") << "no AvatarRenderInfo cap for " << regionp->getName() << LL_ENDL;
		}

		regionp->getRenderInfoRequestTimer().resetWithExpiry(SECS_BETWEEN_REGION_REQUEST);
	}
}


// static
// Called every frame - send render weight requests to every region
void LLAvatarRenderInfoAccountant::idle()
{
	mHttpRequest->update(0); // give any pending http operations a chance to call completion methods
	
	if (mRenderInfoScanTimer.hasExpired())
	{
		LL_DEBUGS("AvatarRenderInfo") << "Scanning regions for render info updates"
									  << LL_ENDL;

		// Check all regions
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
			 iter != LLWorld::getInstance()->getRegionList().end();
			 ++iter)
		{
			LLViewerRegion* regionp = *iter;
			if (   regionp
				&& regionp->isAlive()
				&& regionp->capabilitiesReceived())
			{
				// each of these is further governed by and resets its own timer
				sendRenderInfoToRegion(regionp);
				getRenderInfoFromRegion(regionp);
			}
		}

		// We scanned all the regions, reset the request timer.
		mRenderInfoScanTimer.resetWithExpiry(SECS_BETWEEN_REGION_SCANS);
	}
}

void LLAvatarRenderInfoAccountant::resetRenderInfoScanTimer()
{
	// this will force the next frame to rescan
	mRenderInfoScanTimer.reset();
}

// static
// Called via LLViewerRegion::setCapabilitiesReceived() boost signals when the capabilities
// are returned for a new LLViewerRegion, and is the earliest time to get render info
void LLAvatarRenderInfoAccountant::scanNewRegion(const LLUUID& region_id)
{
	LL_INFOS("AvatarRenderInfo") << region_id << LL_ENDL;

	// Reset the global timer so it will scan regions on the next call to ::idle
	LLAvatarRenderInfoAccountant::getInstance()->resetRenderInfoScanTimer();
	
	LLViewerRegion* regionp = LLWorld::instance().getRegionFromID(region_id);
	if (regionp)
	{	// Reset the region's timers so we will:
		//  * request render info from it immediately
		//  * report on the following scan
		regionp->getRenderInfoRequestTimer().reset();
		regionp->getRenderInfoReportTimer().resetWithExpiry(SECS_BETWEEN_REGION_SCANS);
	}
	else
	{
		LL_WARNS("AvatarRenderInfo") << "unable to resolve region "<<region_id<<LL_ENDL;
	}
}
