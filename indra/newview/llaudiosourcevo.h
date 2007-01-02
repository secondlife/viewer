/** 
 * @file llaudiosourcevo.h
 * @author Douglas Soo, James Cook
 * @brief Audio sources attached to viewer objects
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLAUDIOSOURCEVO_H
#define LL_LLAUDIOSOURCEVO_H

#include "audioengine.h"
#include "llviewerobject.h"

class LLViewerObject;

class LLAudioSourceVO : public LLAudioSource
{
public:
	LLAudioSourceVO(const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain, LLViewerObject *objectp);
	virtual ~LLAudioSourceVO();
	/*virtual*/	void update();
	/*virtual*/ void setGain(const F32 gain);

private:
	void updateGain();

private:
	LLPointer<LLViewerObject>	mObjectp;
	F32							mActualGain; // The "real" gain, when not off due to parcel effects
};

#endif // LL_LLAUDIOSOURCEVO_H
