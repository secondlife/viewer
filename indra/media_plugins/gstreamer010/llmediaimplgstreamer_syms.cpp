/** 
 * @file llmediaimplgstreamer_syms.cpp
 * @brief dynamic GStreamer symbol-grabbing code
 *
 * @cond
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#if LL_GSTREAMER010_ENABLED

#include <string>

extern "C" {
#include <gst/gst.h>

#include "apr_pools.h"
#include "apr_dso.h"
}

#include "llmediaimplgstreamertriviallogging.h"

#define LL_GST_SYM(REQ, GSTSYM, RTN, ...) RTN (*ll##GSTSYM)(__VA_ARGS__) = NULL
#include "llmediaimplgstreamer_syms_raw.inc"
#include "llmediaimplgstreamer_syms_rawv.inc"
#undef LL_GST_SYM

// a couple of stubs for disgusting reasons
GstDebugCategory*
ll_gst_debug_category_new(gchar *name, guint color, gchar *description)
{
	static GstDebugCategory dummy;
	return &dummy;
}
void ll_gst_debug_register_funcptr(GstDebugFuncPtr func, gchar* ptrname)
{
}

static bool sSymsGrabbed = false;
static apr_pool_t *sSymGSTDSOMemoryPool = NULL;
static apr_dso_handle_t *sSymGSTDSOHandleG = NULL;
static apr_dso_handle_t *sSymGSTDSOHandleV = NULL;


bool grab_gst_syms(std::string gst_dso_name,
		   std::string gst_dso_name_vid)
{
	if (sSymsGrabbed)
	{
		// already have grabbed good syms
		return TRUE;
	}

	bool sym_error = false;
	bool rtn = false;
	apr_status_t rv;
	apr_dso_handle_t *sSymGSTDSOHandle = NULL;

#define LL_GST_SYM(REQ, GSTSYM, RTN, ...) do{rv = apr_dso_sym((apr_dso_handle_sym_t*)&ll##GSTSYM, sSymGSTDSOHandle, #GSTSYM); if (rv != APR_SUCCESS) {INFOMSG("Failed to grab symbol: %s", #GSTSYM); if (REQ) sym_error = true;} else DEBUGMSG("grabbed symbol: %s from %p", #GSTSYM, (void*)ll##GSTSYM);}while(0)

	//attempt to load the shared libraries
	apr_pool_create(&sSymGSTDSOMemoryPool, NULL);
  
	if ( APR_SUCCESS == (rv = apr_dso_load(&sSymGSTDSOHandle,
					       gst_dso_name.c_str(),
					       sSymGSTDSOMemoryPool) ))
	{
		INFOMSG("Found DSO: %s", gst_dso_name.c_str());
#include "llmediaimplgstreamer_syms_raw.inc"
      
		if ( sSymGSTDSOHandle )
		{
			sSymGSTDSOHandleG = sSymGSTDSOHandle;
			sSymGSTDSOHandle = NULL;
		}
      
		if ( APR_SUCCESS ==
		     (rv = apr_dso_load(&sSymGSTDSOHandle,
					gst_dso_name_vid.c_str(),
					sSymGSTDSOMemoryPool) ))
		{
			INFOMSG("Found DSO: %s", gst_dso_name_vid.c_str());
#include "llmediaimplgstreamer_syms_rawv.inc"
			rtn = !sym_error;
		}
		else
		{
			INFOMSG("Couldn't load DSO: %s", gst_dso_name_vid.c_str());
			rtn = false; // failure
		}
	}
	else
	{
		INFOMSG("Couldn't load DSO: %s", gst_dso_name.c_str());
		rtn = false; // failure
	}

	if (sym_error)
	{
		WARNMSG("Failed to find necessary symbols in GStreamer libraries.");
	}
	
	if ( sSymGSTDSOHandle )
	{
		sSymGSTDSOHandleV = sSymGSTDSOHandle;
		sSymGSTDSOHandle = NULL;
	}
#undef LL_GST_SYM

	sSymsGrabbed = !!rtn;
	return rtn;
}


void ungrab_gst_syms()
{ 
	// should be safe to call regardless of whether we've
	// actually grabbed syms.

	if ( sSymGSTDSOHandleG )
	{
		apr_dso_unload(sSymGSTDSOHandleG);
		sSymGSTDSOHandleG = NULL;
	}
	
	if ( sSymGSTDSOHandleV )
	{
		apr_dso_unload(sSymGSTDSOHandleV);
		sSymGSTDSOHandleV = NULL;
	}
	
	if ( sSymGSTDSOMemoryPool )
	{
		apr_pool_destroy(sSymGSTDSOMemoryPool);
		sSymGSTDSOMemoryPool = NULL;
	}
	
	// NULL-out all of the symbols we'd grabbed
#define LL_GST_SYM(REQ, GSTSYM, RTN, ...) do{ll##GSTSYM = NULL;}while(0)
#include "llmediaimplgstreamer_syms_raw.inc"
#include "llmediaimplgstreamer_syms_rawv.inc"
#undef LL_GST_SYM

	sSymsGrabbed = false;
}


#endif // LL_GSTREAMER010_ENABLED
