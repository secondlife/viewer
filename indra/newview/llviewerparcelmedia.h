/**
 * @file llviewerparcelmedia.h
 * @brief Handlers for multimedia on a per-parcel basis
 *
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
 */

#ifndef LLVIEWERPARCELMEDIA_H
#define LLVIEWERPARCELMEDIA_H

#include "llviewermedia.h"

class LLMessageSystem;
class LLParcel;
class LLViewerParcelMediaNavigationObserver;


// This class understands land parcels, network traffic, LSL media
// transport commands, and talks to the LLViewerMedia class to actually
// do playback.  It allows us to remove code from LLViewerParcelMgr.
class LLViewerParcelMedia : public LLViewerMediaObserver
{
	LOG_CLASS(LLViewerParcelMedia);
	public:
		static void initClass();
		static void cleanupClass();

		static void update(LLParcel* parcel);
			// called when the agent's parcel has a new URL, or the agent has
			// walked on to a new parcel with media

		static void play(LLParcel* parcel);
			// user clicked play button in media transport controls

		static void stop();
			// user clicked stop button in media transport controls

		static void pause();
		static void start();
			// restart after pause - no need for all the setup

		static void focus(bool focus);

		static void seek(F32 time);
		    // jump to timecode time

		static LLPluginClassMediaOwner::EMediaStatus getStatus();
		static std::string getMimeType();
		static std::string getURL();
		static std::string getName();
		static viewer_media_t getParcelMedia();

		static void processParcelMediaCommandMessage( LLMessageSystem *msg, void ** );
		static void processParcelMediaUpdate( LLMessageSystem *msg, void ** );
		static void sendMediaNavigateMessage(const std::string& url);
		
		// inherited from LLViewerMediaObserver
		virtual void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	public:
		static S32 sMediaParcelLocalID;
		static LLUUID sMediaRegionID;
		// HACK: this will change with Media on a Prim
		static viewer_media_t sMediaImpl;
};


class LLViewerParcelMediaNavigationObserver
{
public:
	std::string mCurrentURL;
	bool mFromMessage;

	// void onNavigateComplete( const EventType& event_in );

};

#endif
