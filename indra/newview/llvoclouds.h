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


class LLVOClouds : public LLAlphaObject
{
public:
	LLVOClouds(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp );

	// Initialize data that's only inited once per class.
	static void initClass();

	void updateDrawable(BOOL force_damped); 

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		getGeometry(S32 te, 
							LLStrider<LLVector3>& verticesp, 
							LLStrider<LLVector3>& normalsp, 
							LLStrider<LLVector2>& texcoordsp, 
							LLStrider<LLColor4U>& colorsp, 
							LLStrider<U32>& indicesp);

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	BOOL isParticle();
	F32 getPartSize(S32 idx);

	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area
	
	void updateFaceSize(S32 idx) { }
	BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	virtual U32 getPartitionType() const;

	void setCloudGroup(LLCloudGroup *cgp)		{ mCloudGroupp = cgp; }
protected:
	virtual ~LLVOClouds();

	LLCloudGroup *mCloudGroupp;
};

extern LLUUID gCloudTextureID;

#endif // LL_VO_CLOUDS_H
