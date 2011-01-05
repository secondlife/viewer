/**
 * @file media_plugin_example.cpp
 * @brief Example plugin for LLMedia API plugin system
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

#include <time.h>

////////////////////////////////////////////////////////////////////////////////
//
class MediaPluginExample :
        public MediaPluginBase
{
    public:
        MediaPluginExample( LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data );
        ~MediaPluginExample();

        /*virtual*/ void receiveMessage( const char* message_string );

    private:
        bool init();
        void update( F64 milliseconds );
        void write_pixel( int x, int y, unsigned char r, unsigned char g, unsigned char b );
        bool mFirstTime;

        time_t mLastUpdateTime;
        enum Constants { ENumObjects = 10 };
        unsigned char* mBackgroundPixels;
        int mColorR[ ENumObjects ];
        int mColorG[ ENumObjects ];
        int mColorB[ ENumObjects ];
        int mXpos[ ENumObjects ];
        int mYpos[ ENumObjects ];
        int mXInc[ ENumObjects ];
        int mYInc[ ENumObjects ];
        int mBlockSize[ ENumObjects ];
        bool mMouseButtonDown;
        bool mStopAction;
};

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginExample::MediaPluginExample( LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data ) :
    MediaPluginBase( host_send_func, host_user_data )
{
    mFirstTime = true;
    mWidth = 0;
    mHeight = 0;
    mDepth = 4;
    mPixels = 0;
    mMouseButtonDown = false;
    mStopAction = false;
    mLastUpdateTime = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginExample::~MediaPluginExample()
{
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginExample::receiveMessage( const char* message_string )
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
                LLPluginMessage message("base", "init_response");
                LLSD versions = LLSD::emptyMap();
                versions[LLPLUGIN_MESSAGE_CLASS_BASE] = LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
                versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] = LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
                versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER] = LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION;
                message.setValueLLSD("versions", versions);

                std::string plugin_version = "Example plugin 1.0..0";
                message.setValue("plugin_version", plugin_version);
                sendMessage(message);
            }
            else if(message_name == "idle")
            {
                // no response is necessary here.
                F64 time = message_in.getValueReal("time");

                // Convert time to milliseconds for update()
                update((int)(time * 1000.0f));
            }
            else if(message_name == "cleanup")
            {
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
                message.setValueU32("internalformat", GL_RGBA);
                message.setValueU32("format", GL_RGBA);
                message.setValueU32("type", GL_UNSIGNED_BYTE);
                message.setValueBoolean("coords_opengl", true);
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
//          std::cerr << "MediaPluginWebKit::receiveMessage: unknown message class: " << message_class << std::endl;
        };
    }
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginExample::write_pixel( int x, int y, unsigned char r, unsigned char g, unsigned char b )
{
    // make sure we don't write outside the buffer
    if ( ( x < 0 ) || ( x >= mWidth ) || ( y < 0 ) || ( y >= mHeight ) )
        return;

    if ( mBackgroundPixels != NULL )
    {
        unsigned char *pixel = mBackgroundPixels;
        pixel += y * mWidth * mDepth;
        pixel += ( x * mDepth );
        pixel[ 0 ] = b;
        pixel[ 1 ] = g;
        pixel[ 2 ] = r;

        setDirty( x, y, x + 1, y + 1 );
    };
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginExample::update( F64 milliseconds )
{
    if ( mWidth < 1 || mWidth > 2048 || mHeight < 1 || mHeight > 2048 )
        return;

    if ( mPixels == 0 )
            return;

    if ( mFirstTime )
    {
        for( int n = 0; n < ENumObjects; ++n )
        {
            mXpos[ n ] = ( mWidth / 2 ) + rand() % ( mWidth / 16 ) - ( mWidth / 32 );
            mYpos[ n ] = ( mHeight / 2 ) + rand() % ( mHeight / 16 ) - ( mHeight / 32 );

            mColorR[ n ] = rand() % 0x60 + 0x60;
            mColorG[ n ] = rand() % 0x60 + 0x60;
            mColorB[ n ] = rand() % 0x60 + 0x60;

            mXInc[ n ] = 0;
            while ( mXInc[ n ] == 0 )
                mXInc[ n ] = rand() % 7 - 3;

            mYInc[ n ] = 0;
            while ( mYInc[ n ] == 0 )
                mYInc[ n ] = rand() % 9 - 4;

            mBlockSize[ n ] = rand() % 0x30 + 0x10;
        };

        delete [] mBackgroundPixels;

        mBackgroundPixels = new unsigned char[ mWidth * mHeight * mDepth ];

        mFirstTime = false;
    };

    if ( mStopAction )
        return;

    if ( time( NULL ) > mLastUpdateTime + 3 )
    {
        const int num_squares = rand() % 20 + 4;
        int sqr1_r = rand() % 0x80 + 0x20;
        int sqr1_g = rand() % 0x80 + 0x20;
        int sqr1_b = rand() % 0x80 + 0x20;
        int sqr2_r = rand() % 0x80 + 0x20;
        int sqr2_g = rand() % 0x80 + 0x20;
        int sqr2_b = rand() % 0x80 + 0x20;

        for ( int y1 = 0; y1 < num_squares; ++y1 )
        {
            for ( int x1 = 0; x1 < num_squares; ++x1 )
            {
                int px_start = mWidth * x1 / num_squares;
                int px_end = ( mWidth * ( x1 + 1 ) ) / num_squares;
                int py_start = mHeight * y1 / num_squares;
                int py_end = ( mHeight * ( y1 + 1 ) ) / num_squares;

                for( int y2 = py_start; y2 < py_end; ++y2 )
                {
                    for( int x2 = px_start; x2 < px_end; ++x2 )
                    {
                        int rowspan = mWidth * mDepth;

                        if ( ( y1 % 2 ) ^ ( x1 % 2 ) )
                        {
                            mBackgroundPixels[ y2 * rowspan + x2 * mDepth + 0 ] = sqr1_r;
                            mBackgroundPixels[ y2 * rowspan + x2 * mDepth + 1 ] = sqr1_g;
                            mBackgroundPixels[ y2 * rowspan + x2 * mDepth + 2 ] = sqr1_b;
                        }
                        else
                        {
                            mBackgroundPixels[ y2 * rowspan + x2 * mDepth + 0 ] = sqr2_r;
                            mBackgroundPixels[ y2 * rowspan + x2 * mDepth + 1 ] = sqr2_g;
                            mBackgroundPixels[ y2 * rowspan + x2 * mDepth + 2 ] = sqr2_b;
                        };
                    };
                };
            };
        };

        time( &mLastUpdateTime );
    };

    memcpy( mPixels, mBackgroundPixels, mWidth * mHeight * mDepth );

    for( int n = 0; n < ENumObjects; ++n )
    {
        if ( rand() % 50 == 0 )
        {
                mXInc[ n ] = 0;
                while ( mXInc[ n ] == 0 )
                    mXInc[ n ] = rand() % 7 - 3;

                mYInc[ n ] = 0;
                while ( mYInc[ n ] == 0 )
                    mYInc[ n ] = rand() % 9 - 4;
        };

        if ( mXpos[ n ] + mXInc[ n ] < 0 || mXpos[ n ] + mXInc[ n ] >= mWidth - mBlockSize[ n ] )
            mXInc[ n ] =- mXInc[ n ];

        if ( mYpos[ n ] + mYInc[ n ] < 0 || mYpos[ n ] + mYInc[ n ] >= mHeight - mBlockSize[ n ] )
            mYInc[ n ] =- mYInc[ n ];

        mXpos[ n ] += mXInc[ n ];
        mYpos[ n ] += mYInc[ n ];

        for( int y = 0; y < mBlockSize[ n ]; ++y )
        {
            for( int x = 0; x < mBlockSize[ n ]; ++x )
            {
                mPixels[ ( mXpos[ n ] + x ) * mDepth + ( mYpos[ n ] + y ) * mDepth * mWidth + 0 ] = mColorR[ n ];
                mPixels[ ( mXpos[ n ] + x ) * mDepth + ( mYpos[ n ] + y ) * mDepth * mWidth + 1 ] = mColorG[ n ];
                mPixels[ ( mXpos[ n ] + x ) * mDepth + ( mYpos[ n ] + y ) * mDepth * mWidth + 2 ] = mColorB[ n ];
            };
        };
    };

    setDirty( 0, 0, mWidth, mHeight );
};

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginExample::init()
{
    LLPluginMessage message( LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text" );
    message.setValue( "name", "Example Plugin" );
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
    MediaPluginExample* self = new MediaPluginExample( host_send_func, host_user_data );
    *plugin_send_func = MediaPluginExample::staticReceiveMessage;
    *plugin_user_data = ( void* )self;

    return 0;
}

