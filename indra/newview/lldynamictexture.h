/** 
 * @file lldynamictexture.h
 * @brief Implementation of LLViewerDynamicTexture class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
