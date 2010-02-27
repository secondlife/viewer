/**
 * @file media_plugin_quicktime.cpp
 * @brief QuickTime plugin for LLMedia API plugin system
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
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

#if LL_QUICKTIME_ENABLED

#if defined(LL_DARWIN)
	#include <QuickTime/QuickTime.h>
#elif defined(LL_WINDOWS)
	#include "MacTypes.h"
	#include "QTML.h"
	#include "Movies.h"
	#include "QDoffscreen.h"
	#include "FixMath.h"
	#include "QTLoadLibraryUtils.h"
#endif

// TODO: Make sure that the only symbol exported from this library is LLPluginInitEntryPoint
////////////////////////////////////////////////////////////////////////////////
//
class MediaPluginQuickTime : public MediaPluginBase
{
public:
	MediaPluginQuickTime(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginQuickTime();

	/* virtual */ void receiveMessage(const char *message_string);

private:

	int mNaturalWidth;
	int mNaturalHeight;
	Movie mMovieHandle;
	GWorldPtr mGWorldHandle;
	ComponentInstance mMovieController;
	int mCurVolume;
	bool mMediaSizeChanging;
	bool mIsLooping;
	std::string mMovieTitle;
	bool mReceivedTitle;
	const int mMinWidth;
	const int mMaxWidth;
	const int mMinHeight;
	const int mMaxHeight;
	F64 mPlayRate;
	std::string mNavigateURL;

	enum ECommand {
		COMMAND_NONE,
		COMMAND_STOP,
		COMMAND_PLAY,
		COMMAND_FAST_FORWARD,
		COMMAND_FAST_REWIND,
		COMMAND_PAUSE,
		COMMAND_SEEK,
	};
	ECommand mCommand;

	// Override this to add current time and duration to the message
	/*virtual*/ void setDirty(int left, int top, int right, int bottom)
	{
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "updated");

		message.setValueS32("left", left);
		message.setValueS32("top", top);
		message.setValueS32("right", right);
		message.setValueS32("bottom", bottom);

		if(mMovieHandle)
		{
			message.setValueReal("current_time", getCurrentTime());
			message.setValueReal("duration", getDuration());
			message.setValueReal("current_rate", Fix2X(GetMovieRate(mMovieHandle)));
		}

		sendMessage(message);
	}


	static Rect rectFromSize(int width, int height)
	{
		Rect result;


		result.left = 0;
		result.top = 0;
		result.right = width;
		result.bottom = height;

		return result;
	}

	Fixed getPlayRate(void)
	{
		Fixed result;
		if(mPlayRate == 0.0f)
		{
			// Default to the movie's preferred rate
			result = GetMoviePreferredRate(mMovieHandle);
			if(result == 0)
			{
				// Don't return a 0 play rate, ever.
				std::cerr << "Movie's preferred rate is 0, forcing to 1.0." << std::endl;
				result = X2Fix(1.0f);
			}
		}
		else
		{
			result = X2Fix(mPlayRate);
		}

		return result;
	}

	void load( const std::string url )
	{

		if ( url.empty() )
			return;

		// Stop and unload any existing movie before starting another one.
		unload();

		setStatus(STATUS_LOADING);

		//In case std::string::c_str() makes a copy of the url data,
		//make sure there is memory to hold it before allocating memory for handle.
		//if fails, NewHandleClear(...) should return NULL.
		const char* url_string = url.c_str() ;
		Handle handle = NewHandleClear( ( Size )( url.length() + 1 ) );

		if ( NULL == handle || noErr != MemError() || NULL == *handle )
		{
			setStatus(STATUS_ERROR);
			return;
		}

		BlockMove( url_string, *handle, ( Size )( url.length() + 1 ) );

		OSErr err = NewMovieFromDataRef( &mMovieHandle, newMovieActive | newMovieDontInteractWithUser | newMovieAsyncOK | newMovieIdleImportOK, nil, handle, URLDataHandlerSubType );
		DisposeHandle( handle );
		if ( noErr != err )
		{
			setStatus(STATUS_ERROR);
			return;
		};
		
		mNavigateURL = url;
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_begin");
		message.setValue("uri", mNavigateURL);
		sendMessage(message);

		// do pre-roll actions (typically fired for streaming movies but not always)
		PrePrerollMovie( mMovieHandle, 0, getPlayRate(), moviePrePrerollCompleteCallback, ( void * )this );

		Rect movie_rect = rectFromSize(mWidth, mHeight);

		// make a new movie controller
		mMovieController = NewMovieController( mMovieHandle, &movie_rect, mcNotVisible | mcTopLeftMovie );

		// movie controller
		MCSetActionFilterWithRefCon( mMovieController, mcActionFilterCallBack, ( long )this );

		SetMoviePlayHints( mMovieHandle, hintsAllowDynamicResize, hintsAllowDynamicResize );

		// function that gets called when a frame is drawn
		SetMovieDrawingCompleteProc( mMovieHandle, movieDrawingCallWhenChanged, movieDrawingCompleteCallback, ( long )this );

		setStatus(STATUS_LOADED);

		sizeChanged();
	};

	bool unload()
	{
		// new movie and have to get title again
		mReceivedTitle = false;

		if ( mMovieHandle )
		{
			StopMovie( mMovieHandle );
			if ( mMovieController )
			{
				MCMovieChanged( mMovieController, mMovieHandle );
			};
		};

		if ( mMovieController )
		{
			MCSetActionFilterWithRefCon( mMovieController, NULL, (long)this );
			DisposeMovieController( mMovieController );
			mMovieController = NULL;
		};

		if ( mMovieHandle )
		{
			SetMovieDrawingCompleteProc( mMovieHandle, movieDrawingCallWhenChanged, nil, ( long )this );
			DisposeMovie( mMovieHandle );
			mMovieHandle = NULL;
		};

		if ( mGWorldHandle )
		{
			DisposeGWorld( mGWorldHandle );
			mGWorldHandle = NULL;
		};

		setStatus(STATUS_NONE);

		return true;
	}

	bool navigateTo( const std::string url )
	{
		unload();
		load( url );

		return true;
	};

	bool sizeChanged()
	{
		if ( ! mMovieHandle )
			return false;

		// Check to see whether the movie's natural size has updated
		{
			int width, height;
			getMovieNaturalSize(&width, &height);
			if((width != 0) && (height != 0) && ((width != mNaturalWidth) || (height != mNaturalHeight)))
			{
				mNaturalWidth = width;
				mNaturalHeight = height;

				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_request");
				message.setValue("name", mTextureSegmentName);
				message.setValueS32("width", width);
				message.setValueS32("height", height);
				sendMessage(message);
				//std::cerr << "<--- Sending size change request to application with name: " << mTextureSegmentName << " - size is " << width << " x " << height << std::endl;
			}
		}

		// sanitize destination size
		Rect dest_rect = rectFromSize(mWidth, mHeight);

		// media depth won't change
		int depth_bits = mDepth * 8;
		long rowbytes = mDepth * mTextureWidth;

		GWorldPtr old_gworld_handle = mGWorldHandle;

		if(mPixels != NULL)
		{
			// We have pixels.  Set up a GWorld pointing at the texture.
			OSErr result = NewGWorldFromPtr( &mGWorldHandle, depth_bits, &dest_rect, NULL, NULL, 0, (Ptr)mPixels, rowbytes);
			if ( noErr != result )
			{
				// TODO: unrecoverable??  throw exception?  return something?
				return false;
			}
		}
		else
		{
			// We don't have pixels. Create a fake GWorld we can point the movie at when it's not safe to render normally.
			Rect tempRect = rectFromSize(1, 1);
			OSErr result = NewGWorld( &mGWorldHandle, depth_bits, &tempRect, NULL, NULL, 0);
			if ( noErr != result )
			{
				// TODO: unrecoverable??  throw exception?  return something?
				return false;
			}
		}

		SetMovieGWorld( mMovieHandle, mGWorldHandle, GetGWorldDevice( mGWorldHandle ) );

		// If the GWorld was already set up, delete it.
		if(old_gworld_handle != NULL)
		{
			DisposeGWorld( old_gworld_handle );
		}

		// Set up the movie display matrix
		{
			// scale movie to fit rect and invert vertically to match opengl image format
			MatrixRecord transform;
			SetIdentityMatrix( &transform );	// transforms are additive so start from identify matrix
			double scaleX = (double) mWidth / mNaturalWidth;
			double scaleY = -1.0 * (double) mHeight / mNaturalHeight;
			double centerX = mWidth / 2.0;
			double centerY = mHeight / 2.0;
			ScaleMatrix( &transform, X2Fix( scaleX ), X2Fix( scaleY ), X2Fix( centerX ), X2Fix( centerY ) );
			SetMovieMatrix( mMovieHandle, &transform );
		}

		// update movie controller
		if ( mMovieController )
		{
			MCSetControllerPort( mMovieController, mGWorldHandle );
			MCPositionController( mMovieController, &dest_rect, &dest_rect,
								  mcTopLeftMovie | mcPositionDontInvalidate );
			MCMovieChanged( mMovieController, mMovieHandle );
		}


		// Emit event with size change so the calling app knows about it too
		// TODO:
		//LLMediaEvent event( this );
		//mEventEmitter.update( &LLMediaObserver::onMediaSizeChange, event );

		return true;
	}
	static Boolean mcActionFilterCallBack( MovieController mc, short action, void *params, long ref )
	{
		Boolean result = false;

		MediaPluginQuickTime* self = ( MediaPluginQuickTime* )ref;

		switch( action )
		{
			// handle window resizing
			case mcActionControllerSizeChanged:
				// Ensure that the movie draws correctly at the new size
				self->sizeChanged();
				break;

			// Block any movie controller actions that open URLs.
			case mcActionLinkToURL:
			case mcActionGetNextURL:
			case mcActionLinkToURLExtended:
				// Prevent the movie controller from handling the message
				result = true;
				break;

			default:
				break;
		};

		return result;
	};

	static OSErr movieDrawingCompleteCallback( Movie call_back_movie, long ref )
	{
		MediaPluginQuickTime* self = ( MediaPluginQuickTime* )ref;

		// IMPORTANT: typically, a consumer who is observing this event will set a flag
		// when this event is fired then render later. Be aware that the media stream
		// can change during this period - dimensions, depth, format etc.
		//LLMediaEvent event( self );
//		self->updateQuickTime();
		// TODO ^^^


		if ( self->mWidth > 0 && self->mHeight > 0 )
			self->setDirty( 0, 0, self->mWidth, self->mHeight );

		return noErr;
	};

	static void moviePrePrerollCompleteCallback( Movie movie, OSErr preroll_err, void *ref )
	{
		MediaPluginQuickTime* self = ( MediaPluginQuickTime* )ref;

		// TODO:
		//LLMediaEvent event( self );
		//self->mEventEmitter.update( &LLMediaObserver::onMediaPreroll, event );
		
		// Send a "navigate complete" event.
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_complete");
		message.setValue("uri", self->mNavigateURL);
		message.setValueS32("result_code", 200);
		message.setValue("result_string", "OK");
		self->sendMessage(message);
	};


	void rewind()
	{
		GoToBeginningOfMovie( mMovieHandle );
		MCMovieChanged( mMovieController, mMovieHandle );
	};

	bool processState()
	{
		if ( mCommand == COMMAND_PLAY )
		{
			if ( mStatus == STATUS_LOADED || mStatus == STATUS_PAUSED || mStatus == STATUS_PLAYING || mStatus == STATUS_DONE )
			{
				long state = GetMovieLoadState( mMovieHandle );

				if ( state >= kMovieLoadStatePlaythroughOK )
				{
					// if the movie is at the end (generally because it reached it naturally)
					// and we play is requested, jump back to the start of the movie.
					// note: this is different from having loop flag set.
					if ( IsMovieDone( mMovieHandle ) )
					{
						Fixed rate = X2Fix( 0.0 );
						MCDoAction( mMovieController, mcActionPlay, (void*)rate );
						rewind();
					};

					MCDoAction( mMovieController, mcActionPrerollAndPlay, (void*)getPlayRate() );
					MCDoAction( mMovieController, mcActionSetVolume, (void*)mCurVolume );
					setStatus(STATUS_PLAYING);
					mCommand = COMMAND_NONE;
				};
			};
		}
		else
		if ( mCommand == COMMAND_STOP )
		{
			if ( mStatus == STATUS_PLAYING || mStatus == STATUS_PAUSED || mStatus == STATUS_DONE )
			{
				if ( GetMovieLoadState( mMovieHandle ) >= kMovieLoadStatePlaythroughOK )
				{
					Fixed rate = X2Fix( 0.0 );
					MCDoAction( mMovieController, mcActionPlay, (void*)rate );
					rewind();

					setStatus(STATUS_LOADED);
					mCommand = COMMAND_NONE;
				};
			};
		}
		else
		if ( mCommand == COMMAND_PAUSE )
		{
			if ( mStatus == STATUS_PLAYING )
			{
				if ( GetMovieLoadState( mMovieHandle ) >= kMovieLoadStatePlaythroughOK )
				{
					Fixed rate = X2Fix( 0.0 );
					MCDoAction( mMovieController, mcActionPlay, (void*)rate );
					setStatus(STATUS_PAUSED);
					mCommand = COMMAND_NONE;
				};
			};
		};

		return true;
	};

	void play(F64 rate)
	{
		mPlayRate = rate;
		mCommand = COMMAND_PLAY;
	};

	void stop()
	{
		mCommand = COMMAND_STOP;
	};

	void pause()
	{
		mCommand = COMMAND_PAUSE;
	};

	void getMovieNaturalSize(int *movie_width, int *movie_height)
	{
		Rect rect;

		GetMovieNaturalBoundsRect( mMovieHandle, &rect );

		int width  = ( rect.right - rect.left );
		int height = ( rect.bottom - rect.top );

		// make sure width and height fall in valid range
		if ( width < mMinWidth )
			width = mMinWidth;

		if ( width > mMaxWidth )
			width = mMaxWidth;

		if ( height < mMinHeight )
			height = mMinHeight;

		if ( height > mMaxHeight )
			height = mMaxHeight;

		// return the new rect
		*movie_width = width;
		*movie_height = height;
	}

	void updateQuickTime(int milliseconds)
	{
		if ( ! mMovieHandle )
			return;

		if ( ! mMovieController )
			return;

		// this wasn't required in 1.xx viewer but we have to manually 
		// work the Windows message pump now
		#if defined( LL_WINDOWS )
		MSG msg;
		while ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
		{
			GetMessage( &msg, NULL, 0, 0 );
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		};
		#endif

		MCIdle( mMovieController );

		if ( ! mGWorldHandle )
			return;

		if ( mMediaSizeChanging )
			return;

		// update state machine
		processState();

		// see if title arrived and if so, update member variable with contents
		checkTitle();
		
		// QT call to see if we are at the end - can't do with controller
		if ( IsMovieDone( mMovieHandle ) )
		{
			// special code for looping - need to rewind at the end of the movie
			if ( mIsLooping )
			{
				// go back to start
				rewind();

				if ( mMovieController )
				{
					// kick off new play
					MCDoAction( mMovieController, mcActionPrerollAndPlay, (void*)getPlayRate() );

					// set the volume
					MCDoAction( mMovieController, mcActionSetVolume, (void*)mCurVolume );
				};
			}
			else
			{
				if(mStatus == STATUS_PLAYING)
				{
					setStatus(STATUS_DONE);
				}
			}
		}

	};

	int getDataWidth() const
	{
		if ( mGWorldHandle )
		{
			int depth = mDepth;

			if (depth < 1)
				depth = 1;

			// ALWAYS use the row bytes from the PixMap if we have a GWorld because
			// sometimes it's not the same as mMediaDepth * mMediaWidth !
			PixMapHandle pix_map_handle = GetGWorldPixMap( mGWorldHandle );
			return QTGetPixMapHandleRowBytes( pix_map_handle ) / depth;
		}
		else
		{
			// TODO :   return LLMediaImplCommon::getaDataWidth();
			return 0;
		}
	};

	void seek( F64 time )
	{
		if ( mMovieController )
		{
			TimeRecord when;
			when.scale = GetMovieTimeScale( mMovieHandle );
			when.base = 0;

			// 'time' is in (floating point) seconds.  The timebase time will be in 'units', where
			// there are 'scale' units per second.
			SInt64 raw_time = ( SInt64 )( time * (double)( when.scale ) );

			when.value.hi = ( SInt32 )( raw_time >> 32 );
			when.value.lo = ( SInt32 )( ( raw_time & 0x00000000FFFFFFFF ) );

			MCDoAction( mMovieController, mcActionGoToTime, &when );
		};
	};

	F64 getLoadedDuration() 	  	 
	{ 	  	 
		TimeValue duration; 	  	 
		if(GetMaxLoadedTimeInMovie( mMovieHandle, &duration ) != noErr) 	  	 
		{ 	  	 
			// If GetMaxLoadedTimeInMovie returns an error, return the full duration of the movie. 	  	 
			duration = GetMovieDuration( mMovieHandle ); 	  	 
		} 	  	 
		TimeValue scale = GetMovieTimeScale( mMovieHandle ); 	  	 

		return (F64)duration / (F64)scale; 	  	 
	}; 	  	 

	F64 getDuration()
	{
		TimeValue duration = GetMovieDuration( mMovieHandle );
		TimeValue scale = GetMovieTimeScale( mMovieHandle );

		return (F64)duration / (F64)scale;
	};

	F64 getCurrentTime()
	{
		TimeValue curr_time = GetMovieTime( mMovieHandle, 0 );
		TimeValue scale = GetMovieTimeScale( mMovieHandle );

		return (F64)curr_time / (F64)scale;
	};

	void setVolume( F64 volume )
	{
		mCurVolume = (short)(volume * ( double ) 0x100 );

		if ( mMovieController )
		{
			MCDoAction( mMovieController, mcActionSetVolume, (void*)mCurVolume );
		};
	};

	////////////////////////////////////////////////////////////////////////////////
	//
	void update(int milliseconds = 0)
	{
		updateQuickTime(milliseconds);
	};

	////////////////////////////////////////////////////////////////////////////////
	//
	void mouseDown( int x, int y )
	{
	};

	////////////////////////////////////////////////////////////////////////////////
	//
	void mouseUp( int x, int y )
	{
	};

	////////////////////////////////////////////////////////////////////////////////
	//
	void mouseMove( int x, int y )
	{
	};

	////////////////////////////////////////////////////////////////////////////////
	//
	void keyPress( unsigned char key )
	{
	};

	////////////////////////////////////////////////////////////////////////////////
	// Grab movie title into mMovieTitle - should be called repeatedly
	// until it returns true since movie title takes a while to become 
	// available.
	const bool getMovieTitle()
	{
		// grab meta data from movie
		QTMetaDataRef media_data_ref;
		OSErr result = QTCopyMovieMetaData( mMovieHandle, &media_data_ref );
		if ( noErr != result ) 
			return false;

		// look up "Display Name" in meta data
		OSType meta_data_key = kQTMetaDataCommonKeyDisplayName;
		QTMetaDataItem item = kQTMetaDataItemUninitialized;
		result = QTMetaDataGetNextItem( media_data_ref, kQTMetaDataStorageFormatWildcard, 
										0, kQTMetaDataKeyFormatCommon, 
										(const UInt8 *)&meta_data_key, 
										sizeof( meta_data_key ), &item );
		if ( noErr != result ) 
			return false;

		// find the size of the title
		ByteCount size;
		result = QTMetaDataGetItemValue( media_data_ref, item, NULL, 0, &size );
		if ( noErr != result || size <= 0 /*|| size > 1024  FIXME: arbitrary limit */ ) 
			return false;

		// allocate some space and grab it
		UInt8* item_data = new UInt8[ size + 1 ];
		memset( item_data, 0, ( size + 1 ) * sizeof( UInt8 ) );
		result = QTMetaDataGetItemValue( media_data_ref, item, item_data, size, NULL );
		if ( noErr != result ) 
		{
			delete [] item_data;
			return false;
		};

		// save it
		if ( strlen( (char*)item_data ) )
			mMovieTitle = std::string( (char* )item_data );
		else
			mMovieTitle = "";

		// clean up
		delete [] item_data;

		return true;
	};

	// called regularly to see if title changed
	void checkTitle()
	{
		// we did already receive title so keep checking
		if ( ! mReceivedTitle )
		{
			// grab title from movie meta data
			if ( getMovieTitle() )
			{
				// pass back to host application
				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
				message.setValue("name", mMovieTitle );
				sendMessage( message );

				// stop looking once we find a title for this movie.
				// TODO: this may to be reset if movie title changes
				// during playback but this is okay for now
				mReceivedTitle = true;
			};
		};
	};
};

MediaPluginQuickTime::MediaPluginQuickTime(
	LLPluginInstance::sendMessageFunction host_send_func,
	void *host_user_data ) :
	MediaPluginBase(host_send_func, host_user_data),
	mMinWidth( 0 ),
	mMaxWidth( 2048 ),
	mMinHeight( 0 ),
	mMaxHeight( 2048 )
{
//	std::cerr << "MediaPluginQuickTime constructor" << std::endl;

	mNaturalWidth = -1;
	mNaturalHeight = -1;
	mMovieHandle = 0;
	mGWorldHandle = 0;
	mMovieController = 0;
	mCurVolume = 0x99;
	mMediaSizeChanging = false;
	mIsLooping = false;
	mMovieTitle = std::string();
	mReceivedTitle = false;
	mCommand = COMMAND_NONE;
	mPlayRate = 0.0f;
	mStatus = STATUS_NONE;
}

MediaPluginQuickTime::~MediaPluginQuickTime()
{
//	std::cerr << "MediaPluginQuickTime destructor" << std::endl;

	ExitMovies();

#ifdef LL_WINDOWS
	TerminateQTML();
//		std::cerr << "QuickTime closing down" << std::endl;
#endif
}


void MediaPluginQuickTime::receiveMessage(const char *message_string)
{
//	std::cerr << "MediaPluginQuickTime::receiveMessage: received message: \"" << message_string << "\"" << std::endl;
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
				// Normally a plugin would only specify one of these two subclasses, but this is a demo...
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME] = LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME_VERSION;
				message.setValueLLSD("versions", versions);

				#ifdef LL_WINDOWS

				// QuickTime 7.6.4 has an issue (that was not present in 7.6.2) with initializing QuickTime
				// according to this article: http://lists.apple.com/archives/QuickTime-API/2009/Sep/msg00097.html
				// The solution presented there appears to work.
				QTLoadLibrary("qtcf.dll");

				// main initialization for QuickTime - only required on Windows
				OSErr result = InitializeQTML( 0L );
				if ( result != noErr )
				{
					//TODO: If no QT on Windows, this fails - respond accordingly.
				}
				else
				{
					//std::cerr << "QuickTime initialized" << std::endl;
				};
				#endif

				// required for both Windows and Mac
				EnterMovies();

				std::string plugin_version = "QuickTime media plugin, QuickTime version ";

				long version = 0;
				Gestalt( gestaltQuickTimeVersion, &version );
				std::ostringstream codec( "" );
				codec << std::hex << version << std::dec;
				plugin_version += codec.str();
				message.setValue("plugin_version", plugin_version);
				sendMessage(message);

				// Plugin gets to decide the texture parameters to use.
				message.setMessage(LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params");
				#if defined(LL_WINDOWS)
					// Values for Windows
					mDepth = 3;
					message.setValueU32("format", GL_RGB);
					message.setValueU32("type", GL_UNSIGNED_BYTE);

					// We really want to pad the texture width to a multiple of 32 bytes, but since we're using 3-byte pixels, it doesn't come out even.
					// Padding to a multiple of 3*32 guarantees it'll divide out properly.
					message.setValueU32("padding", 32 * 3);
				#else
					// Values for Mac
					mDepth = 4;
					message.setValueU32("format", GL_BGRA_EXT);
					#ifdef __BIG_ENDIAN__
						message.setValueU32("type", GL_UNSIGNED_INT_8_8_8_8_REV );
					#else
						message.setValueU32("type", GL_UNSIGNED_INT_8_8_8_8);
					#endif

					// Pad texture width to a multiple of 32 bytes, to line up with cache lines.
					message.setValueU32("padding", 32);
				#endif
				message.setValueS32("depth", mDepth);
				message.setValueU32("internalformat", GL_RGB);
				message.setValueBoolean("coords_opengl", true);	// true == use OpenGL-style coordinates, false == (0,0) is upper left.
				message.setValueBoolean("allow_downsample", true);
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
				// TODO: clean up here
			}
			else if(message_name == "shm_added")
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer("address");
				info.mSize = (size_t)message_in.getValueS32("size");
				std::string name = message_in.getValue("name");
//				std::cerr << "MediaPluginQuickTime::receiveMessage: shared memory added, name: " << name
//					<< ", size: " << info.mSize
//					<< ", address: " << info.mAddress
//					<< std::endl;

				mSharedSegments.insert(SharedSegmentMap::value_type(name, info));

			}
			else if(message_name == "shm_remove")
			{
				std::string name = message_in.getValue("name");

//				std::cerr << "MediaPluginQuickTime::receiveMessage: shared memory remove, name = " << name << std::endl;

				SharedSegmentMap::iterator iter = mSharedSegments.find(name);
				if(iter != mSharedSegments.end())
				{
					if(mPixels == iter->second.mAddress)
					{
						// This is the currently active pixel buffer.  Make sure we stop drawing to it.
						mPixels = NULL;
						mTextureSegmentName.clear();

						// Make sure the movie GWorld is no longer pointed at the shared segment.
						sizeChanged();
					}
					mSharedSegments.erase(iter);
				}
				else
				{
//					std::cerr << "MediaPluginQuickTime::receiveMessage: unknown shared memory region!" << std::endl;
				}

				// Send the response so it can be cleaned up.
				LLPluginMessage message("base", "shm_remove_response");
				message.setValue("name", name);
				sendMessage(message);
			}
			else
			{
//				std::cerr << "MediaPluginQuickTime::receiveMessage: unknown base message: " << message_name << std::endl;
			}
		}
		else if(message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
		{
			if(message_name == "size_change")
			{
				std::string name = message_in.getValue("name");
				S32 width = message_in.getValueS32("width");
				S32 height = message_in.getValueS32("height");
				S32 texture_width = message_in.getValueS32("texture_width");
				S32 texture_height = message_in.getValueS32("texture_height");

				//std::cerr << "---->Got size change instruction from application with name: " << name << " - size is " << width << " x " << height << std::endl;

				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_response");
				message.setValue("name", name);
				message.setValueS32("width", width);
				message.setValueS32("height", height);
				message.setValueS32("texture_width", texture_width);
				message.setValueS32("texture_height", texture_height);
				sendMessage(message);

				if(!name.empty())
				{
					// Find the shared memory region with this name
					SharedSegmentMap::iterator iter = mSharedSegments.find(name);
					if(iter != mSharedSegments.end())
					{
//						std::cerr << "%%% Got size change, new size is " << width << " by " << height << std::endl;
//						std::cerr << "%%%%  texture size is " << texture_width << " by " << texture_height << std::endl;

						mPixels = (unsigned char*)iter->second.mAddress;
						mTextureSegmentName = name;
						mWidth = width;
						mHeight = height;

						mTextureWidth = texture_width;
						mTextureHeight = texture_height;

						mMediaSizeChanging = false;

						sizeChanged();

						update();
					};
				};
			}
			else if(message_name == "load_uri")
			{
				std::string uri = message_in.getValue("uri");
				load( uri );
				sendStatus();
			}
			else if(message_name == "mouse_event")
			{
				std::string event = message_in.getValue("event");
				S32 x = message_in.getValueS32("x");
				S32 y = message_in.getValueS32("y");

				if(event == "down")
				{
					mouseDown(x, y);
				}
				else if(event == "up")
				{
					mouseUp(x, y);
				}
				else if(event == "move")
				{
					mouseMove(x, y);
				};
			};
		}
		else if(message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME)
		{
			if(message_name == "stop")
			{
				stop();
			}
			else if(message_name == "start")
			{
				F64 rate = 0.0;
				if(message_in.hasValue("rate"))
				{
					rate = message_in.getValueReal("rate");
				}
				play(rate);
			}
			else if(message_name == "pause")
			{
				pause();
			}
			else if(message_name == "seek")
			{
				F64 time = message_in.getValueReal("time");
				seek(time);
			}
			else if(message_name == "set_loop")
			{
				bool loop = message_in.getValueBoolean("loop");
				mIsLooping = loop;
			}
			else if(message_name == "set_volume")
			{
				F64 volume = message_in.getValueReal("volume");
				setVolume(volume);
			}
		}
		else
		{
//			std::cerr << "MediaPluginQuickTime::receiveMessage: unknown message class: " << message_class << std::endl;
		};
	};
}

int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data, LLPluginInstance::sendMessageFunction *plugin_send_func, void **plugin_user_data)
{
	MediaPluginQuickTime *self = new MediaPluginQuickTime(host_send_func, host_user_data);
	*plugin_send_func = MediaPluginQuickTime::staticReceiveMessage;
	*plugin_user_data = (void*)self;

	return 0;
}

#else // LL_QUICKTIME_ENABLED

// Stubbed-out class with constructor/destructor (necessary or windows linker
// will just think its dead code and optimize it all out)
class MediaPluginQuickTime : public MediaPluginBase
{
public:
	MediaPluginQuickTime(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginQuickTime();
	/* virtual */ void receiveMessage(const char *message_string);
};

MediaPluginQuickTime::MediaPluginQuickTime(
	LLPluginInstance::sendMessageFunction host_send_func,
	void *host_user_data ) :
	MediaPluginBase(host_send_func, host_user_data)
{
    // no-op
}

MediaPluginQuickTime::~MediaPluginQuickTime()
{
    // no-op
}

void MediaPluginQuickTime::receiveMessage(const char *message_string)
{
    // no-op
}

// We're building without quicktime enabled.  Just refuse to initialize.
int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data, LLPluginInstance::sendMessageFunction *plugin_send_func, void **plugin_user_data)
{
    return -1;
}

#endif // LL_QUICKTIME_ENABLED
