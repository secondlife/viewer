/** 
 * @file llsoundconverter.cpp
 * @brief Implementation of LLSoundConverterThread class
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------

#include "llviewerprecompiledheaders.h"

#include "llsoundconverter.h"


#if defined(_MSC_VER)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "vlc/vlc.h"

//-----------------------------------------------------------------------------
// LLSoundConventorThread
//-----------------------------------------------------------------------------

LLSoundConverterThread::LLSoundConverterThread(const std::string& filename, on_complition_cb callback)
    : LLThread("LLSoundConventorThread", nullptr)
    , mInFilename(filename)
    , mOnCompletionCallback(callback)
{
}

void event_callbacks(const libvlc_event_t* event, void* ptr)
{

}

void LLSoundConverterThread::run()
{
    libvlc_instance_t* vlc_inst = libvlc_new(0, nullptr);
    libvlc_media_t* vlc_media = libvlc_media_new_path(vlc_inst, mInFilename.c_str());
    libvlc_event_manager_t* vlc_event_mgr = libvlc_media_event_manager(vlc_media);

    if (vlc_event_mgr)
    {
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerOpening, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerPlaying, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerPaused, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerStopped, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerEndReached, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerEncounteredError, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerTimeChanged, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerPositionChanged, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerLengthChanged, event_callbacks, this);
        libvlc_event_attach(vlc_event_mgr, libvlc_MediaPlayerTitleChanged, event_callbacks, this);
    }
    else
    {
        // todo: error handling
        return;
    }

    std::string temp_file = gDirUtilp->getTempFilename();
    std::string cmd;
    cmd = ":sout=#transcode{acodec=s16l, ab=16, channels=1, samplerate=16000}:std{access=file, mux=wav, dst='"
           + temp_file + "'}";
    libvlc_media_add_option(vlc_media, cmd.c_str());
    libvlc_media_player_t* vlc_player = libvlc_media_player_new_from_media(vlc_media);
    libvlc_audio_set_mute(vlc_player, true);
    libvlc_media_player_play(vlc_player);

    while (true)
    {
        if (libvlc_media_get_state(vlc_media) == libvlc_Ended)
        {
            break;
        }
        ms_sleep(100);
    }

    libvlc_media_player_release(vlc_player);
    libvlc_media_release(vlc_media);
    libvlc_release(vlc_inst);

    mOnCompletionCallback(temp_file);
}

// End
