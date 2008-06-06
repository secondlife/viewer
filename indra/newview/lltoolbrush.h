/** 
 * @file lltoolbrush.h
 * @brief toolbrush class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

protected:
	F32 mStartingZ;
	S32 mMouseX;
	S32 mMouseY;
	S32 mBrushIndex;
	BOOL mGotHover;
	BOOL mBrushSelected;
	// Order doesn't matter and we do check for existance of regions, so use a set
	region_list_t mLastAffectedRegions;
};


#endif // LL_LLTOOLBRUSH_H
