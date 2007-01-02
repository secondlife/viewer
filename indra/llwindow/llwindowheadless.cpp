/** 
 * @file llwindowheadless.cpp
 * @brief Headless implementation of LLWindow class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "indra_constants.h"

#include "llwindowheadless.h"

//
// LLWindowHeadless
//
LLWindowHeadless::LLWindowHeadless(char *title, char *name, S32 x, S32 y, S32 width, S32 height,
							 U32 flags,  BOOL fullscreen, BOOL clearBg,
							 BOOL disable_vsync, BOOL use_gl, BOOL ignore_pixel_depth)
	: LLWindow(fullscreen, flags)
{
}


LLWindowHeadless::~LLWindowHeadless()
{
}

void LLWindowHeadless::swapBuffers()
{
}

