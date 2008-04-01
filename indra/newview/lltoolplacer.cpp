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
#include "llui.h"

//Headers added for functions moved from viewer.cpp
#include "llvograss.h"
#include "llvotree.h"
#include "llvolumemessage.h"
#include "llhudmanager.h"
#include "llagent.h"
#include "audioengine.h"
#include "llhudeffecttrail.h"
#include "llviewerobjectlist.h"
#include "llviewercamera.h"
#include "llviewerstats.h"

const LLVector3 DEFAULT_OBJECT_SCALE(0.5f, 0.5f, 0.5f);

//static 
LLPCode	LLToolPlacer::sObjectType = LL_PCODE_CUBE;

LLToolPlacer::LLToolPlacer()
:	LLTool( "Create" )
{
}

BOOL LLToolPlacer::raycastForNewObjPos( S32 x, S32 y, LLViewerObject** hit_obj, S32* hit_face, 
							 BOOL* b_hit_land, LLVector3* ray_start_region, LLVector3* ray_end_region, LLViewerRegion** region )
{
	F32 max_dist_from_camera = gSavedSettings.getF32( "MaxSelectDistance" ) - 1.f;

	// Viewer-side pick to find the right sim to create the object on.  
	// First find the surface the object will be created on.
	gViewerWindow->hitObjectOrLandGlobalImmediate(x, y, NULL, FALSE);
	
	// Note: use the frontmost non-flora version because (a) plants usually have lots of alpha and (b) pants' Havok
	// representations (if any) are NOT the same as their viewer representation.
	*hit_obj = gObjectList.findObject( gLastHitNonFloraObjectID );
	*hit_face = gLastHitNonFloraObjectFace;
	*b_hit_land = !(*hit_obj) && !gLastHitNonFloraPosGlobal.isExactlyZero();
	LLVector3d land_pos_global = gLastHitNonFloraPosGlobal;

	// Make sure there's a surface to place the new object on.
	BOOL bypass_sim_raycast = FALSE;
	LLVector3d	surface_pos_global;
	if (*b_hit_land)
	{
		surface_pos_global = land_pos_global; 
		bypass_sim_raycast = TRUE;
	}
	else 
	if (*hit_obj)
	{
		surface_pos_global = (*hit_obj)->getPositionGlobal();
	}
	else
	{
		return FALSE;
	}

	// Make sure the surface isn't too far away.
	LLVector3d ray_start_global = gAgent.getCameraPositionGlobal();
	F32 dist_to_surface_sq = (F32)((surface_pos_global - ray_start_global).magVecSquared());
	if( dist_to_surface_sq > (max_dist_from_camera * max_dist_from_camera) )
	{
		return FALSE;
	}

	// Find the sim where the surface lives.
	LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(surface_pos_global);
	if (!regionp)
	{
		llwarns << "Trying to add object outside of all known regions!" << llendl;
		return FALSE;
	}

	// Find the simulator-side ray that will be used to place the object accurately
	LLVector3d		mouse_direction;
	mouse_direction.setVec( gViewerWindow->mouseDirectionGlobal( x, y ) );

	*region = regionp;
	*ray_start_region =	regionp->getPosRegionFromGlobal( ray_start_global );
	F32 near_clip = LLViewerCamera::getInstance()->getNear() + 0.01f;  // Include an epsilon to avoid rounding issues.
	*ray_start_region += LLViewerCamera::getInstance()->getAtAxis() * near_clip;

	if( bypass_sim_raycast )
	{
		// Hack to work around Havok's inability to ray cast onto height fields
		*ray_end_region = regionp->getPosRegionFromGlobal( surface_pos_global );  // ray end is the viewer's intersection point
	}
	else
	{
		LLVector3d		ray_end_global = ray_start_global + (1.f + max_dist_from_camera) * mouse_direction;  // add an epsilon to the sim version of the ray to avoid rounding problems.
		*ray_end_region = regionp->getPosRegionFromGlobal( ray_end_global );
	}

	return TRUE;
}


BOOL LLToolPlacer::addObject( LLPCode pcode, S32 x, S32 y, U8 use_physics )
{
	LLVector3 ray_start_region;
	LLVector3 ray_end_region;
	LLViewerRegion* regionp = NULL;
	BOOL b_hit_land = FALSE;
	S32 hit_face = -1;
	LLViewerObject* hit_obj = NULL;
	U8 state = 0;
	BOOL success = raycastForNewObjPos( x, y, &hit_obj, &hit_face, &b_hit_land, &ray_start_region, &ray_end_region, &regionp );
	if( !success )
	{
		return FALSE;
	}

	if( hit_obj && (hit_obj->isAvatar() || hit_obj->isAttachment()) )
	{
		// Can't create objects on avatars or attachments
		return FALSE;
	}

	if (NULL == regionp)
	{
		llwarns << "regionp was NULL; aborting function." << llendl;
		return FALSE;
	}

	if (regionp->getRegionFlags() & REGION_FLAGS_SANDBOX)
	{
		LLFirstUse::useSandbox();
	}

	// Set params for new object based on its PCode.
	LLQuaternion	rotation;
	LLVector3		scale = DEFAULT_OBJECT_SCALE;
	U8				material = LL_MCODE_WOOD;
	BOOL			create_selected = FALSE;
	LLVolumeParams	volume_params;
	
	switch (pcode) 
	{
	case LL_PCODE_LEGACY_GRASS:
		//  Randomize size of grass patch 
		scale.setVec(10.f + ll_frand(20.f), 10.f + ll_frand(20.f),  1.f + ll_frand(2.f));
		state = rand() % LLVOGrass::sMaxGrassSpecies;
		break;


	case LL_PCODE_LEGACY_TREE:
	case LL_PCODE_TREE_NEW:
		state = rand() % LLVOTree::sMaxTreeSpecies;
		break;

	case LL_PCODE_SPHERE:
	case LL_PCODE_CONE:
	case LL_PCODE_CUBE:
	case LL_PCODE_CYLINDER:
	case LL_PCODE_TORUS:
	case LLViewerObject::LL_VO_SQUARE_TORUS:
	case LLViewerObject::LL_VO_TRIANGLE_TORUS:
	default:
		create_selected = TRUE;
		break;
	}

	// Play creation sound
	if (gAudiop)
	{
		F32 volume = gSavedSettings.getBOOL("MuteUI") ? 0.f : gSavedSettings.getF32("AudioLevelUI");
		gAudiop->triggerSound( LLUUID(gSavedSettings.getString("UISndObjectCreate")), gAgent.getID(), volume);
	}

	gMessageSystem->newMessageFast(_PREHASH_ObjectAdd);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU8Fast(_PREHASH_Material,	material);

	U32 flags = 0;		// not selected
	if (use_physics)
	{
		flags |= FLAGS_USE_PHYSICS;
	}
	if (create_selected)
	{
		flags |= FLAGS_CREATE_SELECTED;
	}
	gMessageSystem->addU32Fast(_PREHASH_AddFlags, flags );

	LLPCode volume_pcode;	// ...PCODE_VOLUME, or the original on error
	switch (pcode)
	{
	case LL_PCODE_SPHERE:
		rotation.setQuat(90.f * DEG_TO_RAD, LLVector3::y_axis);

		volume_params.setType( LL_PCODE_PROFILE_CIRCLE_HALF, LL_PCODE_PATH_CIRCLE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1, 1 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_TORUS:
		rotation.setQuat(90.f * DEG_TO_RAD, LLVector3::y_axis);

		volume_params.setType( LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_CIRCLE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1.f, 0.25f );	// "top size"
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LLViewerObject::LL_VO_SQUARE_TORUS:
		rotation.setQuat(90.f * DEG_TO_RAD, LLVector3::y_axis);

		volume_params.setType( LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_CIRCLE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1.f, 0.25f );	// "top size"
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LLViewerObject::LL_VO_TRIANGLE_TORUS:
		rotation.setQuat(90.f * DEG_TO_RAD, LLVector3::y_axis);

		volume_params.setType( LL_PCODE_PROFILE_EQUALTRI, LL_PCODE_PATH_CIRCLE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1.f, 0.25f );	// "top size"
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_SPHERE_HEMI:
		volume_params.setType( LL_PCODE_PROFILE_CIRCLE_HALF, LL_PCODE_PATH_CIRCLE );
		//volume_params.setBeginAndEndS( 0.5f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 0.5f );
		volume_params.setRatio	( 1, 1 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_CUBE:
		volume_params.setType( LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1, 1 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_PRISM:
		volume_params.setType( LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 0, 1 );
		volume_params.setShear	( -0.5f, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_PYRAMID:
		volume_params.setType( LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 0, 0 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_TETRAHEDRON:
		volume_params.setType( LL_PCODE_PROFILE_EQUALTRI, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 0, 0 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_CYLINDER:
		volume_params.setType( LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1, 1 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_CYLINDER_HEMI:
		volume_params.setType( LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.25f, 0.75f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 1, 1 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_CONE:
		volume_params.setType( LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.f, 1.f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 0, 0 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	case LL_PCODE_CONE_HEMI:
		volume_params.setType( LL_PCODE_PROFILE_CIRCLE, LL_PCODE_PATH_LINE );
		volume_params.setBeginAndEndS( 0.25f, 0.75f );
		volume_params.setBeginAndEndT( 0.f, 1.f );
		volume_params.setRatio	( 0, 0 );
		volume_params.setShear	( 0, 0 );
		LLVolumeMessage::packVolumeParams(&volume_params, gMessageSystem);
		volume_pcode = LL_PCODE_VOLUME;
		break;

	default:
		LLVolumeMessage::packVolumeParams(0, gMessageSystem);
		volume_pcode = pcode;
		break;
	}
	gMessageSystem->addU8Fast(_PREHASH_PCode, volume_pcode);

	gMessageSystem->addVector3Fast(_PREHASH_Scale,			scale );
	gMessageSystem->addQuatFast(_PREHASH_Rotation,			rotation );
	gMessageSystem->addVector3Fast(_PREHASH_RayStart,		ray_start_region );
	gMessageSystem->addVector3Fast(_PREHASH_RayEnd,			ray_end_region );
	gMessageSystem->addU8Fast(_PREHASH_BypassRaycast,		(U8)b_hit_land );
	gMessageSystem->addU8Fast(_PREHASH_RayEndIsIntersection, (U8)FALSE );
	gMessageSystem->addU8Fast(_PREHASH_State, state);

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
	gMessageSystem->addUUIDFast(_PREHASH_RayTargetID,			ray_target_id );
	
	// Pack in name value pairs
	gMessageSystem->sendReliable(regionp->getHost());

	// Spawns a message, so must be after above send
	if (create_selected)
	{
		LLSelectMgr::getInstance()->deselectAll();
		gViewerWindow->getWindow()->incBusyCount();
	}

	// VEFFECT: AddObject
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject((LLViewerObject*)gAgent.getAvatarObject());
	effectp->setPositionGlobal(regionp->getPosGlobalFromRegion(ray_end_region));
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CREATE_COUNT);

	return TRUE;
}

// Used by the placer tool to add copies of the current selection.
// Inspired by add_object().  JC
BOOL LLToolPlacer::addDuplicate(S32 x, S32 y)
{
	LLVector3 ray_start_region;
	LLVector3 ray_end_region;
	LLViewerRegion* regionp = NULL;
	BOOL b_hit_land = FALSE;
	S32 hit_face = -1;
	LLViewerObject* hit_obj = NULL;
	BOOL success = raycastForNewObjPos( x, y, &hit_obj, &hit_face, &b_hit_land, &ray_start_region, &ray_end_region, &regionp );
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

	LLSelectMgr::getInstance()->selectDuplicateOnRay(ray_start_region,
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
		added = addDuplicate(x, y);
	}
	else
	{
		added = addObject( sObjectType, x, y, FALSE );
	}

	// ...and go back to the default tool
	if (added && !gSavedSettings.getBOOL("CreateToolKeepSelected"))
	{
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolCompTranslate::getInstance() );
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
