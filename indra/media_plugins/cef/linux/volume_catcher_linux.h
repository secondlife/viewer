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

class VolumeCatcherPulseAudio : public virtual VolumeCatcherImpl
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

class VolumeCatcherPipeWire : public virtual VolumeCatcherImpl
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

	pw_thread_loop* mThreadLoop;
	pw_context* mContext;
	pw_core* mCore;
	pw_registry* mRegistry;
	spa_hook mRegistryListener;

	std::unordered_set<ChildNode*> mChildNodes;
	std::mutex mChildNodesMutex;
};

// static void debugClear()
// {
// 	auto file = fopen("volume-catcher-log.txt", "w");
// 	fprintf(file, "\n");
// 	fclose(file);
// }

// static void debugPrint(const char* format, ...)
// {
//     va_list args;
//     va_start(args, format);
// 	auto file = fopen("volume-catcher-log.txt", "a");
//     vfprintf(file, format, args);
// 	fclose(file);
//     va_end(args);
// }

#endif // VOLUME_CATCHER_LINUX_H
