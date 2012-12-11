/** 
 * @file llwindowmesaheadless.cpp
 * @brief Platform-dependent implementation of llwindow
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
LLWindowMesaHeadless::LLWindowMesaHeadless(LLWindowCallbacks* callbacks,
                                           const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height,
							 U32 flags,  BOOL fullscreen, BOOL clearBg,
							 BOOL disable_vsync, BOOL ignore_pixel_depth)
	: LLWindow(callbacks, fullscreen, flags)
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


LLWindowMesaHeadless::~LLWindowMesaHeadless()
{
	delete mMesaBuffer;
	OSMesaDestroyContext( mMesaContext );
}

void LLWindowMesaHeadless::swapBuffers()
{
	glFinish();
}
