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

#include "llgltypes.h"
#include "llrefcount.h"
#include "llrender.h"

class LLFontGL ;
class LLImageRaw ;

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

	enum EBoostLevel
	{
		BOOST_NONE 			= 0,
		BOOST_AVATAR_BAKED	,
		BOOST_AVATAR		,
		BOOST_CLOUDS		,
		BOOST_SCULPTED      ,
		
		BOOST_HIGH 			= 10,
		BOOST_BUMP          ,
		BOOST_TERRAIN		, // has to be high priority for minimap / low detail
		BOOST_SELECTED		,		
		BOOST_AVATAR_BAKED_SELF	,
		BOOST_AVATAR_SELF	, // needed for baking avatar
		BOOST_SUPER_HIGH    , //textures higher than this need to be downloaded at the required resolution without delay.
		BOOST_HUD			,
		BOOST_ICON			,
		BOOST_UI			,
		BOOST_PREVIEW		,
		BOOST_MAP			,
		BOOST_MAP_VISIBLE	,		
		BOOST_MAX_LEVEL,

		//other texture Categories
		LOCAL = BOOST_MAX_LEVEL,
		AVATAR_SCRATCH_TEX,
		DYNAMIC_TEX,
		MEDIA,
		ATLAS,
		OTHER,
		MAX_GL_IMAGE_CATEGORY
	};

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
	virtual BOOL       hasGLTexture() const  = 0;
	virtual BOOL       createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename = 0, BOOL to_create = TRUE, S32 category = LLTexture::OTHER) = 0;
	virtual void       setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, BOOL swap_bytes = FALSE) = 0;
	virtual void       setAddressMode(LLTexUnit::eTextureAddressMode mode) = 0;
	virtual LLTexUnit::eTextureAddressMode getAddressMode(void) const = 0;
	virtual S8         getComponents() const = 0;
	virtual            const LLUUID& getID() const = 0;

private:
	//note: do not make this function public.
	virtual LLImageGL* getGLTexture() const = 0 ;

	virtual void updateBindStatsForTester() = 0 ;
};
#endif

