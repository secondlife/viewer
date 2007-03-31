/** 
 * @file lltool.h
 * @brief LLTool class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOL_H
#define LL_LLTOOL_H

#include "llkeyboard.h"
#include "llmousehandler.h"
#include "llcoord.h"
#include "v3math.h"
#include "v3dmath.h"

class LLViewerObject;
class LLToolComposite;
class LLView;
class LLPanel;

class LLTool
:	public LLMouseHandler
{
public:
	LLTool( const LLString& name, LLToolComposite* composite = NULL );
	virtual ~LLTool();

	// Hack to support LLFocusMgr
	virtual BOOL isView() { return FALSE; }

	// Virtual functions inherited from LLMouseHandler
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen);
		// Return FALSE to allow context menu to be shown.
	virtual void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
							{ *local_x = screen_x; *local_y = screen_y;	}
	virtual void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
							{ *screen_x = local_x; *screen_y = local_y;	}

	virtual const LLString& getName() const	{ return mName; }

	// New virtual functions
	virtual LLViewerObject*	getEditingObject()		{ return NULL; }
	virtual LLVector3d		getEditingPointGlobal()	{ return LLVector3d(); }
	virtual BOOL			isEditing()				{ return (getEditingObject() != NULL); }
	virtual void			stopEditing()			{}

	virtual BOOL			clipMouseWhenDown()		{ return TRUE; }

	virtual void			handleSelect()			{ }	// do stuff when your tool is selected
	virtual void			handleDeselect()		{ }	// clean up when your tool is deselected

	virtual LLTool*			getOverrideTool(MASK mask);

	// isAlwaysRendered() - return true if this is a tool that should
	// always be rendered regardless of selection.
	virtual BOOL isAlwaysRendered() { return FALSE; }

	virtual void			render() {}				// draw tool specific 3D content in world
	virtual void			draw();					// draw tool specific 2D overlay

	virtual BOOL			handleKey(KEY key, MASK mask);

	// Note: NOT virtual.  Subclasses should call this version.
	void					setMouseCapture(BOOL b);
	BOOL					hasMouseCapture();
	virtual void			onMouseCaptureLost() {}  // override this one as needed.

protected:
	LLToolComposite*	mComposite;  // Composite will handle mouse captures.
	LLString			mName;
	
public:
	static const LLString sNameNull;
};

#endif
