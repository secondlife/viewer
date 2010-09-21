/**
 * @file llviewerparcelmedia.h
 * @brief Handlers for multimedia on a per-parcel basis
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
