/** 
 * @file lltexture.h
 * @brief LLTexture definition
 *
 *	This class acts as a wrapper for OpenGL calls.
 *	The goal of this class is to minimize the number of api calls due to legacy rendering
 *	code, to define an interface for a multiple rendering API abstraction of the UI
 *	rendering, and to abstract out direct rendering calls in a way that is cleaner and easier to maintain.
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

#ifndef LL_TEXTURE_H
#define LL_TEXTURE_H

#include "llrefcount.h"
class LLImageGL ;
class LLTexUnit ;
class LLFontGL ;

//
//this is an abstract class as the parent for the class LLGLTexture
//
class LLTexture : public virtual LLRefCount
{
	friend class LLTexUnit ;
	friend class LLFontGL ;

protected:
	virtual ~LLTexture();

public:
	LLTexture(){}

	//
	//interfaces to access LLGLTexture
	//
	virtual S8         getType() const = 0 ;
	virtual void       setKnownDrawSize(S32 width, S32 height) = 0 ;
	virtual bool       bindDefaultImage(const S32 stage = 0) = 0 ;
	virtual void       forceImmediateUpdate() = 0 ;
	virtual void       setActive() = 0 ;
	virtual S32	       getWidth(S32 discard_level = -1) const = 0 ;
	virtual S32	       getHeight(S32 discard_level = -1) const = 0 ;

private:
	//note: do not make this function public.
	virtual LLImageGL* getGLTexture() const = 0 ;

	virtual void updateBindStatsForTester() = 0 ;
};
#endif

