/** 
 * @file lltoolobjpicker.h
 * @brief LLToolObjPicker class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TOOLOBJPICKER_H
#define LL_TOOLOBJPICKER_H

#include "lltool.h"
#include "v3math.h"
#include "lluuid.h"

class LLToolObjPicker : public LLTool
{
public:
	LLToolObjPicker();

	virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL		handleHover(S32 x, S32 y, MASK mask);

	virtual void 		handleSelect();
	virtual void 		handleDeselect();

	virtual void		onMouseCaptureLost();

	virtual void 		setExitCallback(void (*callback)(void *), void *callback_data);

	LLUUID				getObjectID() const { return mHitObjectID; }

	static void			pickCallback(S32 x, S32 y, MASK mask);

protected:
	BOOL				mPicked;
	LLUUID				mHitObjectID;
	void 				(*mExitCallback)(void *callback_data);
	void 				*mExitCallbackData;
};

extern LLToolObjPicker* gToolObjPicker;

#endif  
