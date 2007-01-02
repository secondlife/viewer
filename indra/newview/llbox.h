/** 
 * @file llbox.h
 * @brief Draws a box using display lists for speed.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLBOX_H
#define LL_LLBOX_H

#include "stdtypes.h"

class LLBox
{
protected:
//	GLuint	mDisplayList;
	F32		mVertex[8][3];
	U32		mTriangleCount;
public:
	void	prerender();
	void	cleanupGL();

	void	renderface(S32 which_face);
	void	render();

	U32		getTriangleCount()			{ return mTriangleCount; }
};

extern LLBox gBox;

#endif
