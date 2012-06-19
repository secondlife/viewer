/** 
 * @file lltoolbrush.h
 * @brief toolbrush class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLTOOLBRUSH_H
#define LL_LLTOOLBRUSH_H

#include "lltool.h"
#include "v3math.h"
#include "lleditmenuhandler.h"

class LLSurface;
class LLVector3d;
class LLViewerRegion;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLToolBrushLand
//
// A toolbrush that modifies the land.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLToolBrushLand : public LLTool, public LLEditMenuHandler, public LLSingleton<LLToolBrushLand>
{
	typedef std::set<LLViewerRegion*> region_list_t;

public:
	LLToolBrushLand();
	
	// x,y in window coords, 0,0 = left,bot
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );		
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	virtual void handleSelect();
	virtual void handleDeselect();

	// isAlwaysRendered() - return true if this is a tool that should
	// always be rendered regardless of selection.
	virtual BOOL isAlwaysRendered() { return TRUE; }

	// Draw the area that will be affected.
	virtual void render();

	// on Idle is where the land modification actually occurs
	static void onIdle(void* brush_tool);  

	void			onMouseCaptureLost();

	void modifyLandInSelectionGlobal();
	virtual void	undo();
	virtual BOOL	canUndo() const	{ return TRUE; }

protected:
	void brush( void );
	void modifyLandAtPointGlobal( const LLVector3d &spot, MASK mask );

	void determineAffectedRegions(region_list_t& regions,
								  const LLVector3d& spot) const;
	void renderOverlay(LLSurface& land, const LLVector3& pos_region,
					   const LLVector3& pos_world);

	// Does region allow terraform, or are we a god?
	bool canTerraform(LLViewerRegion* regionp) const;

	// Modal dialog that you can't terraform the region
	void alertNoTerraform(LLViewerRegion* regionp);

	// Modal dialog that you can't terraform in frozen mode
	void alertNoTerraformWhileFrozen();

protected:
	F32 mStartingZ;
	S32 mMouseX;
	S32 mMouseY;
	F32 mBrushSize;
	BOOL mGotHover;
	BOOL mBrushSelected;
	// Order doesn't matter and we do check for existance of regions, so use a set
	region_list_t mLastAffectedRegions;

private:
	U8 getBrushIndex();
};


#endif // LL_LLTOOLBRUSH_H
