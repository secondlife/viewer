/** 
 * @file llviewerparcelmedia.cpp
 * @author Callum Prentice & James Cook
 * @brief Handlers for multimedia on a per-parcel basis
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#include "llviewerprecompiledheaders.h"
#include "llviewerparcelmedia.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerregion.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "lluuid.h"
#include "message.h"
#include "llviewerparcelmediaautoplay.h"
#include "llfirstuse.h"

// Static Variables

S32 LLViewerParcelMedia::sMediaParcelLocalID = 0;
LLUUID LLViewerParcelMedia::sMediaRegionID;

// Move this to its own file.
// helper class that tries to download a URL from a web site and calls a method 
// on the Panel Land Media and to discover the MIME type
class LLMimeDiscoveryResponder : public LLHTTPClient::Responder
{
public:
	LLMimeDiscoveryResponder( ) 
	  {}



	  virtual void completedHeader(U32 status, const std::string& reason, const LLSD& content)
	  {
		  std::string media_type = content["content-type"].asString();
		  std::string::size_type idx1 = media_type.find_first_of(";");
		  std::string mime_type = media_type.substr(0, idx1);
		  completeAny(status, mime_type);
	  }

	  virtual void error( U32 status, const std::string& reason )
	  {
		  completeAny(status, "none/none");
	  }

	  void completeAny(U32 status, const std::string& mime_type)
	  {
		  LLViewerMedia::setMimeType(mime_type);
	  }
};

// static
void LLViewerParcelMedia::initClass()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->setHandlerFunc("ParcelMediaCommandMessage", processParcelMediaCommandMessage );
	msg->setHandlerFunc("ParcelMediaUpdate", processParcelMediaUpdate );
	LLViewerParcelMediaAutoPlay::initClass();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerParcelMedia::update(LLParcel* parcel)
{
	if (/*LLViewerMedia::hasMedia()*/ true)
	{
		// we have a player
		if (parcel)
		{
			// we're in a parcel
			bool new_parcel = false;
			S32 parcelid = parcel->getLocalID();
			LLUUID regionid = gAgent.getRegion()->getRegionID();
			if (parcelid != sMediaParcelLocalID || regionid != sMediaRegionID)
			{
				sMediaParcelLocalID = parcelid;
				sMediaRegionID = regionid;
				new_parcel = true;
			}

			std::string mediaUrl = std::string ( parcel->getMediaURL () );
			LLString::trim(mediaUrl);

			// has something changed?
			if (  ( LLViewerMedia::getMediaURL() != mediaUrl )
				|| ( LLViewerMedia::getMediaTextureID() != parcel->getMediaID () ) )
			{
				bool video_was_playing = FALSE;
				bool same_media_id = LLViewerMedia::getMediaTextureID() == parcel->getMediaID ();

				if (LLViewerMedia::isMediaPlaying())
				{
					video_was_playing = TRUE;
				}

				if ( !mediaUrl.empty() && same_media_id && ! new_parcel)
				{
					// Someone has "changed the channel", changing the URL of a video
					// you were already watching.  Automatically play provided the texture ID is the same
					if (video_was_playing)
					{
						// Poke the mime type in before calling play.
						// This is necessary because in this instance we are not waiting
						// for the results of a header curl.  In order to change the channel
						// a mime type MUST be provided.
						LLViewerMedia::setMimeType(parcel->getMediaType());
						play(parcel);
					}
				}
				else
				{
					stop();
				}

				// Discover the MIME type
				// Disabled for the time being.  Get the mime type from the parcel.
				if(gSavedSettings.getBOOL("AutoMimeDiscovery"))
				{
					LLHTTPClient::getHeaderOnly( mediaUrl, new LLMimeDiscoveryResponder());
				}
				else
				{
					LLViewerMedia::setMimeType(parcel->getMediaType());
				}

			}
		}
		else
		{
			stop();
		}
	}
	/*
	else
	{
		// no audio player, do a first use dialog if their is media here
		if (parcel)
		{
			std::string mediaUrl = std::string ( parcel->getMediaURL () );
			if (!mediaUrl.empty ())
			{
				if (gSavedSettings.getWarning("QuickTimeInstalled"))
				{
					gSavedSettings.setWarning("QuickTimeInstalled", FALSE);

					LLNotifyBox::showXml("NoQuickTime" );
				};
			}
		}
	}
	*/
}

// static
void LLViewerParcelMedia::play(LLParcel* parcel)
{
	llinfos << "play" << llendl;
	
	if (!parcel) return;

	if (!gSavedSettings.getBOOL("AudioStreamingVideo"))
		return;
	
	std::string media_url = parcel->getMediaURL();
	std::string mime_type = parcel->getMediaType();
	LLUUID placeholder_texture_id = parcel->getMediaID();
	U8 media_auto_scale = parcel->getMediaAutoScale();
	U8 media_loop = parcel->getMediaLoop();
	S32 media_width = parcel->getMediaWidth();
	S32 media_height = parcel->getMediaHeight();
	LLViewerMedia::play(media_url, mime_type, placeholder_texture_id,
						media_width, media_height, media_auto_scale,
						media_loop);
	LLFirstUse::useMedia();

	LLViewerParcelMediaAutoPlay::playStarted();
}

// static
void LLViewerParcelMedia::stop()
{

	LLViewerMedia::stop();
}

// static
void LLViewerParcelMedia::pause()
{
	LLViewerMedia::pause();
}

// static
void LLViewerParcelMedia::start()
{
	LLViewerMedia::start();
	LLFirstUse::useMedia();

	LLViewerParcelMediaAutoPlay::playStarted();
}

// static
void LLViewerParcelMedia::seek(F32 time)
{
	LLViewerMedia::seek(time);
}


// static
LLMediaBase::EStatus LLViewerParcelMedia::getStatus()
{
	return LLViewerMedia::getStatus();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerParcelMedia::processParcelMediaCommandMessage( LLMessageSystem *msg, void ** )
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
		if( command == PARCEL_MEDIA_COMMAND_PLAY )
		{
			if (LLViewerMedia::isMediaPaused())
			{
				start();
			}
			else
			{
				LLParcel *parcel = gParcelMgr->getAgentParcel();
				play(parcel);
			}
		}
		else
		// loop
		if( command == PARCEL_MEDIA_COMMAND_LOOP )
		{
			//llinfos << ">>> LLMediaEngine::process_parcel_media with command = " <<( '0' + command ) << llendl;

			// huh? what is play?
			//convertImageAndLoadUrl( play );
			//convertImageAndLoadUrl( true, false, std::string() );
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
		if(! LLViewerMedia::hasMedia())
		{
			LLParcel *parcel = gParcelMgr->getAgentParcel();
			play(parcel);
		}
		seek(time);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerParcelMedia::processParcelMediaUpdate( LLMessageSystem *msg, void ** )
{
	LLUUID media_id;
	std::string media_url;
	std::string media_type;
	S32 media_width = 0;
	S32 media_height = 0;
	U8 media_auto_scale = FALSE;
	U8 media_loop = FALSE;
	
	msg->getUUID( "DataBlock", "MediaID", media_id );
	char media_url_buffer[257];
	msg->getString( "DataBlock", "MediaURL", 255, media_url_buffer );
	media_url = media_url_buffer;
	msg->getU8("DataBlock", "MediaAutoScale", media_auto_scale);
	
	if (msg->getNumberOfBlocks("DataBlockExtended")) // do we have the extended data?
	{
		char media_type_buffer[257];
		msg->getString("DataBlockExtended", "MediaType", 255, media_type_buffer);
		media_type = media_type_buffer;
		msg->getU8("DataBlockExtended", "MediaLoop", media_loop);
		msg->getS32("DataBlockExtended", "MediaWidth", media_width);
		msg->getS32("DataBlockExtended", "MediaHeight", media_height);
	}

	LLParcel *parcel = gParcelMgr->getAgentParcel();
	BOOL same = FALSE;
	if (parcel)
	{
		same = ((parcel->getMediaURL() == media_url) &&
				(parcel->getMediaType() == media_type) &&
				(parcel->getMediaID() == media_id) &&
				(parcel->getMediaWidth() == media_width) &&
				(parcel->getMediaHeight() == media_height) &&
				(parcel->getMediaAutoScale() == media_auto_scale) &&
				(parcel->getMediaLoop() == media_loop));
	}

	if (!same)
		LLViewerMedia::play(media_url, media_type, media_id,
							media_auto_scale, media_width, media_height,
							media_loop);
}
