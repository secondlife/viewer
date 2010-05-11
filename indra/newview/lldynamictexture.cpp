/** 
 * @file lldynamictexture.cpp
 * @brief Implementation of LLViewerDynamicTexture class
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
#include "llviewertexture.h"
#include "llvertexbuffer.h"
#include "llviewerdisplay.h"
#include "llrender.h"

// static
LLViewerDynamicTexture::instance_list_t LLViewerDynamicTexture::sInstances[ LLViewerDynamicTexture::ORDER_COUNT ];
S32 LLViewerDynamicTexture::sNumRenders = 0;

//-----------------------------------------------------------------------------
// LLViewerDynamicTexture()
//-----------------------------------------------------------------------------
LLViewerDynamicTexture::LLViewerDynamicTexture(S32 width, S32 height, S32 components, EOrder order, BOOL clamp) : 
	LLViewerTexture(width, height, components, FALSE),
	mClamp(clamp)
{
	llassert((1 <= components) && (components <= 4));

	generateGLTexture();

	llassert( 0 <= order && order < ORDER_COUNT );
	LLViewerDynamicTexture::sInstances[ order ].insert(this);
}

//-----------------------------------------------------------------------------
// LLViewerDynamicTexture()
//-----------------------------------------------------------------------------
LLViewerDynamicTexture::~LLViewerDynamicTexture()
{
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		LLViewerDynamicTexture::sInstances[order].erase(this);  // will fail in all but one case.
	}
}

//virtual 
S8 LLViewerDynamicTexture::getType() const
{
	return LLViewerTexture::DYNAMIC_TEXTURE ;
}

//-----------------------------------------------------------------------------
// generateGLTexture()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::generateGLTexture()
{
	LLViewerTexture::generateGLTexture() ;
	generateGLTexture(-1, 0, 0, FALSE);
}

void LLViewerDynamicTexture::generateGLTexture(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes)
{
	if (mComponents < 1 || mComponents > 4)
	{
		llerrs << "Bad number of components in dynamic texture: " << mComponents << llendl;
	}
	
	LLPointer<LLImageRaw> raw_image = new LLImageRaw(mFullWidth, mFullHeight, mComponents);
	if (internal_format >= 0)
	{
		setExplicitFormat(internal_format, primary_format, type_format, swap_bytes);
	}
	createGLTexture(0, raw_image, 0, TRUE, LLViewerTexture::DYNAMIC_TEX);
	setAddressMode((mClamp) ? LLTexUnit::TAM_CLAMP : LLTexUnit::TAM_WRAP);
	mGLTexturep->setGLTextureCreated(false);
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLViewerDynamicTexture::render()
{
	return FALSE;
}

//-----------------------------------------------------------------------------
// preRender()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::preRender(BOOL clear_depth)
{
	{
		// force rendering to on-screen portion of frame buffer
		LLCoordScreen window_pos;
		gViewerWindow->getWindow()->getPosition( &window_pos );
		mOrigin.set(0, gViewerWindow->getWindowHeightRaw() - mFullHeight);  // top left corner

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
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	mCamera.setOrigin(*camera);
	mCamera.setAxes(*camera);
	mCamera.setAspect(camera->getAspect());
	mCamera.setView(camera->getView());
	mCamera.setNear(camera->getNear());

	glViewport(mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight);
	if (clear_depth)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

//-----------------------------------------------------------------------------
// postRender()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::postRender(BOOL success)
{
	{
		if (success)
		{
			if(mGLTexturep.isNull())
			{
				generateGLTexture() ;
			}
			llcallstacks << "class type: " << (S32)getType() << llcallstacksendl ;

			success = mGLTexturep->setSubImageFromFrameBuffer(0, 0, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight);
		}
	}

	// restore viewport
	gViewerWindow->setup2DViewport();

	// restore camera
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	camera->setOrigin(mCamera);
	camera->setAxes(mCamera);
	camera->setAspect(mCamera.getAspect());
	camera->setView(mCamera.getView());
	camera->setNear(mCamera.getNear());
}

//-----------------------------------------------------------------------------
// static
// updateDynamicTextures()
// Calls update on each dynamic texture.  Calls each group in order: "first," then "middle," then "last."
//-----------------------------------------------------------------------------
BOOL LLViewerDynamicTexture::updateAllInstances()
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
		for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
			 iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLViewerDynamicTexture *dynamicTexture = *iter;
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

//-----------------------------------------------------------------------------
// static
// destroyGL()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::destroyGL()
{
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
			 iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLViewerDynamicTexture *dynamicTexture = *iter;
			dynamicTexture->destroyGLTexture() ;
		}
	}
}

//-----------------------------------------------------------------------------
// static
// restoreGL()
//-----------------------------------------------------------------------------
void LLViewerDynamicTexture::restoreGL()
{
	if (gGLManager.mIsDisabled)
	{
		return ;
	}			
	
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
			 iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLViewerDynamicTexture *dynamicTexture = *iter;
			dynamicTexture->restoreGLTexture() ;
		}
	}
}
