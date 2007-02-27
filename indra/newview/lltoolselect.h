/** 
 * @file lltoolselect.h
 * @brief LLToolSelect class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TOOLSELECT_H
#define LL_TOOLSELECT_H

#include "lltool.h"
#include "v3math.h"
#include "lluuid.h"

class LLObjectSelection;

class LLToolSelect : public LLTool
{
public:
	LLToolSelect( LLToolComposite* composite );

	virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);

	virtual void		stopEditing();

	static LLHandle<LLObjectSelection>	handleObjectSelection(LLViewerObject *object, MASK mask, BOOL ignore_group, BOOL temp_select);

	virtual void		onMouseCaptureLost();
	virtual void		handleDeselect();

protected:
	BOOL				mIgnoreGroup;
	LLUUID				mSelectObjectID;
};

extern LLToolSelect *gToolSelect;

#endif  // LL_TOOLSELECTION_H
