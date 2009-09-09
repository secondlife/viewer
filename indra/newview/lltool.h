/** 
 * @file lltool.h
 * @brief LLTool class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
:	public LLMouseHandler
{
public:
	LLTool( const std::string& name, LLToolComposite* composite = NULL );
	virtual ~LLTool();

	// Hack to support LLFocusMgr
	virtual BOOL isView() const { return FALSE; }

	// Virtual functions inherited from LLMouseHandler
	virtual BOOL	handleAnyMouseClick(S32 x, S32 y, MASK mask, LLMouseHandler::EClickType clicktype, BOOL down);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMiddleMouseUp(S32 x, S32 y, MASK mask);

	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect& sticky_rect_screen);

		// Return FALSE to allow context menu to be shown.
	virtual void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
							{ *local_x = screen_x; *local_y = screen_y;	}
	virtual void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
							{ *screen_x = local_x; *screen_y = local_y;	}

	virtual const std::string& getName() const	{ return mName; }

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
	std::string			mName;
	
public:
	static const std::string sNameNull;
};

#endif
