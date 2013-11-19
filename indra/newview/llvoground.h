/** 
 * @file llvoground.h
 * @brief LLVOGround class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLVOGROUND_H
#define LL_LLVOGROUND_H

#include "stdtypes.h"
#include "v3color.h"
#include "v4coloru.h"
#include "llviewertexture.h"
#include "llviewerobject.h"

class LLVOGround : public LLStaticViewerObject
{
protected:
	~LLVOGround();

public:
	LLVOGround(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ void idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	
	// Graphical stuff for objects - maybe broken out into render class
	// later?
	/*virtual*/ void updateTextures();
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);

	void cleanupGL();
};

#endif // LL_LLVOGROUND_H
