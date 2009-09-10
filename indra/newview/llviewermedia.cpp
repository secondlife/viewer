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
#include "llviewermediafocus.h"
#include "llmimetypes.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llversionviewer.h"
#include "llviewertexturelist.h"
#include "llpluginclassmedia.h"

#include "llevent.h"		// LLSimpleListener
#include "llnotifications.h"
#include "lluuid.h"

#include <boost/bind.hpp>	// for SkinFolder listener
#include <boost/signals2.hpp>

// Move this to its own file.

LLViewerMediaEventEmitter::~LLViewerMediaEventEmitter()
{
	observerListType::iterator iter = mObservers.begin();

	while( iter != mObservers.end() )
	{
		LLViewerMediaObserver *self = *iter;
		iter++;
		remObserver(self);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaEventEmitter::addObserver( LLViewerMediaObserver* observer )
{
	if ( ! observer )
		return false;

	if ( std::find( mObservers.begin(), mObservers.end(), observer ) != mObservers.end() )
		return false;

	mObservers.push_back( observer );
	observer->mEmitters.push_back( this );

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaEventEmitter::remObserver( LLViewerMediaObserver* observer )
{
	if ( ! observer )
		return false;

	mObservers.remove( observer );
	observer->mEmitters.remove(this);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void LLViewerMediaEventEmitter::emitEvent( LLPluginClassMedia* media, LLPluginClassMediaOwner::EMediaEvent event )
{
	observerListType::iterator iter = mObservers.begin();

	while( iter != mObservers.end() )
	{
		LLViewerMediaObserver *self = *iter;
		++iter;
		self->handleMediaEvent( media, event );
	}
}

// Move this to its own file.
LLViewerMediaObserver::~LLViewerMediaObserver()
{
	std::list<LLViewerMediaEventEmitter *>::iterator iter = mEmitters.begin();

	while( iter != mEmitters.end() )
	{
		LLViewerMediaEventEmitter *self = *iter;
		iter++;
		self->remObserver( this );
	}
}


// Move this to its own file.
// helper class that tries to download a URL from a web site and calls a method
// on the Panel Land Media and to discover the MIME type
class LLMimeDiscoveryResponder : public LLHTTPClient::Responder
{
LOG_CLASS(LLMimeDiscoveryResponder);
public:
	LLMimeDiscoveryResponder( viewer_media_t media_impl)
		: mMediaImpl(media_impl),
		  mInitialized(false)
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
		// completeAny(status, "none/none");
	}

	void completeAny(U32 status, const std::string& mime_type)
	{
		if(!mInitialized && ! mime_type.empty())
		{
			if (mMediaImpl->initializeMedia(mime_type))
			{
				mInitialized = true;
				mMediaImpl->play();
			}
		}
	}

	public:
		viewer_media_t mMediaImpl;
		bool mInitialized;
};
typedef std::vector<LLViewerMediaImpl*> impl_list;
static impl_list sViewerMediaImplList;

//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMedia

//////////////////////////////////////////////////////////////////////////////////////////
// static
viewer_media_t LLViewerMedia::newMediaImpl(const std::string& media_url,
														 const LLUUID& texture_id,
														 S32 media_width, S32 media_height, U8 media_auto_scale,
														 U8 media_loop,
														 std::string mime_type)
{
	LLViewerMediaImpl* media_impl = getMediaImplFromTextureID(texture_id);
	if(media_impl == NULL || texture_id.isNull())
	{
		// Create the media impl
		media_impl = new LLViewerMediaImpl(media_url, texture_id, media_width, media_height, media_auto_scale, media_loop, mime_type);
		sViewerMediaImplList.push_back(media_impl);
	}
	else
	{
		media_impl->stop();
		media_impl->mTextureId = texture_id;
		media_impl->mMediaURL = media_url;
		media_impl->mMediaWidth = media_width;
		media_impl->mMediaHeight = media_height;
		media_impl->mMediaAutoScale = media_auto_scale;
		media_impl->mMediaLoop = media_loop;
		if(! media_url.empty())
			media_impl->navigateTo(media_url, mime_type, true);
	}
	return media_impl;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::removeMedia(LLViewerMediaImpl* media)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	
	for(; iter != end; iter++)
	{
		if(media == *iter)
		{
			sViewerMediaImplList.erase(iter);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
LLViewerMediaImpl* LLViewerMedia::getMediaImplFromTextureID(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* media_impl = *iter;
		if(media_impl->getMediaTextureID() == texture_id)
		{
			return media_impl;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
std::string LLViewerMedia::getCurrentUserAgent()
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
	
	return codec.str();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::updateBrowserUserAgent()
{
	std::string user_agent = getCurrentUserAgent();
	
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->mMediaSource && pimpl->mMediaSource->pluginSupportsMediaBrowser())
		{
			pimpl->mMediaSource->setBrowserUserAgent(user_agent);
		}
	}

}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::handleSkinCurrentChanged(const LLSD& /*newvalue*/)
{
	// gSavedSettings is already updated when this function is called.
	updateBrowserUserAgent();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::textureHasMedia(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->getMediaTextureID() == texture_id)
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::setVolume(F32 volume)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->setVolume(volume);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::updateMedia()
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->update();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::cleanupClass()
{
	// This is no longer necessary, since the list is no longer smart pointers.
#if 0
	while(!sViewerMediaImplList.empty())
	{
		sViewerMediaImplList.pop_back();
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMediaImpl
//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::LLViewerMediaImpl(const std::string& media_url, 
										  const LLUUID& texture_id, 
										  S32 media_width, 
										  S32 media_height, 
										  U8 media_auto_scale, 
										  U8 media_loop,
										  const std::string& mime_type)
:	
	mMediaSource( NULL ),
	mMovieImageHasMips(false),
	mTextureId(texture_id),
	mMediaWidth(media_width),
	mMediaHeight(media_height),
	mMediaAutoScale(media_auto_scale),
	mMediaLoop(media_loop),
	mMediaURL(media_url),
	mMimeType(mime_type),
	mNeedsNewTexture(true),
	mSuspendUpdates(false),
	mVisible(true)
{ 
	createMediaSource();
}

//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::~LLViewerMediaImpl()
{
	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}
	
	destroyMediaSource();
	LLViewerMedia::removeMedia(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializeMedia(const std::string& mime_type)
{
	if((mMediaSource == NULL) || (mMimeType != mime_type))
	{
		if(! initializePlugin(mime_type))
		{
			LL_WARNS("Plugin") << "plugin intialization failed for mime type: " << mime_type << LL_ENDL;
			LLSD args;
			args["MIME_TYPE"] = mime_type;
			LLNotifications::instance().add("NoPlugin", args);

			return false;
		}
	}

	// play();
	return (mMediaSource != NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::createMediaSource()
{
	if(! mMediaURL.empty())
	{
		navigateTo(mMediaURL, mMimeType, true);
	}
	else if(! mMimeType.empty())
	{
		initializeMedia(mMimeType);
	}
	
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::destroyMediaSource()
{
	mNeedsNewTexture = true;
	if(! mMediaSource)
	{
		return;
	}
	// Restore the texture
	updateMovieImage(LLUUID::null, false);
	delete mMediaSource;
	mMediaSource = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setMediaType(const std::string& media_type)
{
	mMimeType = media_type;
}

//////////////////////////////////////////////////////////////////////////////////////////
/*static*/
LLPluginClassMedia* LLViewerMediaImpl::newSourceFromMediaType(std::string media_type, LLPluginClassMediaOwner *owner /* may be NULL */, S32 default_width, S32 default_height)
{
	std::string plugin_basename = LLMIMETypes::implType(media_type);

	if(plugin_basename.empty())
	{
		LL_WARNS("Media") << "Couldn't find plugin for media type " << media_type << LL_ENDL;
	}
	else
	{
		std::string plugins_path = gDirUtilp->getLLPluginDir();
		plugins_path += gDirUtilp->getDirDelimiter();
		
		std::string launcher_name = gDirUtilp->getLLPluginLauncher();
		std::string plugin_name = gDirUtilp->getLLPluginFilename(plugin_basename);

		// See if the plugin executable exists
		llstat s;
		if(LLFile::stat(launcher_name, &s))
		{
			LL_WARNS("Media") << "Couldn't find launcher at " << launcher_name << LL_ENDL;
		}
		else if(LLFile::stat(plugin_name, &s))
		{
			LL_WARNS("Media") << "Couldn't find plugin at " << plugin_name << LL_ENDL;
		}
		else
		{
			LLPluginClassMedia* media_source = new LLPluginClassMedia(owner);
			media_source->setSize(default_width, default_height);
			if (media_source->init(launcher_name, plugin_name))
			{
				return media_source;
			}
			else
			{
				LL_WARNS("Media") << "Failed to init plugin.  Destroying." << LL_ENDL;
				delete media_source;
			}
		}
	}
	
	return NULL;
}							

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializePlugin(const std::string& media_type)
{
	if(mMediaSource)
	{
		// Save the previous media source's last set size before destroying it.
		mMediaWidth = mMediaSource->getSetWidth();
		mMediaHeight = mMediaSource->getSetHeight();
	}
	
	// Always delete the old media impl first.
	destroyMediaSource();
	
	// and unconditionally set the mime type
	mMimeType = media_type;

	LLPluginClassMedia* media_source = newSourceFromMediaType(media_type, this, mMediaWidth, mMediaHeight);
	
	if (media_source)
	{
		media_source->setDisableTimeout(gSavedSettings.getBOOL("DebugPluginDisableTimeout"));
		media_source->setLoop(mMediaLoop);
		media_source->setAutoScale(mMediaAutoScale);
		media_source->setBrowserUserAgent(LLViewerMedia::getCurrentUserAgent());
		
		mMediaSource = media_source;
		return true;
	}

	return false;
}

void LLViewerMediaImpl::setSize(int width, int height)
{
	mMediaWidth = width;
	mMediaHeight = height;
	if(mMediaSource)
	{
		mMediaSource->setSize(width, height);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::play()
{
	// first stop any previously playing media
	// stop();

	// mMediaSource->addObserver( this );
	if(mMediaSource == NULL)
	{
	 	if(!initializePlugin(mMimeType))
		{
			// Plugin failed initialization... should assert or something
			return;
		}
	}
	
	// updateMovieImage(mTextureId, true);

	mMediaSource->loadURI( mMediaURL );
	if(/*mMediaSource->pluginSupportsMediaTime()*/ true)
	{
		start();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::stop()
{
	if(mMediaSource)
	{
		mMediaSource->stop();
		// destroyMediaSource();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::pause()
{
	if(mMediaSource)
	{
		mMediaSource->pause();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::start()
{
	if(mMediaSource)
	{
		mMediaSource->start();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::seek(F32 time)
{
	if(mMediaSource)
	{
		mMediaSource->seek(time);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVolume(F32 volume)
{
	if(mMediaSource)
	{
		mMediaSource->setVolume(volume);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::focus(bool focus)
{
	if (mMediaSource)
	{
		// call focus just for the hell of it, even though this apopears to be a nop
		mMediaSource->focus(focus);
		if (focus)
		{
			// spoof a mouse click to *actually* pass focus
			// Don't do this anymore -- it actually clicks through now.
//			mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, 1, 1, 0);
//			mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 1, 1, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDown(S32 x, S32 y)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, x, y, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseUp(S32 x, S32 y)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, x, y, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseMove(S32 x, S32 y)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_MOVE, x, y, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseLeftDoubleClick(S32 x, S32 y)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOUBLE_CLICK, x, y, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::onMouseCaptureLost()
{
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, mLastMouseX, mLastMouseY, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
BOOL LLViewerMediaImpl::handleMouseUp(S32 x, S32 y, MASK mask) 
{ 
	// NOTE: this is called when the mouse is released when we have capture.
	// Due to the way mouse coordinates are mapped to the object, we can't use the x and y coordinates that come in with the event.
	
	if(hasMouseCapture())
	{
		// Release the mouse -- this will also send a mouseup to the media
		gFocusMgr.setMouseCapture( FALSE );
	}

	return TRUE; 
}
//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateHome()
{
	if(mMediaSource)
	{
		mMediaSource->loadURI( mHomeURL );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateTo(const std::string& url, const std::string& mime_type,  bool rediscover_type)
{
	if(rediscover_type)
	{

		LLURI uri(url);
		std::string scheme = uri.scheme();

		if(scheme.empty() || "http" == scheme || "https" == scheme)
		{
			LLHTTPClient::getHeaderOnly( url, new LLMimeDiscoveryResponder(this));
		}
		else if("data" == scheme || "file" == scheme || "about" == scheme)
		{
			// FIXME: figure out how to really discover the type for these schemes
			// We use "data" internally for a text/html url for loading the login screen
			if(initializeMedia("text/html"))
			{
				mMediaSource->loadURI( url );
			}
		}
		else
		{
			// This catches 'rtsp://' urls
			if(initializeMedia(scheme))
			{
				mMediaSource->loadURI( url );
			}
		}
	}
	else if (mMediaSource)
	{
		mMediaSource->loadURI( url );
	}
	else if(initializeMedia(mime_type) && mMediaSource)
	{
		mMediaSource->loadURI( url );
	}
	else
	{
		LL_WARNS("Media") << "Couldn't navigate to: " << url << " as there is no media type for: " << mime_type << LL_ENDL;
		return;
	}
	mMediaURL = url;

}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateStop()
{
	if(mMediaSource)
	{
		mMediaSource->browse_stop();
	}

}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleKeyHere(KEY key, MASK mask)
{
	bool result = false;
	
	if (mMediaSource)
	{
		result = mMediaSource->keyEvent(LLPluginClassMedia::KEY_EVENT_DOWN ,key, mask);
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleUnicodeCharHere(llwchar uni_char)
{
	bool result = false;
	
	if (mMediaSource)
	{
		mMediaSource->textInput(wstring_to_utf8str(LLWString(1, uni_char)));
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateForward()
{
	BOOL result = FALSE;
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryForwardAvailable();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateBack()
{
	BOOL result = FALSE;
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryBackAvailable();
	}
	return result;
}


//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateMovieImage(const LLUUID& uuid, BOOL active)
{
	// IF the media image hasn't changed, do nothing
	if (mTextureId == uuid)
	{
		return;
	}
	// If we have changed media uuid, restore the old one
	if (!mTextureId.isNull())
	{
		LLViewerMediaTexture* old_image = LLViewerTextureManager::findMediaTexture( mTextureId );
		if (old_image)
		{
			old_image->setPlaying(FALSE);
			LLViewerTexture* original_texture = old_image->getOldTexture();
			if(original_texture)
			{
				old_image->switchToTexture(original_texture);
			}
		}
	}
	// If the movie is playing, set the new media image
	if (active && !uuid.isNull())
	{
		LLViewerMediaTexture* viewerImage = LLViewerTextureManager::findMediaTexture( uuid );
		if( viewerImage )
		{
			mTextureId = uuid;
			
			// Can't use mipmaps for movies because they don't update the full image
			mMovieImageHasMips = viewerImage->getUseMipMaps();
			viewerImage->reinit(FALSE);
			// FIXME
//			viewerImage->mIsMediaTexture = TRUE;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::update()
{
	if(mMediaSource == NULL)
	{
		return;
	}
	
	mMediaSource->idle();
	
	if(mMediaSource->isPluginExited())
	{
		destroyMediaSource();
		return;
	}

	if(!mMediaSource->textureValid())
	{
		return;
	}
	
	if(mSuspendUpdates || !mVisible)
	{
		return;
	}
	
	LLViewerMediaTexture* placeholder_image = updatePlaceholderImage();
		
	if(placeholder_image)
	{
		LLRect dirty_rect;
		if(mMediaSource->getDirty(&dirty_rect))
		{
			// Constrain the dirty rect to be inside the texture
			S32 x_pos = llmax(dirty_rect.mLeft, 0);
			S32 y_pos = llmax(dirty_rect.mBottom, 0);
			S32 width = llmin(dirty_rect.mRight, placeholder_image->getWidth()) - x_pos;
			S32 height = llmin(dirty_rect.mTop, placeholder_image->getHeight()) - y_pos;
			
			if(width > 0 && height > 0)
			{

				U8* data = mMediaSource->getBitsData();

				// Offset the pixels pointer to match x_pos and y_pos
				data += ( x_pos * mMediaSource->getTextureDepth() * mMediaSource->getBitsWidth() );
				data += ( y_pos * mMediaSource->getTextureDepth() );
				
				placeholder_image->setSubImage(
						data, 
						mMediaSource->getBitsWidth(), 
						mMediaSource->getBitsHeight(),
						x_pos, 
						y_pos, 
						width, 
						height);

			}
			
			mMediaSource->resetDirty();
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateImagesMediaStreams()
{
}


//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaTexture* LLViewerMediaImpl::updatePlaceholderImage()
{
	if(mTextureId.isNull())
	{
		// The code that created this instance will read from the plugin's bits.
		return NULL;
	}
	
	LLViewerMediaTexture* placeholder_image = LLViewerTextureManager::getMediaTexture( mTextureId );
	
	if (mNeedsNewTexture 
		|| placeholder_image->getUseMipMaps()
//		|| ! placeholder_image->getType() == LLViewerTexture::MEDIA_TEXTURE
		|| placeholder_image->getWidth() != mMediaSource->getTextureWidth()
		|| placeholder_image->getHeight() != mMediaSource->getTextureHeight())
	{
		llinfos << "initializing media placeholder" << llendl;
		llinfos << "movie image id " << mTextureId << llendl;

		int texture_width = mMediaSource->getTextureWidth();
		int texture_height = mMediaSource->getTextureHeight();
		int texture_depth = mMediaSource->getTextureDepth();
		
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
		placeholder_image->setExplicitFormat(mMediaSource->getTextureFormatInternal(),
											 mMediaSource->getTextureFormatPrimary(),
											 mMediaSource->getTextureFormatType(),
											 mMediaSource->getTextureFormatSwapBytes());

		placeholder_image->createGLTexture(discard_level, raw);

		// placeholder_image->setExplicitFormat()
		placeholder_image->setUseMipMaps(FALSE);

		// MEDIAOPT: set this dynamically on play/stop
		// FIXME
//		placeholder_image->mIsMediaTexture = true;
		mNeedsNewTexture = false;
	}
	
	return placeholder_image;
}


//////////////////////////////////////////////////////////////////////////////////////////
LLUUID LLViewerMediaImpl::getMediaTextureID()
{
	return mTextureId;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVisible(bool visible)
{
	mVisible = visible;
	
	if(mVisible)
	{
		if(mMediaSource && mMediaSource->isPluginExited())
		{
			destroyMediaSource();
		}
		
		if(!mMediaSource)
		{
			createMediaSource();
		}
	}
	
	if(mMediaSource)
	{
		mMediaSource->setPriority(mVisible?LLPluginClassMedia::PRIORITY_NORMAL:LLPluginClassMedia::PRIORITY_HIDDEN);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseCapture()
{
	gFocusMgr.setMouseCapture(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scaleMouse(S32 *mouse_x, S32 *mouse_y)
{
#if 0
	S32 media_width, media_height;
	S32 texture_width, texture_height;
	getMediaSize( &media_width, &media_height );
	getTextureSize( &texture_width, &texture_height );
	S32 y_delta = texture_height - media_height;

	*mouse_y -= y_delta;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPlaying()
{
	bool result = false;
	
	if(mMediaSource)
	{
		EMediaStatus status = mMediaSource->getStatus();
		if(status == MEDIA_PLAYING || status == MEDIA_LOADING)
			result = true;
	}
	
	return result;
}
//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPaused()
{
	bool result = false;

	if(mMediaSource)
	{
		if(mMediaSource->getStatus() == MEDIA_PAUSED)
			result = true;
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::hasMedia()
{
	return mMediaSource != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::handleMediaEvent(LLPluginClassMedia* self, LLPluginClassMediaOwner::EMediaEvent event)
{
	switch(event)
	{
		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mMimeType);
			LLNotifications::instance().add("MediaPluginFailed", args);
		}
		break;
		default:
		break;
	}
	// Just chain the event to observers.
	emitEvent(self, event);
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::cut()
{
	if (mMediaSource)
		mMediaSource->cut();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL
LLViewerMediaImpl::canCut() const
{
	if (mMediaSource)
		return mMediaSource->canCut();
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::copy()
{
	if (mMediaSource)
		mMediaSource->copy();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL
LLViewerMediaImpl::canCopy() const
{
	if (mMediaSource)
		return mMediaSource->canCopy();
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::paste()
{
	if (mMediaSource)
		mMediaSource->paste();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL
LLViewerMediaImpl::canPaste() const
{
	if (mMediaSource)
		return mMediaSource->canPaste();
	else
		return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////////
//static
void LLViewerMedia::toggleMusicPlay(void*)
{
// FIXME: This probably doesn't belong here
#if 0
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
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
void LLViewerMedia::toggleMediaPlay(void*)
{
// FIXME: This probably doesn't belong here
#if 0
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
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
void LLViewerMedia::mediaStop(void*)
{
// FIXME: This probably doesn't belong here
#if 0
	LLViewerParcelMedia::stop();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//static 
bool LLViewerMedia::isMusicPlaying()
{	
// FIXME: This probably doesn't belong here
// FIXME: make this work
	return false;	
}
