/** 
 * @file lltool.h
 * @brief LLTool class header file
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
:	public LLMouseHandler, public LLThreadSafeRefCount
{
public:
	LLTool( const std::string& name, LLToolComposite* composite = NULL );
	virtual ~LLTool();

	// Hack to support LLFocusMgr
	virtual bool isView() const { return false; }

	// Virtual functions inherited from LLMouseHandler
	virtual bool	handleAnyMouseClick(S32 x, S32 y, MASK mask, EMouseClickType clicktype, bool down);
	virtual bool	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual bool	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual bool	handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	virtual bool	handleMiddleMouseUp(S32 x, S32 y, MASK mask);

	virtual bool	handleHover(S32 x, S32 y, MASK mask);
	virtual bool	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual bool	handleScrollHWheel(S32 x, S32 y, S32 clicks);
	virtual bool	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual bool	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual bool	handleRightMouseUp(S32 x, S32 y, MASK mask);
	virtual bool	handleToolTip(S32 x, S32 y, MASK mask);

		// Return false to allow context menu to be shown.
	virtual void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
							{ *local_x = screen_x; *local_y = screen_y;	}
	virtual void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
							{ *screen_x = local_x; *screen_y = local_y;	}

	virtual const std::string& getName() const	{ return mName; }

	// New virtual functions
	virtual LLViewerObject*	getEditingObject()		{ return NULL; }
	virtual LLVector3d		getEditingPointGlobal()	{ return LLVector3d(); }
	virtual bool			isEditing()				{ return (getEditingObject() != NULL); }
	virtual void			stopEditing()			{}

	virtual bool			clipMouseWhenDown()		{ return true; }

	virtual void			handleSelect()			{ }	// do stuff when your tool is selected
	virtual void			handleDeselect()		{ }	// clean up when your tool is deselected

	virtual LLTool*			getOverrideTool(MASK mask);

	// isAlwaysRendered() - return true if this is a tool that should
	// always be rendered regardless of selection.
	virtual bool isAlwaysRendered() { return false; }

	virtual void			render() {}				// draw tool specific 3D content in world
	virtual void			draw();					// draw tool specific 2D overlay

	virtual bool			handleKey(KEY key, MASK mask);

	// Note: NOT virtual.  Subclasses should call this version.
	void					setMouseCapture(bool b);
	bool					hasMouseCapture();
	virtual void			onMouseCaptureLost() {}  // override this one as needed.

protected:
	LLToolComposite*	mComposite;  // Composite will handle mouse captures.
	std::string			mName;
	
public:
	static const std::string sNameNull;
};

#endif
