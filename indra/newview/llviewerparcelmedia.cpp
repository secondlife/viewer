/**
 * @file llviewerparcelmedia.cpp
 * @brief Handlers for multimedia on a per-parcel basis
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
#include "llviewerparcelmedia.h"

#include "llagent.h"
#include "llaudioengine.h"
#include "llmimetypes.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerregion.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "lluuid.h"
#include "message.h"
#include "llviewermediafocus.h"
#include "llviewerparcelmediaautoplay.h"
#include "llnotificationsutil.h"
//#include "llfirstuse.h"
#include "llpluginclassmedia.h"
#include "llviewertexture.h"
#include "llcorehttputil.h"


LLViewerParcelMedia::LLViewerParcelMedia():
mMediaParcelLocalID(0)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->setHandlerFunc("ParcelMediaCommandMessage", parcelMediaCommandMessageHandler );
	msg->setHandlerFunc("ParcelMediaUpdate", parcelMediaUpdateHandler );

    // LLViewerParcelMediaAutoPlay will regularly check and autoplay media,
    // might be good idea to just integrate it into LLViewerParcelMedia
    LLSingleton<LLViewerParcelMediaAutoPlay>::getInstance();
}

LLViewerParcelMedia::~LLViewerParcelMedia()
{
	// This needs to be destroyed before global destructor time.
	mMediaImpl = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerParcelMedia::update(LLParcel* parcel)
{
	if (/*LLViewerMedia::hasMedia()*/ true)
	{
		// we have a player
		if (parcel)
		{
			if(!gAgent.getRegion())
			{
				mMediaRegionID = LLUUID() ;
				stop() ;
				LL_DEBUGS("Media") << "no agent region, bailing out." << LL_ENDL;
				return ;				
			}

			// we're in a parcel
			S32 parcelid = parcel->getLocalID();						

			LLUUID regionid = gAgent.getRegion()->getRegionID();
			bool location_changed = false;
			if (parcelid != mMediaParcelLocalID || regionid != mMediaRegionID)
			{
				LL_DEBUGS("Media") << "New parcel, parcel id = " << parcelid << ", region id = " << regionid << LL_ENDL;
				mMediaParcelLocalID = parcelid;
				mMediaRegionID = regionid;
				location_changed = true;
			}

			std::string mediaUrl = std::string ( parcel->getMediaURL () );
			std::string mediaCurrentUrl = std::string( parcel->getMediaCurrentURL());

			// if we have a current (link sharing) url, use it instead
			if (mediaCurrentUrl != "" && parcel->getMediaType() == HTTP_CONTENT_TEXT_HTML)
			{
				mediaUrl = mediaCurrentUrl;
			}
			
			LLStringUtil::trim(mediaUrl);
			
			// If no parcel media is playing, nothing left to do
			if(mMediaImpl.isNull())

			{
				// media will be autoplayed by LLViewerParcelMediaAutoPlay
				return;
			}

			// Media is playing...has something changed?
			else if (( mMediaImpl->getMediaURL() != mediaUrl )
				|| ( mMediaImpl->getMediaTextureID() != parcel->getMediaID() )
				|| ( mMediaImpl->getMimeType() != parcel->getMediaType() ))
			{
				// Only play if the media types are the same and parcel stays same.
				if(mMediaImpl->getMimeType() == parcel->getMediaType()
					&& !location_changed)
				{
					play(parcel);
				}

				else
				{
					stop();
				}
			}
		}
		else
		{
			stop();
		}
	}
}

// static
void LLViewerParcelMedia::play(LLParcel* parcel)
{
	LL_DEBUGS() << "LLViewerParcelMedia::play" << LL_ENDL;

	if (!parcel) return;

	if (!gSavedSettings.getBOOL("AudioStreamingMedia"))
		return;

	std::string media_url = parcel->getMediaURL();
	std::string media_current_url = parcel->getMediaCurrentURL();
	std::string mime_type = parcel->getMediaType();
	LLUUID placeholder_texture_id = parcel->getMediaID();
	U8 media_auto_scale = parcel->getMediaAutoScale();
	U8 media_loop = parcel->getMediaLoop();
	S32 media_width = parcel->getMediaWidth();
	S32 media_height = parcel->getMediaHeight();

	if(mMediaImpl)
	{
		// If the url and mime type are the same, call play again
		if(mMediaImpl->getMediaURL() == media_url 
			&& mMediaImpl->getMimeType() == mime_type
			&& mMediaImpl->getMediaTextureID() == placeholder_texture_id)
		{
			LL_DEBUGS("Media") << "playing with existing url " << media_url << LL_ENDL;

			mMediaImpl->play();
		}
		// Else if the texture id's are the same, navigate and rediscover type
		// MBW -- This causes other state from the previous parcel (texture size, autoscale, and looping) to get re-used incorrectly.
		// It's also not really necessary -- just creating a new instance is fine.
//		else if(mMediaImpl->getMediaTextureID() == placeholder_texture_id)
//		{
//			mMediaImpl->navigateTo(media_url, mime_type, true);
//		}
		else
		{
			// Since the texture id is different, we need to generate a new impl

			// Delete the old one first so they don't fight over the texture.
			mMediaImpl = NULL;
			
			// A new impl will be created below.
		}
	}
	
	// Don't ever try to play if the media type is set to "none/none"
	if(stricmp(mime_type.c_str(), LLMIMETypes::getDefaultMimeType().c_str()) != 0)
	{
		if(!mMediaImpl)
		{
			LL_DEBUGS("Media") << "new media impl with mime type " << mime_type << ", url " << media_url << LL_ENDL;

			// There is no media impl, make a new one
			mMediaImpl = LLViewerMedia::getInstance()->newMediaImpl(
				placeholder_texture_id,
				media_width, 
				media_height, 
				media_auto_scale,
				media_loop);
			mMediaImpl->setIsParcelMedia(true);
			mMediaImpl->navigateTo(media_url, mime_type, true);
		}

		//LLFirstUse::useMedia();

		LLViewerParcelMediaAutoPlay::playStarted();
	}
}

// static
void LLViewerParcelMedia::stop()
{
	if(mMediaImpl.isNull())
	{
		return;
	}
	
	// We need to remove the media HUD if it is up.
	LLViewerMediaFocus::getInstance()->clearFocus();

	// This will unload & kill the media instance.
	mMediaImpl = NULL;
}

// static
void LLViewerParcelMedia::pause()
{
	if(mMediaImpl.isNull())
	{
		return;
	}
	mMediaImpl->pause();
}

// static
void LLViewerParcelMedia::start()
{
	if(mMediaImpl.isNull())
	{
		return;
	}
	mMediaImpl->start();

	//LLFirstUse::useMedia();

	LLViewerParcelMediaAutoPlay::playStarted();
}

// static
void LLViewerParcelMedia::seek(F32 time)
{
	if(mMediaImpl.isNull())
	{
		return;
	}
	mMediaImpl->seek(time);
}

// static
void LLViewerParcelMedia::focus(bool focus)
{
	mMediaImpl->focus(focus);
}

// static
LLPluginClassMediaOwner::EMediaStatus LLViewerParcelMedia::getStatus()
{	
	LLPluginClassMediaOwner::EMediaStatus result = LLPluginClassMediaOwner::MEDIA_NONE;
	
	if(mMediaImpl.notNull() && mMediaImpl->hasMedia())
	{
		result = mMediaImpl->getMediaPlugin()->getStatus();
	}
	
	return result;
}

// static
std::string LLViewerParcelMedia::getMimeType()
{
	return mMediaImpl.notNull() ? mMediaImpl->getMimeType() : LLMIMETypes::getDefaultMimeType();
}

//static 
std::string LLViewerParcelMedia::getURL()
{
	std::string url;
	if(mMediaImpl.notNull())
		url = mMediaImpl->getMediaURL();
	
	if(stricmp(LLViewerParcelMgr::getInstance()->getAgentParcel()->getMediaType().c_str(), LLMIMETypes::getDefaultMimeType().c_str()) != 0)
	{
		if (url.empty())
			url = LLViewerParcelMgr::getInstance()->getAgentParcel()->getMediaCurrentURL();
		
		if (url.empty())
			url = LLViewerParcelMgr::getInstance()->getAgentParcel()->getMediaURL();
	}
	
	return url;
}

//static 
std::string LLViewerParcelMedia::getName()
{
	if(mMediaImpl.notNull())
		return mMediaImpl->getName();
	return "";
}

viewer_media_t LLViewerParcelMedia::getParcelMedia()
{
	return mMediaImpl;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerParcelMedia::parcelMediaCommandMessageHandler(LLMessageSystem *msg, void **)
{
    getInstance()->processParcelMediaCommandMessage(msg);
}

void LLViewerParcelMedia::processParcelMediaCommandMessage( LLMessageSystem *msg)
{
	// extract the agent id
	//	LLUUID agent_id;
	//	msg->getUUID( agent_id );

	U32 flags;
	U32 command;
	F32 time;
	msg->getU32( "CommandBlock", "Flags", flags );
	msg->getU32( "CommandBlock", "Command", command);
	msg->getF32( "CommandBlock", "Time", time );

	if (flags &( (1<<PARCEL_MEDIA_COMMAND_STOP)
				| (1<<PARCEL_MEDIA_COMMAND_PAUSE)
				| (1<<PARCEL_MEDIA_COMMAND_PLAY)
				| (1<<PARCEL_MEDIA_COMMAND_LOOP)
				| (1<<PARCEL_MEDIA_COMMAND_UNLOAD) ))
	{
		// stop
		if( command == PARCEL_MEDIA_COMMAND_STOP )
		{
			stop();
		}
		else
		// pause
		if( command == PARCEL_MEDIA_COMMAND_PAUSE )
		{
			pause();
		}
		else
		// play
		if(( command == PARCEL_MEDIA_COMMAND_PLAY ) ||
		   ( command == PARCEL_MEDIA_COMMAND_LOOP ))
		{
			if (getStatus() == LLViewerMediaImpl::MEDIA_PAUSED)
			{
				start();
			}
			else
			{
				LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
				play(parcel);
			}
		}
		else
		// unload
		if( command == PARCEL_MEDIA_COMMAND_UNLOAD )
		{
			stop();
		}
	}

	if (flags & (1<<PARCEL_MEDIA_COMMAND_TIME))
	{
		if(mMediaImpl.isNull())
		{
			LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
			play(parcel);
		}
		seek(time);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerParcelMedia::parcelMediaUpdateHandler(LLMessageSystem *msg, void **)
{
    getInstance()->processParcelMediaUpdate(msg);
}

void LLViewerParcelMedia::processParcelMediaUpdate( LLMessageSystem *msg)
{
	LLUUID media_id;
	std::string media_url;
	std::string media_type;
	S32 media_width = 0;
	S32 media_height = 0;
	U8 media_auto_scale = 0;
	U8 media_loop = 0;

	msg->getUUID( "DataBlock", "MediaID", media_id );
	char media_url_buffer[257];
	msg->getString( "DataBlock", "MediaURL", 255, media_url_buffer );
	media_url = media_url_buffer;
	msg->getU8("DataBlock", "MediaAutoScale", media_auto_scale);

	if (msg->has("DataBlockExtended")) // do we have the extended data?
	{
		char media_type_buffer[257];
		msg->getString("DataBlockExtended", "MediaType", 255, media_type_buffer);
		media_type = media_type_buffer;
		msg->getU8("DataBlockExtended", "MediaLoop", media_loop);
		msg->getS32("DataBlockExtended", "MediaWidth", media_width);
		msg->getS32("DataBlockExtended", "MediaHeight", media_height);
	}

	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		bool same = ((parcel->getMediaURL() == media_url) &&
					 (parcel->getMediaType() == media_type) &&
					 (parcel->getMediaID() == media_id) &&
					 (parcel->getMediaWidth() == media_width) &&
					 (parcel->getMediaHeight() == media_height) &&
					 (parcel->getMediaAutoScale() == media_auto_scale) &&
					 (parcel->getMediaLoop() == media_loop));

		if (!same)
		{
			// temporarily store these new values in the parcel
			parcel->setMediaURL(media_url);
			parcel->setMediaType(media_type);
			parcel->setMediaID(media_id);
			parcel->setMediaWidth(media_width);
			parcel->setMediaHeight(media_height);
			parcel->setMediaAutoScale(media_auto_scale);
			parcel->setMediaLoop(media_loop);

			play(parcel);
		}
	}
}
// Static
/////////////////////////////////////////////////////////////////////////////////////////
// *TODO: I can not find any active code where this method is called...
void LLViewerParcelMedia::sendMediaNavigateMessage(const std::string& url)
{
	std::string region_url = gAgent.getRegionCapability("ParcelNavigateMedia");
	if (!region_url.empty())
	{
		// send navigate event to sim for link sharing
		LLSD body;
		body["agent-id"] = gAgent.getID();
		body["local-id"] = LLViewerParcelMgr::getInstance()->getAgentParcel()->getLocalID();
		body["url"] = url;

        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(region_url, body,
            "Media Navigation sent to sim.", "Media Navigation failed to send to sim.");
	}
	else
	{
		LL_WARNS() << "can't get ParcelNavigateMedia capability" << LL_ENDL;
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
// inherited from LLViewerMediaObserver
// virtual 
void LLViewerParcelMedia::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	switch(event)
	{
		case MEDIA_EVENT_DEBUG_MESSAGE:
		{
			// LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_DEBUG_MESSAGE " << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_CONTENT_UPDATED:
		{
			// LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CONTENT_UPDATED " << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_TIME_DURATION_UPDATED:
		{
			// LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_TIME_DURATION_UPDATED, time is " << self->getCurrentTime() << " of " << self->getDuration() << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_SIZE_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_SIZE_CHANGED " << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_CURSOR_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CURSOR_CHANGED, new cursor is " << self->getCursorName() << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_NAVIGATE_BEGIN:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_BEGIN " << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_COMPLETE, result string is: " << self->getNavigateResultString() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_PROGRESS_UPDATED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PROGRESS_UPDATED, loading at " << self->getProgressPercent() << "%" << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_STATUS_TEXT_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_STATUS_TEXT_CHANGED, new status text is: " << self->getStatusText() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_LOCATION_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_LOCATION_CHANGED, new uri is: " << self->getLocation() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_NAVIGATE_ERROR_PAGE:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_ERROR_PAGE" << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_CLICK_LINK_HREF:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, target is \"" << self->getClickTarget() << "\", uri is " << self->getClickURL() << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is " << self->getClickURL() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED" << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED_LAUNCH" << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_NAME_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAME_CHANGED" << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_CLOSE_REQUEST:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLOSE_REQUEST" << LL_ENDL;
		}
		break;
		
		case MEDIA_EVENT_PICK_FILE_REQUEST:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PICK_FILE_REQUEST" << LL_ENDL;
		}
		break;
		
		case MEDIA_EVENT_FILE_DOWNLOAD:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_FILE_DOWNLOAD" << LL_ENDL;
		}
		break;
		
		case MEDIA_EVENT_GEOMETRY_CHANGE:
		{
			LL_DEBUGS("Media") << "Media event:  MEDIA_EVENT_GEOMETRY_CHANGE, uuid is " << self->getClickUUID() << LL_ENDL;
		}
		break;

		case MEDIA_EVENT_AUTH_REQUEST:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_AUTH_REQUEST, url " << self->getAuthURL() << ", realm " << self->getAuthRealm() << LL_ENDL;
		}
		break;

		case MEDIA_EVENT_LINK_HOVERED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_LINK_HOVERED, hover text is: " << self->getHoverText() << LL_ENDL;
		};
		break;
	};
}

// TODO: observer
/*
void LLViewerParcelMediaNavigationObserver::onNavigateComplete( const EventType& event_in )
{
	std::string url = event_in.getStringValue();

	if (mCurrentURL != url && ! mFromMessage)
	{
		LLViewerParcelMedia::sendMediaNavigateMessage(url);
	}

	mCurrentURL = url;
	mFromMessage = false;

}
*/
