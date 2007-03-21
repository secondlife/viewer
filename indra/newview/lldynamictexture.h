/** 
 * @file lldynamictexture.h
 * @brief Implementation of LLDynamicTexture class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDYNAMICTEXTURE_H
#define LL_LLDYNAMICTEXTURE_H

#include "llgl.h"
#include "linked_lists.h"
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
	virtual void bindTexture();
	virtual void unbindTexture();

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
	static LLLinkedList<LLDynamicTexture> sInstances[ LLDynamicTexture::ORDER_COUNT ];
	static S32 sNumRenders;
};

#endif
