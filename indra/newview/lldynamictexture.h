/** 
 * @file lldynamictexture.h
 * @brief Implementation of LLViewerDynamicTexture class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLDYNAMICTEXTURE_H
#define LL_LLDYNAMICTEXTURE_H

#include "llcamera.h"
#include "llgl.h"
#include "llcoord.h"
#include "llviewertexture.h"
#include "llcamera.h"

class LLViewerDynamicTexture : public LLViewerTexture
{
public:
	enum
	{
		LL_VIEWER_DYNAMIC_TEXTURE = LLViewerTexture::DYNAMIC_TEXTURE,
		LL_TEX_LAYER_SET_BUFFER = LLViewerTexture::INVALID_TEXTURE_TYPE + 1,
		LL_VISUAL_PARAM_HINT,
		LL_VISUAL_PARAM_RESET,
		LL_PREVIEW_ANIMATION,
		LL_IMAGE_PREVIEW_SCULPTED,
		LL_IMAGE_PREVIEW_AVATAR,
		INVALID_DYNAMIC_TEXTURE
	};

protected:
	/*virtual*/ ~LLViewerDynamicTexture();

public:
	enum EOrder { ORDER_FIRST = 0, ORDER_MIDDLE = 1, ORDER_LAST = 2, ORDER_RESET = 3, ORDER_COUNT = 4 };

	LLViewerDynamicTexture(S32 width,
					 S32 height,
					 S32 components,		// = 4,
					 EOrder order,			// = ORDER_MIDDLE,
					 BOOL clamp);
	
	/*virtual*/ S8 getType() const ;

	S32			getOriginX()	{ return mOrigin.mX; }
	S32			getOriginY()	{ return mOrigin.mY; }
	
	S32			getSize()		{ return mFullWidth * mFullHeight * mComponents; }

	virtual BOOL needsRender() { return TRUE; }
	virtual void preRender(BOOL clear_depth = TRUE);
	virtual BOOL render();
	virtual void postRender(BOOL success);

	virtual void restoreGLTexture() {}
	virtual void destroyGLTexture() {}

	static BOOL	updateAllInstances();
	static void destroyGL() ;
	static void restoreGL() ;
protected:
	void generateGLTexture();
	void generateGLTexture(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes = FALSE);

protected:
	BOOL mClamp;
	LLCoordGL mOrigin;
	LLCamera mCamera;
	
	typedef std::set<LLViewerDynamicTexture*> instance_list_t;
	static instance_list_t sInstances[ LLViewerDynamicTexture::ORDER_COUNT ];
	static S32 sNumRenders;
};

#endif
