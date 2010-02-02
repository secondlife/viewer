/** 
 * @file lltoolplacer.h
 * @brief Tool for placing new objects into the world
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

#ifndef LL_TOOLPLACER_H
#define LL_TOOLPLACER_H

#include "llpanel.h"
#include "lltool.h"

class LLButton;
class LLViewerRegion;

////////////////////////////////////////////////////
// LLToolPlacer

class LLToolPlacer
 :	public LLTool
{
public:
	LLToolPlacer();

	virtual BOOL	placeObject(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual void	handleSelect();	// do stuff when your tool is selected
	virtual void	handleDeselect();	// clean up when your tool is deselected

	static void	setObjectType( LLPCode type )		{ sObjectType = type; }
	static LLPCode getObjectType()					{ return sObjectType; }

protected:
	static LLPCode	sObjectType;

private:
	BOOL addObject( LLPCode pcode, S32 x, S32 y, U8 use_physics );
	BOOL raycastForNewObjPos( S32 x, S32 y, LLViewerObject** hit_obj, S32* hit_face, 
							  BOOL* b_hit_land, LLVector3* ray_start_region, LLVector3* ray_end_region, LLViewerRegion** region );
	BOOL addDuplicate(S32 x, S32 y);
};

#endif
