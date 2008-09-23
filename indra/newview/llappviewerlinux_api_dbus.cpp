/** 
 * @file llappviewerlinux_api_dbus.cpp
 * @brief dynamic DBus symbol-grabbing code
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
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
 */

#if LL_DBUS_ENABLED

#include "linden_common.h"

extern "C" {
#include <dbus/dbus-glib.h>

#include "apr_pools.h"
#include "apr_dso.h"
}

#define DEBUGMSG(...) lldebugs << llformat(__VA_ARGS__) << llendl
#define INFOMSG(...) llinfos << llformat(__VA_ARGS__) << llendl
#define WARNMSG(...) llwarns << llformat(__VA_ARGS__) << llendl

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
