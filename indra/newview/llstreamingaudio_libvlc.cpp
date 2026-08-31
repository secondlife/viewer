/**
 * @file llstreamingaudio_libvlc.cpp
 * @brief LLStreamingAudio_LibVLC implementation -- see llstreamingaudio_libvlc.h.
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

#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include "llstreamingaudio_libvlc.h"

#include <vlc/vlc.h>

LLStreamingAudio_LibVLC::LLStreamingAudio_LibVLC()
    : mLibVLC(nullptr)
    , mLibVLCMediaPlayer(nullptr)
    , mGain(1.f)
{
    // Audio only -- this class never plays anything with a video track, so tell libvlc not to
    // bother setting up any video output subsystem at all.
    char const* vlc_argv[] = { "--no-video" };
    mLibVLC = libvlc_new(1, vlc_argv);
    if (!mLibVLC)
    {
        LL_WARNS() << "libvlc_new() failed -- parcel audio/music streaming will not work" << LL_ENDL;
    }
}

LLStreamingAudio_LibVLC::~LLStreamingAudio_LibVLC()
{
    destroyPlayer();
    if (mLibVLC)
    {
        libvlc_release(mLibVLC);
        mLibVLC = nullptr;
    }
}

void LLStreamingAudio_LibVLC::start(const std::string& url)
{
    if (!mLibVLC)
        return;

    destroyPlayer();

    if (url.empty())
    {
        LL_INFOS() << "setting parcel audio stream to NULL" << LL_ENDL;
        mURL.clear();
        return;
    }

    mURL = url; // keep the original url here for comparison purposes (matches getURL() callers' expectations)

    std::string stream_url = url;
    LLStringUtil::trim(stream_url);
    size_t pos = stream_url.find(' ');
    if (pos != std::string::npos)
    {
        // Some parcel owners label their stream this way, e.g. "http://example.com/stream My Station"
        // -- ignore the label, matches the pre-existing plugin-backed implementation's own parsing.
        stream_url = stream_url.substr(0, pos);
    }

    LL_INFOS() << "Starting parcel audio stream: " << stream_url << LL_ENDL;

    libvlc_media_t* media = libvlc_media_new_location(mLibVLC, stream_url.c_str());
    if (!media)
    {
        LL_WARNS() << "libvlc_media_new_location() failed for " << stream_url << LL_ENDL;
        return;
    }

    mLibVLCMediaPlayer = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media); // player takes its own reference -- safe to release ours now

    if (!mLibVLCMediaPlayer)
    {
        LL_WARNS() << "libvlc_media_player_new_from_media() failed for " << stream_url << LL_ENDL;
        return;
    }

    applyGain();
    libvlc_media_player_play(mLibVLCMediaPlayer);
}

void LLStreamingAudio_LibVLC::stop()
{
    LL_INFOS() << "Stopping parcel audio stream." << LL_ENDL;
    destroyPlayer();
    mURL.clear();
}

void LLStreamingAudio_LibVLC::pause(int pause)
{
    if (!mLibVLCMediaPlayer)
        return;

    if (pause)
    {
        LL_INFOS() << "Pausing parcel audio stream." << LL_ENDL;
        libvlc_media_player_set_pause(mLibVLCMediaPlayer, 1);
    }
    else
    {
        LL_INFOS() << "Unpausing parcel audio stream." << LL_ENDL;
        libvlc_media_player_set_pause(mLibVLCMediaPlayer, 0);
    }
}

void LLStreamingAudio_LibVLC::update()
{
    // libvlc runs its own internal playback thread -- nothing to pump from here, unlike the old
    // plugin-process-backed implementation this replaces (its update() drove a subprocess's
    // message-queue idle() every frame; there is no subprocess here at all).
}

int LLStreamingAudio_LibVLC::isPlaying()
{
    if (!mLibVLCMediaPlayer)
        return 0; // stopped

    switch (libvlc_media_player_get_state(mLibVLCMediaPlayer))
    {
    case libvlc_Opening:
    case libvlc_Buffering:
    case libvlc_Playing:
        return 1; // active and playing
    case libvlc_Paused:
        return 2; // paused
    default:
        return 0; // stopped
    }
}

void LLStreamingAudio_LibVLC::setGain(F32 vol)
{
    mGain = vol;
    applyGain();
}

F32 LLStreamingAudio_LibVLC::getGain()
{
    return mGain;
}

std::string LLStreamingAudio_LibVLC::getURL()
{
    return mURL;
}

void LLStreamingAudio_LibVLC::applyGain()
{
    if (!mLibVLCMediaPlayer)
        return;

    F32 vol = llclamp(mGain, 0.f, 1.f);
    libvlc_audio_set_volume(mLibVLCMediaPlayer, (int)(vol * 100));
}

void LLStreamingAudio_LibVLC::destroyPlayer()
{
    if (mLibVLCMediaPlayer)
    {
        libvlc_media_player_stop(mLibVLCMediaPlayer);
        libvlc_media_player_release(mLibVLCMediaPlayer);
        mLibVLCMediaPlayer = nullptr;
    }
}
