/** 
 * @file media_plugin_gstreamer010.cpp
 * @brief GStreamer-0.10 plugin for LLMedia API plugin system
 *
 * @cond
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 *
 * Copyright (c) 2007, Linden Research, Inc.
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

#if LL_GSTREAMER010_ENABLED

extern "C" {
#include <gst/gst.h>
}

#include "llmediaimplgstreamer.h"
#include "llmediaimplgstreamertriviallogging.h"

#include "llmediaimplgstreamervidplug.h"

#include "llmediaimplgstreamer_syms.h"

//////////////////////////////////////////////////////////////////////////////
//
class MediaPluginGStreamer010 : public MediaPluginBase
{
public:
	MediaPluginGStreamer010(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginGStreamer010();

	/* virtual */ void receiveMessage(const char *message_string);

	static bool startup();
	static bool closedown();

	gboolean processGSTEvents(GstBus     *bus,
				  GstMessage *message);

private:
	std::string getVersion();
	bool navigateTo( const std::string urlIn );
	bool seek( double time_sec );
	bool setVolume( float volume );
	
	// misc
	bool pause();
	bool stop();
	bool play(double rate);
	bool getTimePos(double &sec_out);

	static const double MIN_LOOP_SEC = 1.0F;

	bool mIsLooping;

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

private:
	bool unload();
	bool load();

	bool update(int milliseconds);
        void mouseDown( int x, int y );
        void mouseUp( int x, int y );
        void mouseMove( int x, int y );

        void sizeChanged();
	
	static bool mDoneInit;
	
	guint mBusWatchID;
	
	float mVolume;

	int mDepth;

	// media NATURAL size
	int mNaturalWidth;
	int mNaturalHeight;
	// media current size
	int mCurrentWidth;
	int mCurrentHeight;
	int mCurrentRowbytes;
	  // previous media size so we can detect changes
	  int mPreviousWidth;
	  int mPreviousHeight;
	// desired render size from host
	int mWidth;
	int mHeight;
	// padded texture size we need to write into
	int mTextureWidth;
	int mTextureHeight;
	
	int mTextureFormatPrimary;
	int mTextureFormatType;

	bool mSeekWanted;
	double mSeekDestination;
	
	// Very GStreamer-specific
	GMainLoop *mPump; // event pump for this media
	GstElement *mPlaybin;
	GstSLVideo *mVideoSink;
};

//static
bool MediaPluginGStreamer010::mDoneInit = false;

MediaPluginGStreamer010::MediaPluginGStreamer010(
	LLPluginInstance::sendMessageFunction host_send_func,
	void *host_user_data ) :
	MediaPluginBase(host_send_func, host_user_data),
	mBusWatchID ( 0 ),
	mCurrentRowbytes ( 4 ),
	mTextureFormatPrimary ( GL_RGBA ),
	mTextureFormatType ( GL_UNSIGNED_INT_8_8_8_8_REV ),
	mSeekWanted(false),
	mSeekDestination(0.0),
	mPump ( NULL ),
	mPlaybin ( NULL ),
	mVideoSink ( NULL ),
	mCommand ( COMMAND_NONE )
{
	std::ostringstream str;
	INFOMSG("MediaPluginGStreamer010 constructor - my PID=%u", U32(getpid()));
}

///////////////////////////////////////////////////////////////////////////////
//
//#define LL_GST_REPORT_STATE_CHANGES
#ifdef LL_GST_REPORT_STATE_CHANGES
static char* get_gst_state_name(GstState state)
{
	switch (state) {
	case GST_STATE_VOID_PENDING: return "VOID_PENDING";
	case GST_STATE_NULL: return "NULL";
	case GST_STATE_READY: return "READY";
	case GST_STATE_PAUSED: return "PAUSED";
	case GST_STATE_PLAYING: return "PLAYING";
	}
	return "(unknown)";
}
#endif // LL_GST_REPORT_STATE_CHANGES

gboolean
MediaPluginGStreamer010::processGSTEvents(GstBus     *bus,
					  GstMessage *message)
{
	if (!message) 
		return TRUE; // shield against GStreamer bug

	if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_STATE_CHANGED &&
	    GST_MESSAGE_TYPE(message) != GST_MESSAGE_BUFFERING)
	{
		DEBUGMSG("Got GST message type: %s",
			LLGST_MESSAGE_TYPE_NAME (message));
	}
	else
	{
		// TODO: grok 'duration' message type
		DEBUGMSG("Got GST message type: %s",
			 LLGST_MESSAGE_TYPE_NAME (message));
	}

	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_BUFFERING: {
		// NEEDS GST 0.10.11+
		if (llgst_message_parse_buffering)
		{
			gint percent = 0;
			llgst_message_parse_buffering(message, &percent);
			DEBUGMSG("GST buffering: %d%%", percent);
		}
		break;
	}
	case GST_MESSAGE_STATE_CHANGED: {
		GstState old_state;
		GstState new_state;
		GstState pending_state;
		llgst_message_parse_state_changed(message,
						&old_state,
						&new_state,
						&pending_state);
#ifdef LL_GST_REPORT_STATE_CHANGES
		// not generally very useful, and rather spammy.
		DEBUGMSG("state change (old,<new>,pending): %s,<%s>,%s",
			 get_gst_state_name(old_state),
			 get_gst_state_name(new_state),
			 get_gst_state_name(pending_state));
#endif // LL_GST_REPORT_STATE_CHANGES

		switch (new_state) {
		case GST_STATE_VOID_PENDING:
			break;
		case GST_STATE_NULL:
			break;
		case GST_STATE_READY:
			setStatus(STATUS_LOADED);
			break;
		case GST_STATE_PAUSED:
			setStatus(STATUS_PAUSED);
			break;
		case GST_STATE_PLAYING:
			setStatus(STATUS_PLAYING);
			break;
		}
		break;
	}
	case GST_MESSAGE_ERROR: {
		GError *err = NULL;
		gchar *debug = NULL;

		llgst_message_parse_error (message, &err, &debug);
		WARNMSG("GST error: %s", err?err->message:"(unknown)");
		if (err)
			g_error_free (err);
		g_free (debug);

		mCommand = COMMAND_STOP;

		setStatus(STATUS_ERROR);

		break;
	}
	case GST_MESSAGE_INFO: {
		if (llgst_message_parse_info)
		{
			GError *err = NULL;
			gchar *debug = NULL;
			
			llgst_message_parse_info (message, &err, &debug);
			INFOMSG("GST info: %s", err?err->message:"(unknown)");
			if (err)
				g_error_free (err);
			g_free (debug);
		}
		break;
	}
	case GST_MESSAGE_WARNING: {
		GError *err = NULL;
		gchar *debug = NULL;

		llgst_message_parse_warning (message, &err, &debug);
		WARNMSG("GST warning: %s", err?err->message:"(unknown)");
		if (err)
			g_error_free (err);
		g_free (debug);

		break;
	}
	case GST_MESSAGE_EOS:
		/* end-of-stream */
		DEBUGMSG("GST end-of-stream.");
		if (mIsLooping)
		{
			DEBUGMSG("looping media...");
			double eos_pos_sec = 0.0F;
			bool got_eos_position = getTimePos(eos_pos_sec);

			if (got_eos_position && eos_pos_sec < MIN_LOOP_SEC)
			{
				// if we know that the movie is really short, don't
				// loop it else it can easily become a time-hog
				// because of GStreamer spin-up overhead
				DEBUGMSG("really short movie (%0.3fsec) - not gonna loop this, pausing instead.", eos_pos_sec);
				// inject a COMMAND_PAUSE
				mCommand = COMMAND_PAUSE;
			}
			else
			{
#undef LLGST_LOOP_BY_SEEKING
// loop with a stop-start instead of a seek, because it actually seems rather
// faster than seeking on remote streams.
#ifdef LLGST_LOOP_BY_SEEKING
				// first, try looping by an explicit rewind
				bool seeksuccess = seek(0.0);
				if (seeksuccess)
				{
					play(1.0);
				}
				else
#endif // LLGST_LOOP_BY_SEEKING
				{  // use clumsy stop-start to loop
					DEBUGMSG("didn't loop by rewinding - stopping and starting instead...");
					stop();
					play(1.0);
				}
			}
		}
		else // not a looping media
		{
			// inject a COMMAND_STOP
			mCommand = COMMAND_STOP;
		}
		break;
	default:
		/* unhandled message */
		break;
	}

	/* we want to be notified again the next time there is a message
	 * on the bus, so return true (false means we want to stop watching
	 * for messages on the bus and our callback should not be called again)
	 */
	return TRUE;
}

extern "C" {
gboolean
llmediaimplgstreamer_bus_callback (GstBus     *bus,
				   GstMessage *message,
				   gpointer    data)
{
	MediaPluginGStreamer010 *impl = (MediaPluginGStreamer010*)data;
	return impl->processGSTEvents(bus, message);
}
} // extern "C"



bool
MediaPluginGStreamer010::navigateTo ( const std::string urlIn )
{
	if (!mDoneInit)
		return false; // error

	setStatus(STATUS_LOADING);

	DEBUGMSG("Setting media URI: %s", urlIn.c_str());

	mSeekWanted = false;

	if (NULL == mPump ||
	    NULL == mPlaybin)
	{
		setStatus(STATUS_ERROR);
		return false; // error
	}

	// set URI
	g_object_set (G_OBJECT (mPlaybin), "uri", urlIn.c_str(), NULL);
	//g_object_set (G_OBJECT (mPlaybin), "uri", "file:///tmp/movie", NULL);

	// navigateTo implicitly plays, too.
	play(1.0);

	return true;
}


bool
MediaPluginGStreamer010::update(int milliseconds)
{
	if (!mDoneInit)
		return false; // error

	DEBUGMSG("updating media...");
	
	// sanity check
	if (NULL == mPump ||
	    NULL == mPlaybin)
	{
		DEBUGMSG("dead media...");
		return false;
	}

	// see if there's an outstanding seek wanted
	if (mSeekWanted &&
	    // bleh, GST has to be happy that the movie is really truly playing
	    // or it may quietly ignore the seek (with rtsp:// at least).
	    (GST_STATE(mPlaybin) == GST_STATE_PLAYING))
	{
		seek(mSeekDestination);
		mSeekWanted = false;
	}

	// *TODO: time-limit - but there isn't a lot we can do here, most
	// time is spent in gstreamer's own opaque worker-threads.  maybe
	// we can do something sneaky like only unlock the video object
	// for 'milliseconds' and otherwise hold the lock.
	while (g_main_context_pending(g_main_loop_get_context(mPump)))
	{
	       g_main_context_iteration(g_main_loop_get_context(mPump), FALSE);
	}

	// check for availability of a new frame
	
	if (mVideoSink)
	{
	        GST_OBJECT_LOCK(mVideoSink);
		if (mVideoSink->retained_frame_ready)
		{
			DEBUGMSG("NEW FRAME READY");

			if (mVideoSink->retained_frame_width != mCurrentWidth ||
			    mVideoSink->retained_frame_height != mCurrentHeight)
				// *TODO: also check for change in format
			{
				// just resize container, don't consume frame
				int neww = mVideoSink->retained_frame_width;
				int newh = mVideoSink->retained_frame_height;

				int newd = 4;
				mTextureFormatPrimary = GL_RGBA;
				mTextureFormatType = GL_UNSIGNED_INT_8_8_8_8_REV;

				/*
				int newd = SLVPixelFormatBytes[mVideoSink->retained_frame_format];
				if (SLV_PF_BGRX == mVideoSink->retained_frame_format)
				{
					mTextureFormatPrimary = GL_BGRA;
					mTextureFormatType = GL_UNSIGNED_INT_8_8_8_8_REV;
				}
				else
				{
					mTextureFormatPrimary = GL_RGBA;
					mTextureFormatType = GL_UNSIGNED_INT_8_8_8_8_REV;
				}
				*/

				GST_OBJECT_UNLOCK(mVideoSink);

				mCurrentRowbytes = neww * newd;
				DEBUGMSG("video container resized to %dx%d",
					 neww, newh);

				mDepth = newd;
				mCurrentWidth = neww;
				mCurrentHeight = newh;
				sizeChanged();
				return true;
			}

			if (mPixels &&
			    mCurrentHeight <= mHeight &&
			    mCurrentWidth <= mWidth &&
			    !mTextureSegmentName.empty())
			{
				// we're gonna totally consume this frame - reset 'ready' flag
				mVideoSink->retained_frame_ready = FALSE;
				int destination_rowbytes = mWidth * mDepth;
				for (int row=0; row<mCurrentHeight; ++row)
				{
					memcpy(&mPixels
					        [destination_rowbytes * row],
					       &mVideoSink->retained_frame_data
					        [mCurrentRowbytes * row],
					       mCurrentRowbytes);
				}

				GST_OBJECT_UNLOCK(mVideoSink);
				DEBUGMSG("NEW FRAME REALLY TRULY CONSUMED, TELLING HOST");

				setDirty(0,0,mCurrentWidth,mCurrentHeight);
			}
			else
			{
				// new frame ready, but we're not ready to
				// consume it.

				GST_OBJECT_UNLOCK(mVideoSink);

				DEBUGMSG("NEW FRAME not consumed, still waiting for a shm segment and/or shm resize");
			}

			return true;
		}
		else
		{
			// nothing to do yet.
			GST_OBJECT_UNLOCK(mVideoSink);
			return true;
		}
	}

	return true;
}


void
MediaPluginGStreamer010::mouseDown( int x, int y )
{
  // do nothing
}

void
MediaPluginGStreamer010::mouseUp( int x, int y )
{
  // do nothing
}

void
MediaPluginGStreamer010::mouseMove( int x, int y )
{
  // do nothing
}


bool
MediaPluginGStreamer010::pause()
{
	DEBUGMSG("pausing media...");
	// todo: error-check this?
	llgst_element_set_state(mPlaybin, GST_STATE_PAUSED);
	return true;
}

bool
MediaPluginGStreamer010::stop()
{
	DEBUGMSG("stopping media...");
	// todo: error-check this?
	llgst_element_set_state(mPlaybin, GST_STATE_READY);
	return true;
}

bool
MediaPluginGStreamer010::play(double rate)
{
	// NOTE: we don't actually support non-natural rate.

        DEBUGMSG("playing media... rate=%f", rate);
	// todo: error-check this?
	llgst_element_set_state(mPlaybin, GST_STATE_PLAYING);
	return true;
}

bool
MediaPluginGStreamer010::setVolume( float volume )
{
	// we try to only update volume as conservatively as
	// possible, as many gst-plugins-base versions up to at least
	// November 2008 have critical race-conditions in setting volume - sigh
	if (mVolume == volume)
		return true; // nothing to do, everything's fine

	mVolume = volume;
	if (mDoneInit && mPlaybin)
	{
		g_object_set(mPlaybin, "volume", mVolume, NULL);
		return true;
	}

	return false;
}

bool
MediaPluginGStreamer010::seek(double time_sec)
{
	bool success = false;
	if (mDoneInit && mPlaybin)
	{
		success = llgst_element_seek(mPlaybin, 1.0F, GST_FORMAT_TIME,
				GstSeekFlags(GST_SEEK_FLAG_FLUSH |
					     GST_SEEK_FLAG_KEY_UNIT),
				GST_SEEK_TYPE_SET, gint64(time_sec*GST_SECOND),
				GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	}
	DEBUGMSG("MEDIA SEEK REQUEST to %fsec result was %d",
		 float(time_sec), int(success));
	return success;
}

bool
MediaPluginGStreamer010::getTimePos(double &sec_out)
{
	bool got_position = false;
	if (mPlaybin)
	{
		gint64 pos;
		GstFormat timefmt = GST_FORMAT_TIME;
		got_position =
			llgst_element_query_position &&
			llgst_element_query_position(mPlaybin,
						     &timefmt,
						     &pos);
		got_position = got_position
			&& (timefmt == GST_FORMAT_TIME);
		// GStreamer may have other ideas, but we consider the current position
		// undefined if not PLAYING or PAUSED
		got_position = got_position &&
			(GST_STATE(mPlaybin) == GST_STATE_PLAYING ||
			 GST_STATE(mPlaybin) == GST_STATE_PAUSED);
		if (got_position && !GST_CLOCK_TIME_IS_VALID(pos))
		{
			if (GST_STATE(mPlaybin) == GST_STATE_PLAYING)
			{
				// if we're playing then we treat an invalid clock time
				// as 0, for complicated reasons (insert reason here)
				pos = 0;
			}
			else
			{
				got_position = false;
			}
			
		}
		// If all the preconditions succeeded... we can trust the result.
		if (got_position)
		{
			sec_out = double(pos) / double(GST_SECOND); // gst to sec
		}
	}
	return got_position;
}

bool
MediaPluginGStreamer010::load()
{
	if (!mDoneInit)
		return false; // error

	setStatus(STATUS_LOADING);

	DEBUGMSG("setting up media...");

	mIsLooping = false;
	mVolume = 0.1234567; // minor hack to force an initial volume update

	// Create a pumpable main-loop for this media
	mPump = g_main_loop_new (NULL, FALSE);
	if (!mPump)
	{
		setStatus(STATUS_ERROR);
		return false; // error
	}

	// instantiate a playbin element to do the hard work
	mPlaybin = llgst_element_factory_make ("playbin", "play");
	if (!mPlaybin)
	{
		setStatus(STATUS_ERROR);
		return false; // error
	}

	// get playbin's bus
	GstBus *bus = llgst_pipeline_get_bus (GST_PIPELINE (mPlaybin));
	if (!bus)
	{
		setStatus(STATUS_ERROR);
		return false; // error
	}
	mBusWatchID = llgst_bus_add_watch (bus,
					   llmediaimplgstreamer_bus_callback,
					   this);
	llgst_object_unref (bus);

	if (NULL == getenv("LL_GSTREAMER_EXTERNAL")) {
		// instantiate a custom video sink
		mVideoSink =
			GST_SLVIDEO(llgst_element_factory_make ("private-slvideo", "slvideo"));
		if (!mVideoSink)
		{
			WARNMSG("Could not instantiate private-slvideo element.");
			// todo: cleanup.
			setStatus(STATUS_ERROR);
			return false; // error
		}

		// connect the pieces
		g_object_set(mPlaybin, "video-sink", mVideoSink, NULL);
	}

	return true;
}

bool
MediaPluginGStreamer010::unload ()
{
	if (!mDoneInit)
		return false; // error

	DEBUGMSG("unloading media...");
	
	// stop getting callbacks for this bus
	g_source_remove(mBusWatchID);
	mBusWatchID = 0;

	if (mPlaybin)
	{
		llgst_element_set_state (mPlaybin, GST_STATE_NULL);
		llgst_object_unref (GST_OBJECT (mPlaybin));
		mPlaybin = NULL;
	}

	if (mPump)
	{
		g_main_loop_quit(mPump);
		mPump = NULL;
	}

	mVideoSink = NULL;

	setStatus(STATUS_NONE);

	return true;
}


//static
bool
MediaPluginGStreamer010::startup()
{
	// first - check if GStreamer is explicitly disabled
	if (NULL != getenv("LL_DISABLE_GSTREAMER"))
		return false;

	// only do global GStreamer initialization once.
	if (!mDoneInit)
	{
		g_thread_init(NULL);

		// Init the glib type system - we need it.
		g_type_init();

		// Get symbols!
#if LL_DARWIN
		if (! grab_gst_syms("libgstreamer-0.10.dylib",
				    "libgstvideo-0.10.dylib") )
#elseif LL_WINDOWS
		if (! grab_gst_syms("libgstreamer-0.10.dll",
				    "libgstvideo-0.10.dll") )
#else // linux or other ELFy unixoid
		if (! grab_gst_syms("libgstreamer-0.10.so.0",
				    "libgstvideo-0.10.so.0") )
#endif
		{
			WARNMSG("Couldn't find suitable GStreamer 0.10 support on this system - video playback disabled.");
			return false;
		}

		if (llgst_segtrap_set_enabled)
		{
			llgst_segtrap_set_enabled(FALSE);
		}
		else
		{
			WARNMSG("gst_segtrap_set_enabled() is not available; plugin crashes won't be caught.");
		}

#if LL_LINUX
		// Gstreamer tries a fork during init, waitpid-ing on it,
		// which conflicts with any installed SIGCHLD handler...
		struct sigaction tmpact, oldact;
		if (llgst_registry_fork_set_enabled) {
			// if we can disable SIGCHLD-using forking behaviour,
			// do it.
			llgst_registry_fork_set_enabled(false);
		}
		else {
			// else temporarily install default SIGCHLD handler
			// while GStreamer initialises
			tmpact.sa_handler = SIG_DFL;
			sigemptyset( &tmpact.sa_mask );
			tmpact.sa_flags = SA_SIGINFO;
			sigaction(SIGCHLD, &tmpact, &oldact);
		}
#endif // LL_LINUX

		// Protect against GStreamer resetting the locale, yuck.
		static std::string saved_locale;
		saved_locale = setlocale(LC_ALL, NULL);

		// finally, try to initialize GStreamer!
		GError *err = NULL;
		gboolean init_gst_success = llgst_init_check(NULL, NULL, &err);

		// restore old locale
		setlocale(LC_ALL, saved_locale.c_str() );

#if LL_LINUX
		// restore old SIGCHLD handler
		if (!llgst_registry_fork_set_enabled)
			sigaction(SIGCHLD, &oldact, NULL);
#endif // LL_LINUX

		if (!init_gst_success) // fail
		{
			if (err)
			{
				WARNMSG("GST init failed: %s", err->message);
				g_error_free(err);
			}
			else
			{
				WARNMSG("GST init failed for unspecified reason.");
			}
			return false;
		}
		
		// Init our custom plugins - only really need do this once.
		gst_slvideo_init_class();

		mDoneInit = true;
	}

	return true;
}


void
MediaPluginGStreamer010::sizeChanged()
{
	// the shared writing space has possibly changed size/location/whatever

	// Check to see whether the movie's NATURAL size has been set yet
	if (1 == mNaturalWidth &&
	    1 == mNaturalHeight)
	{
		mNaturalWidth = mCurrentWidth;
		mNaturalHeight = mCurrentHeight;
		DEBUGMSG("Media NATURAL size better detected as %dx%d",
			 mNaturalWidth, mNaturalHeight);
	}

	// if the size has changed then the shm has changed and the app needs telling
	if (mCurrentWidth != mPreviousWidth ||
	    mCurrentHeight != mPreviousHeight)
	{
		mPreviousWidth = mCurrentWidth;
		mPreviousHeight = mCurrentHeight;

		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_request");
		message.setValue("name", mTextureSegmentName);
		message.setValueS32("width", mNaturalWidth);
		message.setValueS32("height", mNaturalHeight);
		DEBUGMSG("<--- Sending size change request to application with name: '%s' - natural size is %d x %d", mTextureSegmentName.c_str(), mNaturalWidth, mNaturalHeight);
		sendMessage(message);
	}
}



//static
bool
MediaPluginGStreamer010::closedown()
{
	if (!mDoneInit)
		return false; // error

	ungrab_gst_syms();

	mDoneInit = false;

	return true;
}

MediaPluginGStreamer010::~MediaPluginGStreamer010()
{
	DEBUGMSG("MediaPluginGStreamer010 destructor");

	closedown();

	DEBUGMSG("GStreamer010 closing down");
}


std::string
MediaPluginGStreamer010::getVersion()
{
	std::string plugin_version = "GStreamer010 media plugin, GStreamer version ";
	if (mDoneInit &&
	    llgst_version)
	{
		guint major, minor, micro, nano;
		llgst_version(&major, &minor, &micro, &nano);
		plugin_version += llformat("%u.%u.%u.%u (runtime), %u.%u.%u.%u (headers)", (unsigned int)major, (unsigned int)minor, (unsigned int)micro, (unsigned int)nano, (unsigned int)GST_VERSION_MAJOR, (unsigned int)GST_VERSION_MINOR, (unsigned int)GST_VERSION_MICRO, (unsigned int)GST_VERSION_NANO);
	}
	else
	{
		plugin_version += "(unknown)";
	}
	return plugin_version;
}

void MediaPluginGStreamer010::receiveMessage(const char *message_string)
{
	//std::cerr << "MediaPluginGStreamer010::receiveMessage: received message: \"" << message_string << "\"" << std::endl;

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
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME] = LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME_VERSION;
				message.setValueLLSD("versions", versions);

				if ( load() )
				{
					DEBUGMSG("GStreamer010 media instance set up");
				}
				else
				{
					WARNMSG("GStreamer010 media instance failed to set up");
				}

				message.setValue("plugin_version", getVersion());
				sendMessage(message);
			}
			else if(message_name == "idle")
			{
				// no response is necessary here.
				double time = message_in.getValueReal("time");
				
				// Convert time to milliseconds for update()
				update((int)(time * 1000.0f));
			}
			else if(message_name == "cleanup")
			{
				unload();
				closedown();
			}
			else if(message_name == "shm_added")
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer("address");
				info.mSize = (size_t)message_in.getValueS32("size");
				std::string name = message_in.getValue("name");

				std::ostringstream str;
				INFOMSG("MediaPluginGStreamer010::receiveMessage: shared memory added, name: %s, size: %d, address: %p", name.c_str(), int(info.mSize), info.mAddress);

				mSharedSegments.insert(SharedSegmentMap::value_type(name, info));
			}
			else if(message_name == "shm_remove")
			{
				std::string name = message_in.getValue("name");

				DEBUGMSG("MediaPluginGStreamer010::receiveMessage: shared memory remove, name = %s", name.c_str());
				
				SharedSegmentMap::iterator iter = mSharedSegments.find(name);
				if(iter != mSharedSegments.end())
				{
					if(mPixels == iter->second.mAddress)
					{
						// This is the currently active pixel buffer.  Make sure we stop drawing to it.
						mPixels = NULL;
						mTextureSegmentName.clear();
						
						// Make sure the movie decoder is no longer pointed at the shared segment.
						sizeChanged();						
					}
					mSharedSegments.erase(iter);
				}
				else
				{
					WARNMSG("MediaPluginGStreamer010::receiveMessage: unknown shared memory region!");
				}

				// Send the response so it can be cleaned up.
				LLPluginMessage message("base", "shm_remove_response");
				message.setValue("name", name);
				sendMessage(message);
			}
			else
			{
				std::ostringstream str;
				INFOMSG("MediaPluginGStreamer010::receiveMessage: unknown base message: %s", message_name.c_str());
			}
		}
		else if(message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
		{
			if(message_name == "init")
			{
				// Plugin gets to decide the texture parameters to use.
				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params");
				// lame to have to decide this now, it depends on the movie.  Oh well.
				mDepth = 4;

				mCurrentWidth = 1;
				mCurrentHeight = 1;
				mPreviousWidth = 1;
				mPreviousHeight = 1;
				mNaturalWidth = 1;
				mNaturalHeight = 1;
				mWidth = 1;
				mHeight = 1;
				mTextureWidth = 1;
				mTextureHeight = 1;

				message.setValueU32("format", GL_RGBA);
				message.setValueU32("type", GL_UNSIGNED_INT_8_8_8_8_REV);

				message.setValueS32("depth", mDepth);
				message.setValueS32("default_width", mWidth);
				message.setValueS32("default_height", mHeight);
				message.setValueU32("internalformat", GL_RGBA8);
				message.setValueBoolean("coords_opengl", true);	// true == use OpenGL-style coordinates, false == (0,0) is upper left.
				message.setValueBoolean("allow_downsample", true); // we respond with grace and performance if asked to downscale
				sendMessage(message);
			}
			else if(message_name == "size_change")
			{
				std::string name = message_in.getValue("name");
				S32 width = message_in.getValueS32("width");
				S32 height = message_in.getValueS32("height");
				S32 texture_width = message_in.getValueS32("texture_width");
				S32 texture_height = message_in.getValueS32("texture_height");

				std::ostringstream str;
				INFOMSG("---->Got size change instruction from application with shm name: %s - size is %d x %d", name.c_str(), width, height);

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
						INFOMSG("*** Got size change with matching shm, new size is %d x %d", width, height);
						INFOMSG("*** Got size change with matching shm, texture size size is %d x %d", texture_width, texture_height);

						mPixels = (unsigned char*)iter->second.mAddress;
						mTextureSegmentName = name;
						mWidth = width;
						mHeight = height;

						if (texture_width > 1 ||
						    texture_height > 1) // not a dummy size from the app, a real explicit forced size
						{
							INFOMSG("**** = REAL RESIZE REQUEST FROM APP");
							
							GST_OBJECT_LOCK(mVideoSink);
							mVideoSink->resize_forced_always = true;
							mVideoSink->resize_try_width = texture_width;
							mVideoSink->resize_try_height = texture_height;
							GST_OBJECT_UNLOCK(mVideoSink);
 						}

						mTextureWidth = texture_width;
						mTextureHeight = texture_height;
					}
				}
			}
			else if(message_name == "load_uri")
			{
				std::string uri = message_in.getValue("uri");
				navigateTo( uri );
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
				double rate = 0.0;
				if(message_in.hasValue("rate"))
				{
					rate = message_in.getValueReal("rate");
				}
				// NOTE: we don't actually support rate.
				play(rate);
			}
			else if(message_name == "pause")
			{
				pause();
			}
			else if(message_name == "seek")
			{
				double time = message_in.getValueReal("time");
				// defer the actual seek in case we haven't
				// really truly started yet in which case there
				// is nothing to seek upon
				mSeekWanted = true;
				mSeekDestination = time;
			}
			else if(message_name == "set_loop")
			{
				bool loop = message_in.getValueBoolean("loop");
				mIsLooping = loop;
			}
			else if(message_name == "set_volume")
			{
				double volume = message_in.getValueReal("volume");
				setVolume(volume);
			}
		}
		else
		{
			INFOMSG("MediaPluginGStreamer010::receiveMessage: unknown message class: %s", message_class.c_str());
		}
	}
}

int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data, LLPluginInstance::sendMessageFunction *plugin_send_func, void **plugin_user_data)
{
	if (MediaPluginGStreamer010::startup())
	{
		MediaPluginGStreamer010 *self = new MediaPluginGStreamer010(host_send_func, host_user_data);
		*plugin_send_func = MediaPluginGStreamer010::staticReceiveMessage;
		*plugin_user_data = (void*)self;
		
		return 0; // okay
	}
	else 
	{
		return -1; // failed to init
	}
}

#else // LL_GSTREAMER010_ENABLED

// Stubbed-out class with constructor/destructor (necessary or windows linker
// will just think its dead code and optimize it all out)
class MediaPluginGStreamer010 : public MediaPluginBase
{
public:
	MediaPluginGStreamer010(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginGStreamer010();
	/* virtual */ void receiveMessage(const char *message_string);
};

MediaPluginGStreamer010::MediaPluginGStreamer010(
	LLPluginInstance::sendMessageFunction host_send_func,
	void *host_user_data ) :
	MediaPluginBase(host_send_func, host_user_data)
{
    // no-op
}

MediaPluginGStreamer010::~MediaPluginGStreamer010()
{
    // no-op
}

void MediaPluginGStreamer010::receiveMessage(const char *message_string)
{
    // no-op 
}

// We're building without GStreamer enabled.  Just refuse to initialize.
int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data, LLPluginInstance::sendMessageFunction *plugin_send_func, void **plugin_user_data)
{
    return -1;
}

#endif // LL_GSTREAMER010_ENABLED
