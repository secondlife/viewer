/**
 * @file llviewerparcelmediaautoplay.h
 * @author Karl Stiefvater
 * @brief timer to automatically play media in a parcel
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LLVIEWERPARCELMEDIAAUTOPLAY_H
#define LLVIEWERPARCELMEDIAAUTOPLAY_H

#include "llmediabase.h"
#include "lltimer.h"

// timer to automatically play media
class LLViewerParcelMediaAutoPlay : LLEventTimer
{
 public:
	LLViewerParcelMediaAutoPlay();
	virtual BOOL tick();
	static void initClass();
	static void cleanupClass();
	static void playStarted();
	
 private:
	S32 mLastParcelID;
	BOOL mPlayed;
	F32 mTimeInParcel;
};


#endif // LLVIEWERPARCELMEDIAAUTOPLAY_H
