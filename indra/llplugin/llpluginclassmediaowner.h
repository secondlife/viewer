/** 
 * @file llpluginclassmediaowner.h
 * @brief LLPluginClassMedia handles interaction with a plugin which knows about the "media" message class.
 *
 * @cond
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
 * @endcond
 */

#ifndef LL_LLPLUGINCLASSMEDIAOWNER_H
#define LL_LLPLUGINCLASSMEDIAOWNER_H

#include "llpluginprocessparent.h"
#include "llrect.h"
#include <queue>

class LLPluginClassMedia;
class LLPluginCookieStore;

class LLPluginClassMediaOwner
{
public:
	typedef enum
	{
		MEDIA_EVENT_CONTENT_UPDATED,		// contents/dirty rect have updated 
		MEDIA_EVENT_TIME_DURATION_UPDATED,	// current time and/or duration have updated
		MEDIA_EVENT_SIZE_CHANGED,			// media size has changed
		MEDIA_EVENT_CURSOR_CHANGED,			// plugin has requested a cursor change
		
		MEDIA_EVENT_NAVIGATE_BEGIN,			// browser has begun navigation
		MEDIA_EVENT_NAVIGATE_COMPLETE,		// browser has finished navigation
		MEDIA_EVENT_PROGRESS_UPDATED,		// browser has updated loading progress
		MEDIA_EVENT_STATUS_TEXT_CHANGED,	// browser has updated the status text
		MEDIA_EVENT_NAME_CHANGED,			// browser has updated the name of the media (typically <title> tag)
		MEDIA_EVENT_LOCATION_CHANGED,		// browser location (URL) has changed (maybe due to internal navagation/frames/etc)
		MEDIA_EVENT_CLICK_LINK_HREF,		// I'm not entirely sure what the semantics of these two are
		MEDIA_EVENT_CLICK_LINK_NOFOLLOW,
		
		MEDIA_EVENT_PLUGIN_FAILED_LAUNCH,	// The plugin failed to launch 
		MEDIA_EVENT_PLUGIN_FAILED			// The plugin died unexpectedly
		
	} EMediaEvent;
	
	typedef enum
	{
		MEDIA_NONE,			// Uninitialized -- no useful state
		MEDIA_LOADING,		// loading or navigating
		MEDIA_LOADED,		// navigation/preroll complete
		MEDIA_ERROR,		// navigation/preroll failed
		MEDIA_PLAYING,		// playing (only for time-based media)
		MEDIA_PAUSED,		// paused (only for time-based media)
		MEDIA_DONE			// finished playing (only for time-based media)
	
	} EMediaStatus;
	
	virtual ~LLPluginClassMediaOwner() {};
	virtual void handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent /*event*/) {};
	virtual void handleCookieSet(LLPluginClassMedia* /*self*/, const std::string &/*cookie*/) {};
};

#endif // LL_LLPLUGINCLASSMEDIAOWNER_H
