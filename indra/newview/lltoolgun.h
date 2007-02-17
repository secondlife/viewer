/** 
 * @file lltoolgun.h
 * @brief LLToolGun class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TOOLGUN_H
#define LL_TOOLGUN_H

#include "lltool.h"
#include "llviewerimage.h"


class LLToolGun : public LLTool
{
public:
	LLToolGun( LLToolComposite* composite=NULL );

	virtual void	draw();

	virtual void	handleSelect();
	virtual void	handleDeselect();

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);

	virtual LLTool*	getOverrideTool(MASK mask) { return NULL; }
	virtual BOOL	clipMouseWhenDown()		{ return FALSE; }

private:
	LLPointer<LLViewerImage>	mCrosshairImg;
};

#endif
