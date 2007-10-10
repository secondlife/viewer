/** 
 * @file lltoolplacer.cpp
 * @brief Tool for placing new objects into the world
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

// self header
#include "lltoolplacer.h"

// linden library headers
#include "llprimitive.h"

// viewer headers
#include "llbutton.h"
#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfloatertools.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "viewer.h"
#include "llui.h"

//static 
LLPCode	LLToolPlacer::sObjectType = LL_PCODE_CUBE;

LLToolPlacer::LLToolPlacer()
:	LLTool( "Create" )
{
}

// Used by the placer tool to add copies of the current selection.
// Inspired by add_object().  JC
BOOL add_duplicate(S32 x, S32 y)
{
	LLVector3 ray_start_region;
	LLVector3 ray_end_region;
	LLViewerRegion* regionp = NULL;
	BOOL b_hit_land = FALSE;
	S32 hit_face = -1;
	LLViewerObject* hit_obj = NULL;
	BOOL success = raycast_for_new_obj_pos( x, y, &hit_obj, &hit_face, &b_hit_land, &ray_start_region, &ray_end_region, &regionp );
	if( !success )
	{
		make_ui_sound("UISndInvalidOp");
		return FALSE;
	}
	if( hit_obj && (hit_obj->isAvatar() || hit_obj->isAttachment()) )
	{
		// Can't create objects on avatars or attachments
		make_ui_sound("UISndInvalidOp");
		return FALSE;
	}


	// Limit raycast to a single object.  
	// Speeds up server raycast + avoid problems with server ray hitting objects
	// that were clipped by the near plane or culled on the viewer.
	LLUUID ray_target_id;
	if( hit_obj )
	{
		ray_target_id = hit_obj->getID();
	}
	else
	{
		ray_target_id.setNull();
	}

	gSelectMgr->selectDuplicateOnRay(ray_start_region,
										ray_end_region,
										b_hit_land,			// suppress raycast
										FALSE,				// intersection
										ray_target_id,
										gSavedSettings.getBOOL("CreateToolCopyCenters"),
										gSavedSettings.getBOOL("CreateToolCopyRotates"),
										FALSE);				// select copy

	if (regionp
		&& (regionp->getRegionFlags() & REGION_FLAGS_SANDBOX))
	{
		LLFirstUse::useSandbox();
	}

	return TRUE;
}


BOOL LLToolPlacer::placeObject(S32 x, S32 y, MASK mask)
{
	BOOL added = TRUE;
	
	if (gSavedSettings.getBOOL("CreateToolCopySelection"))
	{
		added = add_duplicate(x, y);
	}
	else
	{
		added = add_object( sObjectType, x, y, NO_PHYSICS );
	}

	// ...and go back to the default tool
	if (added && !gSavedSettings.getBOOL("CreateToolKeepSelected"))
	{
		gToolMgr->getCurrentToolset()->selectTool( gToolTranslate );
	}

	return added;
}

BOOL LLToolPlacer::handleHover(S32 x, S32 y, MASK mask)
{
	lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolPlacer" << llendl;		
	gViewerWindow->getWindow()->setCursor(UI_CURSOR_TOOLCREATE);
	return TRUE;
}

void LLToolPlacer::handleSelect()
{
	gFloaterTools->setStatusText("place");
}

void LLToolPlacer::handleDeselect()
{
}

//////////////////////////////////////////////////////
// LLToolPlacerPanel

// static
LLPCode LLToolPlacerPanel::sCube		= LL_PCODE_CUBE;
LLPCode LLToolPlacerPanel::sPrism		= LL_PCODE_PRISM;
LLPCode LLToolPlacerPanel::sPyramid		= LL_PCODE_PYRAMID;
LLPCode LLToolPlacerPanel::sTetrahedron	= LL_PCODE_TETRAHEDRON;
LLPCode LLToolPlacerPanel::sCylinder	= LL_PCODE_CYLINDER;
LLPCode LLToolPlacerPanel::sCylinderHemi= LL_PCODE_CYLINDER_HEMI;
LLPCode LLToolPlacerPanel::sCone		= LL_PCODE_CONE;
LLPCode LLToolPlacerPanel::sConeHemi	= LL_PCODE_CONE_HEMI;
LLPCode LLToolPlacerPanel::sTorus		= LL_PCODE_TORUS;
LLPCode LLToolPlacerPanel::sSquareTorus = LLViewerObject::LL_VO_SQUARE_TORUS;
LLPCode LLToolPlacerPanel::sTriangleTorus = LLViewerObject::LL_VO_TRIANGLE_TORUS;
LLPCode LLToolPlacerPanel::sSphere		= LL_PCODE_SPHERE;
LLPCode LLToolPlacerPanel::sSphereHemi	= LL_PCODE_SPHERE_HEMI;
LLPCode LLToolPlacerPanel::sTree		= LL_PCODE_LEGACY_TREE;
LLPCode LLToolPlacerPanel::sGrass		= LL_PCODE_LEGACY_GRASS;

S32			LLToolPlacerPanel::sButtonsAdded = 0;
LLButton*	LLToolPlacerPanel::sButtons[ TOOL_PLACER_NUM_BUTTONS ];

LLToolPlacerPanel::LLToolPlacerPanel(const std::string& name, const LLRect& rect)
	:
	LLPanel( name, rect )
{
	/* DEPRECATED - JC
	addButton( "UIImgCubeUUID",			"UIImgCubeSelectedUUID",		&LLToolPlacerPanel::sCube );
	addButton( "UIImgPrismUUID",		"UIImgPrismSelectedUUID",		&LLToolPlacerPanel::sPrism );
	addButton( "UIImgPyramidUUID",		"UIImgPyramidSelectedUUID",		&LLToolPlacerPanel::sPyramid );
	addButton( "UIImgTetrahedronUUID",	"UIImgTetrahedronSelectedUUID",	&LLToolPlacerPanel::sTetrahedron );
	addButton( "UIImgCylinderUUID",		"UIImgCylinderSelectedUUID",	&LLToolPlacerPanel::sCylinder );
	addButton( "UIImgHalfCylinderUUID",	"UIImgHalfCylinderSelectedUUID",&LLToolPlacerPanel::sCylinderHemi );
	addButton( "UIImgConeUUID",			"UIImgConeSelectedUUID",		&LLToolPlacerPanel::sCone );
	addButton( "UIImgHalfConeUUID",		"UIImgHalfConeSelectedUUID",	&LLToolPlacerPanel::sConeHemi );
	addButton( "UIImgSphereUUID",		"UIImgSphereSelectedUUID",		&LLToolPlacerPanel::sSphere );
	addButton( "UIImgHalfSphereUUID",	"UIImgHalfSphereSelectedUUID",	&LLToolPlacerPanel::sSphereHemi );
	addButton( "UIImgTreeUUID",			"UIImgTreeSelectedUUID",		&LLToolPlacerPanel::sTree );
	addButton( "UIImgGrassUUID",		"UIImgGrassSelectedUUID",		&LLToolPlacerPanel::sGrass );
	addButton( "ObjectTorusImageID",	"ObjectTorusActiveImageID",		&LLToolPlacerPanel::sTorus );
	addButton( "ObjectTubeImageID",		"ObjectTubeActiveImageID",		&LLToolPlacerPanel::sSquareTorus );
	*/
}

void LLToolPlacerPanel::addButton( const LLString& up_state, const LLString& down_state, LLPCode* pcode )
{
	const S32 TOOL_SIZE = 32;
	const S32 HORIZ_SPACING = TOOL_SIZE + 5;
	const S32 VERT_SPACING = TOOL_SIZE + 5;
	const S32 VPAD = 10;
	const S32 HPAD = 7;

	S32 row = sButtonsAdded / 4;
	S32 column = sButtonsAdded % 4; 

	LLRect help_rect = gSavedSettings.getRect("ToolHelpRect");

	// Build the rectangle, recalling the origin is at lower left
	// and we want the icons to build down from the top.
	LLRect rect;
	rect.setLeftTopAndSize(
		HPAD + (column * HORIZ_SPACING),
		help_rect.mBottom - VPAD - (row * VERT_SPACING),
		TOOL_SIZE,
		TOOL_SIZE );

	LLButton* btn = new LLButton(
		"ToolPlacerOptBtn",
		rect,
		up_state,
		down_state,
		"", &LLToolPlacerPanel::setObjectType,
		pcode,
		LLFontGL::sSansSerif);
	btn->setFollowsBottom();
	btn->setFollowsLeft();
	addChild(btn);

	sButtons[sButtonsAdded] = btn;
	sButtonsAdded++;
}

// static 
void	LLToolPlacerPanel::setObjectType( void* data )
{
	LLPCode pcode = *(LLPCode*) data;
	LLToolPlacer::setObjectType( pcode );
}
