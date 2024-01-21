/** 
 * @file media_plugin_gstreamer10.cpp
 * @brief GStreamer-1.0 plugin for LLMedia API plugin system
 *
 * @cond
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2016, Linden Research, Inc. / Nicky Dasmijn
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

#define FLIP_Y

#include "linden_common.h"

#include "llgl.h"

#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "media_plugin_base.h"

#define G_DISABLE_CAST_CHECKS
extern "C" {
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

}

#include "llmediaimplgstreamer.h"
#include "llmediaimplgstreamer_syms.h"

static inline void llgst_caps_unref( GstCaps * caps )
{
    llgst_mini_object_unref( GST_MINI_OBJECT_CAST( caps ) );
}

static inline void llgst_sample_unref( GstSample *aSample )
{
    llgst_mini_object_unref( GST_MINI_OBJECT_CAST( aSample ) );
}

//////////////////////////////////////////////////////////////////////////////
//
class MediaPluginGStreamer10 : public MediaPluginBase
{
public:
    MediaPluginGStreamer10(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
    ~MediaPluginGStreamer10();

    /* virtual */ void receiveMessage(const char *message_string);

    static bool startup();
    static bool closedown();

    gboolean processGSTEvents(GstBus *bus, GstMessage *message);

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

    double MIN_LOOP_SEC = 1.0F;
    U32 INTERNAL_TEXTURE_SIZE = 1024;
    
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

    static bool mDoneInit;
    
    guint mBusWatchID;
    
    float mVolume;

    int mDepth;

    // padded texture size we need to write into
    int mTextureWidth;
    int mTextureHeight;
    
    bool mSeekWanted;
    double mSeekDestination;
    
    // Very GStreamer-specific
    GMainLoop *mPump; // event pump for this media
    GstElement *mPlaybin;
    GstAppSink *mAppSink;
};

//static
bool MediaPluginGStreamer10::mDoneInit = false;

MediaPluginGStreamer10::MediaPluginGStreamer10( LLPluginInstance::sendMessageFunction host_send_func,
                                                void *host_user_data )
    : MediaPluginBase(host_send_func, host_user_data)
    , mBusWatchID ( 0 )
    , mSeekWanted(false)
    , mSeekDestination(0.0)
    , mPump ( NULL )
    , mPlaybin ( NULL )
    , mAppSink ( NULL )
    , mCommand ( COMMAND_NONE )
{
}

gboolean MediaPluginGStreamer10::processGSTEvents(GstBus *bus, GstMessage *message)
{
    if (!message) 
        return TRUE; // shield against GStreamer bug

    switch (GST_MESSAGE_TYPE (message))
    {
        case GST_MESSAGE_BUFFERING:
        {
            // NEEDS GST 0.10.11+
            if (llgst_message_parse_buffering)
            {
                gint percent = 0;
                llgst_message_parse_buffering(message, &percent);
            }
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
        {
            GstState old_state;
            GstState new_state;
            GstState pending_state;
            llgst_message_parse_state_changed(message,
                                              &old_state,
                                              &new_state,
                                              &pending_state);

            switch (new_state)
            {
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
        case GST_MESSAGE_ERROR:
        {
            GError *err = NULL;
            gchar *debug = NULL;

            llgst_message_parse_error (message, &err, &debug);
            if (err)
                llg_error_free (err);
            llg_free (debug);

            mCommand = COMMAND_STOP;

            setStatus(STATUS_ERROR);

            break;
        }
        case GST_MESSAGE_INFO:
        {
            if (llgst_message_parse_info)
            {
                GError *err = NULL;
                gchar *debug = NULL;
            
                llgst_message_parse_info (message, &err, &debug);
                if (err)
                    llg_error_free (err);
                llg_free (debug);
            }
            break;
        }
        case GST_MESSAGE_WARNING:
        {
            GError *err = NULL;
            gchar *debug = NULL;
            
            llgst_message_parse_warning (message, &err, &debug);
            if (err)
                llg_error_free (err);
            llg_free (debug);

            break;
        }
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            if (mIsLooping)
            {
                double eos_pos_sec = 0.0F;
                bool got_eos_position = getTimePos(eos_pos_sec);
                
                if (got_eos_position && eos_pos_sec < MIN_LOOP_SEC)
                {
                    // if we know that the movie is really short, don't
                    // loop it else it can easily become a time-hog
                    // because of GStreamer spin-up overhead
                    // inject a COMMAND_PAUSE
                    mCommand = COMMAND_PAUSE;
                }
                else
                {
                    stop();
                    play(1.0);
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
    gboolean llmediaimplgstreamer_bus_callback (GstBus     *bus,
                                                GstMessage *message,
                                                gpointer    data)
    {
        MediaPluginGStreamer10 *impl = (MediaPluginGStreamer10*)data;
        return impl->processGSTEvents(bus, message);
    }
} // extern "C"



bool MediaPluginGStreamer10::navigateTo ( const std::string urlIn )
{
    if (!mDoneInit)
        return false; // error

    setStatus(STATUS_LOADING);

    mSeekWanted = false;

    if (NULL == mPump ||  NULL == mPlaybin)
    {
        setStatus(STATUS_ERROR);
        return false; // error
    }

    llg_object_set (G_OBJECT (mPlaybin), "uri", urlIn.c_str(), NULL);

    // navigateTo implicitly plays, too.
    play(1.0);

    return true;
}


class GstSampleUnref
{
    GstSample *mT;
public:
    GstSampleUnref( GstSample *aT )
        : mT( aT )
    { llassert_always( mT ); }

    ~GstSampleUnref( )
    { llgst_sample_unref( mT ); }
};

bool MediaPluginGStreamer10::update(int milliseconds)
{
    if (!mDoneInit)
        return false; // error

    //  DEBUGMSG("updating media...");
    
    // sanity check
    if (NULL == mPump || NULL == mPlaybin)
    {
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
    while (llg_main_context_pending(llg_main_loop_get_context(mPump)))
    {
           llg_main_context_iteration(llg_main_loop_get_context(mPump), FALSE);
    }

    // check for availability of a new frame
    
    if( !mAppSink )
        return true;

    if( GST_STATE(mPlaybin) != GST_STATE_PLAYING) // Do not try to pull a sample if not in playing state
        return true;
    
    GstSample *pSample = llgst_app_sink_pull_sample( mAppSink );
    if(!pSample)
        return false; // Done playing

    GstSampleUnref oSampleUnref( pSample );
    GstCaps *pCaps = llgst_sample_get_caps ( pSample );
    if (!pCaps)
        return false;

    gint width, height;
    GstStructure *pStruct = llgst_caps_get_structure ( pCaps, 0);

    int res = llgst_structure_get_int ( pStruct, "width", &width);
    res |= llgst_structure_get_int ( pStruct, "height", &height);

    if( !mPixels )
        return true;
    
    GstBuffer *pBuffer = llgst_sample_get_buffer ( pSample );
    GstMapInfo map;
    llgst_buffer_map ( pBuffer, &map, GST_MAP_READ);

    // Our render buffer is always 1kx1k

    U32 rowSkip = INTERNAL_TEXTURE_SIZE / mTextureHeight;
    U32 colSkip = INTERNAL_TEXTURE_SIZE / mTextureWidth;

    for (int row = 0; row < mTextureHeight; ++row)
    {
        U8 const *pTexelIn = map.data + (row*rowSkip * width *3);
#ifndef FLIP_Y
        U8 *pTexelOut = mPixels + (row * mTextureWidth * mDepth );
#else
        U8 *pTexelOut = mPixels + ((mTextureHeight-row-1) * mTextureWidth * mDepth );
#endif      
        for( int col = 0; col < mTextureWidth; ++col )
        {
            pTexelOut[ 0 ] = pTexelIn[0];
            pTexelOut[ 1 ] = pTexelIn[1];
            pTexelOut[ 2 ] = pTexelIn[2];
            pTexelOut += mDepth;
            pTexelIn += colSkip*3;
        }
    }

    llgst_buffer_unmap( pBuffer, &map );
    setDirty(0,0,mTextureWidth,mTextureHeight);

    return true;
}

void MediaPluginGStreamer10::mouseDown( int x, int y )
{
  // do nothing
}

void MediaPluginGStreamer10::mouseUp( int x, int y )
{
  // do nothing
}

void MediaPluginGStreamer10::mouseMove( int x, int y )
{
  // do nothing
}


bool MediaPluginGStreamer10::pause()
{
    // todo: error-check this?
    if (mDoneInit && mPlaybin)
    {
        llgst_element_set_state(mPlaybin, GST_STATE_PAUSED);
        return true;
    }
    return false;
}

bool MediaPluginGStreamer10::stop()
{
    // todo: error-check this?
    if (mDoneInit && mPlaybin)
    {
        llgst_element_set_state(mPlaybin, GST_STATE_READY);
        return true;
    }
    return false;
}

bool MediaPluginGStreamer10::play(double rate)
{
    // NOTE: we don't actually support non-natural rate.

    // todo: error-check this?
    if (mDoneInit && mPlaybin)
    {
        llgst_element_set_state(mPlaybin, GST_STATE_PLAYING);
        return true;
    }
    return false;
}

bool MediaPluginGStreamer10::setVolume( float volume )
{
    // we try to only update volume as conservatively as
    // possible, as many gst-plugins-base versions up to at least
    // November 2008 have critical race-conditions in setting volume - sigh
    if (mVolume == volume)
        return true; // nothing to do, everything's fine

    mVolume = volume;
    if (mDoneInit && mPlaybin)
    {
        llg_object_set(mPlaybin, "volume", mVolume, NULL);
        return true;
    }

    return false;
}

bool MediaPluginGStreamer10::seek(double time_sec)
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
    return success;
}

bool MediaPluginGStreamer10::getTimePos(double &sec_out)
{
    bool got_position = false;
    if (mDoneInit && mPlaybin)
    {
        gint64 pos(0);
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

bool MediaPluginGStreamer10::load()
{
    if (!mDoneInit)
        return false; // error

    setStatus(STATUS_LOADING);

    mIsLooping = false;
    mVolume = 0.1234567f; // minor hack to force an initial volume update

    // Create a pumpable main-loop for this media
    mPump = llg_main_loop_new (NULL, FALSE);
    if (!mPump)
    {
        setStatus(STATUS_ERROR);
        return false; // error
    }

    // instantiate a playbin element to do the hard work
    mPlaybin = llgst_element_factory_make ("playbin", "");
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

    mAppSink = (GstAppSink*)(llgst_element_factory_make ("appsink", ""));

    GstCaps* pCaps = llgst_caps_new_simple( "video/x-raw",
                                            "format", G_TYPE_STRING, "RGB",
                                            "width", G_TYPE_INT, INTERNAL_TEXTURE_SIZE,
                                            "height", G_TYPE_INT, INTERNAL_TEXTURE_SIZE,
                                            NULL );

    llgst_app_sink_set_caps( mAppSink, pCaps );
    llgst_caps_unref( pCaps );

    if (!mAppSink)
    {
        setStatus(STATUS_ERROR);
        return false;
    }
    
    llg_object_set(mPlaybin, "video-sink", mAppSink, NULL);

    return true;
}

bool MediaPluginGStreamer10::unload ()
{
    if (!mDoneInit)
        return false; // error

    // stop getting callbacks for this bus
    llg_source_remove(mBusWatchID);
    mBusWatchID = 0;

    if (mPlaybin)
    {
        llgst_element_set_state (mPlaybin, GST_STATE_NULL);
        llgst_object_unref (GST_OBJECT (mPlaybin));
        mPlaybin = NULL;
    }

    if (mPump)
    {
        llg_main_loop_quit(mPump);
        mPump = NULL;
    }

    mAppSink = NULL;

    setStatus(STATUS_NONE);

    return true;
}

void LogFunction(GstDebugCategory *category, GstDebugLevel level, const gchar *file, const gchar *function, gint line, GObject *object, GstDebugMessage *message, gpointer user_data )
#ifndef LL_LINUX // Docu says we need G_GNUC_NO_INSTRUMENT, but GCC says 'error'
    G_GNUC_NO_INSTRUMENT
#endif
{
#ifdef LL_LINUX
    std::cerr << file << ":" << line << "(" << function << "): " << llgst_debug_message_get( message ) << std::endl;
#endif
}

//static
bool MediaPluginGStreamer10::startup()
{
    // first - check if GStreamer is explicitly disabled
    if (NULL != getenv("LL_DISABLE_GSTREAMER"))
        return false;

    // only do global GStreamer initialization once.
    if (!mDoneInit)
    {
        ll_init_apr();

        // Get symbols!
        std::vector< std::string > vctDSONames;
#if LL_DARWIN
#elif LL_WINDOWS
        vctDSONames.push_back( "libgstreamer-1.0-0.dll"  );
        vctDSONames.push_back( "libgstapp-1.0-0.dll"  );
        vctDSONames.push_back( "libglib-2.0-0.dll" );
        vctDSONames.push_back( "libgobject-2.0-0.dll" );
#else // linux or other ELFy unixoid
        vctDSONames.push_back( "libgstreamer-1.0.so.0"  );
        vctDSONames.push_back( "libgstapp-1.0.so.0"  );
        vctDSONames.push_back( "libglib-2.0.so.0" );
        vctDSONames.push_back( "libgobject-2.0.so" );
#endif
        if( !grab_gst_syms( vctDSONames ) )
        {
            return false;
        }

        if (llgst_segtrap_set_enabled)
        {
            llgst_segtrap_set_enabled(FALSE);
        }
#if LL_LINUX
        // Gstreamer tries a fork during init, waitpid-ing on it,
        // which conflicts with any installed SIGCHLD handler...
        struct sigaction tmpact, oldact;
        if (llgst_registry_fork_set_enabled ) {
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
        
//      _putenv_s( "GST_PLUGIN_PATH", "E:\\gstreamer\\1.0\\x86\\lib\\gstreamer-1.0" );

        llgst_debug_set_default_threshold( GST_LEVEL_WARNING );
        llgst_debug_add_log_function( LogFunction, NULL, NULL );
        llgst_debug_set_active( false );

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
                llg_error_free(err);
            }
            return false;
        }
        
        mDoneInit = true;
    }

    return true;
}

//static
bool MediaPluginGStreamer10::closedown()
{
    if (!mDoneInit)
        return false; // error

    ungrab_gst_syms();

    mDoneInit = false;

    return true;
}

MediaPluginGStreamer10::~MediaPluginGStreamer10()
{
    closedown();
}


std::string MediaPluginGStreamer10::getVersion()
{
    std::string plugin_version = "GStreamer10 media plugin, GStreamer version ";
    if (mDoneInit &&
        llgst_version)
    {
        guint major, minor, micro, nano;
        llgst_version(&major, &minor, &micro, &nano);
        plugin_version += llformat("%u.%u.%u.%u (runtime), %u.%u.%u.%u (headers)", (unsigned int)major, (unsigned int)minor,
                                   (unsigned int)micro, (unsigned int)nano, (unsigned int)GST_VERSION_MAJOR, (unsigned int)GST_VERSION_MINOR,
                                   (unsigned int)GST_VERSION_MICRO, (unsigned int)GST_VERSION_NANO);
    }
    else
    {
        plugin_version += "(unknown)";
    }
    return plugin_version;
}

void MediaPluginGStreamer10::receiveMessage(const char *message_string)
{
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

                load();

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

                // Send the response so it can be cleaned up.
                LLPluginMessage message("base", "shm_remove_response");
                message.setValue("name", name);
                sendMessage(message);
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

                mTextureWidth = 1;
                mTextureHeight = 1;

                message.setValueU32("format", GL_RGBA);
                message.setValueU32("type", GL_UNSIGNED_INT_8_8_8_8_REV);

                message.setValueS32("depth", mDepth);
                message.setValueS32("default_width", INTERNAL_TEXTURE_SIZE );
                message.setValueS32("default_height", INTERNAL_TEXTURE_SIZE );
                message.setValueU32("internalformat", GL_RGBA8);
                message.setValueBoolean("coords_opengl", true); // true == use OpenGL-style coordinates, false == (0,0) is upper left.
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
                        mPixels = (unsigned char*)iter->second.mAddress;
                        mTextureSegmentName = name;

                        mTextureWidth = texture_width;
                        mTextureHeight = texture_height;
                        memset( mPixels, 0, mTextureWidth*mTextureHeight*mDepth );
                    }
                    
                    LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_request");
                    message.setValue("name", mTextureSegmentName);
                    message.setValueS32("width", INTERNAL_TEXTURE_SIZE );
                    message.setValueS32("height", INTERNAL_TEXTURE_SIZE );
                    sendMessage(message);

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
    }
}

int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data, LLPluginInstance::sendMessageFunction *plugin_send_func, void **plugin_user_data)
{
    if( MediaPluginGStreamer10::startup() )
    {
        MediaPluginGStreamer10 *self = new MediaPluginGStreamer10(host_send_func, host_user_data);
        *plugin_send_func = MediaPluginGStreamer10::staticReceiveMessage;
        *plugin_user_data = (void*)self;
        
        return 0; // okay
    }
    else 
    {
        return -1; // failed to init
    }
}
