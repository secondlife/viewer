/** 
 * @file llvoground.h
 * @brief LLVOGround class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOGROUND_H
#define LL_LLVOGROUND_H

#include "stdtypes.h"
#include "v3color.h"
#include "v4coloru.h"
#include "llviewerimage.h"
#include "llviewerobject.h"

class LLVOGround : public LLStaticViewerObject
{
protected:
	~LLVOGround();

public:
	LLVOGround(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	
	// Graphical stuff for objects - maybe broken out into render class
	// later?
	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);

	void cleanupGL();
};

#endif // LL_LLVOGROUND_H
