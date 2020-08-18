/**
 * @file llviewerparcelmediaautoplay.cpp
 * @brief timer to automatically play media in a parcel
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerparcelmediaautoplay.h"

#include "llparcel.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerparcelaskplay.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"         // for texture stats
#include "message.h"
#include "llagent.h"
#include "llmimetypes.h"

const F32 AUTOPLAY_TIME  = 5;          // how many seconds before we autoplay
const F32 AUTOPLAY_SIZE  = 24*24;      // how big the texture must be (pixel area) before we autoplay
const F32 AUTOPLAY_SPEED = 0.1f;        // how slow should the agent be moving to autoplay

LLViewerParcelMediaAutoPlay::LLViewerParcelMediaAutoPlay() :
	LLEventTimer(1),
	
	mLastParcelID(-1),
	mPlayed(FALSE),
	mTimeInParcel(0)
{
}

// static
void LLViewerParcelMediaAutoPlay::playStarted()
{
    LLSingleton<LLViewerParcelMediaAutoPlay>::getInstance()->mPlayed = TRUE;
}

BOOL LLViewerParcelMediaAutoPlay::tick()
{
	LLParcel *this_parcel = NULL;
	LLViewerRegion *this_region = NULL;
	std::string this_media_url;
	std::string this_media_type;
	LLUUID this_media_texture_id;
	S32 this_parcel_id = 0;
	LLUUID this_region_id;

	this_region = gAgent.getRegion();
	
	if (this_region)
	{
		this_region_id = this_region->getRegionID();
	}

	this_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	if (this_parcel)
	{
		this_media_url = this_parcel->getMediaURL();
		
		this_media_type = this_parcel->getMediaType();

		this_media_texture_id = this_parcel->getMediaID();

		this_parcel_id = this_parcel->getLocalID();
	}

	if (this_parcel_id != mLastParcelID ||
	    this_region_id != mLastRegionID)
	{
		// we've entered a new parcel
		mPlayed    = FALSE;                   // we haven't autoplayed yet
		mTimeInParcel = 0;                    // reset our timer
		mLastParcelID = this_parcel_id;
		mLastRegionID = this_region_id;
	}

	mTimeInParcel += mPeriod;					// increase mTimeInParcel by the amount of time between ticks

	if ((!mPlayed) &&							// if we've never played
		(mTimeInParcel > AUTOPLAY_TIME) &&		// and if we've been here for so many seconds
		(!this_media_url.empty()) &&			// and if the parcel has media
		(stricmp(this_media_type.c_str(), LLMIMETypes::getDefaultMimeType().c_str()) != 0) &&
		(!LLViewerParcelMedia::getInstance()->hasParcelMedia()))	// and if the media is not already playing
	{
		if (this_media_texture_id.notNull())	// and if the media texture is good
		{
			LLViewerMediaTexture *image = LLViewerTextureManager::getMediaTexture(this_media_texture_id, FALSE) ;

			F32 image_size = 0;

			if (image)
			{
				image_size = image->getMaxVirtualSize() ;
			}

			if (gAgent.getVelocity().magVec() < AUTOPLAY_SPEED) // and if the agent is stopped (slow enough)
			{
				if (image_size > AUTOPLAY_SIZE)    // and if the target texture is big enough on screen
				{
					if (this_parcel)
					{
						static LLCachedControl<S32> autoplay_mode(gSavedSettings, "ParcelMediaAutoPlayEnable");

						switch (autoplay_mode())
						{
							case 0:
								// Disabled
								break;
							case 1:
								// Play, default value for ParcelMediaAutoPlayEnable
								LLViewerParcelMedia::getInstance()->play(this_parcel);
								break;
							case 2:
							default:
							{
								// Ask
								LLViewerParcelAskPlay::getInstance()->askToPlay(this_region->getRegionID(),
																				this_parcel->getLocalID(),
																				this_parcel->getMediaURL(),
																				onStartMediaResponse);
								break;
  							}
  						}
					}

					mPlayed = TRUE;
				}
			}
		}
	}


	return FALSE; // continue ticking forever please.
}

//static
void LLViewerParcelMediaAutoPlay::onStartMediaResponse(const LLUUID &region_id, const S32 &parcel_id, const std::string &url, const bool &play)
{
    if (play)
    {
        LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

        // make sure we are still there
        if (parcel->getLocalID() == parcel_id && gAgent.getRegion()->getRegionID() == region_id)
        {
            LLViewerParcelMedia::getInstance()->play(parcel);
        }
    }
}

