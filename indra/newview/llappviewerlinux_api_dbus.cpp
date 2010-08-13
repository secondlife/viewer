/** 
 * @file llappviewerlinux_api_dbus.cpp
 * @brief dynamic DBus symbol-grabbing code
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 */

#if LL_DBUS_ENABLED

#include "linden_common.h"

extern "C" {
#include <dbus/dbus-glib.h>

#include "apr_pools.h"
#include "apr_dso.h"
}

#define DEBUGMSG(...) do { lldebugs << llformat(__VA_ARGS__) << llendl; } while(0)
#define INFOMSG(...) do { llinfos << llformat(__VA_ARGS__) << llendl; } while(0)
#define WARNMSG(...) do { llwarns << llformat(__VA_ARGS__) << llendl; } while(0)

#define LL_DBUS_SYM(REQUIRED, DBUSSYM, RTN, ...) RTN (*ll##DBUSSYM)(__VA_ARGS__) = NULL
#include "llappviewerlinux_api_dbus_syms_raw.inc"
#undef LL_DBUS_SYM

static bool sSymsGrabbed = false;
static apr_pool_t *sSymDBUSDSOMemoryPool = NULL;
static apr_dso_handle_t *sSymDBUSDSOHandleG = NULL;

bool grab_dbus_syms(std::string dbus_dso_name)
{
	if (sSymsGrabbed)
	{
		// already have grabbed good syms
		return TRUE;
	}

	bool sym_error = false;
	bool rtn = false;
	apr_status_t rv;
	apr_dso_handle_t *sSymDBUSDSOHandle = NULL;

#define LL_DBUS_SYM(REQUIRED, DBUSSYM, RTN, ...) do{rv = apr_dso_sym((apr_dso_handle_sym_t*)&ll##DBUSSYM, sSymDBUSDSOHandle, #DBUSSYM); if (rv != APR_SUCCESS) {INFOMSG("Failed to grab symbol: %s", #DBUSSYM); if (REQUIRED) sym_error = true;} else DEBUGMSG("grabbed symbol: %s from %p", #DBUSSYM, (void*)ll##DBUSSYM);}while(0)

	//attempt to load the shared library
	apr_pool_create(&sSymDBUSDSOMemoryPool, NULL);
  
	if ( APR_SUCCESS == (rv = apr_dso_load(&sSymDBUSDSOHandle,
					       dbus_dso_name.c_str(),
					       sSymDBUSDSOMemoryPool) ))
	{
		INFOMSG("Found DSO: %s", dbus_dso_name.c_str());

#include "llappviewerlinux_api_dbus_syms_raw.inc"
      
		if ( sSymDBUSDSOHandle )
		{
			sSymDBUSDSOHandleG = sSymDBUSDSOHandle;
			sSymDBUSDSOHandle = NULL;
		}
      
		rtn = !sym_error;
	}
	else
	{
		INFOMSG("Couldn't load DSO: %s", dbus_dso_name.c_str());
		rtn = false; // failure
	}

	if (sym_error)
	{
		WARNMSG("Failed to find necessary symbols in DBUS-GLIB libraries.");
	}
#undef LL_DBUS_SYM

	sSymsGrabbed = rtn;
	return rtn;
}


void ungrab_dbus_syms()
{ 
	// should be safe to call regardless of whether we've
	// actually grabbed syms.

	if ( sSymDBUSDSOHandleG )
	{
		apr_dso_unload(sSymDBUSDSOHandleG);
		sSymDBUSDSOHandleG = NULL;
	}
	
	if ( sSymDBUSDSOMemoryPool )
	{
		apr_pool_destroy(sSymDBUSDSOMemoryPool);
		sSymDBUSDSOMemoryPool = NULL;
	}
	
	// NULL-out all of the symbols we'd grabbed
#define LL_DBUS_SYM(REQUIRED, DBUSSYM, RTN, ...) do{ll##DBUSSYM = NULL;}while(0)
#include "llappviewerlinux_api_dbus_syms_raw.inc"
#undef LL_DBUS_SYM

	sSymsGrabbed = false;
}

#endif // LL_DBUS_ENABLED
