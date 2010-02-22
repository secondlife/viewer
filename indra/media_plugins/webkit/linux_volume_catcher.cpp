/** 
 * @file linux_volume_catcher.cpp
 * @brief A Linux-specific, PulseAudio-specific hack to detect and volume-adjust new audio sources
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#include <glib.h>

#include <pulse/introspect.h>
#include <pulse/context.h>
#include <pulse/subscribe.h>
#include <pulse/glib-mainloop.h> // There's no special reason why we want the *glib* PA mainloop, but the generic polling implementation seems broken.

#include "linux_volume_catcher.h"

////////////////////////////////////////////////////

// PulseAudio requires a chain of callbacks with C linkage
extern "C" {
	void callback_discovered_sinkinput(pa_context *context, const pa_sink_input_info *i, int eol, void *userdata);
	void callback_subscription_alert(pa_context *context, pa_subscription_event_type_t t, uint32_t index, void *userdata);
	void callback_context_state(pa_context *context, void *userdata);
}


class LinuxVolumeCatcherImpl
{
public:
	LinuxVolumeCatcherImpl();
	~LinuxVolumeCatcherImpl();

	void setVol(F32 volume);
	void pump(void);

	// for internal use - can't be private because used from our C callbacks

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
};

LinuxVolumeCatcherImpl::LinuxVolumeCatcherImpl()
	: mDesiredVolume(0.0f),
	  mMainloop(NULL),
	  mPAContext(NULL),
	  mConnected(false)
{
	init();
}

LinuxVolumeCatcherImpl::~LinuxVolumeCatcherImpl()
{
	cleanup();
}

void LinuxVolumeCatcherImpl::init()
{
	// try to be as robust as possible because PA's interface is a
	// bit fragile and (for our purposes) we'd rather simply not function
	// than crash
	mMainloop = pa_glib_mainloop_new(g_main_context_default());
	if (mMainloop)
	{
		pa_mainloop_api *api = pa_glib_mainloop_get_api(mMainloop);
		if (api)
		{
			pa_proplist *proplist = pa_proplist_new();
			if (proplist)
			{
				pa_proplist_sets(proplist, PA_PROP_APPLICATION_ICON_NAME, "multimedia-player");
				pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, "com.secondlife.viewer.mediaplugvoladjust");
				pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "SL Plugin Volume Adjuster");
				pa_proplist_sets(proplist, PA_PROP_APPLICATION_VERSION, "1");

				// plain old pa_context_new() is broken!
				mPAContext = pa_context_new_with_proplist(api, NULL, proplist);
				pa_proplist_free(proplist);
			}
		}
	}

	// Now we've set up a PA context and mainloop, try connecting the
	// PA context to a PA daemon.
	if (mPAContext)
	{
		pa_context_set_state_callback(mPAContext, callback_context_state, this);
		pa_context_flags_t cflags = (pa_context_flags)0; // maybe add PA_CONTEXT_NOAUTOSPAWN?
		if (pa_context_connect(mPAContext, NULL, cflags, NULL) >= 0)
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

void LinuxVolumeCatcherImpl::cleanup()
{
	// there's some cleanup we could do, but do nothing... for now.

	mConnected = false;
}

void LinuxVolumeCatcherImpl::setVol(F32 volume)
{
	mDesiredVolume = volume;
	
	if (mConnected && mPAContext)
	{
		update_all_volumes(mDesiredVolume);
	}

	pump();
}

void LinuxVolumeCatcherImpl::pump()
{
	g_main_context_iteration(g_main_context_default());
}

void LinuxVolumeCatcherImpl::connected_okay()
{
	pa_operation *op;

	// fetch global list of existing sinkinputs
	if (op = pa_context_get_sink_input_info_list(mPAContext,
						     callback_discovered_sinkinput,
						     this))
	{
		pa_operation_unref(op);
	}

	// subscribe to future global sinkinput changes
	pa_context_set_subscribe_callback(mPAContext,
					  callback_subscription_alert,
					  this);
	if (op =  pa_context_subscribe(mPAContext, (pa_subscription_mask_t)
				       (PA_SUBSCRIPTION_MASK_SINK_INPUT),
				       NULL, NULL))
	{
		pa_operation_unref(op);
	}
}

void LinuxVolumeCatcherImpl::update_all_volumes(F32 volume)
{
	for (std::set<U32>::iterator it = mSinkInputIndices.begin();
	     it != mSinkInputIndices.end(); ++it)
	{
		update_index_volume(*it, volume);
	}
}

void LinuxVolumeCatcherImpl::update_index_volume(U32 index, F32 volume)
{
	static pa_cvolume cvol;
	pa_cvolume_set(&cvol, mSinkInputNumChannels[index],
		       pa_sw_volume_from_linear(volume));
	
	pa_context *c = mPAContext;
	uint32_t idx = index;
	const pa_cvolume *volume = &cvol;
	pa_context_success_cb_t cb = NULL; // okay as null
	void *userdata = NULL; // okay as null
	
	if (op = pa_context_set_sink_input_volume(c, idx, volume, cb, userdata))
	{
		pa_operation_unref(op);
	}
}


void callback_discovered_sinkinput(pa_context *context, const pa_sink_input_info *sii, int eol, void *userdata)
{
	LinuxVolumeCatcherImpl *impl = dynamic_cast<LinuxVolumeCatcherImpl*>(userdata);
	llassert(impl);

	if (0 == eol)
	{
		pa_proplist *proplist = sii->proplist;
		pid_t sinkpid = atoll(pa_proplist_gets(proplist, PA_PROP_APPLICATION_PROCESS_ID));
		fprintf(stderr, "Found sinkinput #%d, name=%s, pid=%d\t", sii->index, sii->name, int(sinkpid));
		
		if (sinkpid == getpid()) // does the discovered sinkinput belong to this process?
		{
			bool is_new = (impl->mSinkInputIndices.find(sii->index) ==
				       impl->mSinkInputIndices.end());
			
			impl->mSinkInputIndices.insert(sii->index);
			impl->mSinkInputNumChannels[sii->index] = sii->channel_map.channels;
			
			if (is_new)
			{
				// new!
				fprintf(stderr, "\nJACKPOT!\n");
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
	LinuxVolumeCatcherImpl *impl = dynamic_cast<LinuxVolumeCatcherImpl*>(userdata);
	llassert(impl);

	switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
		if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) ==
		    PA_SUBSCRIPTION_EVENT_REMOVE)
		{
			// forget this sinkinput, if we were caring about it
			impl->mSinkInputIndices.erase(index);
			impl->mSinkInputNumChannels.erase(index);
		}
		else
		{
			// ask for more info about this new sinkinput
			pa_operation *op;
			if (op = pa_context_get_sink_input_info(impl->mPAContext, index, callback_discovered_sinkinput, impl))
			{
				pa_operation_unref(o);
			}
		}
		break;
		
	default:;
	}
}

void callback_context_state(pa_context *context, void *userdata)
{
	LinuxVolumeCatcherImpl *impl = dynamic_cast<LinuxVolumeCatcherImpl*>(userdata);
	llassert(impl);
	
	switch (pa_context_get_state(c))
	{
	case PA_CONTEXT_READY:
		impl->mConnected = true;
		impl->connected_okay(c);
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

LinuxVolumeCatcher::LinuxVolumeCatcher()
{
	pimpl = new LinuxVolumeCatcherImpl();
}

LinuxVolumeCatcher::LinuxVolumeCatcher~()
{
	delete pimpl;
	pimpl = NULL;
}

void LinuxVolumeCatcher::setVol(F32 volume)
{
	llassert(pimpl);
	pimpl->setVol(volume);
}

void LinuxVolumeCatcher::pump()
{
	llassert(pimpl);
	pimpl->pump();
}

