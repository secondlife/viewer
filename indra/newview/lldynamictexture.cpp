/** 
 * @file lldynamictexture.cpp
 * @brief Implementation of LLDynamicTexture class
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

#include "llviewerprecompiledheaders.h"

#include "lldynamictexture.h"

// Linden library includes
#include "llglheaders.h"
#include "llwindow.h"			// getPosition()

// Viewer includes
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerimage.h"
#include "llvertexbuffer.h"
#include "llviewerdisplay.h"
#include "llrender.h"

// static
LLDynamicTexture::instance_list_t LLDynamicTexture::sInstances[ LLDynamicTexture::ORDER_COUNT ];
S32 LLDynamicTexture::sNumRenders = 0;

//-----------------------------------------------------------------------------
// LLDynamicTexture()
//-----------------------------------------------------------------------------
LLDynamicTexture::LLDynamicTexture(S32 width, S32 height, S32 components, EOrder order, BOOL clamp) : 
	mWidth(width), 
	mHeight(height),
	mComponents(components),
	mTexture(NULL),
	mLastBindTime(0),
	mClamp(clamp)
{
	llassert((1 <= components) && (components <= 4));

	generateGLTexture();

	llassert( 0 <= order && order < ORDER_COUNT );
	LLDynamicTexture::sInstances[ order ].insert(this);
}

//-----------------------------------------------------------------------------
// LLDynamicTexture()
//-----------------------------------------------------------------------------
LLDynamicTexture::~LLDynamicTexture()
{
	releaseGLTexture();
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		LLDynamicTexture::sInstances[order].erase(this);  // will fail in all but one case.
	}
}

//-----------------------------------------------------------------------------
// releaseGLTexture()
//-----------------------------------------------------------------------------
void LLDynamicTexture::releaseGLTexture()
{
	if (mTexture.notNull())
	{
// 		llinfos << "RELEASING " << (mWidth*mHeight*mComponents)/1024 << "K" << llendl;
		mTexture = NULL;
	}
}

//-----------------------------------------------------------------------------
// generateGLTexture()
//-----------------------------------------------------------------------------
void LLDynamicTexture::generateGLTexture()
{
	generateGLTexture(-1, 0, 0, FALSE);
}

void LLDynamicTexture::generateGLTexture(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes)
{
	if (mComponents < 1 || mComponents > 4)
	{
		llerrs << "Bad number of components in dynamic texture: " << mComponents << llendl;
	}
	releaseGLTexture();
	LLPointer<LLImageRaw> raw_image = new LLImageRaw(mWidth, mHeight, mComponents);
	mTexture = new LLViewerImage(mWidth, mHeight, mComponents, FALSE);
	if (internal_format >= 0)
	{
		mTexture->setExplicitFormat(internal_format, primary_format, type_format, swap_bytes);
	}
// 	llinfos << "ALLOCATING " << (mWidth*mHeight*mComponents)/1024 << "K" << llendl;
	mTexture->createGLTexture(0, raw_image);
	mTexture->setAddressMode((mClamp) ? LLTexUnit::TAM_CLAMP : LLTexUnit::TAM_WRAP);
	mTexture->setGLTextureCreated(false);
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLDynamicTexture::render()
{
	return FALSE;
}

//-----------------------------------------------------------------------------
// preRender()
//-----------------------------------------------------------------------------
void LLDynamicTexture::preRender(BOOL clear_depth)
{
	{
		// force rendering to on-screen portion of frame buffer
		LLCoordScreen window_pos;
		gViewerWindow->getWindow()->getPosition( &window_pos );
		mOrigin.set(0, gViewerWindow->getWindowDisplayHeight() - mHeight);  // top left corner

		if (window_pos.mX < 0)
		{
			mOrigin.mX = -window_pos.mX;
		}
		if (window_pos.mY < 0)
		{
			mOrigin.mY += window_pos.mY;
			mOrigin.mY = llmax(mOrigin.mY, 0) ;
		}

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}
	// Set up camera
	mCamera.setOrigin(*LLViewerCamera::getInstance());
	mCamera.setAxes(*LLViewerCamera::getInstance());
	mCamera.setAspect(LLViewerCamera::getInstance()->getAspect());
	mCamera.setView(LLViewerCamera::getInstance()->getView());
	mCamera.setNear(LLViewerCamera::getInstance()->getNear());

	glViewport(mOrigin.mX, mOrigin.mY, mWidth, mHeight);
	if (clear_depth)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

//-----------------------------------------------------------------------------
// postRender()
//-----------------------------------------------------------------------------
void LLDynamicTexture::postRender(BOOL success)
{
	{
		if (success)
		{
			success = mTexture->setSubImageFromFrameBuffer(0, 0, mOrigin.mX, mOrigin.mY, mWidth, mHeight);
		}
	}

	// restore viewport
	gViewerWindow->setup2DViewport();

	// restore camera
	LLViewerCamera::getInstance()->setOrigin(mCamera);
	LLViewerCamera::getInstance()->setAxes(mCamera);
	LLViewerCamera::getInstance()->setAspect(mCamera.getAspect());
	LLViewerCamera::getInstance()->setView(mCamera.getView());
	LLViewerCamera::getInstance()->setNear(mCamera.getNear());
}

//-----------------------------------------------------------------------------
// static
// updateDynamicTextures()
// Calls update on each dynamic texture.  Calls each group in order: "first," then "middle," then "last."
//-----------------------------------------------------------------------------
BOOL LLDynamicTexture::updateAllInstances()
{
	sNumRenders = 0;
	if (gGLManager.mIsDisabled)
	{
		return TRUE;
	}

	BOOL result = FALSE;
	BOOL ret = FALSE ;
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLDynamicTexture::sInstances[order].begin();
			 iter != LLDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLDynamicTexture *dynamicTexture = *iter;
			if (dynamicTexture->needsRender())
			{
				glClear(GL_DEPTH_BUFFER_BIT);
				gDepthDirty = TRUE;
				
				
				gGL.color4f(1,1,1,1);
				dynamicTexture->preRender();	// Must be called outside of startRender()
				result = FALSE;
				if (dynamicTexture->render())
				{
					ret = TRUE ;
					result = TRUE;
					sNumRenders++;
				}
				gGL.flush();
				LLVertexBuffer::unbind();
				
				dynamicTexture->postRender(result);
			}
		}
	}

	return ret;
}

//virtual
void LLDynamicTexture::restoreGLTexture() 
{
	generateGLTexture() ;
}

//virtual
void LLDynamicTexture::destroyGLTexture() 
{
	releaseGLTexture() ;
}

//-----------------------------------------------------------------------------
// static
// destroyGL()
//-----------------------------------------------------------------------------
void LLDynamicTexture::destroyGL()
{
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLDynamicTexture::sInstances[order].begin();
			 iter != LLDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLDynamicTexture *dynamicTexture = *iter;
			dynamicTexture->destroyGLTexture() ;
		}
	}
}

//-----------------------------------------------------------------------------
// static
// restoreGL()
//-----------------------------------------------------------------------------
void LLDynamicTexture::restoreGL()
{
	if (gGLManager.mIsDisabled)
	{
		return ;
	}			
	
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLDynamicTexture::sInstances[order].begin();
			 iter != LLDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLDynamicTexture *dynamicTexture = *iter;
			dynamicTexture->restoreGLTexture() ;
		}
	}
}
