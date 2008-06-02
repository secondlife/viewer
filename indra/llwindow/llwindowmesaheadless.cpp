/** 
 * @file llwindowmesaheadless.cpp
 * @brief Platform-dependent implementation of llwindow
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "linden_common.h"
#include "indra_constants.h"

#include "llwindowmesaheadless.h"
#include "llgl.h"

#define MESA_CHANNEL_TYPE GL_UNSIGNED_SHORT
#define MESA_CHANNEL_SIZE 2

U16 *gMesaBuffer = NULL;

//
// LLWindowMesaHeadless
//
LLWindowMesaHeadless::LLWindowMesaHeadless(const char *title, const char *name, S32 x, S32 y, S32 width, S32 height,
							 U32 flags,  BOOL fullscreen, BOOL clearBg,
							 BOOL disable_vsync, BOOL use_gl, BOOL ignore_pixel_depth)
	: LLWindow(fullscreen, flags)
{
	if (use_gl)
	{
		llinfos << "MESA Init" << llendl;
		mMesaContext = OSMesaCreateContextExt( GL_RGBA, 32, 0, 0, NULL );

		/* Allocate the image buffer */
		mMesaBuffer = new unsigned char [width * height * 4 * MESA_CHANNEL_SIZE];
		llassert(mMesaBuffer);

		gMesaBuffer = (U16*)mMesaBuffer;

		/* Bind the buffer to the context and make it current */
		if (!OSMesaMakeCurrent( mMesaContext, mMesaBuffer, MESA_CHANNEL_TYPE, width, height ))
		{
			llerrs << "MESA: OSMesaMakeCurrent failed!" << llendl;
		}

		llverify(gGLManager.initGL());
	}
}


LLWindowMesaHeadless::~LLWindowMesaHeadless()
{
	delete mMesaBuffer;
	OSMesaDestroyContext( mMesaContext );
}

void LLWindowMesaHeadless::swapBuffers()
{
	glFinish();
}
