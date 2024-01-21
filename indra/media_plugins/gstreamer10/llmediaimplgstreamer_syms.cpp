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

#include <string>
#include <iostream>
#include <vector>

#ifdef LL_WINDOWS
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#include <Windows.h>
#endif

#include "linden_common.h"

extern "C" {
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
}

#include "apr_pools.h"
#include "apr_dso.h"

#ifdef LL_WINDOWS

#ifndef _M_AMD64
#define GSTREAMER_REG_KEY "Software\\GStreamer1.0\\x86"
#define GSTREAMER_DIR_SUFFIX "1.0\\x86\\bin\\"
#else
#define GSTREAMER_REG_KEY "Software\\GStreamer1.0\\x86_64"
#define GSTREAMER_DIR_SUFFIX "1.0\\x86_64\\bin\\"
#endif

bool openRegKey( HKEY &aKey )
{
	// Try native (32 bit view/64 bit view) of registry first.
	if( ERROR_SUCCESS == ::RegOpenKeyExA( HKEY_LOCAL_MACHINE, GSTREAMER_REG_KEY, 0, KEY_QUERY_VALUE, &aKey ) )
		return true;

	// If native view fails, use 32 bit view or registry.
	if( ERROR_SUCCESS == ::RegOpenKeyExA( HKEY_LOCAL_MACHINE, GSTREAMER_REG_KEY, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &aKey ) )
		return true;

	return false;
}

std::string getGStreamerDir()
{
	std::string ret;
	HKEY hKey;

	if( openRegKey( hKey ) )
	{
		DWORD dwLen(0);
		::RegQueryValueExA( hKey, "InstallDir", nullptr, nullptr, nullptr, &dwLen );

		if( dwLen > 0 )
		{
			std::vector< char > vctBuffer;
			vctBuffer.resize( dwLen );
			::RegQueryValueExA( hKey, "InstallDir", nullptr, nullptr, reinterpret_cast< LPBYTE>(&vctBuffer[ 0 ]), &dwLen );
			ret = &vctBuffer[0];

			if( ret[ dwLen-1 ] != '\\' )
				ret += "\\";
			ret += GSTREAMER_DIR_SUFFIX;

			SetDllDirectoryA( ret.c_str() );
		}
		::RegCloseKey( hKey );
	}
	return ret;
}
#else
std::string getGStreamerDir() { return ""; }
#endif

#define LL_GST_SYM(REQ, GSTSYM, RTN, ...) RTN (*ll##GSTSYM)(__VA_ARGS__) = NULL;
#include "llmediaimplgstreamer_syms_raw.inc"
#undef LL_GST_SYM

struct Symloader
{
	bool mRequired;
	char const *mName;
	apr_dso_handle_sym_t *mPPFunc;
};

#define LL_GST_SYM(REQ, GSTSYM, RTN, ...) { REQ, #GSTSYM , (apr_dso_handle_sym_t*)&ll##GSTSYM}, 
Symloader sSyms[] = {
#include "llmediaimplgstreamer_syms_raw.inc"
{ false, 0, 0 } };
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

std::vector< apr_dso_handle_t* > sLoadedLibraries;

bool grab_gst_syms( std::vector< std::string > const &aDSONames )
{
	if (sSymsGrabbed)
		return true;

	//attempt to load the shared libraries
	apr_pool_create(&sSymGSTDSOMemoryPool, NULL);
  
	for( std::vector< std::string >::const_iterator itr = aDSONames.begin(); itr != aDSONames.end(); ++itr )
	{
		apr_dso_handle_t *pDSO(NULL);
		std::string strDSO = getGStreamerDir() + *itr;
		if( APR_SUCCESS == apr_dso_load( &pDSO, strDSO.c_str(), sSymGSTDSOMemoryPool ))
			sLoadedLibraries.push_back( pDSO );
		
		for( int i = 0; sSyms[ i ].mName; ++i )
		{
			if( !*sSyms[ i ].mPPFunc )
			{
				apr_dso_sym( sSyms[ i ].mPPFunc, pDSO, sSyms[ i ].mName );
			}
		}
	}

	std::stringstream strm;
	bool sym_error = false;
	for( int i = 0; sSyms[ i ].mName; ++i )
	{
		if( sSyms[ i ].mRequired && ! *sSyms[ i ].mPPFunc )
		{
			sym_error = true;
			strm << sSyms[ i ].mName << std::endl;
		}
	}

	sSymsGrabbed = !sym_error;
	return sSymsGrabbed;
}


void ungrab_gst_syms()
{ 
	// should be safe to call regardless of whether we've
	// actually grabbed syms.

	for( std::vector< apr_dso_handle_t* >::iterator itr = sLoadedLibraries.begin(); itr != sLoadedLibraries.end(); ++itr )
		apr_dso_unload( *itr );

	sLoadedLibraries.clear();

	if ( sSymGSTDSOMemoryPool )
	{
		apr_pool_destroy(sSymGSTDSOMemoryPool);
		sSymGSTDSOMemoryPool = NULL;
	}

	for( int i = 0; sSyms[ i ].mName; ++i )
		*sSyms[ i ].mPPFunc = NULL;

	sSymsGrabbed = false;
}

