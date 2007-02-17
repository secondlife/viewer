/** 
 * @file llglsandbox.cpp
 * @brief GL functionality access
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/** 
 * Contains ALL methods which directly access GL functionality 
 * except for core rendering engine functionality.
 */

#include "llviewerprecompiledheaders.h"

#include "llviewercontrol.h"

#include "llgl.h"
#include "llglheaders.h"
#include "llparcel.h"
#include "llui.h"

#include "lldrawable.h"
#include "lltextureentry.h"
#include "llviewercamera.h"

#include "llvoavatar.h"
#include "llagent.h"
#include "lltoolmgr.h"
#include "llselectmgr.h"
#include "llhudmanager.h"
#include "llsphere.h"
#include "llviewerobjectlist.h"
#include "lltoolselectrect.h"
#include "llviewerwindow.h"
#include "viewer.h"
#include "llcompass.h"
#include "llsurface.h"
#include "llwind.h"
#include "llworld.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "pipeline.h"
 
BOOL LLAgent::setLookAt(ELookAtType target_type, LLViewerObject *object, LLVector3 position)
{
	if(object && object->isAttachment())
	{
		LLViewerObject* parent = object;
		while(parent)
		{
			if (parent == mAvatarObject)
			{
				// looking at an attachment on ourselves, which we don't want to do
				object = mAvatarObject;
				position.clearVec();
			}
			parent = (LLViewerObject*)parent->getParent();
		}
	}
	if(!mLookAt || mLookAt->isDead())
	{
		mLookAt = (LLHUDEffectLookAt *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_LOOKAT);
		mLookAt->setSourceObject(mAvatarObject);
	}

	return mLookAt->setLookAt(target_type, object, position);
}

BOOL LLAgent::setPointAt(EPointAtType target_type, LLViewerObject *object, LLVector3 position)
{
	// disallow pointing at attachments and avatars
	if (object && (object->isAttachment() || object->isAvatar()))
	{
		return FALSE;
	}

	if(!mPointAt || mPointAt->isDead())
	{
		mPointAt = (LLHUDEffectPointAt *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINTAT);
		mPointAt->setSourceObject(mAvatarObject);
	}

	return mPointAt->setPointAt(target_type, object, position);
}

ELookAtType LLAgent::getLookAtType()
{ 
	if (mLookAt) 
	{
		return mLookAt->getLookAtType();
	}

	return LOOKAT_TARGET_NONE;
}

EPointAtType LLAgent::getPointAtType()
{ 
	if (mPointAt) 
	{
		return mPointAt->getPointAtType();
	}

	return POINTAT_TARGET_NONE;
}

// Draw a representation of current autopilot target
void LLAgent::renderAutoPilotTarget()
{
	if (mAutoPilot)
	{
		F32 height_meters;
		LLVector3d target_global;

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		// not textured
		LLGLSNoTexture no_texture;

		// lovely green
		glColor4f(0.f, 1.f, 1.f, 1.f);

		target_global = mAutoPilotTargetGlobal;

		glTranslatef((F32)(target_global.mdV[VX]), (F32)(target_global.mdV[VY]), (F32)(target_global.mdV[VZ]));

		/*
		LLVector3 offset = target_global - mCamera.getOrigin();
		F32 range = offset.magVec();
		if (range > 0.001f)
		{
			// range != zero
			F32 fraction_of_fov = height_pixels / (F32) mCamera.getViewHeightInPixels();
			F32 apparent_angle = fraction_of_fov * mCamera.getView();
			height_meters = range * tan(apparent_angle);
		}
		else
		{
			// range == zero
			height_meters = 1.0f;
		}
		*/
		height_meters = 1.f;

		glScalef(height_meters, height_meters, height_meters);

		gSphere.render(1500.f);

		glPopMatrix();
	}
}

extern BOOL gDebugSelect;

// Returns true if you got at least one object
void LLToolSelectRect::handleRectangleSelection(S32 x, S32 y, MASK mask)
{
	LLVector3 av_pos = gAgent.getPositionAgent();
	F32 select_dist_squared = gSavedSettings.getF32("MaxSelectDistance");
	select_dist_squared = select_dist_squared * select_dist_squared;

	x = llround((F32)x * LLUI::sGLScaleFactor.mV[VX]);
	y = llround((F32)y * LLUI::sGLScaleFactor.mV[VY]);

	BOOL deselect = (mask == MASK_CONTROL);
	S32 left =	llmin(x, mDragStartX);
	S32 right =	llmax(x, mDragStartX);
	S32 top =	llmax(y, mDragStartY);
	S32 bottom =llmin(y, mDragStartY);

	F32 old_far_plane = gCamera->getFar();
	F32 old_near_plane = gCamera->getNear();

	S32 width = right - left + 1;
	S32 height = top - bottom + 1;

	BOOL grow_selection = FALSE;
	BOOL shrink_selection = FALSE;

	if (height > mDragLastHeight || width > mDragLastWidth)
	{
		grow_selection = TRUE;
	}
	if (height < mDragLastHeight || width < mDragLastWidth)
	{
		shrink_selection = TRUE;
	}

	if (!grow_selection && !shrink_selection)
	{
		// nothing to do
		return;
	}

	mDragLastHeight = height;
	mDragLastWidth = width;

	S32 center_x = (left + right) / 2;
	S32 center_y = (top + bottom) / 2;

	// save drawing mode
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	BOOL limit_select_distance = gSavedSettings.getBOOL("LimitSelectDistance");
	if (limit_select_distance)
	{
		// ...select distance from control
		LLVector3 relative_av_pos = av_pos;
		relative_av_pos -= gCamera->getOrigin();

		F32 new_far = relative_av_pos * gCamera->getAtAxis() + gSavedSettings.getF32("MaxSelectDistance");
		F32 new_near = relative_av_pos * gCamera->getAtAxis() - gSavedSettings.getF32("MaxSelectDistance");

		new_near = llmax(new_near, 0.1f);

		gCamera->setFar(new_far);
		gCamera->setNear(new_near);
	}
	gCamera->setPerspective(FOR_SELECTION, 
							center_x-width/2, center_y-height/2, width, height, 
							limit_select_distance);

	if (shrink_selection)
	{
		LLObjectSelectionHandle highlighted_objects = gSelectMgr->getHighlightedObjects();

		for (LLViewerObject* vobjp = highlighted_objects->getFirstObject();
			vobjp;
			vobjp = highlighted_objects->getNextObject())
			{
				LLDrawable* drawable = vobjp->mDrawable;
				if (!drawable || vobjp->getPCode() != LL_PCODE_VOLUME || vobjp->isAttachment())
				{
					continue;
				}

				S32 result = gCamera->sphereInFrustum(drawable->getWorldPosition(), drawable->getRadius());
				switch (result)
				{
				case 0:
					gSelectMgr->unhighlightObjectOnly(vobjp);
					break;
				case 1:
					// check vertices
					if (!gCamera->areVertsVisible(vobjp, LLSelectMgr::sRectSelectInclusive))
					{
						gSelectMgr->unhighlightObjectOnly(vobjp);
					}
					break;
				default:
					break;
				}
			}
	}

	if (grow_selection)
	{
		std::vector<LLDrawable*> potentials;
		
		if (gPipeline.mObjectPartition) 
		{
			gPipeline.mObjectPartition->cull(*gCamera, &potentials, TRUE);
		}

		for (std::vector<LLDrawable*>::iterator iter = potentials.begin();
			 iter != potentials.end(); iter++)
		{
			LLDrawable* drawable = *iter;
			LLViewerObject* vobjp = drawable->getVObj();

			if (!drawable || !vobjp ||
				vobjp->getPCode() != LL_PCODE_VOLUME || 
				vobjp->isAttachment() ||
				(deselect && !vobjp->isSelected()))
			{
				continue;
			}

			if (limit_select_distance && dist_vec_squared(drawable->getWorldPosition(), av_pos) > select_dist_squared)
			{
				continue;
			}

			S32 result = gCamera->sphereInFrustum(drawable->getWorldPosition(), drawable->getRadius());
			if (result)
			{
				switch (result)
				{
				case 1:
					// check vertices
					if (gCamera->areVertsVisible(vobjp, LLSelectMgr::sRectSelectInclusive))
					{
						gSelectMgr->highlightObjectOnly(vobjp);
					}
					break;
				case 2:
					gSelectMgr->highlightObjectOnly(vobjp);
					break;
				default:
					break;
				}
			}
		}
	}

	// restore drawing mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	// restore camera
	gCamera->setFar(old_far_plane);
	gCamera->setNear(old_near_plane);
	gViewerWindow->setup3DRender();
}


const F32 COMPASS_SIZE = 64;
static const F32 COMPASS_RANGE = 0.33f;

void LLCompass::draw()
{
//	S32 left, top, right, bottom;

	if (!getVisible()) return;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	S32 width = 32;
	S32 height = 32;

	LLGLSUIDefault gls_ui;

	glTranslatef( COMPASS_SIZE/2.f, COMPASS_SIZE/2.f, 0.f);

	if (mBkgndTexture)
	{
		mBkgndTexture->bind();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		
		glBegin(GL_QUADS);
		
		glTexCoord2f(1.f, 1.f);
		glVertex2i(width, height);
		
		glTexCoord2f(0.f, 1.f);
		glVertex2i(-width, height);
		
		glTexCoord2f(0.f, 0.f);
		glVertex2i(-width, -height);
		
		glTexCoord2f(1.f, 0.f);
		glVertex2i(width, -height);
		
		glEnd();
	}

	// rotate subsequent draws to agent rotation
	F32 rotation = atan2( gAgent.getFrameAgent().getAtAxis().mV[VX], gAgent.getFrameAgent().getAtAxis().mV[VY] );
	glRotatef( - rotation * RAD_TO_DEG, 0.f, 0.f, -1.f);
	
	if (mTexture)
	{
		mTexture->bind();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		
		glBegin(GL_QUADS);
		
		glTexCoord2f(1.f, 1.f);
		glVertex2i(width, height);
		
		glTexCoord2f(0.f, 1.f);
		glVertex2i(-width, height);
		
		glTexCoord2f(0.f, 0.f);
		glVertex2i(-width, -height);
		
		glTexCoord2f(1.f, 0.f);
		glVertex2i(width, -height);
		
		glEnd();
	}

	glPopMatrix();

}



void LLHorizontalCompass::draw()
{
	if (!getVisible()) return;

	LLGLSUIDefault gls_ui;
	
	S32 width = mRect.getWidth();
	S32 height = mRect.getHeight();
	S32 half_width = width / 2;

	if( mTexture )
	{
		const LLVector3& at_axis = gCamera->getAtAxis();
		F32 center = atan2( at_axis.mV[VX], at_axis.mV[VY] );

		center += F_PI;
		center = llclamp( center, 0.0f, F_TWO_PI ); // probably not necessary...
		center /= F_TWO_PI;
		F32 left = center - COMPASS_RANGE;
		F32 right = center + COMPASS_RANGE;

		mTexture->bind();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f );
		glBegin( GL_QUADS );

		glTexCoord2f(right, 1.f);
		glVertex2i(width, height);

		glTexCoord2f(left, 1.f);
		glVertex2i(0, height);

		glTexCoord2f(left, 0.f);
		glVertex2i(0, 0);

		glTexCoord2f(right, 0.f);
		glVertex2i(width, 0);

		glEnd();
	}

	// Draw the focus line
	{
		LLGLSNoTexture gls_no_texture;
		glColor4fv( mFocusColor.mV );
		gl_line_2d( half_width, 0, half_width, height );
	}
}


const F32 WIND_ALTITUDE			= 180.f;


void LLWind::renderVectors()
{
	// Renders the wind as vectors (used for debug)
	S32 i,j;
	F32 x,y;

	F32 region_width_meters = gWorldPointer->getRegionWidthInMeters();

	LLGLSNoTexture gls_no_texture;
	glPushMatrix();
	LLVector3 origin_agent;
	origin_agent = gAgent.getPosAgentFromGlobal(mOriginGlobal);
	glTranslatef(origin_agent.mV[VX], origin_agent.mV[VY], WIND_ALTITUDE);
	for (j = 0; j < mSize; j++)
	{
		for (i = 0; i < mSize; i++)
		{
			x = mCloudVelX[i + j*mSize] * WIND_SCALE_HACK;
			y = mCloudVelY[i + j*mSize] * WIND_SCALE_HACK;
			glPushMatrix();
			glTranslatef((F32)i * region_width_meters/mSize, (F32)j * region_width_meters/mSize, 0.0);
			glColor3f(0,1,0);
			glBegin(GL_POINTS);
				glVertex3f(0,0,0);
			glEnd();
			glColor3f(1,0,0);
			glBegin(GL_LINES);
				glVertex3f(x * 0.1f, y * 0.1f ,0.f);
				glVertex3f(x, y, 0.f);
			glEnd();
			glPopMatrix();
		}
	}
	glPopMatrix();
}




// Used by lltoolselectland
void LLViewerParcelMgr::renderRect(const LLVector3d &west_south_bottom_global, 
								   const LLVector3d &east_north_top_global )
{
	LLGLSUIDefault gls_ui;
	LLGLSNoTexture gls_no_texture;
	LLGLDepthTest gls_depth(GL_TRUE);

	LLVector3 west_south_bottom_agent = gAgent.getPosAgentFromGlobal(west_south_bottom_global);
	F32 west	= west_south_bottom_agent.mV[VX];
	F32 south	= west_south_bottom_agent.mV[VY];
//	F32 bottom	= west_south_bottom_agent.mV[VZ] - 1.f;

	LLVector3 east_north_top_agent = gAgent.getPosAgentFromGlobal(east_north_top_global);
	F32 east	= east_north_top_agent.mV[VX];
	F32 north	= east_north_top_agent.mV[VY];
//	F32 top		= east_north_top_agent.mV[VZ] + 1.f;

	// HACK: At edge of last region of world, we need to make sure the region
	// resolves correctly so we can get a height value.
	const F32 FUDGE = 0.01f;

	F32 sw_bottom = gWorldp->resolveLandHeightAgent( LLVector3( west, south, 0.f ) );
	F32 se_bottom = gWorldp->resolveLandHeightAgent( LLVector3( east-FUDGE, south, 0.f ) );
	F32 ne_bottom = gWorldp->resolveLandHeightAgent( LLVector3( east-FUDGE, north-FUDGE, 0.f ) );
	F32 nw_bottom = gWorldp->resolveLandHeightAgent( LLVector3( west, north-FUDGE, 0.f ) );

	F32 sw_top = sw_bottom + PARCEL_POST_HEIGHT;
	F32 se_top = se_bottom + PARCEL_POST_HEIGHT;
	F32 ne_top = ne_bottom + PARCEL_POST_HEIGHT;
	F32 nw_top = nw_bottom + PARCEL_POST_HEIGHT;

	LLUI::setLineWidth(2.f);
	glColor4f(1.f, 1.f, 0.f, 1.f);

	// Cheat and give this the same pick-name as land
	glBegin(GL_LINES);

	glVertex3f(west, north, nw_bottom);
	glVertex3f(west, north, nw_top);

	glVertex3f(east, north, ne_bottom);
	glVertex3f(east, north, ne_top);

	glVertex3f(east, south, se_bottom);
	glVertex3f(east, south, se_top);

	glVertex3f(west, south, sw_bottom);
	glVertex3f(west, south, sw_top);

	glEnd();

	glColor4f(1.f, 1.f, 0.f, 0.2f);
	glBegin(GL_QUADS);

	glVertex3f(west, north, nw_bottom);
	glVertex3f(west, north, nw_top);
	glVertex3f(east, north, ne_top);
	glVertex3f(east, north, ne_bottom);

	glVertex3f(east, north, ne_bottom);
	glVertex3f(east, north, ne_top);
	glVertex3f(east, south, se_top);
	glVertex3f(east, south, se_bottom);

	glVertex3f(east, south, se_bottom);
	glVertex3f(east, south, se_top);
	glVertex3f(west, south, sw_top);
	glVertex3f(west, south, sw_bottom);

	glVertex3f(west, south, sw_bottom);
	glVertex3f(west, south, sw_top);
	glVertex3f(west, north, nw_top);
	glVertex3f(west, north, nw_bottom);

	glEnd();

	LLUI::setLineWidth(1.f);
}

/*
void LLViewerParcelMgr::renderParcel(LLParcel* parcel )
{
	S32 i;
	S32 count = parcel->getBoxCount();
	for (i = 0; i < count; i++)
	{
		const LLParcelBox& box = parcel->getBox(i);

		F32 west = box.mMin.mV[VX];
		F32 south = box.mMin.mV[VY];

		F32 east = box.mMax.mV[VX];
		F32 north = box.mMax.mV[VY];

		// HACK: At edge of last region of world, we need to make sure the region
		// resolves correctly so we can get a height value.
		const F32 FUDGE = 0.01f;

		F32 sw_bottom = gWorldp->resolveLandHeightAgent( LLVector3( west, south, 0.f ) );
		F32 se_bottom = gWorldp->resolveLandHeightAgent( LLVector3( east-FUDGE, south, 0.f ) );
		F32 ne_bottom = gWorldp->resolveLandHeightAgent( LLVector3( east-FUDGE, north-FUDGE, 0.f ) );
		F32 nw_bottom = gWorldp->resolveLandHeightAgent( LLVector3( west, north-FUDGE, 0.f ) );

		// little hack to make nearby lines not Z-fight
		east -= 0.1f;
		north -= 0.1f;

		F32 sw_top = sw_bottom + POST_HEIGHT;
		F32 se_top = se_bottom + POST_HEIGHT;
		F32 ne_top = ne_bottom + POST_HEIGHT;
		F32 nw_top = nw_bottom + POST_HEIGHT;

		LLGLSNoTexture gls_no_texture;
		LLGLDepthTest gls_depth(GL_TRUE);

		LLUI::setLineWidth(2.f);
		glColor4f(0.f, 1.f, 1.f, 1.f);

		// Cheat and give this the same pick-name as land
		glBegin(GL_LINES);

		glVertex3f(west, north, nw_bottom);
		glVertex3f(west, north, nw_top);

		glVertex3f(east, north, ne_bottom);
		glVertex3f(east, north, ne_top);

		glVertex3f(east, south, se_bottom);
		glVertex3f(east, south, se_top);

		glVertex3f(west, south, sw_bottom);
		glVertex3f(west, south, sw_top);

		glEnd();

		glColor4f(0.f, 1.f, 1.f, 0.2f);
		glBegin(GL_QUADS);

		glVertex3f(west, north, nw_bottom);
		glVertex3f(west, north, nw_top);
		glVertex3f(east, north, ne_top);
		glVertex3f(east, north, ne_bottom);

		glVertex3f(east, north, ne_bottom);
		glVertex3f(east, north, ne_top);
		glVertex3f(east, south, se_top);
		glVertex3f(east, south, se_bottom);

		glVertex3f(east, south, se_bottom);
		glVertex3f(east, south, se_top);
		glVertex3f(west, south, sw_top);
		glVertex3f(west, south, sw_bottom);

		glVertex3f(west, south, sw_bottom);
		glVertex3f(west, south, sw_top);
		glVertex3f(west, north, nw_top);
		glVertex3f(west, north, nw_bottom);

		glEnd();

		LLUI::setLineWidth(1.f);
	}
}
*/


// north = a wall going north/south.  Need that info to set up texture
// coordinates correctly.
void LLViewerParcelMgr::renderOneSegment(F32 x1, F32 y1, F32 x2, F32 y2, F32 height, U8 direction, LLViewerRegion* regionp)
{
	// HACK: At edge of last region of world, we need to make sure the region
	// resolves correctly so we can get a height value.
	const F32 BORDER = REGION_WIDTH_METERS - 0.1f;

	F32 clamped_x1 = x1;
	F32 clamped_y1 = y1;
	F32 clamped_x2 = x2;
	F32 clamped_y2 = y2;

	if (clamped_x1 > BORDER) clamped_x1 = BORDER;
	if (clamped_y1 > BORDER) clamped_y1 = BORDER;
	if (clamped_x2 > BORDER) clamped_x2 = BORDER;
	if (clamped_y2 > BORDER) clamped_y2 = BORDER;

	F32 z;
	F32 z1;
	F32 z2;

	z1 = regionp->getLand().resolveHeightRegion( LLVector3( clamped_x1, clamped_y1, 0.f ) );
	z2 = regionp->getLand().resolveHeightRegion( LLVector3( clamped_x2, clamped_y2, 0.f ) );

	// Convert x1 and x2 from region-local to agent coords.
	LLVector3 origin = regionp->getOriginAgent();
	x1 += origin.mV[VX];
	x2 += origin.mV[VX];
	y1 += origin.mV[VY];
	y2 += origin.mV[VY];

	if (height < 1.f)
	{
		z = z1+height;
		glVertex3f(x1, y1, z);

		glVertex3f(x1, y1, z1);

		glVertex3f(x2, y2, z2);

		z = z2+height;
		glVertex3f(x2, y2, z);
	}
	else
	{
		F32 tex_coord1;
		F32 tex_coord2;

		if (WEST_MASK == direction)
		{
			tex_coord1 = y1;
			tex_coord2 = y2;
		}
		else if (SOUTH_MASK == direction)
		{
			tex_coord1 = x1;
			tex_coord2 = x2;
		}
		else if (EAST_MASK == direction)
		{
			tex_coord1 = y2;
			tex_coord2 = y1;
		}
		else /* (NORTH_MASK == direction) */
		{
			tex_coord1 = x2;
			tex_coord2 = x1;
		}


		glTexCoord2f(tex_coord1*0.5f+0.5f, z1*0.5f);
		glVertex3f(x1, y1, z1);

		glTexCoord2f(tex_coord2*0.5f+0.5f, z2*0.5f);
		glVertex3f(x2, y2, z2);

		// top edge stairsteps
		z = llmax(z2+height, z1+height);
		glTexCoord2f(tex_coord2*0.5f+0.5f, z*0.5f);
		glVertex3f(x2, y2, z);

		glTexCoord2f(tex_coord1*0.5f+0.5f, z*0.5f);
		glVertex3f(x1, y1, z);
	}
}


void LLViewerParcelMgr::renderHighlightSegments(const U8* segments, LLViewerRegion* regionp)
{
	S32 x, y;
	F32 x1, y1;	// start point
	F32 x2, y2;	// end point

	LLGLSUIDefault gls_ui;
	LLGLSNoTexture gls_no_texture;
	LLGLDepthTest gls_depth(GL_TRUE);

	glColor4f(1.f, 1.f, 0.f, 0.2f);

	// Cheat and give this the same pick-name as land
	glBegin(GL_QUADS);

	const S32 STRIDE = (mParcelsPerEdge+1);
	for (y = 0; y < STRIDE; y++)
	{
		for (x = 0; x < STRIDE; x++)
		{
			U8 segment_mask = segments[x + y*STRIDE];

			if (segment_mask & SOUTH_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1 + PARCEL_GRID_STEP_METERS;
				y2 = y1;

				renderOneSegment(x1, y1, x2, y2, PARCEL_POST_HEIGHT, SOUTH_MASK, regionp);
			}

			if (segment_mask & WEST_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1;
				y2 = y1 + PARCEL_GRID_STEP_METERS;

				renderOneSegment(x1, y1, x2, y2, PARCEL_POST_HEIGHT, WEST_MASK, regionp);
			}
		}
	}

	glEnd();
}


void LLViewerParcelMgr::renderCollisionSegments(U8* segments, BOOL use_pass, LLViewerRegion* regionp)
{

	S32 x, y;
	F32 x1, y1;	// start point
	F32 x2, y2;	// end point
	F32 alpha = 0;
	F32 dist = 0;
	F32 dx, dy;
	F32 collision_height;

	const S32 STRIDE = (mParcelsPerEdge+1);
	
	LLVector3 pos = gAgent.getPositionAgent();

	F32 pos_x = pos.mV[VX];
	F32 pos_y = pos.mV[VY];

	LLGLSUIDefault gls_ui;
	LLGLDepthTest gls_depth(GL_TRUE);
	LLGLDisable cull(GL_CULL_FACE);
	
	if (mCollisionBanned == BA_BANNED)
	{
		collision_height = BAN_HEIGHT;
	}
	else
	{
		collision_height = PARCEL_HEIGHT;
	}

	
	if (use_pass && (mCollisionBanned == BA_NOT_ON_LIST))
	{
		LLViewerImage::bindTexture(mPassImage);
	}
	else
	{
		LLViewerImage::bindTexture(mBlockedImage);
	}

	glBegin(GL_QUADS);

	for (y = 0; y < STRIDE; y++)
	{
		for (x = 0; x < STRIDE; x++)
		{
			U8 segment_mask = segments[x + y*STRIDE];
			U8 direction;
			const F32 MAX_ALPHA = 0.95f;
			const S32 DIST_OFFSET = 5;
			const S32 MIN_DIST_SQ = DIST_OFFSET*DIST_OFFSET;
			const S32 MAX_DIST_SQ = 169;

			if (segment_mask & SOUTH_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1 + PARCEL_GRID_STEP_METERS;
				y2 = y1;

				if (gRenderForSelect)
				{
					LLColor4U color((U8)(GL_NAME_PARCEL_WALL >> 16), (U8)(GL_NAME_PARCEL_WALL >> 8), (U8)GL_NAME_PARCEL_WALL);
					glColor4ubv(color.mV);
				}
				else
				{
					dy = (pos_y - y1) + DIST_OFFSET;
					
					if (pos_x < x1)
						dx = pos_x - x1;
					else if (pos_x > x2)
						dx = pos_x - x2;
					else 
						dx = 0;
					
					dist = dx*dx+dy*dy;

					if (dist < MIN_DIST_SQ)
						alpha = MAX_ALPHA;
					else if (dist > MAX_DIST_SQ)
						alpha = 0.0f;
					else
						alpha = 30/dist;

					alpha = llclamp(alpha, 0.0f, MAX_ALPHA);

					glColor4f(1.f, 1.f, 1.f, alpha);
				}

				if ((pos_y - y1) < 0) direction = SOUTH_MASK;
				else 		direction = NORTH_MASK;

				// avoid Z fighting
				renderOneSegment(x1+0.1f, y1+0.1f, x2+0.1f, y2+0.1f, collision_height, direction, regionp);

			}

			if (segment_mask & WEST_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1;
				y2 = y1 + PARCEL_GRID_STEP_METERS;

				if (gRenderForSelect)
				{
					LLColor4U color((U8)(GL_NAME_PARCEL_WALL >> 16), (U8)(GL_NAME_PARCEL_WALL >> 8), (U8)GL_NAME_PARCEL_WALL);
					glColor4ubv(color.mV);
				}
				else
				{					
					dx = (pos_x - x1) + DIST_OFFSET;
		
					if (pos_y < y1) 
						dy = pos_y - y1;
					else if (pos_y > y2)
						dy = pos_y - y2;
					else 
						dy = 0;

					dist = dx*dx+dy*dy;
					
					if (dist < MIN_DIST_SQ) 
						alpha = MAX_ALPHA;
					else if (dist > MAX_DIST_SQ)
						alpha = 0.0f;
					else
						alpha = 30/dist;

					alpha = llclamp(alpha, 0.0f, MAX_ALPHA);

					glColor4f(1.f, 1.f, 1.f, alpha);
				}

				if ((pos_x - x1) > 0) direction = WEST_MASK;
				else 		direction = EAST_MASK;
				
				// avoid Z fighting
				renderOneSegment(x1+0.1f, y1+0.1f, x2+0.1f, y2+0.1f, collision_height, direction, regionp);

			}
		}
	}

	glEnd();
}


const S32 CLIENT_RECT_VPAD = 4;
void LLPreviewTexture::draw()
{
	if( getVisible() )
	{
		updateAspectRatio();

		LLPreview::draw();

		if (!mMinimized)
		{
			LLGLSUIDefault gls_ui;
			LLGLSNoTexture gls_notex;
			
			const LLRect& border = mClientRect;
			LLRect interior = mClientRect;
			interior.stretch( -PREVIEW_BORDER_WIDTH );

			// ...border
			gl_rect_2d( border, LLColor4(0.f, 0.f, 0.f, 1.f));
			gl_rect_2d_checkerboard( interior );

			if ( mImage.notNull() )
			{
				LLGLSTexture gls_no_texture;
				// Draw the texture
				glColor3f( 1.f, 1.f, 1.f );
				gl_draw_scaled_image(interior.mLeft,
									interior.mBottom,
									interior.getWidth(),
									interior.getHeight(),
									mImage);

				// Pump the texture priority
				F32 pixel_area = mLoadingFullImage ? (F32)MAX_IMAGE_AREA  : (F32)(interior.getWidth() * interior.getHeight() );
				mImage->addTextureStats( pixel_area );

				// Don't bother decoding more than we can display, unless
				// we're loading the full image.
				if (!mLoadingFullImage)
				{
					S32 int_width = interior.getWidth();
					S32 int_height = interior.getHeight();
					mImage->setKnownDrawSize(int_width, int_height);
				}
				else
				{
					// Don't use this feature
					mImage->setKnownDrawSize(0, 0);
				}

				if( mLoadingFullImage )
				{
					LLFontGL::sSansSerif->renderUTF8("Receiving:", 0,
						interior.mLeft + 4, 
						interior.mBottom + 4,
						LLColor4::white, LLFontGL::LEFT, LLFontGL::BOTTOM,
						LLFontGL::DROP_SHADOW);
					
					F32 data_progress = 0.0f;
					F32 decode_progress = mImage->getDecodeProgress(&data_progress);
					
					// Draw the progress bar.
					const S32 BAR_HEIGHT = 12;
					const S32 BAR_LEFT_PAD = 80;
					S32 left = interior.mLeft + 4 + BAR_LEFT_PAD;
					S32 bar_width = mRect.getWidth() - left - RESIZE_HANDLE_WIDTH - 2;
					S32 top = interior.mBottom + 4 + BAR_HEIGHT;
					S32 right = left + bar_width;
					S32 bottom = top - BAR_HEIGHT;

					LLColor4 background_color(0.f, 0.f, 0.f, 0.75f);
					LLColor4 decoded_color(0.f, 1.f, 0.f, 1.0f);
					LLColor4 downloaded_color(0.f, 0.5f, 0.f, 1.0f);

					gl_rect_2d(left, top, right, bottom, background_color);

					if (data_progress > 0.0f)
					{
						// Decoded bytes
						right = left + llfloor(decode_progress * (F32)bar_width);

						if (left < right)
						{
							gl_rect_2d(left, top, right, bottom, decoded_color);
						}

						// Downloaded bytes
						left = right;
						right = left + llfloor((data_progress - decode_progress) * (F32)bar_width);

						if (left < right)
						{
							gl_rect_2d(left, top, right, bottom, downloaded_color);
						}
					}
				}
				else
				if( !mSavedFileTimer.hasExpired() )
				{
					LLFontGL::sSansSerif->renderUTF8("File Saved", 0,
						interior.mLeft + 4,
						interior.mBottom + 4,
						LLColor4::white, LLFontGL::LEFT, LLFontGL::BOTTOM,
						LLFontGL::DROP_SHADOW);
				}
			}
		}
	}
}


void draw_line_cube(F32 width, const LLVector3& center)
{
	width = 0.5f * width;
	glVertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] + width);

	glVertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] - width);

	glVertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] - width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] + width);
	glVertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] - width);
}


void LLViewerObjectList::renderObjectBeacons()
{
	S32 i;
	//const LLFontGL *font = gResMgr->getRes(LLFONT_SANSSERIF);

	LLGLSUIDefault gls_ui;

	S32 last_line_width = -1;
	
	{
		LLGLSNoTexture gls_ui_no_texture;
		glBegin(GL_LINES);
		for (i = 0; i < mDebugBeacons.count(); i++)
		{
			const LLDebugBeacon &debug_beacon = mDebugBeacons[i];
			LLColor4 color = debug_beacon.mColor;
			color.mV[3] *= 0.25f;
			S32 line_width = debug_beacon.mLineWidth;
			if (line_width != last_line_width)
			{
				glEnd();
				glLineWidth( (F32)line_width );
				last_line_width = line_width;
				glBegin(GL_LINES);
			}

			const LLVector3 &thisline = debug_beacon.mPositionAgent;
			glColor4fv(color.mV);
			glVertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] - 50.f);
			glVertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] + 50.f);
			glVertex3f(thisline.mV[VX] - 2.f,thisline.mV[VY],thisline.mV[VZ]);
			glVertex3f(thisline.mV[VX] + 2.f,thisline.mV[VY],thisline.mV[VZ]);
			glVertex3f(thisline.mV[VX],thisline.mV[VY] - 2.f,thisline.mV[VZ]);
			glVertex3f(thisline.mV[VX],thisline.mV[VY] + 2.f,thisline.mV[VZ]);

			draw_line_cube(0.10f, thisline);
		}
		glEnd();
	}

	{
		LLGLSNoTexture gls_ui_no_texture;
		LLGLDepthTest gls_depth(GL_TRUE);
		
		glBegin(GL_LINES);
		last_line_width = -1;
		for (i = 0; i < mDebugBeacons.count(); i++)
		{
			const LLDebugBeacon &debug_beacon = mDebugBeacons[i];

			S32 line_width = debug_beacon.mLineWidth;
			if (line_width != last_line_width)
			{
				glEnd();
				glLineWidth( (F32)line_width );
				last_line_width = line_width;
				glBegin(GL_LINES);
			}

			const LLVector3 &thisline = debug_beacon.mPositionAgent;
			glColor4fv(debug_beacon.mColor.mV);
			glVertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] - 0.5f);
			glVertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] + 0.5f);
			glVertex3f(thisline.mV[VX] - 0.5f,thisline.mV[VY],thisline.mV[VZ]);
			glVertex3f(thisline.mV[VX] + 0.5f,thisline.mV[VY],thisline.mV[VZ]);
			glVertex3f(thisline.mV[VX],thisline.mV[VY] - 0.5f,thisline.mV[VZ]);
			glVertex3f(thisline.mV[VX],thisline.mV[VY] + 0.5f,thisline.mV[VZ]);

			draw_line_cube(0.10f, thisline);
		}
		glEnd();
	
		glLineWidth(1.f);

		for (i = 0; i < mDebugBeacons.count(); i++)
		{
			LLDebugBeacon &debug_beacon = mDebugBeacons[i];
			if (debug_beacon.mString == "")
			{
				continue;
			}
			LLHUDText *hud_textp = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);

			hud_textp->setZCompare(FALSE);
			LLColor4 color;
			color = debug_beacon.mTextColor;
			color.mV[3] *= 1.f;

			hud_textp->setString(utf8str_to_wstring(debug_beacon.mString.c_str()));
			hud_textp->setColor(color);
			hud_textp->setPositionAgent(debug_beacon.mPositionAgent);
			debug_beacon.mHUDObject = hud_textp;
		}
	}
}


void pre_show_depth_buffer()
{
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS,0,0);
	glStencilOp(GL_INCR,GL_INCR,GL_INCR);
}

void post_show_depth_buffer()
{
	int xsize =500, ysize =500;
	U8 *buf = new U8[xsize*ysize];

	glReadPixels(0,0,xsize,ysize,GL_STENCIL_INDEX,GL_UNSIGNED_BYTE, buf);

	int total = 0;
	int i;
	for (i=0;i<xsize*ysize;i++) 
	{
		total += buf[i];
		buf[i] <<= 3;
	}

	float DC = (float)total/(float)(ysize*xsize);
	int DCline = llfloor((xsize-20) * DC / 10.0f);
	int stride = xsize / 10;

	int y = 2;

	for (i=0;i<DCline;i++)
	{
		if (i % stride == 0) i+=2;
		if (i > xsize) y=6;
		buf[ysize*(y+0)+i]=255;
		buf[ysize*(y+1)+i]=255;
		buf[ysize*(y+2)+i]=255;
	}
	glDrawPixels(xsize,ysize,GL_RED,GL_UNSIGNED_BYTE,buf);

	delete buf;
}
