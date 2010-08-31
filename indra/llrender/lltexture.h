/** 
 * @file lltexture.h
 * @brief LLTexture definition
 *
 *	This class acts as a wrapper for OpenGL calls.
 *	The goal of this class is to minimize the number of api calls due to legacy rendering
 *	code, to define an interface for a multiple rendering API abstraction of the UI
 *	rendering, and to abstract out direct rendering calls in a way that is cleaner and easier to maintain.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_TEXTURE_H
#define LL_TEXTURE_H

#include "llrefcount.h"
class LLImageGL ;
class LLTexUnit ;
class LLFontGL ;

//
//this is an abstract class as the parent for the class LLViewerTexture
//through the following virtual functions, the class LLViewerTexture can be reached from /llrender.
//
class LLTexture : public LLRefCount
{
	friend class LLTexUnit ;
	friend class LLFontGL ;

protected:
	virtual ~LLTexture();

public:
	LLTexture(){}

	//
	//interfaces to access LLViewerTexture
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

