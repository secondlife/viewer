/** 
 * @file llvoclouds.h
 * @brief Description of LLVOClouds class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOCLOUDS_H
#define LL_LLVOCLOUDS_H

#include "llviewerobject.h"
#include "lltable.h"
#include "v4coloru.h"

class LLViewerImage;
class LLViewerCloudGroup;

class LLCloudGroup;


class LLVOClouds : public LLViewerObject
{
public:
	LLVOClouds(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp );
	virtual ~LLVOClouds();

	// Initialize data that's only inited once per class.
	static void initClass();

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.

	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	void setCloudGroup(LLCloudGroup *cgp)		{ mCloudGroupp = cgp; }
protected:
	LLCloudGroup *mCloudGroupp;
};

extern LLUUID gCloudTextureID;

#endif // LL_VO_CLOUDS_H
