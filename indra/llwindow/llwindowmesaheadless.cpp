/** 
 * @file llwindowmesaheadless.cpp
 * @brief Platform-dependent implementation of llwindow
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#if LL_MESA_HEADLESS

#include "linden_common.h"
#include "indra_constants.h"

#include "llwindowmesaheadless.h"
#include "llgl.h"
#include "llglheaders.h"

#define MESA_CHANNEL_TYPE GL_UNSIGNED_SHORT
#define MESA_CHANNEL_SIZE 2

U16 *gMesaBuffer = NULL;

//
// LLWindowMesaHeadless
//
LLWindowMesaHeadless::LLWindowMesaHeadless(char *title, char *name, S32 x, S32 y, S32 width, S32 height,
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

#endif
