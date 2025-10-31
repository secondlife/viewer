/**
 * @file volume_catcher_pulseaudio.cpp
 * @brief A Linux-specific, PulseAudio-specific hack to detect and volume-adjust new audio sources
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

/*
  The high-level design is as follows:
  1) Connect to the PulseAudio daemon
  2) Watch for the creation of new audio players connecting to the daemon (this includes ALSA clients running on the PulseAudio emulation layer, such as Flash plugins)
  3) Examine any new audio player's PID to see if it belongs to our own process
  4) If so, tell PA to adjust the volume of that audio player ('sink input' in PA parlance)
  5) Keep a list of all living audio players that we care about, adjust the volumes of all of them when we get a new setVolume() call
 */

#include "linden_common.h"

#include "volume_catcher_linux.h"

extern "C" {
#include <glib.h>
#include <glib-object.h>

#include <pulse/introspect.h>

#include <pulse/subscribe.h>
}

SymbolGrabber paSymbolGrabber;

#include "volume_catcher_pulseaudio_syms.inc"
#include "volume_catcher_pulseaudio_glib_syms.inc"

////////////////////////////////////////////////////

// PulseAudio requires a chain of callbacks with C linkage
extern "C" {
    void callback_discovered_sinkinput(pa_context *context, const pa_sink_input_info *i, int eol, void *userdata);
    void callback_subscription_alert(pa_context *context, pa_subscription_event_type_t t, uint32_t index, void *userdata);
    void callback_context_state(pa_context *context, void *userdata);
}

VolumeCatcherPulseAudio::VolumeCatcherPulseAudio()
    : mDesiredVolume(0.0f),
      mMainloop(nullptr),
      mPAContext(nullptr),
      mConnected(false),
      mGotSyms(false)
{
    init();
}

VolumeCatcherPulseAudio::~VolumeCatcherPulseAudio()
{
    cleanup();
}

bool VolumeCatcherPulseAudio::loadsyms(std::string pulse_dso_name)
{
    return paSymbolGrabber.grabSymbols({ pulse_dso_name });
}

void VolumeCatcherPulseAudio::init()
{
    // try to be as defensive as possible because PA's interface is a
    // bit fragile and (for our purposes) we'd rather simply not function
    // than crash

    // we cheat and rely upon libpulse-mainloop-glib.so.0 to pull-in
    // libpulse.so.0 - this isn't a great assumption, and the two DSOs should
    // probably be loaded separately.  Our Linux DSO framework needs refactoring,
    // we do this sort of thing a lot with practically identical logic...
    mGotSyms = loadsyms("libpulse-mainloop-glib.so.0");

    if (!mGotSyms)
        mGotSyms = loadsyms("libpulse.so.0");

    if (!mGotSyms)
        return;

    mMainloop = llpa_glib_mainloop_new(g_main_context_default());

    if (mMainloop)
    {
        pa_mainloop_api *api = llpa_glib_mainloop_get_api(mMainloop);

        if (api)
        {
            pa_proplist *proplist = llpa_proplist_new();

            if (proplist)
            {
                llpa_proplist_sets(proplist, PA_PROP_APPLICATION_ICON_NAME, "multimedia-player");
                llpa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, "com.secondlife.viewer.mediaplugvoladjust");
                llpa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "SL Plugin Volume Adjuster");
                llpa_proplist_sets(proplist, PA_PROP_APPLICATION_VERSION, "1");

                // plain old pa_context_new() is broken!
                mPAContext = llpa_context_new_with_proplist(api, nullptr, proplist);

                llpa_proplist_free(proplist);
            }
        }
    }

    // Now we've set up a PA context and mainloop, try connecting the
    // PA context to a PA daemon.
    if (mPAContext)
    {
        llpa_context_set_state_callback(mPAContext, callback_context_state, this);
        pa_context_flags_t cflags = (pa_context_flags)0; // maybe add PA_CONTEXT_NOAUTOSPAWN?
        if (llpa_context_connect(mPAContext, nullptr, cflags, nullptr) >= 0)
        {
            // Okay!  We haven't definitely connected, but we
            // haven't definitely failed yet.
        }
        else
        {
            // Failed to connect to PA manager... we'll leave
            // things like that.  Perhaps we should try again later.
        }
    }
}

void VolumeCatcherPulseAudio::cleanup()
{
    mConnected = false;

    if (mGotSyms && mPAContext)
    {
        llpa_context_disconnect(mPAContext);
        llpa_context_unref(mPAContext);
    }

    mPAContext = nullptr;

    if (mGotSyms && mMainloop)
        llpa_glib_mainloop_free(mMainloop);

    mMainloop = nullptr;
}

void VolumeCatcherPulseAudio::setVolume(F32 volume)
{
    mDesiredVolume = volume;

    if (!mGotSyms)
        return;

    if (mConnected && mPAContext)
    {
        update_all_volumes(mDesiredVolume);
    }

    pump();
}

void VolumeCatcherPulseAudio::setPan(F32 pan)
{
}

void VolumeCatcherPulseAudio::pump()
{
    gboolean may_block = FALSE;
    g_main_context_iteration(g_main_context_default(), may_block);
}

void VolumeCatcherPulseAudio::connected_okay()
{
    pa_operation *op;

    // fetch global list of existing sinkinputs
    if ((op = llpa_context_get_sink_input_info_list(mPAContext,
                            callback_discovered_sinkinput,
                            this)))
    {
        llpa_operation_unref(op);
    }

    // subscribe to future global sinkinput changes
    llpa_context_set_subscribe_callback(mPAContext,
                        callback_subscription_alert,
                        this);
    if ((op = llpa_context_subscribe(mPAContext, (pa_subscription_mask_t)
                     (PA_SUBSCRIPTION_MASK_SINK_INPUT),
                                     nullptr, nullptr)))
    {
        llpa_operation_unref(op);
    }
}

void VolumeCatcherPulseAudio::update_all_volumes(F32 volume)
{
    for (std::set<U32>::iterator it = mSinkInputIndices.begin();
         it != mSinkInputIndices.end(); ++it)
    {
        update_index_volume(*it, volume);
    }
}

void VolumeCatcherPulseAudio::update_index_volume(U32 index, F32 volume)
{
    static pa_cvolume cvol;
    llpa_cvolume_set(&cvol, mSinkInputNumChannels[index],
             llpa_sw_volume_from_linear(volume));

    pa_context *c = mPAContext;
    uint32_t idx = index;
    const pa_cvolume *cvolumep = &cvol;
    pa_context_success_cb_t cb = nullptr; // okay as null
    void *userdata = nullptr; // okay as null

    pa_operation *op;
    if ((op = llpa_context_set_sink_input_volume(c, idx, cvolumep, cb, userdata)))
        llpa_operation_unref(op);
}

void callback_discovered_sinkinput(pa_context *context, const pa_sink_input_info *sii, int eol, void *userdata)
{
    VolumeCatcherPulseAudio *impl = dynamic_cast<VolumeCatcherPulseAudio*>((VolumeCatcherPulseAudio*)userdata);
    llassert(impl);

    if (0 == eol)
    {
        pa_proplist *proplist = sii->proplist;
        pid_t sinkpid = atoll(llpa_proplist_gets(proplist, PA_PROP_APPLICATION_PROCESS_ID));

        if (isPluginPid( sinkpid )) // does the discovered sinkinput belong to this process?
        {
            bool is_new = (impl->mSinkInputIndices.find(sii->index) == impl->mSinkInputIndices.end());

            impl->mSinkInputIndices.insert(sii->index);
            impl->mSinkInputNumChannels[sii->index] = sii->channel_map.channels;

            if (is_new)
            {
                // new!
                impl->update_index_volume(sii->index, impl->mDesiredVolume);
            }
            else
            {
                // seen it already, do nothing.
            }
        }
    }
}

void callback_subscription_alert(pa_context *context, pa_subscription_event_type_t t, uint32_t index, void *userdata)
{
    VolumeCatcherPulseAudio *impl = dynamic_cast<VolumeCatcherPulseAudio*>((VolumeCatcherPulseAudio*)userdata);
    llassert(impl);

    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)
    {
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
            if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==  PA_SUBSCRIPTION_EVENT_REMOVE)
            {
                // forget this sinkinput, if we were caring about it
                impl->mSinkInputIndices.erase(index);
                impl->mSinkInputNumChannels.erase(index);
            }
            else if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW)
            {
                // ask for more info about this new sinkinput
                pa_operation *op;
                if ((op = llpa_context_get_sink_input_info(impl->mPAContext, index, callback_discovered_sinkinput, impl)))
                {
                    llpa_operation_unref(op);
                }
            }
            else
            {
                // property change on this sinkinput - we don't care.
            }
            break;

        default:;
    }
}

void callback_context_state(pa_context *context, void *userdata)
{
    VolumeCatcherPulseAudio *impl = dynamic_cast<VolumeCatcherPulseAudio*>((VolumeCatcherPulseAudio*)userdata);
    llassert(impl);

    switch (llpa_context_get_state(context))
    {
    case PA_CONTEXT_READY:
        impl->mConnected = true;
        impl->connected_okay();
        break;
    case PA_CONTEXT_TERMINATED:
        impl->mConnected = false;
        break;
    case PA_CONTEXT_FAILED:
        impl->mConnected = false;
        break;
    default:;
    }
}
