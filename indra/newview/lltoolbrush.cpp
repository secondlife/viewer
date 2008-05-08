/** 
 * @file lltoolbrush.cpp
 * @brief Implementation of the toolbrushes
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

#include "lltoolbrush.h"
#include "lltoolselectland.h"

#include "llgl.h"
#include "llglimmediate.h"

#include "message.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llviewercontrol.h"
#include "llfloatertools.h"
#include "llregionposition.h"
#include "llstatusbar.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewerparcelmgr.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llappviewer.h"
#include "llparcel.h"

#include "llglheaders.h"

const std::string REGION_BLOCKS_TERRAFORM_MSG = "This region does not allow terraforming.\n"
				"You will need to buy land in another part of the world to terraform it.";


///============================================================================
/// Local function declarations, constants, enums, and typedefs
///============================================================================

const S32 LAND_BRUSH_SIZE_COUNT = 3;
const F32 LAND_BRUSH_SIZE[LAND_BRUSH_SIZE_COUNT] = {1.0f, 2.0f, 4.0f};
const S32 LAND_STEPS = 3;
const F32 LAND_METERS_PER_SECOND = 1.0f;
enum
{
	E_LAND_LEVEL	= 0,
	E_LAND_RAISE	= 1,
	E_LAND_LOWER	= 2,
	E_LAND_SMOOTH	= 3,
	E_LAND_NOISE	= 4,
	E_LAND_REVERT	= 5,
	E_LAND_INVALID 	= 6,
};
const LLColor4 OVERLAY_COLOR(1.0f, 1.0f, 1.0f, 1.0f);

///============================================================================
/// Class LLToolBrushLand
///============================================================================

// constructor
LLToolBrushLand::LLToolBrushLand()
:	LLTool("Land"),
	mStartingZ( 0.0f ),
	mMouseX( 0 ),
	mMouseY(0),
	mGotHover(FALSE),
	mLastShowParcelOwners(FALSE),
	mBrushSelected(FALSE)
{
	mBrushIndex = gSavedSettings.getS32("RadioLandBrushSize");
}

void LLToolBrushLand::modifyLandAtPointGlobal(const LLVector3d &pos_global,
											  MASK mask)
{
	S32 radioAction = gSavedSettings.getS32("RadioLandBrushAction");

	determineAffectedRegions(mLastAffectedRegions, pos_global);
	for(region_list_t::iterator iter = mLastAffectedRegions.begin();
		iter != mLastAffectedRegions.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		//BOOL is_changed = FALSE;
		LLVector3 pos_region = regionp->getPosRegionFromGlobal(pos_global);
		LLSurface &land = regionp->getLand();
		char action = E_LAND_LEVEL;
		switch (radioAction)
		{
		case 0:
		//	// average toward mStartingZ
			action = E_LAND_LEVEL;
			break;
		case 1:
			action = E_LAND_RAISE;
			break;
		case 2:
			action = E_LAND_LOWER;
			break;
		case 3:
			action = E_LAND_SMOOTH;
			break;
		case 4:
			action = E_LAND_NOISE;
			break;
		case 5:
			action = E_LAND_REVERT;
			break;
		default:
			action = E_LAND_INVALID;
			break;
		}

		// Don't send a message to the region if nothing changed.
		//if(!is_changed) continue;

		// Now to update the patch information so it will redraw correctly.
		LLSurfacePatch *patchp= land.resolvePatchRegion(pos_region);
		if (patchp)
		{
			patchp->dirtyZ();
		}

		// Also force the property lines to update, normals to recompute, etc.
		regionp->forceUpdate();

		// tell the simulator what we've done
		F32 seconds = 1.0f / gFPSClamped;
		F32 x_pos = (F32)pos_region.mV[VX];
		F32 y_pos = (F32)pos_region.mV[VY];
		U8 brush_size = (U8)mBrushIndex;
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ModifyLand);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ModifyBlock);
		msg->addU8Fast(_PREHASH_Action, (U8)action);
		msg->addU8Fast(_PREHASH_BrushSize, brush_size);
		msg->addF32Fast(_PREHASH_Seconds, seconds);
		msg->addF32Fast(_PREHASH_Height, mStartingZ);
		msg->nextBlockFast(_PREHASH_ParcelData);
		msg->addS32Fast(_PREHASH_LocalID, -1);
		msg->addF32Fast(_PREHASH_West, x_pos );
		msg->addF32Fast(_PREHASH_South, y_pos );
		msg->addF32Fast(_PREHASH_East, x_pos );
		msg->addF32Fast(_PREHASH_North, y_pos );
		msg->sendMessage(regionp->getHost());
	}
}

void LLToolBrushLand::modifyLandInSelectionGlobal()
{
	if (LLViewerParcelMgr::getInstance()->selectionEmpty())
	{
		return;
	}

	if (LLToolMgr::getInstance()->getCurrentTool() == LLToolSelectLand::getInstance())
	{
		// selecting land, don't do anything
		return;
	}

	LLVector3d min;
	LLVector3d max;

	LLViewerParcelMgr::getInstance()->getSelection(min, max);

	S32 radioAction = gSavedSettings.getS32("RadioLandBrushAction");

	mLastAffectedRegions.clear();

	determineAffectedRegions(mLastAffectedRegions, LLVector3d(min.mdV[VX], min.mdV[VY], 0));
	determineAffectedRegions(mLastAffectedRegions, LLVector3d(min.mdV[VX], max.mdV[VY], 0));
	determineAffectedRegions(mLastAffectedRegions, LLVector3d(max.mdV[VX], min.mdV[VY], 0));
	determineAffectedRegions(mLastAffectedRegions, LLVector3d(max.mdV[VX], max.mdV[VY], 0));

	LLRegionPosition mid_point_region((min + max) * 0.5);
	LLViewerRegion* center_region = mid_point_region.getRegion();
	if (center_region)
	{
		LLVector3 pos_region = mid_point_region.getPositionRegion();
		U32 grids = center_region->getLand().mGridsPerEdge;
		S32 i = llclamp( (S32)pos_region.mV[VX], 0, (S32)grids );
		S32 j = llclamp( (S32)pos_region.mV[VY], 0, (S32)grids );
		mStartingZ = center_region->getLand().getZ(i+j*grids);
	}
	else
	{
		mStartingZ = 0.f;
	}

	// Stop if our selection include a no-terraform region
	for(region_list_t::iterator iter = mLastAffectedRegions.begin();
		iter != mLastAffectedRegions.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		if (!canTerraform(regionp))
		{
			alertNoTerraform(regionp);
			return;
		}
	}

	for(region_list_t::iterator iter = mLastAffectedRegions.begin();
		iter != mLastAffectedRegions.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		//BOOL is_changed = FALSE;
		LLVector3 min_region = regionp->getPosRegionFromGlobal(min);
		LLVector3 max_region = regionp->getPosRegionFromGlobal(max);
	
		min_region.clamp(0.f, regionp->getWidth());
		max_region.clamp(0.f, regionp->getWidth());
		F32 seconds = 1.0f;

		LLSurface &land = regionp->getLand();
		char action = E_LAND_LEVEL;
		switch (radioAction)
		{
		case 0:
		//	// average toward mStartingZ
			action = E_LAND_LEVEL;
			seconds = 1.f;
			break;
		case 1:
			action = E_LAND_RAISE;
			break;
		case 2:
			action = E_LAND_LOWER;
			break;
		case 3:
			action = E_LAND_SMOOTH;
			seconds = 10.f;
			break;
		case 4:
			action = E_LAND_NOISE;
			seconds = 0.5f;
			break;
		case 5:
			action = E_LAND_REVERT;
			seconds = 0.5f;
			break;
		default:
			//action = E_LAND_INVALID;
			//seconds = 0.0f;
			return;
			break;
		}

		// Don't send a message to the region if nothing changed.
		//if(!is_changed) continue;

		// Now to update the patch information so it will redraw correctly.
		LLSurfacePatch *patchp= land.resolvePatchRegion(min_region);
		if (patchp)
		{
			patchp->dirtyZ();
		}

		// Also force the property lines to update, normals to recompute, etc.
		regionp->forceUpdate();

		// tell the simulator what we've done
		U8 brush_size = (U8)mBrushIndex;
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ModifyLand);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ModifyBlock);
		msg->addU8Fast(_PREHASH_Action, (U8)action);
		msg->addU8Fast(_PREHASH_BrushSize, brush_size);
		msg->addF32Fast(_PREHASH_Seconds, seconds);
		msg->addF32Fast(_PREHASH_Height, mStartingZ);

		BOOL parcel_selected = LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected();
		LLParcel* selected_parcel = LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel();

		if (parcel_selected && selected_parcel)
		{
			msg->nextBlockFast(_PREHASH_ParcelData);
			msg->addS32Fast(_PREHASH_LocalID, selected_parcel->getLocalID());
			msg->addF32Fast(_PREHASH_West,  min_region.mV[VX] );
			msg->addF32Fast(_PREHASH_South, min_region.mV[VY] );
			msg->addF32Fast(_PREHASH_East,  max_region.mV[VX] );
			msg->addF32Fast(_PREHASH_North, max_region.mV[VY] );
		}
		else
		{
			msg->nextBlockFast(_PREHASH_ParcelData);
			msg->addS32Fast(_PREHASH_LocalID, -1);
			msg->addF32Fast(_PREHASH_West,  min_region.mV[VX] );
			msg->addF32Fast(_PREHASH_South, min_region.mV[VY] );
			msg->addF32Fast(_PREHASH_East,  max_region.mV[VX] );
			msg->addF32Fast(_PREHASH_North, max_region.mV[VY] );
		}

		msg->sendMessage(regionp->getHost());
	}
}

void LLToolBrushLand::brush( void )
{
	LLVector3d spot;
	if( gViewerWindow->mousePointOnLandGlobal( mMouseX, mMouseY, &spot ) )
	{
		// Round to nearest X,Y grid
		spot.mdV[VX] = floor( spot.mdV[VX] + 0.5 );
		spot.mdV[VY] = floor( spot.mdV[VY] + 0.5 );

		modifyLandAtPointGlobal(spot, gKeyboard->currentMask(TRUE));
	}
}

BOOL LLToolBrushLand::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	// Find the z value of the initial click. 
	LLVector3d spot;
	if( gViewerWindow->mousePointOnLandGlobal( x, y, &spot ) )
	{
		// Round to nearest X,Y grid
		spot.mdV[VX] = floor( spot.mdV[VX] + 0.5 );
		spot.mdV[VY] = floor( spot.mdV[VY] + 0.5 );

		LLRegionPosition region_position( spot );
		LLViewerRegion* regionp = region_position.getRegion();

		if (!canTerraform(regionp))
		{
			alertNoTerraform(regionp);
			return TRUE;
		}

		LLVector3 pos_region = region_position.getPositionRegion();
		U32 grids = regionp->getLand().mGridsPerEdge;
		S32 i = llclamp( (S32)pos_region.mV[VX], 0, (S32)grids );
		S32 j = llclamp( (S32)pos_region.mV[VY], 0, (S32)grids );
		mStartingZ = regionp->getLand().getZ(i+j*grids);
		mMouseX = x;
		mMouseY = y;
		gIdleCallbacks.addFunction( &LLToolBrushLand::onIdle, (void*)this );
		setMouseCapture( TRUE );

		LLViewerParcelMgr::getInstance()->setSelectionVisible(FALSE);
		handled = TRUE;
	}

	return handled;
}

BOOL LLToolBrushLand::handleHover( S32 x, S32 y, MASK mask )
{
	lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolBrushLand ("
								<< (hasMouseCapture() ? "active":"inactive")
								<< ")" << llendl;
	mMouseX = x;
	mMouseY = y;
	mGotHover = TRUE;
	gViewerWindow->getWindow()->setCursor(UI_CURSOR_TOOLLAND);
	return TRUE;
}

BOOL LLToolBrushLand::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	mLastAffectedRegions.clear();
	if( hasMouseCapture() )
	{
		// Release the mouse
		setMouseCapture( FALSE );

		LLViewerParcelMgr::getInstance()->setSelectionVisible(TRUE);

		gIdleCallbacks.deleteFunction( &LLToolBrushLand::onIdle, (void*)this );
		handled = TRUE;
	}

	return handled;
}

void LLToolBrushLand::handleSelect()
{
	gEditMenuHandler = this;

	gFloaterTools->setStatusText("modifyland");
//	if (!mBrushSelected)
	{
		mLastShowParcelOwners = gSavedSettings.getBOOL("ShowParcelOwners");
		gSavedSettings.setBOOL("ShowParcelOwners", mLastShowParcelOwners);
		mBrushSelected = TRUE;
	}
}


void LLToolBrushLand::handleDeselect()
{
	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}
	mLastShowParcelOwners = gSavedSettings.getBOOL("ShowParcelOwners");
	gSavedSettings.setBOOL("ShowParcelOwners", mLastShowParcelOwners);
	LLViewerParcelMgr::getInstance()->setSelectionVisible(TRUE);
	mBrushSelected = FALSE;
}

// Draw the area that will be affected.
void LLToolBrushLand::render()
{
	if(mGotHover)
	{
		//llinfos << "LLToolBrushLand::render()" << llendl;
		LLVector3d spot;
		if(gViewerWindow->mousePointOnLandGlobal(mMouseX, mMouseY, &spot))
		{
			spot.mdV[VX] = floor( spot.mdV[VX] + 0.5 );
			spot.mdV[VY] = floor( spot.mdV[VY] + 0.5 );

			mBrushIndex = gSavedSettings.getS32("RadioLandBrushSize");
			region_list_t regions;
			determineAffectedRegions(regions, spot);

			// Now, for each region, render the overlay
			LLVector3 pos_world = gAgent.getRegion()->getPosRegionFromGlobal(spot);
			for(region_list_t::iterator iter = regions.begin();
				iter != regions.end(); ++iter)
			{
				LLViewerRegion* region = *iter;
				renderOverlay(region->getLand(), 
							  region->getPosRegionFromGlobal(spot),
							  pos_world);
			}
		}
		mGotHover = FALSE;
	}
}

void LLToolBrushLand::renderOverlay(LLSurface& land, const LLVector3& pos_region,
									const LLVector3& pos_world)
{
	glMatrixMode(GL_MODELVIEW);
	LLGLSNoTexture gls_no_texture;
	LLGLDepthTest mDepthTest(GL_TRUE);
	glPushMatrix();
	gGL.color4fv(OVERLAY_COLOR.mV);
	glTranslatef(0.0f, 0.0f, 1.0f);
	
	S32 i = (S32) pos_region.mV[VX];
	S32 j = (S32) pos_region.mV[VY];
	S32 half_edge = llfloor(LAND_BRUSH_SIZE[mBrushIndex]);
	
	gGL.begin(LLVertexBuffer::POINTS);
	for(S32 di = -half_edge; di <= half_edge; di++)
	{
		if((i+di) < 0 || (i+di) >= (S32)land.mGridsPerEdge) continue;
		for(S32 dj = -half_edge; dj <= half_edge; dj++)
		{
			if( (j+dj) < 0 || (j+dj) >= (S32)land.mGridsPerEdge ) continue;
			gGL.vertex3f(pos_world.mV[VX] + di, pos_world.mV[VY] + dj,
					   land.getZ((i+di)+(j+dj)*land.mGridsPerEdge));
		}
	}
	gGL.end();
	glPopMatrix();
}

void LLToolBrushLand::determineAffectedRegions(region_list_t& regions,
											   const LLVector3d& spot ) const
{
	LLVector3d corner(spot);
	corner.mdV[VX] -= (LAND_BRUSH_SIZE[mBrushIndex] / 2);
	corner.mdV[VY] -= (LAND_BRUSH_SIZE[mBrushIndex] / 2);
	LLViewerRegion* region = NULL;
	region = LLWorld::getInstance()->getRegionFromPosGlobal(corner);
	if(region && regions.find(region) == regions.end())
	{
		regions.insert(region);
	}
	corner.mdV[VY] += LAND_BRUSH_SIZE[mBrushIndex];
	region = LLWorld::getInstance()->getRegionFromPosGlobal(corner);
	if(region && regions.find(region) == regions.end())
	{
		regions.insert(region);
	}
	corner.mdV[VX] += LAND_BRUSH_SIZE[mBrushIndex];
	region = LLWorld::getInstance()->getRegionFromPosGlobal(corner);
	if(region && regions.find(region) == regions.end())
	{
		regions.insert(region);
	}
	corner.mdV[VY] -= LAND_BRUSH_SIZE[mBrushIndex];
	region = LLWorld::getInstance()->getRegionFromPosGlobal(corner);
	if(region && regions.find(region) == regions.end())
	{
		regions.insert(region);
	}
}

// static
void LLToolBrushLand::onIdle( void* brush_tool )
{
	LLToolBrushLand* self = reinterpret_cast<LLToolBrushLand*>(brush_tool);

	if( LLToolMgr::getInstance()->getCurrentTool() == self )
	{
		self->brush();
	}
	else
	{
		gIdleCallbacks.deleteFunction( &LLToolBrushLand::onIdle, self );
	}
}

void LLToolBrushLand::onMouseCaptureLost()
{
	gIdleCallbacks.deleteFunction(&LLToolBrushLand::onIdle, this);
}

// static
void LLToolBrushLand::undo()
{
	for(region_list_t::iterator iter = mLastAffectedRegions.begin();
		iter != mLastAffectedRegions.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		gMessageSystem->newMessageFast(_PREHASH_UndoLand);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->sendMessage(regionp->getHost());
	}
}

// static
/*
void LLToolBrushLand::redo()
{
	for(region_list_t::iterator iter = mLastAffectedRegions.begin();
		iter != mLastAffectedRegions.end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		gMessageSystem->newMessageFast(_PREHASH_RedoLand);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->sendMessage(regionp->getHost());
	}
}*/

// static
bool LLToolBrushLand::canTerraform(LLViewerRegion* regionp) const
{
	if (!regionp) return false;
	if (regionp->canManageEstate()) return true;
	return !(regionp->getRegionFlags() & REGION_FLAGS_BLOCK_TERRAFORM);
}

// static
void LLToolBrushLand::alertNoTerraform(LLViewerRegion* regionp)
{
	if (!regionp) return;
	
	LLStringBase<char>::format_map_t args;
	args["[REGION]"] = regionp->getName();
	gViewerWindow->alertXml("RegionNoTerraforming", args);

}

///============================================================================
/// Local function definitions
///============================================================================
