/**
 * @file llstreamingaudio_libvlc.h
 * @brief Definition of LLStreamingAudio_LibVLC, a streaming-audio implementation for parcel
 *        audio/music that links libvlc directly into the Viewer process. Deliberately outside
 *        both the embedded-browser system and the (disabled) media plugin architecture -- no
 *        SLPlugin.exe, no plugin IPC, no video path at all, just an internet audio stream.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#ifndef LL_STREAMINGAUDIO_LIBVLC_H
#define LL_STREAMINGAUDIO_LIBVLC_H

#include "stdtypes.h" // from llcommon

#include "llstreamingaudio.h"

// Opaque libvlc handles -- forward-declared so this header doesn't need to pull in
// <vlc/vlc.h>. Real definitions: typedef struct libvlc_instance_t libvlc_instance_t;
// (libvlc.h) and typedef struct libvlc_media_player_t libvlc_media_player_t;
// (libvlc_media_player.h).
struct libvlc_instance_t;
struct libvlc_media_player_t;

class LLStreamingAudio_LibVLC : public LLStreamingAudioInterface
{
public:
    LLStreamingAudio_LibVLC();
    /*virtual*/ ~LLStreamingAudio_LibVLC();

    /*virtual*/ void start(const std::string& url);
    /*virtual*/ void stop();
    /*virtual*/ void pause(int pause);
    /*virtual*/ void update();
    /*virtual*/ int isPlaying();
    /*virtual*/ void setGain(F32 vol);
    /*virtual*/ F32 getGain();
    /*virtual*/ std::string getURL();

private:
    void applyGain();
    void destroyPlayer();

    libvlc_instance_t* mLibVLC;
    libvlc_media_player_t* mLibVLCMediaPlayer;

    std::string mURL;
    F32 mGain;
};

#endif // LL_STREAMINGAUDIO_LIBVLC_H
