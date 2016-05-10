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
        MediaPluginLibVLC( LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data );
        ~MediaPluginLibVLC();

        /*virtual*/ void receiveMessage( const char* message_string );

    private:
        bool init();
        void update( F64 milliseconds );


		static void* lock(void* data, void** p_pixels);
		void initVLC();
		void playMedia(const std::string url);
		void resetVLC();

		libvlc_instance_t* gLibVLC;
		libvlc_media_t* gLibVLCMedia;
		libvlc_media_player_t* gLibVLCMediaPlayer;


		//float mCurVideoPosition;

		struct gVLCContext
		{
			unsigned char* texture_pixels;
			libvlc_media_player_t* mp;
		};
		struct gVLCContext gVLCCallbackContext;
};

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginLibVLC::MediaPluginLibVLC( LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data ) :
    MediaPluginBase( host_send_func, host_user_data )
{
	mTextureWidth = 64;
	mTextureHeight = 64;
    mWidth = 0;
    mHeight = 0;
    mDepth = 4;
    mPixels = 0;

	gLibVLC = 0;
	gLibVLCMedia = 0;
	gLibVLCMediaPlayer = 0;

	//mCurVideoPosition = 0;
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
	struct gVLCContext* context = (gVLCContext*)data;

	*p_pixels = context->texture_pixels;

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::initVLC()
{
	char const* vlc_argv[] =
	{
		"--no-xlib",
	};

	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
	gLibVLC = libvlc_new(vlc_argc, vlc_argv);

	if (!gLibVLC)
	{
		// TODO: do we need to do anything herE?
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::resetVLC()
{
	libvlc_media_player_stop(gLibVLCMediaPlayer);
	libvlc_media_player_release(gLibVLCMediaPlayer);
	libvlc_release(gLibVLC);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::playMedia(const std::string url)
{
	std::string tmp_url = std::string("https://callum-linden.s3.amazonaws.com/sample_media/jb.mp4");

	if (gLibVLCMediaPlayer)
	{
		libvlc_media_player_stop(gLibVLCMediaPlayer);
		libvlc_media_player_release(gLibVLCMediaPlayer);
	}

	gLibVLCMedia = libvlc_media_new_location(gLibVLC, tmp_url.c_str());
	if (!gLibVLCMedia)
	{
		printf("libvlc_media_new_location failed\n");
	}
	else
	{
		printf("libvlc_media_new_location ok\n");
	}

	gLibVLCMediaPlayer = libvlc_media_player_new_from_media(gLibVLCMedia);
	if (!gLibVLCMediaPlayer)
	{
		printf("libvlc_media_player_new_from_media failed\n");
	}
	else
	{
		printf("libvlc_media_player_new_from_media ok...\n");
	}

	libvlc_media_release(gLibVLCMedia);

	gVLCCallbackContext.texture_pixels = mPixels;
	gVLCCallbackContext.mp = gLibVLCMediaPlayer;

	libvlc_video_set_callbacks(gLibVLCMediaPlayer, lock, NULL, NULL, &gVLCCallbackContext);
	libvlc_video_set_format(gLibVLCMediaPlayer, "RV32", mWidth, mHeight, mWidth * 4);
	libvlc_media_player_play(gLibVLCMediaPlayer);

	//libvlc_media_player_set_position(gLibVLCMediaPlayer, mCurVideoPosition);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginLibVLC::receiveMessage( const char* message_string )
{
//  std::cerr << "MediaPluginWebKit::receiveMessage: received message: \"" << message_string << "\"" << std::endl;
    LLPluginMessage message_in;

    if(message_in.parse(message_string) >= 0)
    {
        std::string message_class = message_in.getClass();
        std::string message_name = message_in.getName();
        if(message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
        {
            if(message_name == "init")
            {
				initVLC();

                LLPluginMessage message("base", "init_response");
                LLSD versions = LLSD::emptyMap();
                versions[LLPLUGIN_MESSAGE_CLASS_BASE] = LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
                versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] = LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
                versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER] = LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION;
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
            else if(message_name == "idle")
            {
				// TODO move to VLC callback when VLC wants to draw a frame
				setDirty(0, 0, mWidth, mHeight);
            }
            else if(message_name == "cleanup")
            {
				resetVLC();
            }
            else if(message_name == "shm_added")
            {
                SharedSegmentInfo info;
                info.mAddress = message_in.getValuePointer("address");
                info.mSize = (size_t)message_in.getValueS32("size");
                std::string name = message_in.getValue("name");

                mSharedSegments.insert(SharedSegmentMap::value_type(name, info));

            }
            else if(message_name == "shm_remove")
            {
                std::string name = message_in.getValue("name");

                SharedSegmentMap::iterator iter = mSharedSegments.find(name);
                if(iter != mSharedSegments.end())
                {
                    if(mPixels == iter->second.mAddress)
                    {
						/// only do thisis URLs are the same if ( )
						//mCurVideoPosition = libvlc_media_player_get_position(gLibVLCMediaPlayer);

						libvlc_media_player_stop(gLibVLCMediaPlayer);
						libvlc_media_player_release(gLibVLCMediaPlayer);
						gLibVLCMediaPlayer = 0;

                        // This is the currently active pixel buffer.  Make sure we stop drawing to it.
                        mPixels = NULL;
                        mTextureSegmentName.clear();
                    }
                    mSharedSegments.erase(iter);
                }
                else
                {
//                  std::cerr << "MediaPluginWebKit::receiveMessage: unknown shared memory region!" << std::endl;
                }

                // Send the response so it can be cleaned up.
                LLPluginMessage message("base", "shm_remove_response");
                message.setValue("name", name);
                sendMessage(message);
            }
            else
            {
//              std::cerr << "MediaPluginWebKit::receiveMessage: unknown base message: " << message_name << std::endl;
            }
        }
        else if(message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
        {
            if(message_name == "init")
            {
                // Plugin gets to decide the texture parameters to use.
                mDepth = 4;
                LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params");
                message.setValueS32("default_width", 1024);
                message.setValueS32("default_height", 1024);
                message.setValueS32("depth", mDepth);
				message.setValueU32("internalformat", GL_RGB);
                message.setValueU32("format", GL_BGRA_EXT);
                message.setValueU32("type", GL_UNSIGNED_BYTE);
                message.setValueBoolean("coords_opengl", false);
                sendMessage(message);
            }
            else if(message_name == "size_change")
            {
                std::string name = message_in.getValue("name");
                S32 width = message_in.getValueS32("width");
                S32 height = message_in.getValueS32("height");
                S32 texture_width = message_in.getValueS32("texture_width");
                S32 texture_height = message_in.getValueS32("texture_height");

                if(!name.empty())
                {
                    // Find the shared memory region with this name
                    SharedSegmentMap::iterator iter = mSharedSegments.find(name);
                    if(iter != mSharedSegments.end())
                    {
                        mPixels = (unsigned char*)iter->second.mAddress;
                        mWidth = width;
                        mHeight = height;
                        mTextureWidth = texture_width;
                        mTextureHeight = texture_height;

						playMedia("");
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
            else if(message_name == "load_uri")
            {
            }
            else if(message_name == "mouse_event")
            {
                std::string event = message_in.getValue("event");
                if(event == "down")
                {
                }
                else if(event == "up")
                {
                }
                else if(event == "double_click")
                {
                }
            }
        }
        else
        {
        };
    }
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginLibVLC::init()
{
    LLPluginMessage message( LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text" );
    message.setValue( "name", "LibVLC Plugin" );
    sendMessage( message );

    return true;
};

////////////////////////////////////////////////////////////////////////////////
//
int init_media_plugin( LLPluginInstance::sendMessageFunction host_send_func,
                        void* host_user_data,
                        LLPluginInstance::sendMessageFunction *plugin_send_func,
                        void **plugin_user_data )
{
    MediaPluginLibVLC* self = new MediaPluginLibVLC( host_send_func, host_user_data );
    *plugin_send_func = MediaPluginLibVLC::staticReceiveMessage;
    *plugin_user_data = ( void* )self;

    return 0;
}
