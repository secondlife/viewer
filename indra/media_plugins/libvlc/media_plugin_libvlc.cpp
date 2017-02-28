/**
* @file media_plugin_libvlc.cpp
* @brief LibVLC plugin for LLMedia API plugin system
*
* @cond
* $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
* @endcond
*/

#include "linden_common.h"

#include "llgl.h"
#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "media_plugin_base.h"

#include "vlc/vlc.h"
#include "vlc/libvlc_version.h"

////////////////////////////////////////////////////////////////////////////////
//
class MediaPluginLibVLC :
	public MediaPluginBase
{
public:
	MediaPluginLibVLC(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginLibVLC();

	/*virtual*/ void receiveMessage(const char* message_string);

private:
	bool init();

	void initVLC();
	void playMedia();
	void resetVLC();
	void setVolume(const F64 volume);
	void updateTitle(const char* title);

	static void* lock(void* data, void** p_pixels);
	static void unlock(void* data, void* id, void* const* raw_pixels);
	static void display(void* data, void* id);

	/*virtual*/ void setDirty(int left, int top, int right, int bottom) /* override, but that is not supported in gcc 4.6 */;

	static void eventCallbacks(const libvlc_event_t* event, void* ptr);

	libvlc_instance_t* mLibVLC;
	libvlc_media_t* mLibVLCMedia;
	libvlc_media_player_t* mLibVLCMediaPlayer;

	struct mLibVLCContext
	{
		unsigned char* texture_pixels;
		libvlc_media_player_t* mp;
		MediaPluginLibVLC* parent;
	};
	struct mLibVLCContext mLibVLCCallbackContext;

	std::string mURL;
	F64 mCurVolume;

	bool mIsLooping;

	float mCurTime;
	float mDuration;
	EStatus mVlcStatus;
};

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginLibVLC::MediaPluginLibVLC(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data) :
MediaPluginBase(host_send_func, host_user_data)
{
	mTextureWidth = 0;
	mTextureHeight = 0;
	mWidth = 0;
	mHeight = 0;
	mDepth = 4;
	mPixels = 0;

	mLibVLC = 0;
	mLibVLCMedia = 0;
	mLibVLCMediaPlayer = 0;

	mCurVolume = 0.0;

	mIsLooping = false;

	mCurTime = 0.0;
	mDuration = 0.0;

	mURL = std::string();

	mVlcStatus = STATUS_NONE;
	setStatus(STATUS_NONE);
}

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginLibVLC::~MediaPluginLibVLC()
{
}

/////////////////////////////////////////////////////////////////////////////////
//
void* MediaPluginLibVLC::lock(void* data, void** p_pixels)
{
	struct mLibVLCContext* context = (mLibVLCContext*)data;

	*p_pixels = context->texture_pixels;

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::unlock(void* data, void* id, void* const* raw_pixels)
{
	// nothing to do here for the moment
	// we *could* modify pixels here to, for example, Y flip, but this is done with
	// a VLC video filter transform. 
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::display(void* data, void* id)
{
	struct mLibVLCContext* context = (mLibVLCContext*)data;

	context->parent->setDirty(0, 0, context->parent->mWidth, context->parent->mHeight);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::initVLC()
{
	char const* vlc_argv[] =
	{
		"--no-xlib",
		"--video-filter=transform{type=vflip}",  // MAINT-6578 Y flip textures in plugin vs client
	};

#if LL_DARWIN
	setenv("VLC_PLUGIN_PATH", ".", 1);
#endif

	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
	mLibVLC = libvlc_new(vlc_argc, vlc_argv);

	if (!mLibVLC)
	{
		// for the moment, if this fails, the plugin will fail and 
		// the media sub-system will tell the viewer something went wrong.
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::resetVLC()
{
	libvlc_media_player_stop(mLibVLCMediaPlayer);
	libvlc_media_player_release(mLibVLCMediaPlayer);
	libvlc_release(mLibVLC);
}

////////////////////////////////////////////////////////////////////////////////
// *virtual*
void MediaPluginLibVLC::setDirty(int left, int top, int right, int bottom)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "updated");

	message.setValueS32("left", left);
	message.setValueS32("top", top);
	message.setValueS32("right", right);
	message.setValueS32("bottom", bottom);

	message.setValueReal("current_time", mCurTime);
	message.setValueReal("duration", mDuration);
	message.setValueReal("current_rate", 1.0f);

	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::eventCallbacks(const libvlc_event_t* event, void* ptr)
{
	MediaPluginLibVLC* parent = (MediaPluginLibVLC*)ptr;
	if (parent == 0)
	{
		return;
	}

	switch (event->type)
	{
	case libvlc_MediaPlayerOpening:
		parent->mVlcStatus = STATUS_LOADING;
		break;

	case libvlc_MediaPlayerPlaying:
		parent->mDuration = (float)(libvlc_media_get_duration(parent->mLibVLCMedia)) / 1000.0f;
		parent->mVlcStatus = STATUS_PLAYING;
		break;

	case libvlc_MediaPlayerPaused:
		parent->mVlcStatus = STATUS_PAUSED;
		break;

	case libvlc_MediaPlayerStopped:
		parent->mVlcStatus = STATUS_DONE;
		break;

	case libvlc_MediaPlayerEndReached:
		parent->mVlcStatus = STATUS_DONE;
		break;

	case libvlc_MediaPlayerEncounteredError:
		parent->mVlcStatus = STATUS_ERROR;
		break;

	case libvlc_MediaPlayerTimeChanged:
		parent->mCurTime = (float)libvlc_media_player_get_time(parent->mLibVLCMediaPlayer) / 1000.0f;
		break;

	case libvlc_MediaPlayerPositionChanged:
		break;

	case libvlc_MediaPlayerLengthChanged:
		parent->mDuration = (float)libvlc_media_get_duration(parent->mLibVLCMedia) / 1000.0f;
		break;

	case libvlc_MediaPlayerTitleChanged:
	{
		char* title = libvlc_media_get_meta(parent->mLibVLCMedia, libvlc_meta_Title);
		if (title)
		{
			parent->updateTitle(title);
		}
	}
	break;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::playMedia()
{
	if (mURL.length() == 0)
	{
		return;
	}

	if (mLibVLCMediaPlayer)
	{
		// stop listening to events while we reset things
		libvlc_event_manager_t* em = libvlc_media_player_event_manager(mLibVLCMediaPlayer);
		if (em)
		{
			libvlc_event_detach(em, libvlc_MediaPlayerOpening, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerPlaying, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerPaused, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerStopped, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerEndReached, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerEncounteredError, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerTimeChanged, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerPositionChanged, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerLengthChanged, eventCallbacks, NULL);
			libvlc_event_detach(em, libvlc_MediaPlayerTitleChanged, eventCallbacks, NULL);
		};

		libvlc_media_player_stop(mLibVLCMediaPlayer);
		libvlc_media_player_release(mLibVLCMediaPlayer);

		mLibVLCMediaPlayer = 0;
	}

	if (mLibVLCMedia)
	{
		libvlc_media_release(mLibVLCMedia);

		mLibVLCMedia = 0;
	}

	mLibVLCMedia = libvlc_media_new_location(mLibVLC, mURL.c_str());
	if (!mLibVLCMedia)
	{
		mLibVLCMediaPlayer = 0;
		setStatus(STATUS_ERROR);
		return;
	}

	mLibVLCMediaPlayer = libvlc_media_player_new_from_media(mLibVLCMedia);
	if (!mLibVLCMediaPlayer)
	{
		setStatus(STATUS_ERROR);
		return;
	}

	// listen to events
	libvlc_event_manager_t* em = libvlc_media_player_event_manager(mLibVLCMediaPlayer);
	if (em)
	{
		libvlc_event_attach(em, libvlc_MediaPlayerOpening, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerPlaying, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerPaused, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerStopped, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerEndReached, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerEncounteredError, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerTimeChanged, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerPositionChanged, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerLengthChanged, eventCallbacks, this);
		libvlc_event_attach(em, libvlc_MediaPlayerTitleChanged, eventCallbacks, this);
	}

	mLibVLCCallbackContext.parent = this;
	mLibVLCCallbackContext.texture_pixels = mPixels;
	mLibVLCCallbackContext.mp = mLibVLCMediaPlayer;

	// Send a "navigate begin" event.
	// This is really a browser message but the QuickTime plugin did it and 
	// the media system relies on this message to update internal state so we must send it too
	// Note: see "navigate_complete" message below too
	// https://jira.secondlife.com/browse/MAINT-6528
	LLPluginMessage message_begin(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_begin");
	message_begin.setValue("uri", mURL);
	message_begin.setValueBoolean("history_back_available", false);
	message_begin.setValueBoolean("history_forward_available", false);
	sendMessage(message_begin);

	// volume level gets set before VLC is initialized (thanks media system) so we have to record
	// it in mCurVolume and set it again here so that volume levels are correctly initialized
	setVolume(mCurVolume);

	setStatus(STATUS_LOADED);

	libvlc_video_set_callbacks(mLibVLCMediaPlayer, lock, unlock, display, &mLibVLCCallbackContext);
	libvlc_video_set_format(mLibVLCMediaPlayer, "RV32", mWidth, mHeight, mWidth * mDepth);

	// note this relies on the "set_loop" message arriving before the "start" (play) one
	// but that appears to always be the case
	if (mIsLooping)
	{
		libvlc_media_add_option(mLibVLCMedia, "input-repeat=-1");
	}

	libvlc_media_player_play(mLibVLCMediaPlayer);

	// send a "location_changed" message - this informs the media system
	// that a new URL is the 'current' one and is used extensively.
	// Again, this is really a browser message but we will use it here.
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "location_changed");
	message.setValue("uri", mURL);
	sendMessage(message);

	// Send a "navigate complete" event.
	// This is really a browser message but the QuickTime plugin did it and 
	// the media system relies on this message to update internal state so we must send it too
	// Note: see "navigate_begin" message above too
	// https://jira.secondlife.com/browse/MAINT-6528
	LLPluginMessage message_complete(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_complete");
	message_complete.setValue("uri", mURL);
	message_complete.setValueS32("result_code", 200);
	message_complete.setValue("result_string", "OK");
	sendMessage(message_complete);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::updateTitle(const char* title)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", title);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::setVolume(const F64 volume)
{
	mCurVolume = volume;

	if (mLibVLCMediaPlayer)
	{
		int result = libvlc_audio_set_volume(mLibVLCMediaPlayer, (int)(volume * 100));
		if (result != 0)
		{
			// volume wasn't set but not much to be done here
		}
	}
	else
	{
		// volume change was requested but VLC wasn't ready.
		// that's okay thought because we saved the value in mCurVolume and 
		// the next volume change after the VLC system is initilzied  will set it
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::receiveMessage(const char* message_string)
{
	LLPluginMessage message_in;

	if (message_in.parse(message_string) >= 0)
	{
		std::string message_class = message_in.getClass();
		std::string message_name = message_in.getName();
		if (message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
		{
			if (message_name == "init")
			{
				initVLC();

				LLPluginMessage message("base", "init_response");
				LLSD versions = LLSD::emptyMap();
				versions[LLPLUGIN_MESSAGE_CLASS_BASE] = LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] = LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME] = LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME_VERSION;
				message.setValueLLSD("versions", versions);

				std::ostringstream s;
				s << "LibVLC plugin ";
				s << LIBVLC_VERSION_MAJOR;
				s << ".";
				s << LIBVLC_VERSION_MINOR;
				s << ".";
				s << LIBVLC_VERSION_REVISION;

				message.setValue("plugin_version", s.str());
				sendMessage(message);
			}
			else if (message_name == "idle")
			{
				setStatus(mVlcStatus);
			}
			else if (message_name == "cleanup")
			{
				resetVLC();
			}
			else if (message_name == "shm_added")
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer("address");
				info.mSize = (size_t)message_in.getValueS32("size");
				std::string name = message_in.getValue("name");

				mSharedSegments.insert(SharedSegmentMap::value_type(name, info));

			}
			else if (message_name == "shm_remove")
			{
				std::string name = message_in.getValue("name");

				SharedSegmentMap::iterator iter = mSharedSegments.find(name);
				if (iter != mSharedSegments.end())
				{
					if (mPixels == iter->second.mAddress)
					{
						libvlc_media_player_stop(mLibVLCMediaPlayer);
						libvlc_media_player_release(mLibVLCMediaPlayer);
						mLibVLCMediaPlayer = 0;

						mPixels = NULL;
						mTextureSegmentName.clear();
					}
					mSharedSegments.erase(iter);
				}
				else
				{
					//std::cerr << "MediaPluginWebKit::receiveMessage: unknown shared memory region!" << std::endl;
				}

				// Send the response so it can be cleaned up.
				LLPluginMessage message("base", "shm_remove_response");
				message.setValue("name", name);
				sendMessage(message);
			}
			else
			{
				//std::cerr << "MediaPluginWebKit::receiveMessage: unknown base message: " << message_name << std::endl;
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
		{
			if (message_name == "init")
			{
				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params");
				message.setValueS32("default_width", 1024);
				message.setValueS32("default_height", 1024);
				message.setValueS32("depth", mDepth);
				message.setValueU32("internalformat", GL_RGB);
				message.setValueU32("format", GL_BGRA_EXT);
				message.setValueU32("type", GL_UNSIGNED_BYTE);
				message.setValueBoolean("coords_opengl", true);
				sendMessage(message);
			}
			else if (message_name == "size_change")
			{
				std::string name = message_in.getValue("name");
				S32 width = message_in.getValueS32("width");
				S32 height = message_in.getValueS32("height");
				S32 texture_width = message_in.getValueS32("texture_width");
				S32 texture_height = message_in.getValueS32("texture_height");

				if (!name.empty())
				{
					// Find the shared memory region with this name
					SharedSegmentMap::iterator iter = mSharedSegments.find(name);
					if (iter != mSharedSegments.end())
					{
						mPixels = (unsigned char*)iter->second.mAddress;
						mWidth = width;
						mHeight = height;
						mTextureWidth = texture_width;
						mTextureHeight = texture_height;

						playMedia();
					};
				};

				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_response");
				message.setValue("name", name);
				message.setValueS32("width", width);
				message.setValueS32("height", height);
				message.setValueS32("texture_width", texture_width);
				message.setValueS32("texture_height", texture_height);
				sendMessage(message);
			}
			else if (message_name == "load_uri")
			{
				mURL = message_in.getValue("uri");
				playMedia();
			}
		}
		else
			if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME)
			{
				if (message_name == "stop")
				{
					if (mLibVLCMediaPlayer)
					{
						libvlc_media_player_stop(mLibVLCMediaPlayer);
					}
				}
				else if (message_name == "start")
				{
					if (mLibVLCMediaPlayer)
					{
						libvlc_media_player_play(mLibVLCMediaPlayer);
					}
				}
				else if (message_name == "pause")
				{
					if (mLibVLCMediaPlayer)
					{
						libvlc_media_player_set_pause(mLibVLCMediaPlayer, 1);
					}
				}
				else if (message_name == "seek")
				{
					if (mDuration > 0)
					{
						F64 normalized_offset = message_in.getValueReal("time") / mDuration;
						libvlc_media_player_set_position(mLibVLCMediaPlayer, normalized_offset);
					}
				}
				else if (message_name == "set_loop")
				{
					mIsLooping = true;
				}
				else if (message_name == "set_volume")
				{
					// volume comes in 0 -> 1.0
					F64 volume = message_in.getValueReal("volume");
					setVolume(volume);
				}
			}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginLibVLC::init()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", "LibVLC Plugin");
	sendMessage(message);

	return true;
};

////////////////////////////////////////////////////////////////////////////////
//
int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func,
	void* host_user_data,
	LLPluginInstance::sendMessageFunction *plugin_send_func,
	void **plugin_user_data)
{
	MediaPluginLibVLC* self = new MediaPluginLibVLC(host_send_func, host_user_data);
	*plugin_send_func = MediaPluginLibVLC::staticReceiveMessage;
	*plugin_user_data = (void*)self;

	return 0;
}
