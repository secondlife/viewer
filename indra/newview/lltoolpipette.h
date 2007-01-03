/** 
 * @file lltoolpipette.h
 * @brief LLToolPipette class header file
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// A tool to pick texture entry infro from objects in world (color/texture)
// This tool assumes it is transient in the codebase and must be used
// accordingly. We should probably restructure the way tools are
// managed so that this is handled automatically.

#ifndef LL_LLTOOLPIPETTE_H
#define LL_LLTOOLPIPETTE_H

#include "lltool.h"
#include "lltextureentry.h"

class LLViewerObject;

class LLToolPipette
:	public LLTool
{
public:
	LLToolPipette();
	virtual ~LLToolPipette();

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect *sticky_rect_screen);

	typedef void (*select_callback)(const LLTextureEntry& te, void *data);
	void setSelectCallback(select_callback callback, void* user_data);
	void setResult(BOOL success, const LLString& msg);

	static void pickCallback(S32 x, S32 y, MASK mask);

protected:
	LLTextureEntry	mTextureEntry;
	select_callback mSelectCallback;
	BOOL			mSuccess;
	LLString		mTooltipMsg;
	void*			mUserData;
};

extern LLToolPipette *gToolPipette;

#endif //LL_LLTOOLPIPETTE_H
