/** 
 * @file llviewermedia_streamingaudio.h
 * @author Tofu Linden, Sam Kolb
 * @brief LLStreamingAudio_MediaPlugins implementation - an implementation of the streaming audio interface which is implemented as a client of the media plugin API.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "linden_common.h"
#include "llpluginclassmedia.h"
#include "llpluginclassmediaowner.h"
#include "llviewermedia.h"

#include "llviewermedia_streamingaudio.h"

#include "llmimetypes.h"
#include "llvfs.h"
#include "lldir.h"


LLStreamingAudio_MediaPlugins::LLStreamingAudio_MediaPlugins() :
	mMediaPlugin(NULL),
	mGain(1.0)
{
	// nothing interesting to do?
	// we will lazily create a media plugin at play-time, if none exists.
}

LLStreamingAudio_MediaPlugins::~LLStreamingAudio_MediaPlugins()
{
	delete mMediaPlugin;
	mMediaPlugin = NULL;
}

void LLStreamingAudio_MediaPlugins::start(const std::string& url)
{
	if (!mMediaPlugin) // lazy-init the underlying media plugin
	{
		mMediaPlugin = initializeMedia("audio/mpeg"); // assumes that whatever media implementation supports mp3 also supports vorbis.
		llinfos << "streaming audio mMediaPlugin is now " << mMediaPlugin << llendl;
	}

	if(!mMediaPlugin)
		return;

	if (!url.empty()) {
		llinfos << "Starting internet stream: " << url << llendl;
		mURL = url;
		mMediaPlugin->loadURI ( url );
		mMediaPlugin->start();
		llinfos << "Playing stream..." << llendl;		
	} else {
		llinfos << "setting stream to NULL"<< llendl;
		mURL.clear();
		mMediaPlugin->stop();
	}
}

void LLStreamingAudio_MediaPlugins::stop()
{
	llinfos << "Stopping internet stream." << llendl;
	if(mMediaPlugin)
	{
		mMediaPlugin->stop();
	}

	mURL.clear();
}

void LLStreamingAudio_MediaPlugins::pause(int pause)
{
	if(!mMediaPlugin)
		return;
	
	if(pause)
	{
		llinfos << "Pausing internet stream." << llendl;
		mMediaPlugin->pause();
	} 
	else 
	{
		llinfos << "Unpausing internet stream." << llendl;
		mMediaPlugin->start();
	}
}

void LLStreamingAudio_MediaPlugins::update()
{
	if (mMediaPlugin)
		mMediaPlugin->idle();
}

int LLStreamingAudio_MediaPlugins::isPlaying()
{
	if (!mMediaPlugin)
		return 0; // stopped
	
	LLPluginClassMediaOwner::EMediaStatus status =
		mMediaPlugin->getStatus();

	switch (status)
	{
	case LLPluginClassMediaOwner::MEDIA_LOADING: // but not MEDIA_LOADED
	case LLPluginClassMediaOwner::MEDIA_PLAYING:
		return 1; // Active and playing
	case LLPluginClassMediaOwner::MEDIA_PAUSED:
		return 2; // paused
	default:
		return 0; // stopped
	}
}

void LLStreamingAudio_MediaPlugins::setGain(F32 vol)
{
	mGain = vol;

	if(!mMediaPlugin)
		return;

	vol = llclamp(vol, 0.f, 1.f);
	mMediaPlugin->setVolume(vol);
}

F32 LLStreamingAudio_MediaPlugins::getGain()
{
	return mGain;
}

std::string LLStreamingAudio_MediaPlugins::getURL()
{
	return mURL;
}

LLPluginClassMedia* LLStreamingAudio_MediaPlugins::initializeMedia(const std::string& media_type)
{
	LLPluginClassMediaOwner* owner = NULL;
	S32 default_size = 1; // audio-only - be minimal, doesn't matter
	LLPluginClassMedia* media_source = LLViewerMediaImpl::newSourceFromMediaType(media_type, owner, default_size, default_size);

	if (media_source)
	{
		media_source->setLoop(false); // audio streams are not expected to loop
	}

	return media_source;
}

