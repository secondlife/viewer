/** 
 * @file lltoolmgr.h
 * @brief LLToolMgr class header file
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

#ifndef LL_TOOLMGR_H
#define LL_TOOLMGR_H

#include "llkeyboard.h"

class LLTool;
class LLToolset;

// Key bindings for common operations
const MASK MASK_VERTICAL		= MASK_CONTROL;
const MASK MASK_SPIN			= MASK_CONTROL | MASK_SHIFT;
const MASK MASK_ZOOM			= MASK_NONE;
const MASK MASK_ORBIT			= MASK_CONTROL;
const MASK MASK_PAN				= MASK_CONTROL | MASK_SHIFT;
const MASK MASK_COPY			= MASK_SHIFT;

class LLToolMgr : public LLSingleton<LLToolMgr>
{
public:
	LLToolMgr();
	~LLToolMgr();

	// Must be called after gSavedSettings set up.
	void			initTools();

	LLTool*			getCurrentTool(); // returns active tool, taking into account keyboard state
	LLTool*			getBaseTool(); // returns active tool when overrides are deactivated

	bool			inEdit();
	bool			canEdit();
	void			toggleBuildMode(const LLSD& sdname);
	
	/* Determines if we are in Build mode or not. */
	bool			inBuildMode();

	void			setTransientTool(LLTool* tool);
	void			clearTransientTool();
	BOOL			usingTransientTool();

	void			setCurrentToolset(LLToolset* current);
	LLToolset*		getCurrentToolset();

	void			onAppFocusGained();
	void			onAppFocusLost();

	void            clearSavedTool();

protected:
	friend class LLToolset;  // to allow access to setCurrentTool();
	void			setCurrentTool(LLTool* tool);
	void			updateToolStatus();

protected:
	LLTool*			mBaseTool;
	LLTool*			mSavedTool;		// The current tool at the time application focus was lost.
	LLTool*			mTransientTool;
	LLTool*			mOverrideTool; // Tool triggered by keyboard override
	LLTool*			mSelectedTool; // last known active tool
	LLToolset*		mCurrentToolset;
};

// Sets of tools for various modes
class LLToolset
{
public:
	LLToolset() : mSelectedTool(NULL), mIsShowFloaterTools(true) {}

	LLTool*			getSelectedTool()				{ return mSelectedTool; }

	void			addTool(LLTool* tool);

	void			selectTool( LLTool* tool );
	void			selectToolByIndex( S32 index );
	void			selectFirstTool();
	void			selectNextTool();
	void			selectPrevTool();

	void			handleScrollWheel(S32 clicks);

	BOOL			isToolSelected( S32 index );

	void            setShowFloaterTools(bool pShowFloaterTools) {mIsShowFloaterTools = pShowFloaterTools;};
	bool            isShowFloaterTools() const                  {return mIsShowFloaterTools;};

protected:
	LLTool*			mSelectedTool;
	typedef std::vector<LLTool*> tool_list_t;
	tool_list_t 	mToolList;
	bool            mIsShowFloaterTools;
};

// Globals

extern LLToolset* gBasicToolset;
extern LLToolset *gCameraToolset;
//extern LLToolset *gLandToolset;
extern LLToolset* gMouselookToolset;
extern LLToolset* gFaceEditToolset;

extern LLTool*		gToolNull;

#endif
