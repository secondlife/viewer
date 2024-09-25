/**
 * @file volume_catcher_impl.h
 * @brief
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

#ifndef VOLUME_CATCHER_LINUX_H
#define VOLUME_CATCHER_LINUX_H

#include "linden_common.h"

#include "../volume_catcher.h"

#include <unordered_set>
#include <mutex>

extern "C" {
// There's no special reason why we want the *glib* PA mainloop, but the generic polling implementation seems broken.
#include <pulse/glib-mainloop.h>
#include <pulse/context.h>

#include <pipewire/pipewire.h>

#include "apr_pools.h"
#include "apr_dso.h"
}

#include "media_plugin_base.h"

class VolumeCatcherImpl
{
public:
    virtual ~VolumeCatcherImpl() = default;

    virtual void setVolume(F32 volume) = 0; // 0.0 - 1.0

    // Set the left-right pan of audio sources
    // where -1.0 = left, 0 = center, and 1.0 = right
    virtual void setPan(F32 pan) = 0;

    virtual void pump() = 0; // call this at least a few times a second if you can - it affects how quickly we can 'catch' a new audio source and adjust its volume
};

class VolumeCatcherPulseAudio : public VolumeCatcherImpl
{
public:
    VolumeCatcherPulseAudio();
    ~VolumeCatcherPulseAudio();

    void setVolume(F32 volume);
    void setPan(F32 pan);
    void pump();

    // for internal use - can't be private because used from our C callbacks

    bool loadsyms(std::string pa_dso_name);
    void init();
    void cleanup();

    void update_all_volumes(F32 volume);
    void update_index_volume(U32 index, F32 volume);
    void connected_okay();

    std::set<U32> mSinkInputIndices;
    std::map<U32,U32> mSinkInputNumChannels;
    F32 mDesiredVolume;
    pa_glib_mainloop *mMainloop;
    pa_context *mPAContext;
    bool mConnected;
    bool mGotSyms;
};

class VolumeCatcherPipeWire : public VolumeCatcherImpl
{
public:
    VolumeCatcherPipeWire();
    ~VolumeCatcherPipeWire();

    bool loadsyms(std::string pw_dso_name);
    void init();
    void cleanup();

    // some of these should be private

    void lock();
    void unlock();

    void setVolume(F32 volume);
    void setPan(F32 pan);
    void pump();

    void handleRegistryEventGlobal(
        uint32_t id, uint32_t permissions, const char* type,
        uint32_t version, const struct spa_dict* props
    );

    class ChildNode
    {
    public:
        bool mActive = false;

        pw_proxy* mProxy = nullptr;
        spa_hook mNodeListener {};
        spa_hook mProxyListener {};
        VolumeCatcherPipeWire* mImpl = nullptr;

        void updateVolume();
        void destroy();
    };

    bool mGotSyms = false;

    F32 mVolume = 1.0f; // max by default
    // F32 mPan = 0.0f; // center

    pw_thread_loop* mThreadLoop = nullptr;
    pw_context* mContext = nullptr;
    pw_core* mCore = nullptr;
    pw_registry* mRegistry = nullptr;
    spa_hook mRegistryListener;

    std::unordered_set<ChildNode*> mChildNodes;
    std::mutex mChildNodesMutex;
    std::mutex mCleanupMutex;
};

#endif // VOLUME_CATCHER_LINUX_H
