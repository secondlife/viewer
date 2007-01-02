/** 
 * @file lltoolface.h
 * @brief A tool to select object faces
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOLFACE_H
#define LL_LLTOOLFACE_H

#include "lltool.h"

class LLViewerObject;

class LLToolFace
:	public LLTool
{
public:
	LLToolFace();
	virtual ~LLToolFace();

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual void	handleSelect();
	virtual void	handleDeselect();
	virtual void	render();			// draw face highlights

	static void pickCallback(S32 x, S32 y, MASK mask);
};

extern LLToolFace *gToolFace;

#endif
