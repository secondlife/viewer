/** 
 * @file lldynamictexture.h
 * @brief Implementation of LLDynamicTexture class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llgl.h"
#include "llcamera.h"
#include "llcoord.h"
#include "llimagegl.h"

class LLDynamicTexture
{
public:
	enum EOrder { ORDER_FIRST = 0, ORDER_MIDDLE = 1, ORDER_LAST = 2, ORDER_RESET = 3, ORDER_COUNT = 4 };

	LLDynamicTexture(S32 width,
					 S32 height,
					 S32 components,		// = 4,
					 EOrder order,			// = ORDER_MIDDLE,
					 BOOL clamp);
	virtual ~LLDynamicTexture();

	S32			getOriginX()	{ return mOrigin.mX; }
	S32			getOriginY()	{ return mOrigin.mY; }
	S32			getWidth()		{ return mWidth; }
	S32			getHeight()		{ return mHeight; }
	S32			getComponents()	{ return mComponents; }
	S32			getSize()		{ return mWidth * mHeight * mComponents; }

	virtual BOOL needsRender() { return TRUE; }
	virtual void preRender(BOOL clear_depth = TRUE);
	virtual BOOL render();
	virtual void postRender(BOOL success);

	LLImageGL* getTexture(void) const { return mTexture; }

	static BOOL	updateAllInstances();

	static void destroyGL();
	static void restoreGL();

protected:
	void releaseGLTexture();
	void generateGLTexture();
	void generateGLTexture(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes = FALSE);

protected:
	S32 mWidth;
	S32 mHeight;
	S32 mComponents;
	LLPointer<LLImageGL> mTexture;
	F32 mLastBindTime;
	BOOL mClamp;
	LLCoordGL mOrigin;

	LLCamera mCamera;
	typedef std::set<LLDynamicTexture*> instance_list_t;
	static instance_list_t sInstances[ LLDynamicTexture::ORDER_COUNT ];
	static S32 sNumRenders;
};

#endif
