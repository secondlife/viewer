/**
 * @file llviewermedia.cpp
 * @brief Client interface to the media engine
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llviewermedia.h"

#include "audioengine.h"

#include "llparcel.h"

#include "llmimetypes.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llversionviewer.h"
#include "llviewertexturelist.h"

#include "llevent.h"		// LLSimpleListener
#include "llmediamanager.h"
#include "lluuid.h"

#include <boost/bind.hpp>	// for SkinFolder listener
#include <boost/signals2.hpp>


// Implementation functions not exported into header file
class LLViewerMediaImpl
	:	public LLMediaObserver
{
public:
	LLViewerMediaImpl()
	:	mMediaSource( NULL ),
		mMovieImageID(),
		mMovieImageHasMips(false)
	{ }

	void destroyMediaSource();

    void play(const std::string& media_url,
			  const std::string& mime_type,
			  const LLUUID& placeholder_texture_id,
			  S32 media_width, S32 media_height, U8 media_auto_scale,
			  U8 media_loop);

	void stop();
	void pause();
	void start();
	void seek(F32 time);
	void setVolume(F32 volume);
	LLMediaBase::EStatus getStatus();

	/*virtual*/ void onMediaSizeChange(const EventType& event_in);
	/*virtual*/ void onMediaContentsChange(const EventType& event_in);

	void restoreMovieImage();
	void updateImagesMediaStreams();
	LLUUID getMediaTextureID();

	// Internally set our desired browser user agent string, including
	// the Second Life version and skin name.  Used because we can
	// switch skins without restarting the app.
	static void updateBrowserUserAgent();

	// Callback for when the SkinCurrent control is changed to
	// switch the user agent string to indicate the new skin.
	static bool handleSkinCurrentChanged(const LLSD& newvalue);

public:

	// a single media url with some data and an impl.
	LLMediaBase* mMediaSource;
	LLUUID mMovieImageID;
	bool  mMovieImageHasMips;
	std::string mMediaURL;
	std::string mMimeType;

private:
	void initializePlaceholderImage(LLViewerMediaTexture *placeholder_image, LLMediaBase *media_source);	
};

static LLViewerMediaImpl sViewerMediaImpl;
//////////////////////////////////////////////////////////////////////////////////////////

void LLViewerMediaImpl::destroyMediaSource()
{
	LLMediaManager* mgr = LLMediaManager::getInstance();
	if ( mMediaSource )
	{
		mMediaSource->remObserver(this);
		mgr->destroySource( mMediaSource );

		// Restore the texture
		restoreMovieImage();

	}
	mMediaSource = NULL;
}

void LLViewerMediaImpl::play(const std::string& media_url,
							 const std::string& mime_type,
							 const LLUUID& placeholder_texture_id,
							 S32 media_width, S32 media_height, U8 media_auto_scale,
							 U8 media_loop)
{
	// first stop any previously playing media
	stop();

	// Save this first, as init/load below may fire events
	mMovieImageID = placeholder_texture_id;

	// If the mime_type passed in is different than the cached one, and
	// Auto-discovery is turned OFF, replace the cached mime_type with the new one.
	if(mime_type != mMimeType &&
		! gSavedSettings.getBOOL("AutoMimeDiscovery"))
	{
		mMimeType = mime_type;
	}
	LLURI url(media_url);
	std::string scheme = url.scheme() != "" ? url.scheme() : "http";

	LLMediaManager* mgr = LLMediaManager::getInstance();
	mMediaSource = mgr->createSourceFromMimeType(scheme, mMimeType );
	if ( !mMediaSource )
	{
		if (mMimeType != "none/none")
		{
			llwarns << "media source create failed " << media_url
					<< " type " << mMimeType
					<< llendl;
		}
		return;
	}

	// Store the URL and Mime Type
	mMediaURL = media_url;

	if ((media_width != 0) && (media_height != 0))
	{
		mMediaSource->setRequestedMediaSize(media_width, media_height);
	}

	mMediaSource->setLooping(media_loop);
	mMediaSource->setAutoScaled(media_auto_scale);
	mMediaSource->addObserver( this );
	mMediaSource->navigateTo( media_url );
	mMediaSource->addCommand(LLMediaBase::COMMAND_START);
}

void LLViewerMediaImpl::stop()
{
	destroyMediaSource();
}

void LLViewerMediaImpl::pause()
{
	if(mMediaSource)
	{
		mMediaSource->addCommand(LLMediaBase::COMMAND_PAUSE);
	}
}

void LLViewerMediaImpl::start()
{
	if(mMediaSource)
	{
		mMediaSource->addCommand(LLMediaBase::COMMAND_START);
	}
}

void LLViewerMediaImpl::seek(F32 time)
{
	if(mMediaSource)
	{
		mMediaSource->seek(time);
	}
}

void LLViewerMediaImpl::setVolume(F32 volume)
{
	if(mMediaSource)
	{
		mMediaSource->setVolume( volume);
	}
}

LLMediaBase::EStatus LLViewerMediaImpl::getStatus()
{
	if (mMediaSource)
	{
		return mMediaSource->getStatus();
	}
	else
	{
		return LLMediaBase::STATUS_UNKNOWN;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::restoreMovieImage()
{
	// IF the media image hasn't changed, do nothing
	if (mMovieImageID.isNull())
	{
		return;
	}

	//restore the movie image to the old one
	LLViewerMediaTexture* media = LLViewerTextureManager::findMediaTexture( mMovieImageID ) ;
	if (media)
	{
		if(media->getOldTexture())//set back to the old texture if it exists
		{
			media->switchToTexture(media->getOldTexture()) ;
			media->setPlaying(FALSE) ;
		}
		media->reinit(mMovieImageHasMips);
	}
	mMovieImageID.setNull();
}


//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateImagesMediaStreams()
{
	LLMediaManager::updateClass();
}

void LLViewerMediaImpl::initializePlaceholderImage(LLViewerMediaTexture *placeholder_image, LLMediaBase *media_source)
{
	int media_width = media_source->getMediaWidth();
	int media_height = media_source->getMediaHeight();
	//int media_rowspan = media_source->getMediaRowSpan();

	// if width & height are invalid, don't bother doing anything
	if ( media_width < 1 || media_height < 1 )
		return;

	llinfos << "initializing media placeholder" << llendl;
	llinfos << "movie image id " << mMovieImageID << llendl;

	int texture_width = LLMediaManager::textureWidthFromMediaWidth( media_width );
	int texture_height = LLMediaManager::textureHeightFromMediaHeight( media_height );
	int texture_depth = media_source->getMediaDepth();

	// MEDIAOPT: check to see if size actually changed before doing work
	placeholder_image->destroyGLTexture();
	// MEDIAOPT: apparently just calling setUseMipMaps(FALSE) doesn't work?
	placeholder_image->reinit(FALSE);	// probably not needed

	// MEDIAOPT: seems insane that we actually have to make an imageraw then
	// immediately discard it
	LLPointer<LLImageRaw> raw = new LLImageRaw(texture_width, texture_height, texture_depth);
	raw->clear(0x0f, 0x0f, 0x0f, 0xff);
	int discard_level = 0;

	// ask media source for correct GL image format constants
	placeholder_image->setExplicitFormat(media_source->getTextureFormatInternal(),
										 media_source->getTextureFormatPrimary(),
										 media_source->getTextureFormatType());

	placeholder_image->createGLTexture(discard_level, raw);

	// placeholder_image->setExplicitFormat()
	placeholder_image->setUseMipMaps(FALSE);
}

// virtual
void LLViewerMediaImpl::onMediaContentsChange(const EventType& event_in)
{
	LLMediaBase* media_source = event_in.getSubject();
	LLViewerMediaTexture* placeholder_image = LLViewerTextureManager::getMediaTexture( mMovieImageID ) ;
	if (placeholder_image && placeholder_image->hasValidGLTexture())
	{
		if (placeholder_image->getUseMipMaps())
		{
			// bad image!  NO MIPMAPS!
			initializePlaceholderImage(placeholder_image, media_source);
		}

		U8* data = media_source->getMediaData();
		S32 x_pos = 0;
		S32 y_pos = 0;
		S32 width = media_source->getMediaWidth();
		S32 height = media_source->getMediaHeight();
		S32 data_width = media_source->getMediaDataWidth();
		S32 data_height = media_source->getMediaDataHeight();
		placeholder_image->setSubImage(data, data_width, data_height,
			x_pos, y_pos, width, height);
	}
}


// virtual
void LLViewerMediaImpl::onMediaSizeChange(const EventType& event_in)
{
	LLMediaBase* media_source = event_in.getSubject();
	LLViewerMediaTexture* placeholder_image = LLViewerTextureManager::getMediaTexture( mMovieImageID ) ;
	if (placeholder_image)
	{
		initializePlaceholderImage(placeholder_image, media_source);
	}
	else
	{
		llinfos << "no placeholder image" << llendl;
	}
}

LLUUID LLViewerMediaImpl::getMediaTextureID()
{
	return mMovieImageID;
}

// static
void LLViewerMediaImpl::updateBrowserUserAgent()
{
	// Don't use user-visible string to avoid 
	// punctuation and strange characters.
	std::string skin_name = gSavedSettings.getString("SkinCurrent");

	// Just in case we need to check browser differences in A/B test
	// builds.
	std::string channel = gSavedSettings.getString("VersionChannelName");

	// append our magic version number string to the browser user agent id
	// See the HTTP 1.0 and 1.1 specifications for allowed formats:
	// http://www.ietf.org/rfc/rfc1945.txt section 10.15
	// http://www.ietf.org/rfc/rfc2068.txt section 3.8
	// This was also helpful:
	// http://www.mozilla.org/build/revised-user-agent-strings.html
	std::ostringstream codec;
	codec << "SecondLife/";
	codec << LL_VERSION_MAJOR << "." << LL_VERSION_MINOR << "." << LL_VERSION_PATCH << "." << LL_VERSION_BUILD;
	codec << " (" << channel << "; " << skin_name << " skin)";
	llinfos << codec.str() << llendl;
	LLMediaManager::setBrowserUserAgent( codec.str() );
}

// static
bool LLViewerMediaImpl::handleSkinCurrentChanged(const LLSD& /*newvalue*/)
{
	// gSavedSettings is already updated when this function is called.
	updateBrowserUserAgent();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Wrapper class
//////////////////////////////////////////////////////////////////////////////////////////

S32 LLViewerMedia::mMusicState = LLViewerMedia::STOPPED;
//////////////////////////////////////////////////////////////////////////////////////////
// The viewer takes a long time to load the start screen.  Part of the problem
// is media initialization -- in particular, QuickTime loads many DLLs and
// hits the disk heavily.  So we initialize only the browser component before
// the login screen, then do the rest later when we have a progress bar. JC
// static
void LLViewerMedia::initBrowser()
{
	LLMediaManagerData* init_data = new LLMediaManagerData;
	buildMediaManagerData( init_data );
	LLMediaManager::initBrowser( init_data );
	delete init_data;

	// We use a custom user agent with viewer version and skin name.
	LLViewerMediaImpl::updateBrowserUserAgent();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::initClass()
{
	// *TODO: This looks like a memory leak to me. JC
	LLMediaManagerData* init_data = new LLMediaManagerData;
	buildMediaManagerData( init_data );
	LLMediaManager::initClass( init_data );
	delete init_data;

	LLMediaManager* mm = LLMediaManager::getInstance();
	LLMIMETypes::mime_info_map_t::const_iterator it;
	for (it = LLMIMETypes::sMap.begin(); it != LLMIMETypes::sMap.end(); ++it)
	{
		const std::string& mime_type = it->first;
		const LLMIMETypes::LLMIMEInfo& info = it->second;
		mm->addMimeTypeImplNameMap( mime_type, info.mImpl );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::buildMediaManagerData( LLMediaManagerData* init_data )
{
//	std::string executable_dir = std::string( arg0 ).substr( 0, std::string( arg0 ).find_last_of("\\/") );
//	std::string component_dir = std::string( executable_dir ).substr( 0, std::string( executable_dir ).find_last_of("\\/") );
//	component_dir = std::string( component_dir ).substr( 0, std::string( component_dir ).find_last_of("\\/") );
//	component_dir = std::string( component_dir ).substr( 0, std::string( component_dir ).find_last_of("\\/") );
//	component_dir += "\\newview\\app_settings\\mozilla";


#if LL_DARWIN
	// For Mac OS, we store both the shared libraries and the runtime files (chrome/, plugins/, etc) in
	// Second Life.app/Contents/MacOS/.  This matches the way Firefox is distributed on the Mac.
	std::string component_dir(gDirUtilp->getExecutableDir());
#elif LL_WINDOWS
	std::string component_dir( gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "" ) );
	component_dir += gDirUtilp->getDirDelimiter();
  #ifdef LL_DEBUG
	component_dir += "mozilla_debug";
  #else // LL_DEBUG
	component_dir += "mozilla";
  #endif // LL_DEBUG
#elif LL_LINUX
	std::string component_dir( gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "" ) );
	component_dir += gDirUtilp->getDirDelimiter();
	component_dir += "mozilla-runtime-linux-i686";
#elif LL_SOLARIS
	std::string component_dir( gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "" ) );
	component_dir += gDirUtilp->getDirDelimiter();
	#ifdef  __sparc
		component_dir += "mozilla-solaris-sparc";
	#else
		component_dir += "mozilla-solaris-i686";
	#endif
#else
	std::string component_dir( gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "" ) );
	component_dir += gDirUtilp->getDirDelimiter();
	component_dir += "mozilla";
#endif

	std::string application_dir = gDirUtilp->getExecutableDir();

	init_data->setBrowserApplicationDir( application_dir );
	std::string profile_dir = gDirUtilp->getExpandedFilename( LL_PATH_MOZILLA_PROFILE, "" );
	init_data->setBrowserProfileDir( profile_dir );
	init_data->setBrowserComponentDir( component_dir );
	std::string profile_name("Second Life");
	init_data->setBrowserProfileName( profile_name );
	init_data->setBrowserParentWindow( gViewerWindow->getMediaWindow() );

	// Users can change skins while client is running, so make sure
	// we pick up on changes.
	gSavedSettings.getControl("SkinCurrent")->getSignal()->connect( 
		boost::bind( LLViewerMediaImpl::handleSkinCurrentChanged, _2 ) );

}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::cleanupClass()
{
	stop() ;
	LLMediaManager::cleanupClass();
}

// static
void LLViewerMedia::play(const std::string& media_url,
						 const std::string& mime_type,
						 const LLUUID& placeholder_texture_id,
						 S32 media_width, S32 media_height, U8 media_auto_scale,
						 U8 media_loop)
{
	sViewerMediaImpl.play(media_url, mime_type, placeholder_texture_id,
						  media_width, media_height, media_auto_scale, media_loop);
}

// static
void LLViewerMedia::stop()
{
	sViewerMediaImpl.stop();
}

// static
void LLViewerMedia::pause()
{
	sViewerMediaImpl.pause();
}

// static
void LLViewerMedia::start()
{
	sViewerMediaImpl.start();
}

// static
void LLViewerMedia::seek(F32 time)
{
	sViewerMediaImpl.seek(time);
}

// static
void LLViewerMedia::setVolume(F32 volume)
{
	sViewerMediaImpl.setVolume(volume);
}

// static
LLMediaBase::EStatus LLViewerMedia::getStatus()
{
	return sViewerMediaImpl.getStatus();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
LLUUID LLViewerMedia::getMediaTextureID()
{
	return sViewerMediaImpl.getMediaTextureID();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::getMediaSize(S32 *media_width, S32 *media_height)
{
	// make sure we're valid

	if ( sViewerMediaImpl.mMediaSource != NULL )
	{
		*media_width = sViewerMediaImpl.mMediaSource->getMediaWidth();
		*media_height = sViewerMediaImpl.mMediaSource->getMediaHeight();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::getTextureSize(S32 *texture_width, S32 *texture_height)
{
	if ( sViewerMediaImpl.mMediaSource != NULL )
	{
		S32 media_width = sViewerMediaImpl.mMediaSource->getMediaWidth();
		S32 media_height = sViewerMediaImpl.mMediaSource->getMediaHeight();
		*texture_width = LLMediaManager::textureWidthFromMediaWidth( media_width );
		*texture_height = LLMediaManager::textureHeightFromMediaHeight( media_height );
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::updateImagesMediaStreams()
{
	sViewerMediaImpl.updateImagesMediaStreams();
}
//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::isMediaPlaying()
{
	LLMediaBase::EStatus status = sViewerMediaImpl.getStatus();
	return (status == LLMediaBase::STATUS_STARTED );
}
//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::isMediaPaused()
{
	LLMediaBase::EStatus status = sViewerMediaImpl.getStatus();
	return (status == LLMediaBase::STATUS_PAUSED);
}
//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::hasMedia()
{
	return sViewerMediaImpl.mMediaSource != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
bool LLViewerMedia::isActiveMediaTexture(const LLUUID& id)
{
	return (id.notNull()
		&& id == getMediaTextureID()
		&& isMediaPlaying());
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
bool LLViewerMedia::isMusicPlaying()
{
	return mMusicState == PLAYING; 
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
std::string LLViewerMedia::getMediaURL()
{
	return sViewerMediaImpl.mMediaURL;
}
//////////////////////////////////////////////////////////////////////////////////////////
// static
std::string LLViewerMedia::getMimeType()
{
	return sViewerMediaImpl.mMimeType;
}
//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::setMimeType(std::string mime_type)
{
	sViewerMediaImpl.mMimeType = mime_type;
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
void LLViewerMedia::toggleMusicPlay(void*)
{
	if (mMusicState != PLAYING)
	{
		mMusicState = PLAYING; // desired state
		if (gAudiop)
		{
			LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
			if ( parcel )
			{
				gAudiop->startInternetStream(parcel->getMusicURL());
			}
		}
	}
	else
	{
		mMusicState = STOPPED; // desired state
		if (gAudiop)
		{
			gAudiop->stopInternetStream();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
void LLViewerMedia::toggleMediaPlay(void*)
{
	if (LLViewerMedia::isMediaPaused())
	{
		LLViewerParcelMedia::start();
	}
	else if(LLViewerMedia::isMediaPlaying())
	{
		LLViewerParcelMedia::pause();
	}
	else
	{
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		if (parcel)
		{
			LLViewerParcelMedia::play(parcel);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
void LLViewerMedia::mediaStop(void*)
{
	LLViewerParcelMedia::stop();
}
