/** 
 * @file linux_volume_catcher.cpp
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

#include "volume_catcher.h"
#include <set>
#include <map>

extern "C" {
#include <glib.h>
#include <glib-object.h>

#include <pulse/introspect.h>
#include <pulse/context.h>
#include <pulse/subscribe.h>
#include <pulse/glib-mainloop.h> // There's no special reason why we want the *glib* PA mainloop, but the generic polling implementation seems broken.

#include "apr_pools.h"
#include "apr_dso.h"
}


////////////////////////////////////////////////////

#define DEBUGMSG(...) do {} while(0)
#define INFOMSG(...) do {} while(0)
#define WARNMSG(...) do {} while(0)

#define LL_PA_SYM(REQUIRED, PASYM, RTN, ...) RTN (*ll##PASYM)(__VA_ARGS__) = NULL
#include "linux_volume_catcher_pa_syms.inc"
#include "linux_volume_catcher_paglib_syms.inc"
#undef LL_PA_SYM

static bool sSymsGrabbed = false;
static apr_pool_t *sSymPADSOMemoryPool = NULL;
static apr_dso_handle_t *sSymPADSOHandleG = NULL;

bool grab_pa_syms(std::string pulse_dso_name)
{
	if (sSymsGrabbed)
	{
		// already have grabbed good syms
		return true;
	}

	bool sym_error = false;
	bool rtn = false;
	apr_status_t rv;
	apr_dso_handle_t *sSymPADSOHandle = NULL;

#define LL_PA_SYM(REQUIRED, PASYM, RTN, ...) do{rv = apr_dso_sym((apr_dso_handle_sym_t*)&ll##PASYM, sSymPADSOHandle, #PASYM); if (rv != APR_SUCCESS) { if (REQUIRED) sym_error = true;} } while(0);

	//attempt to load the shared library
	apr_pool_create(&sSymPADSOMemoryPool, NULL);
  
	if ( APR_SUCCESS == (rv = apr_dso_load(&sSymPADSOHandle,
					       pulse_dso_name.c_str(),
					       sSymPADSOMemoryPool) ))
	{
		INFOMSG("Found DSO: %s", pulse_dso_name.c_str());

#include "linux_volume_catcher_pa_syms.inc"
#include "linux_volume_catcher_paglib_syms.inc"
      
		if ( sSymPADSOHandle )
		{
			sSymPADSOHandleG = sSymPADSOHandle;
			sSymPADSOHandle = NULL;
		}
      
		rtn = !sym_error;
	}
	else
	{
		INFOMSG("Couldn't load DSO: %s", pulse_dso_name.c_str());
		rtn = false; // failure
	}

	if (sym_error)
	{
		WARNMSG("Failed to find necessary symbols in PulseAudio libraries.");
	}
#undef LL_PA_SYM

	sSymsGrabbed = rtn;
	return rtn;
}


void ungrab_pa_syms()
{ 
	// should be safe to call regardless of whether we've
	// actually grabbed syms.

	if ( sSymPADSOHandleG )
	{
		apr_dso_unload(sSymPADSOHandleG);
		sSymPADSOHandleG = NULL;
	}
	
	if ( sSymPADSOMemoryPool )
	{
		apr_pool_destroy(sSymPADSOMemoryPool);
		sSymPADSOMemoryPool = NULL;
	}
	
	// NULL-out all of the symbols we'd grabbed
#define LL_PA_SYM(REQUIRED, PASYM, RTN, ...) do{ll##PASYM = NULL;}while(0)
#include "linux_volume_catcher_pa_syms.inc"
#include "linux_volume_catcher_paglib_syms.inc"
#undef LL_PA_SYM

	sSymsGrabbed = false;
}
////////////////////////////////////////////////////

// PulseAudio requires a chain of callbacks with C linkage
extern "C" {
	void callback_discovered_sinkinput(pa_context *context, const pa_sink_input_info *i, int eol, void *userdata);
	void callback_subscription_alert(pa_context *context, pa_subscription_event_type_t t, uint32_t index, void *userdata);
	void callback_context_state(pa_context *context, void *userdata);
}


class VolumeCatcherImpl
{
public:
	VolumeCatcherImpl();
	~VolumeCatcherImpl();

	void setVolume(F32 volume);
	void pump(void);

	// for internal use - can't be private because used from our C callbacks

	bool loadsyms(std::string pulse_dso_name);
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

VolumeCatcherImpl::VolumeCatcherImpl()
	: mDesiredVolume(0.0f),
	  mMainloop(NULL),
	  mPAContext(NULL),
	  mConnected(false),
	  mGotSyms(false)
{
	init();
}

VolumeCatcherImpl::~VolumeCatcherImpl()
{
	cleanup();
}

bool VolumeCatcherImpl::loadsyms(std::string pulse_dso_name)
{
	return grab_pa_syms(pulse_dso_name);
}

void VolumeCatcherImpl::init()
{
	// try to be as defensive as possible because PA's interface is a
	// bit fragile and (for our purposes) we'd rather simply not function
	// than crash

	// we cheat and rely upon libpulse-mainloop-glib.so.0 to pull-in
	// libpulse.so.0 - this isn't a great assumption, and the two DSOs should
	// probably be loaded separately.  Our Linux DSO framework needs refactoring,
	// we do this sort of thing a lot with practically identical logic...
	mGotSyms = loadsyms("libpulse-mainloop-glib.so.0");
	if (!mGotSyms) return;

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
				mPAContext = llpa_context_new_with_proplist(api, NULL, proplist);

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
		if (llpa_context_connect(mPAContext, NULL, cflags, NULL) >= 0)
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

void VolumeCatcherImpl::cleanup()
{
	mConnected = false;

	if (mGotSyms && mPAContext)
	{
		llpa_context_disconnect(mPAContext);
		llpa_context_unref(mPAContext);
	}
	mPAContext = NULL;

	if (mGotSyms && mMainloop)
	{
		llpa_glib_mainloop_free(mMainloop);
	}
	mMainloop = NULL;
}

void VolumeCatcherImpl::setVolume(F32 volume)
{
	mDesiredVolume = volume;
	
	if (!mGotSyms) return;

	if (mConnected && mPAContext)
	{
		update_all_volumes(mDesiredVolume);
	}

	pump();
}

void VolumeCatcherImpl::pump()
{
	gboolean may_block = FALSE;
	g_main_context_iteration(g_main_context_default(), may_block);
}

void VolumeCatcherImpl::connected_okay()
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
					 NULL, NULL)))
	{
		llpa_operation_unref(op);
	}
}

void VolumeCatcherImpl::update_all_volumes(F32 volume)
{
	for (std::set<U32>::iterator it = mSinkInputIndices.begin();
	     it != mSinkInputIndices.end(); ++it)
	{
		update_index_volume(*it, volume);
	}
}

void VolumeCatcherImpl::update_index_volume(U32 index, F32 volume)
{
	static pa_cvolume cvol;
	llpa_cvolume_set(&cvol, mSinkInputNumChannels[index],
			 llpa_sw_volume_from_linear(volume));
	
	pa_context *c = mPAContext;
	uint32_t idx = index;
	const pa_cvolume *cvolumep = &cvol;
	pa_context_success_cb_t cb = NULL; // okay as null
	void *userdata = NULL; // okay as null

	pa_operation *op;
	if ((op = llpa_context_set_sink_input_volume(c, idx, cvolumep, cb, userdata)))
	{
		llpa_operation_unref(op);
	}
}

pid_t getParentPid( pid_t aPid )
{
	std::stringstream strm;
	strm << "/proc/" << aPid << "/status";
	std::ifstream in{ strm.str() };

	if( !in.is_open() )
		return 0;

	pid_t res {0};
	while( !in.eof() && res == 0 )
	{
		std::string line;
		line.resize( 1024, 0 );
		in.getline( &line[0], line.length() );	

		auto i = line.find( "PPid:"  );
		
		if( i == std::string::npos )
			continue;
		
		char const *pIn = line.c_str() + 5; // Skip over pid;
		while( *pIn != 0 && isspace( *pIn ) )
			   ++pIn;

		if( *pIn )
			res = atoll( pIn );
	}
 	return res;
}


bool isPluginPid( pid_t aPid )
{
	auto myPid = getpid();

	do
	{
		if( aPid == myPid )
			return true;
		aPid = getParentPid( aPid );
	} while( aPid > 1 );

	return false;
}

void callback_discovered_sinkinput(pa_context *context, const pa_sink_input_info *sii, int eol, void *userdata)
{
	VolumeCatcherImpl *impl = dynamic_cast<VolumeCatcherImpl*>((VolumeCatcherImpl*)userdata);
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
	VolumeCatcherImpl *impl = dynamic_cast<VolumeCatcherImpl*>((VolumeCatcherImpl*)userdata);
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
	VolumeCatcherImpl *impl = dynamic_cast<VolumeCatcherImpl*>((VolumeCatcherImpl*)userdata);
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

/////////////////////////////////////////////////////

VolumeCatcher::VolumeCatcher()
{
	pimpl = new VolumeCatcherImpl();
}

VolumeCatcher::~VolumeCatcher()
{
	delete pimpl;
	pimpl = NULL;
}

void VolumeCatcher::setVolume(F32 volume)
{
	llassert(pimpl);
	pimpl->setVolume(volume);
}

void VolumeCatcher::setPan(F32 pan)
{
	// TODO: implement this (if possible)
}

void VolumeCatcher::pump()
{
	llassert(pimpl);
	pimpl->pump();
}
