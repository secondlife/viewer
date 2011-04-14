/** 
 * @file llpluginclassmediaowner.h
 * @brief LLPluginClassMedia handles interaction with a plugin which knows about the "media" message class.
 *
 * @cond
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
		MEDIA_EVENT_NAVIGATE_ERROR_PAGE,	// browser navigated to a page that resulted in an HTTP error
		MEDIA_EVENT_CLICK_LINK_HREF,		// I'm not entirely sure what the semantics of these two are
		MEDIA_EVENT_CLICK_LINK_NOFOLLOW,
		MEDIA_EVENT_CLOSE_REQUEST,			// The plugin requested its window be closed (currently hooked up to javascript window.close in webkit)
		MEDIA_EVENT_PICK_FILE_REQUEST,		// The plugin wants the user to pick a file
		MEDIA_EVENT_GEOMETRY_CHANGE,		// The plugin requested its window geometry be changed (per the javascript window interface)
	
		MEDIA_EVENT_PLUGIN_FAILED_LAUNCH,	// The plugin failed to launch 
		MEDIA_EVENT_PLUGIN_FAILED,			// The plugin died unexpectedly

		MEDIA_EVENT_AUTH_REQUEST,			// The plugin wants to display an auth dialog

		MEDIA_EVENT_LINK_HOVERED			// Got a "link hovered" event from the plugin
		
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
