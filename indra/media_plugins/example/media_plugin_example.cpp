/**
 * @file media_plugin_example.cpp
 * @brief Example plugin for LLMedia API plugin system
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	LLPluginMessage message_in;

	if ( message_in.parse( message_string ) >= 0 )
	{
		std::string message_class = message_in.getClass();
		std::string message_name = message_in.getName();

		if ( message_class == LLPLUGIN_MESSAGE_CLASS_BASE )
		{
			if ( message_name == "init" )
			{
				LLPluginMessage message( "base", "init_response" );
				LLSD versions = LLSD::emptyMap();
				versions[ LLPLUGIN_MESSAGE_CLASS_BASE ] = LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
				versions[ LLPLUGIN_MESSAGE_CLASS_MEDIA ] = LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
				versions[ LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER ] = LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION;
				message.setValueLLSD( "versions", versions );

				std::string plugin_version = "Example media plugin, Example Version 1.0.0.0";
				message.setValue( "plugin_version", plugin_version );
				sendMessage( message );
			}
			else
			if ( message_name == "idle" )
			{
				// no response is necessary here.
				F64 time = message_in.getValueReal( "time" );

				// Convert time to milliseconds for update()
				update( time );
			}
			else
			if ( message_name == "cleanup" )
			{
				// clean up here
			}
			else
			if ( message_name == "shm_added" )
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer( "address" );
				info.mSize = ( size_t )message_in.getValueS32( "size" );
				std::string name = message_in.getValue( "name" );

				mSharedSegments.insert( SharedSegmentMap::value_type( name, info ) );

			}
			else
			if ( message_name == "shm_remove" )
			{
				std::string name = message_in.getValue( "name" );

				SharedSegmentMap::iterator iter = mSharedSegments.find( name );
				if( iter != mSharedSegments.end() )
				{
					if ( mPixels == iter->second.mAddress )
					{
						// This is the currently active pixel buffer.
						// Make sure we stop drawing to it.
						mPixels = NULL;
						mTextureSegmentName.clear();
					};
					mSharedSegments.erase( iter );
				}
				else
				{
					//std::cerr << "MediaPluginExample::receiveMessage: unknown shared memory region!" << std::endl;
				};

				// Send the response so it can be cleaned up.
				LLPluginMessage message( "base", "shm_remove_response" );
				message.setValue( "name", name );
				sendMessage( message );
			}
			else
			{
				//std::cerr << "MediaPluginExample::receiveMessage: unknown base message: " << message_name << std::endl;
			};
		}
		else
		if ( message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA )
		{
			if ( message_name == "init" )
			{
				// Plugin gets to decide the texture parameters to use.
				LLPluginMessage message( LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params" );
				message.setValueS32( "default_width", mWidth );
				message.setValueS32( "default_height", mHeight );
				message.setValueS32( "depth", mDepth );
				message.setValueU32( "internalformat", GL_RGBA );
				message.setValueU32( "format", GL_RGBA );
				message.setValueU32( "type", GL_UNSIGNED_BYTE );
				message.setValueBoolean( "coords_opengl", false );
				sendMessage( message );
			}
			else if ( message_name == "size_change" )
			{
				std::string name = message_in.getValue( "name" );
				S32 width = message_in.getValueS32( "width" );
				S32 height = message_in.getValueS32( "height" );
				S32 texture_width = message_in.getValueS32( "texture_width" );
				S32 texture_height = message_in.getValueS32( "texture_height" );

				if ( ! name.empty() )
				{
					// Find the shared memory region with this name
					SharedSegmentMap::iterator iter = mSharedSegments.find( name );
					if ( iter != mSharedSegments.end() )
					{
						mPixels = ( unsigned char* )iter->second.mAddress;
						mWidth = width;
						mHeight = height;

						mTextureWidth = texture_width;
						mTextureHeight = texture_height;

						init();
					};
				};

				LLPluginMessage message( LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_response" );
				message.setValue( "name", name );
				message.setValueS32( "width", width );
				message.setValueS32( "height", height );
				message.setValueS32( "texture_width", texture_width );
				message.setValueS32( "texture_height", texture_height );
				sendMessage( message );
			}
			else
			if ( message_name == "load_uri" )
			{
				std::string uri = message_in.getValue( "uri" );
				if ( ! uri.empty() )
				{
				};
			}
			else
			if ( message_name == "mouse_event" )
			{
				std::string event = message_in.getValue( "event" );
				S32 button = message_in.getValueS32( "button" );

				// left mouse button
				if ( button == 0 )
				{
					int mouse_x = message_in.getValueS32( "x" );
					int mouse_y = message_in.getValueS32( "y" );
					std::string modifiers = message_in.getValue( "modifiers" );

					if ( event == "move" )
					{
						if ( mMouseButtonDown )
							write_pixel( mouse_x, mouse_y, rand() % 0x80 + 0x80, rand() % 0x80 + 0x80, rand() % 0x80 + 0x80 );
					}
					else
					if ( event == "down" )
					{
						mMouseButtonDown = true;
					}
					else
					if ( event == "up" )
					{
						mMouseButtonDown = false;
					}
					else
					if ( event == "double_click" )
					{
					};
				};
			}
			else
			if ( message_name == "key_event" )
			{
				std::string event = message_in.getValue( "event" );
				S32 key = message_in.getValueS32( "key" );
				std::string modifiers = message_in.getValue( "modifiers" );

				if ( event == "down" )
				{
					if ( key == ' ')
					{
						mLastUpdateTime = 0;
						update( 0.0f );
					};
				};
			}
			else
			{
				//std::cerr << "MediaPluginExample::receiveMessage: unknown media message: " << message_string << std::endl;
			};
		}
		else
		if ( message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER )
		{
			if ( message_name == "browse_reload" )
			{
				mLastUpdateTime = 0;
				mFirstTime = true;
				mStopAction = false;
				update( 0.0f );
			}
			else
			if ( message_name == "browse_stop" )
			{
				for( int n = 0; n < ENumObjects; ++n )
					mXInc[ n ] = mYInc[ n ] = 0;

				mStopAction = true;
				update( 0.0f );
			}
			else
			{
				//std::cerr << "MediaPluginExample::receiveMessage: unknown media_browser message: " << message_string << std::endl;
			};
		}
		else
		{
			//std::cerr << "MediaPluginExample::receiveMessage: unknown message class: " << message_class << std::endl;
		};
	};
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
