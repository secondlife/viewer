/** 
 * @file llviewerparcelmedia.h
 * @author Callum Prentice & James Cook
 * @brief Handlers for multimedia on a per-parcel basis
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LLVIEWERPARCELMEDIA_H
#define LLVIEWERPARCELMEDIA_H

#include "llmediabase.h"

class LLMessageSystem;
class LLParcel;

// This class understands land parcels, network traffic, LSL media
// transport commands, and talks to the LLViewerMedia class to actually
// do playback.  It allows us to remove code from LLViewerParcelMgr.
class LLViewerParcelMedia
{
	public:
		static void initClass();

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

		static void seek(F32 time);
		    // jump to timecode time

		static LLMediaBase::EStatus getStatus();

		static void processParcelMediaCommandMessage( LLMessageSystem *msg, void ** );
		static void processParcelMediaUpdate( LLMessageSystem *msg, void ** );

	public:
		static S32 sMediaParcelLocalID;
		static LLUUID sMediaRegionID;
};

#endif
