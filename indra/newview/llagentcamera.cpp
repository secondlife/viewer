/** 
 * @file llagentcamera.cpp
 * @brief LLAgent class implementation
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

#include "llviewerprecompiledheaders.h"
#include "llagentcamera.h" 

#include "pipeline.h"

#include "llagent.h"
#include "llanimationstates.h"
#include "llfloatercamera.h"
#include "llfloaterreg.h"
#include "llhudmanager.h"
#include "lljoystickbutton.h"
#include "llselectmgr.h"
#include "llsmoothstep.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwindow.h"
#include "llworld.h"

using namespace LLAvatarAppearanceDefines;

extern LLMenuBarGL* gMenuBarView;

// Mousewheel camera zoom
const F32 MIN_ZOOM_FRACTION = 0.25f;
const F32 INITIAL_ZOOM_FRACTION = 1.f;
const F32 MAX_ZOOM_FRACTION = 8.f;

const F32 CAMERA_ZOOM_HALF_LIFE = 0.07f;	// seconds
const F32 FOV_ZOOM_HALF_LIFE = 0.07f;	// seconds

const F32 CAMERA_FOCUS_HALF_LIFE = 0.f;//0.02f;
const F32 CAMERA_LAG_HALF_LIFE = 0.25f;
const F32 MIN_CAMERA_LAG = 0.5f;
const F32 MAX_CAMERA_LAG = 5.f;

const F32 CAMERA_COLLIDE_EPSILON = 0.1f;
const F32 MIN_CAMERA_DISTANCE = 0.1f;

const F32 AVATAR_ZOOM_MIN_X_FACTOR = 0.55f;
const F32 AVATAR_ZOOM_MIN_Y_FACTOR = 0.7f;
const F32 AVATAR_ZOOM_MIN_Z_FACTOR = 1.15f;

const F32 MAX_CAMERA_DISTANCE_FROM_AGENT = 50.f;

const F32 MAX_CAMERA_SMOOTH_DISTANCE = 50.0f;

const F32 HEAD_BUFFER_SIZE = 0.3f;

const F32 CUSTOMIZE_AVATAR_CAMERA_ANIM_SLOP = 0.1f;

const F32 LAND_MIN_ZOOM = 0.15f;

const F32 AVATAR_MIN_ZOOM = 0.5f;
const F32 OBJECT_MIN_ZOOM = 0.02f;

const F32 APPEARANCE_MIN_ZOOM = 0.39f;
const F32 APPEARANCE_MAX_ZOOM = 8.f;

const F32 CUSTOMIZE_AVATAR_CAMERA_DEFAULT_DIST = 3.5f;

const F32 GROUND_TO_AIR_CAMERA_TRANSITION_TIME = 0.5f;
const F32 GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME = 0.5f;

const F32 OBJECT_EXTENTS_PADDING = 0.5f;

// The agent instance.
LLAgentCamera gAgentCamera;

//-----------------------------------------------------------------------------
// LLAgentCamera()
//-----------------------------------------------------------------------------
LLAgentCamera::LLAgentCamera() :
	mInitialized(false),

	mDrawDistance( DEFAULT_FAR_PLANE ),

	mLookAt(NULL),
	mPointAt(NULL),

	mHUDTargetZoom(1.f),
	mHUDCurZoom(1.f),

	mForceMouselook(FALSE),

	mCameraMode( CAMERA_MODE_THIRD_PERSON ),
	mLastCameraMode( CAMERA_MODE_THIRD_PERSON ),

	mCameraPreset(CAMERA_PRESET_REAR_VIEW),

	mCameraAnimating( FALSE ),
	mAnimationCameraStartGlobal(),
	mAnimationFocusStartGlobal(),
	mAnimationTimer(),
	mAnimationDuration(0.33f),
	
	mCameraFOVZoomFactor(0.f),
	mCameraCurrentFOVZoomFactor(0.f),
	mCameraFocusOffset(),
	mCameraFOVDefault(DEFAULT_FIELD_OF_VIEW),

	mCameraCollidePlane(),

	mCurrentCameraDistance(2.f),		// meters, set in init()
	mTargetCameraDistance(2.f),
	mCameraZoomFraction(1.f),			// deprecated
	mThirdPersonHeadOffset(0.f, 0.f, 1.f),
	mSitCameraEnabled(FALSE),
	mCameraSmoothingLastPositionGlobal(),
	mCameraSmoothingLastPositionAgent(),
	mCameraSmoothingStop(false),

	mCameraUpVector(LLVector3::z_axis), // default is straight up

	mFocusOnAvatar(TRUE),
	mFocusGlobal(),
	mFocusTargetGlobal(),
	mFocusObject(NULL),
	mFocusObjectDist(0.f),
	mFocusObjectOffset(),
	mFocusDotRadius( 0.1f ),			// meters
	mTrackFocusObject(TRUE),

	mAtKey(0), // Either 1, 0, or -1... indicates that movement-key is pressed
	mWalkKey(0), // like AtKey, but causes less forward thrust
	mLeftKey(0),
	mUpKey(0),
	mYawKey(0.f),
	mPitchKey(0.f),

	mOrbitLeftKey(0.f),
	mOrbitRightKey(0.f),
	mOrbitUpKey(0.f),
	mOrbitDownKey(0.f),
	mOrbitInKey(0.f),
	mOrbitOutKey(0.f),

	mPanUpKey(0.f),
	mPanDownKey(0.f),
	mPanLeftKey(0.f),
	mPanRightKey(0.f),
	mPanInKey(0.f),
	mPanOutKey(0.f)
{
	mFollowCam.setMaxCameraDistantFromSubject( MAX_CAMERA_DISTANCE_FROM_AGENT );

	clearGeneralKeys();
	clearOrbitKeys();
	clearPanKeys();
}

// Requires gSavedSettings to be initialized.
//-----------------------------------------------------------------------------
// init()
//-----------------------------------------------------------------------------
void LLAgentCamera::init()
{
	// *Note: this is where LLViewerCamera::getInstance() used to be constructed.

	mDrawDistance = gSavedSettings.getF32("RenderFarClip");

	LLViewerCamera::getInstance()->setView(DEFAULT_FIELD_OF_VIEW);
	// Leave at 0.1 meters until we have real near clip management
	LLViewerCamera::getInstance()->setNear(0.1f);
	LLViewerCamera::getInstance()->setFar(mDrawDistance);			// if you want to change camera settings, do so in camera.h
	LLViewerCamera::getInstance()->setAspect( gViewerWindow->getWorldViewAspectRatio() );		// default, overridden in LLViewerWindow::reshape
	LLViewerCamera::getInstance()->setViewHeightInPixels(768);			// default, overridden in LLViewerWindow::reshape

	mCameraFocusOffsetTarget = LLVector4(gSavedSettings.getVector3("CameraOffsetBuild"));
	
	mCameraPreset = (ECameraPreset) gSavedSettings.getU32("CameraPreset");

	mCameraOffsetInitial[CAMERA_PRESET_REAR_VIEW] = gSavedSettings.getControl("CameraOffsetRearView");
	mCameraOffsetInitial[CAMERA_PRESET_FRONT_VIEW] = gSavedSettings.getControl("CameraOffsetFrontView");
	mCameraOffsetInitial[CAMERA_PRESET_GROUP_VIEW] = gSavedSettings.getControl("CameraOffsetGroupView");

	mFocusOffsetInitial[CAMERA_PRESET_REAR_VIEW] = gSavedSettings.getControl("FocusOffsetRearView");
	mFocusOffsetInitial[CAMERA_PRESET_FRONT_VIEW] = gSavedSettings.getControl("FocusOffsetFrontView");
	mFocusOffsetInitial[CAMERA_PRESET_GROUP_VIEW] = gSavedSettings.getControl("FocusOffsetGroupView");

	mCameraCollidePlane.clearVec();
	mCurrentCameraDistance = getCameraOffsetInitial().magVec() * gSavedSettings.getF32("CameraOffsetScale");
	mTargetCameraDistance = mCurrentCameraDistance;
	mCameraZoomFraction = 1.f;
	mTrackFocusObject = gSavedSettings.getBOOL("TrackFocusObject");

	mInitialized = true;
}

//-----------------------------------------------------------------------------
// cleanup()
//-----------------------------------------------------------------------------
void LLAgentCamera::cleanup()
{
	setSitCamera(LLUUID::null);

	if(mLookAt)
	{
		mLookAt->markDead() ;
		mLookAt = NULL;
	}
	if(mPointAt)
	{
		mPointAt->markDead() ;
		mPointAt = NULL;
	}
	setFocusObject(NULL);
}

void LLAgentCamera::setAvatarObject(LLVOAvatarSelf* avatar)
{
	if (!mLookAt)
	{
		mLookAt = (LLHUDEffectLookAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_LOOKAT);
	}
	if (!mPointAt)
	{
		mPointAt = (LLHUDEffectPointAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINTAT);
	}
	
	if (!mLookAt.isNull())
	{
		mLookAt->setSourceObject(avatar);
	}
	if (!mPointAt.isNull())
	{
		mPointAt->setSourceObject(avatar);
	}	
}

//-----------------------------------------------------------------------------
// LLAgent()
//-----------------------------------------------------------------------------
LLAgentCamera::~LLAgentCamera()
{
	cleanup();

	// *Note: this is where LLViewerCamera::getInstance() used to be deleted.
}

// Change camera back to third person, stop the autopilot,
// deselect stuff, etc.
//-----------------------------------------------------------------------------
// resetView()
//-----------------------------------------------------------------------------
void LLAgentCamera::resetView(BOOL reset_camera, BOOL change_camera)
{
	if (gAgent.getAutoPilot())
	{
		gAgent.stopAutoPilot(TRUE);
	}

	LLSelectMgr::getInstance()->unhighlightAll();

	// By popular request, keep land selection while walking around. JC
	// LLViewerParcelMgr::getInstance()->deselectLand();

	// force deselect when walking and attachment is selected
	// this is so people don't wig out when their avatar moves without animating
	if (LLSelectMgr::getInstance()->getSelection()->isAttachment())
	{
		LLSelectMgr::getInstance()->deselectAll();
	}

	if (gMenuHolder != NULL)
	{
		// Hide all popup menus
		gMenuHolder->hideMenus();
	}

	if (change_camera && !gSavedSettings.getBOOL("FreezeTime"))
	{
		changeCameraToDefault();
		
		if (LLViewerJoystick::getInstance()->getOverrideCamera())
		{
			handle_toggle_flycam();
		}

		// reset avatar mode from eventual residual motion
		if (LLToolMgr::getInstance()->inBuildMode())
		{
			LLViewerJoystick::getInstance()->moveAvatar(true);
		}

		//Camera Tool is needed for Free Camera Control Mode
		if (!LLFloaterCamera::inFreeCameraMode())
		{
			LLFloaterReg::hideInstance("build");

			// Switch back to basic toolset
			LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		}
		
		gViewerWindow->showCursor();
	}


	if (reset_camera && !gSavedSettings.getBOOL("FreezeTime"))
	{
		if (!gViewerWindow->getLeftMouseDown() && cameraThirdPerson())
		{
			// leaving mouse-steer mode
			LLVector3 agent_at_axis = gAgent.getAtAxis();
			agent_at_axis -= projected_vec(agent_at_axis, gAgent.getReferenceUpVector());
			agent_at_axis.normalize();
			gAgent.resetAxes(lerp(gAgent.getAtAxis(), agent_at_axis, LLCriticalDamp::getInterpolant(0.3f)));
		}

		setFocusOnAvatar(TRUE, ANIMATE);

		mCameraFOVZoomFactor = 0.f;
	}

	mHUDTargetZoom = 1.f;
}

// Allow camera to be moved somewhere other than behind avatar.
//-----------------------------------------------------------------------------
// unlockView()
//-----------------------------------------------------------------------------
void LLAgentCamera::unlockView()
{
	if (getFocusOnAvatar())
	{
		if (isAgentAvatarValid())
		{
			setFocusGlobal(LLVector3d::zero, gAgentAvatarp->mID);
		}
		setFocusOnAvatar(FALSE, FALSE);	// no animation
	}
}

//-----------------------------------------------------------------------------
// slamLookAt()
//-----------------------------------------------------------------------------
void LLAgentCamera::slamLookAt(const LLVector3 &look_at)
{
	LLVector3 look_at_norm = look_at;
	look_at_norm.mV[VZ] = 0.f;
	look_at_norm.normalize();
	gAgent.resetAxes(look_at_norm);
}

//-----------------------------------------------------------------------------
// calcFocusOffset()
//-----------------------------------------------------------------------------
LLVector3 LLAgentCamera::calcFocusOffset(LLViewerObject *object, LLVector3 original_focus_point, S32 x, S32 y)
{
	LLMatrix4 obj_matrix = object->getRenderMatrix();
	LLQuaternion obj_rot = object->getRenderRotation();
	LLVector3 obj_pos = object->getRenderPosition();

	BOOL is_avatar = object->isAvatar();
	// if is avatar - don't do any funk heuristics to position the focal point
	// see DEV-30589
	if (is_avatar)
	{
		return original_focus_point - obj_pos;
	}
	
	LLQuaternion inv_obj_rot = ~obj_rot; // get inverse of rotation
	LLVector3 object_extents = object->getScale();	
	
	// make sure they object extents are non-zero
	object_extents.clamp(0.001f, F32_MAX);

	// obj_to_cam_ray is unit vector pointing from object center to camera, in the coordinate frame of the object
	LLVector3 obj_to_cam_ray = obj_pos - LLViewerCamera::getInstance()->getOrigin();
	obj_to_cam_ray.rotVec(inv_obj_rot);
	obj_to_cam_ray.normalize();

	// obj_to_cam_ray_proportions are the (positive) ratios of 
	// the obj_to_cam_ray x,y,z components with the x,y,z object dimensions.
	LLVector3 obj_to_cam_ray_proportions;
	obj_to_cam_ray_proportions.mV[VX] = llabs(obj_to_cam_ray.mV[VX] / object_extents.mV[VX]);
	obj_to_cam_ray_proportions.mV[VY] = llabs(obj_to_cam_ray.mV[VY] / object_extents.mV[VY]);
	obj_to_cam_ray_proportions.mV[VZ] = llabs(obj_to_cam_ray.mV[VZ] / object_extents.mV[VZ]);

	// find the largest ratio stored in obj_to_cam_ray_proportions
	// this corresponds to the object's local axial plane (XY, YZ, XZ) that is *most* facing the camera
	LLVector3 longest_object_axis;
	// is x-axis longest?
	if (obj_to_cam_ray_proportions.mV[VX] > obj_to_cam_ray_proportions.mV[VY] 
		&& obj_to_cam_ray_proportions.mV[VX] > obj_to_cam_ray_proportions.mV[VZ])
	{
		// then grab it
		longest_object_axis.setVec(obj_matrix.getFwdRow4());
	}
	// is y-axis longest?
	else if (obj_to_cam_ray_proportions.mV[VY] > obj_to_cam_ray_proportions.mV[VZ])
	{
		// then grab it
		longest_object_axis.setVec(obj_matrix.getLeftRow4());
	}
	// otherwise, use z axis
	else
	{
		longest_object_axis.setVec(obj_matrix.getUpRow4());
	}

	// Use this axis as the normal to project mouse click on to plane with that normal, at the object center.
	// This generates a point behind the mouse cursor that is approximately in the middle of the object in
	// terms of depth.  
	// We do this to allow the camera rotation tool to "tumble" the object by rotating the camera.
	// If the focus point were the object surface under the mouse, camera rotation would introduce an undesirable
	// eccentricity to the object orientation
	LLVector3 focus_plane_normal(longest_object_axis);
	focus_plane_normal.normalize();

	LLVector3d focus_pt_global;
	gViewerWindow->mousePointOnPlaneGlobal(focus_pt_global, x, y, gAgent.getPosGlobalFromAgent(obj_pos), focus_plane_normal);
	LLVector3 focus_pt = gAgent.getPosAgentFromGlobal(focus_pt_global);

	// find vector from camera to focus point in object space
	LLVector3 camera_to_focus_vec = focus_pt - LLViewerCamera::getInstance()->getOrigin();
	camera_to_focus_vec.rotVec(inv_obj_rot);

	// find vector from object origin to focus point in object coordinates
	LLVector3 focus_offset_from_object_center = focus_pt - obj_pos;
	// convert to object-local space
	focus_offset_from_object_center.rotVec(inv_obj_rot);

	// We need to project the focus point back into the bounding box of the focused object.
	// Do this by calculating the XYZ scale factors needed to get focus offset back in bounds along the camera_focus axis
	LLVector3 clip_fraction;

	// for each axis...
	for (U32 axis = VX; axis <= VZ; axis++)
	{
		//...calculate distance that focus offset sits outside of bounding box along that axis...
		//NOTE: dist_out_of_bounds keeps the sign of focus_offset_from_object_center 
		F32 dist_out_of_bounds;
		if (focus_offset_from_object_center.mV[axis] > 0.f)
		{
			dist_out_of_bounds = llmax(0.f, focus_offset_from_object_center.mV[axis] - (object_extents.mV[axis] * 0.5f));
		}
		else
		{
			dist_out_of_bounds = llmin(0.f, focus_offset_from_object_center.mV[axis] + (object_extents.mV[axis] * 0.5f));
		}

		//...then calculate the scale factor needed to push camera_to_focus_vec back in bounds along current axis
		if (llabs(camera_to_focus_vec.mV[axis]) < 0.0001f)
		{
			// don't divide by very small number
			clip_fraction.mV[axis] = 0.f;
		}
		else
		{
			clip_fraction.mV[axis] = dist_out_of_bounds / camera_to_focus_vec.mV[axis];
		}
	}

	LLVector3 abs_clip_fraction = clip_fraction;
	abs_clip_fraction.abs();

	// find axis of focus offset that is *most* outside the bounding box and use that to
	// rescale focus offset to inside object extents
	if (abs_clip_fraction.mV[VX] > abs_clip_fraction.mV[VY]
		&& abs_clip_fraction.mV[VX] > abs_clip_fraction.mV[VZ])
	{
		focus_offset_from_object_center -= clip_fraction.mV[VX] * camera_to_focus_vec;
	}
	else if (abs_clip_fraction.mV[VY] > abs_clip_fraction.mV[VZ])
	{
		focus_offset_from_object_center -= clip_fraction.mV[VY] * camera_to_focus_vec;
	}
	else
	{
		focus_offset_from_object_center -= clip_fraction.mV[VZ] * camera_to_focus_vec;
	}

	// convert back to world space
	focus_offset_from_object_center.rotVec(obj_rot);
	
	// now, based on distance of camera from object relative to object size
	// push the focus point towards the near surface of the object when (relatively) close to the objcet
	// or keep the focus point in the object middle when (relatively) far
	// NOTE: leave focus point in middle of avatars, since the behavior you want when alt-zooming on avatars
	// is almost always "tumble about middle" and not "spin around surface point"
	if (!is_avatar) 
	{
		LLVector3 obj_rel = original_focus_point - object->getRenderPosition();
		
		//now that we have the object relative position, we should bias toward the center of the object 
		//based on the distance of the camera to the focus point vs. the distance of the camera to the focus

		F32 relDist = llabs(obj_rel * LLViewerCamera::getInstance()->getAtAxis());
		F32 viewDist = dist_vec(obj_pos + obj_rel, LLViewerCamera::getInstance()->getOrigin());


		LLBBox obj_bbox = object->getBoundingBoxAgent();
		F32 bias = 0.f;

		// virtual_camera_pos is the camera position we are simulating by backing the camera off
		// and adjusting the FOV
		LLVector3 virtual_camera_pos = gAgent.getPosAgentFromGlobal(mFocusTargetGlobal + (getCameraPositionGlobal() - mFocusTargetGlobal) / (1.f + mCameraFOVZoomFactor));

		// if the camera is inside the object (large, hollow objects, for example)
		// leave focus point all the way to destination depth, away from object center
		if(!obj_bbox.containsPointAgent(virtual_camera_pos))
		{
			// perform magic number biasing of focus point towards surface vs. planar center
			bias = clamp_rescale(relDist/viewDist, 0.1f, 0.7f, 0.0f, 1.0f);
			obj_rel = lerp(focus_offset_from_object_center, obj_rel, bias);
		}
			
		focus_offset_from_object_center = obj_rel;
	}

	return focus_offset_from_object_center;
}

//-----------------------------------------------------------------------------
// calcCameraMinDistance()
//-----------------------------------------------------------------------------
BOOL LLAgentCamera::calcCameraMinDistance(F32 &obj_min_distance)
{
	BOOL soft_limit = FALSE; // is the bounding box to be treated literally (volumes) or as an approximation (avatars)

	if (!mFocusObject || mFocusObject->isDead() || 
		mFocusObject->isMesh() ||
		gSavedSettings.getBOOL("DisableCameraConstraints"))
	{
		obj_min_distance = 0.f;
		return TRUE;
	}

	if (mFocusObject->mDrawable.isNull())
	{
#ifdef LL_RELEASE_FOR_DOWNLOAD
		llwarns << "Focus object with no drawable!" << llendl;
#else
		mFocusObject->dump();
		llerrs << "Focus object with no drawable!" << llendl;
#endif
		obj_min_distance = 0.f;
		return TRUE;
	}
	
	LLQuaternion inv_object_rot = ~mFocusObject->getRenderRotation();
	LLVector3 target_offset_origin = mFocusObjectOffset;
	LLVector3 camera_offset_target(getCameraPositionAgent() - gAgent.getPosAgentFromGlobal(mFocusTargetGlobal));

	// convert offsets into object local space
	camera_offset_target.rotVec(inv_object_rot);
	target_offset_origin.rotVec(inv_object_rot);

	// push around object extents based on target offset
	LLVector3 object_extents = mFocusObject->getScale();
	if (mFocusObject->isAvatar())
	{
		// fudge factors that lets you zoom in on avatars a bit more (which don't do FOV zoom)
		object_extents.mV[VX] *= AVATAR_ZOOM_MIN_X_FACTOR;
		object_extents.mV[VY] *= AVATAR_ZOOM_MIN_Y_FACTOR;
		object_extents.mV[VZ] *= AVATAR_ZOOM_MIN_Z_FACTOR;
		soft_limit = TRUE;
	}
	LLVector3 abs_target_offset = target_offset_origin;
	abs_target_offset.abs();

	LLVector3 target_offset_dir = target_offset_origin;

	BOOL target_outside_object_extents = FALSE;

	for (U32 i = VX; i <= VZ; i++)
	{
		if (abs_target_offset.mV[i] * 2.f > object_extents.mV[i] + OBJECT_EXTENTS_PADDING)
		{
			target_outside_object_extents = TRUE;
		}
		if (camera_offset_target.mV[i] > 0.f)
		{
			object_extents.mV[i] -= target_offset_origin.mV[i] * 2.f;
		}
		else
		{
			object_extents.mV[i] += target_offset_origin.mV[i] * 2.f;
		}
	}

	// don't shrink the object extents so far that the object inverts
	object_extents.clamp(0.001f, F32_MAX);

	// move into first octant
	LLVector3 camera_offset_target_abs_norm = camera_offset_target;
	camera_offset_target_abs_norm.abs();
	// make sure offset is non-zero
	camera_offset_target_abs_norm.clamp(0.001f, F32_MAX);
	camera_offset_target_abs_norm.normalize();

	// find camera position relative to normalized object extents
	LLVector3 camera_offset_target_scaled = camera_offset_target_abs_norm;
	camera_offset_target_scaled.mV[VX] /= object_extents.mV[VX];
	camera_offset_target_scaled.mV[VY] /= object_extents.mV[VY];
	camera_offset_target_scaled.mV[VZ] /= object_extents.mV[VZ];

	if (camera_offset_target_scaled.mV[VX] > camera_offset_target_scaled.mV[VY] && 
		camera_offset_target_scaled.mV[VX] > camera_offset_target_scaled.mV[VZ])
	{
		if (camera_offset_target_abs_norm.mV[VX] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VX] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VX] * 0.5f / camera_offset_target_abs_norm.mV[VX];
		}
	}
	else if (camera_offset_target_scaled.mV[VY] > camera_offset_target_scaled.mV[VZ])
	{
		if (camera_offset_target_abs_norm.mV[VY] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VY] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VY] * 0.5f / camera_offset_target_abs_norm.mV[VY];
		}
	}
	else
	{
		if (camera_offset_target_abs_norm.mV[VZ] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VZ] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VZ] * 0.5f / camera_offset_target_abs_norm.mV[VZ];
		}
	}

	LLVector3 object_split_axis;
	LLVector3 target_offset_scaled = target_offset_origin;
	target_offset_scaled.abs();
	target_offset_scaled.normalize();
	target_offset_scaled.mV[VX] /= object_extents.mV[VX];
	target_offset_scaled.mV[VY] /= object_extents.mV[VY];
	target_offset_scaled.mV[VZ] /= object_extents.mV[VZ];

	if (target_offset_scaled.mV[VX] > target_offset_scaled.mV[VY] && 
		target_offset_scaled.mV[VX] > target_offset_scaled.mV[VZ])
	{
		object_split_axis = LLVector3::x_axis;
	}
	else if (target_offset_scaled.mV[VY] > target_offset_scaled.mV[VZ])
	{
		object_split_axis = LLVector3::y_axis;
	}
	else
	{
		object_split_axis = LLVector3::z_axis;
	}

	LLVector3 camera_offset_object(getCameraPositionAgent() - mFocusObject->getPositionAgent());


	F32 camera_offset_clip = camera_offset_object * object_split_axis;
	F32 target_offset_clip = target_offset_dir * object_split_axis;

	// target has moved outside of object extents
	// check to see if camera and target are on same side 
	if (target_outside_object_extents)
	{
		if (camera_offset_clip > 0.f && target_offset_clip > 0.f)
		{
			return FALSE;
		}
		else if (camera_offset_clip < 0.f && target_offset_clip < 0.f)
		{
			return FALSE;
		}
	}

	// clamp obj distance to diagonal of 10 by 10 cube
	obj_min_distance = llmin(obj_min_distance, 10.f * F_SQRT3);

	obj_min_distance += LLViewerCamera::getInstance()->getNear() + (soft_limit ? 0.1f : 0.2f);
	
	return TRUE;
}

F32 LLAgentCamera::getCameraZoomFraction()
{
	// 0.f -> camera zoomed all the way out
	// 1.f -> camera zoomed all the way in
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// already [0,1]
		return mHUDTargetZoom;
	}
	else if (mFocusOnAvatar && cameraThirdPerson())
	{
		return clamp_rescale(mCameraZoomFraction, MIN_ZOOM_FRACTION, MAX_ZOOM_FRACTION, 1.f, 0.f);
	}
	else if (cameraCustomizeAvatar())
	{
		F32 distance = (F32)mCameraFocusOffsetTarget.magVec();
		return clamp_rescale(distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM, 1.f, 0.f );
	}
	else
	{
		F32 min_zoom;
		const F32 DIST_FUDGE = 16.f; // meters
		F32 max_zoom = llmin(mDrawDistance - DIST_FUDGE, 
								LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE,
								MAX_CAMERA_DISTANCE_FROM_AGENT);

		F32 distance = (F32)mCameraFocusOffsetTarget.magVec();
		if (mFocusObject.notNull())
		{
			if (mFocusObject->isAvatar())
			{
				min_zoom = AVATAR_MIN_ZOOM;
			}
			else
			{
				min_zoom = OBJECT_MIN_ZOOM;
			}
		}
		else
		{
			min_zoom = LAND_MIN_ZOOM;
		}

		return clamp_rescale(distance, min_zoom, max_zoom, 1.f, 0.f);
	}
}

void LLAgentCamera::setCameraZoomFraction(F32 fraction)
{
	// 0.f -> camera zoomed all the way out
	// 1.f -> camera zoomed all the way in
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		mHUDTargetZoom = fraction;
	}
	else if (mFocusOnAvatar && cameraThirdPerson())
	{
		mCameraZoomFraction = rescale(fraction, 0.f, 1.f, MAX_ZOOM_FRACTION, MIN_ZOOM_FRACTION);
	}
	else if (cameraCustomizeAvatar())
	{
		LLVector3d camera_offset_dir = mCameraFocusOffsetTarget;
		camera_offset_dir.normalize();
		mCameraFocusOffsetTarget = camera_offset_dir * rescale(fraction, 0.f, 1.f, APPEARANCE_MAX_ZOOM, APPEARANCE_MIN_ZOOM);
	}
	else
	{
		F32 min_zoom = LAND_MIN_ZOOM;
		const F32 DIST_FUDGE = 16.f; // meters
		F32 max_zoom = llmin(mDrawDistance - DIST_FUDGE, 
								LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE,
								MAX_CAMERA_DISTANCE_FROM_AGENT);

		if (mFocusObject.notNull())
		{
			if (mFocusObject.notNull())
			{
				if (mFocusObject->isAvatar())
				{
					min_zoom = AVATAR_MIN_ZOOM;
				}
				else
				{
					min_zoom = OBJECT_MIN_ZOOM;
				}
			}
		}

		LLVector3d camera_offset_dir = mCameraFocusOffsetTarget;
		camera_offset_dir.normalize();
		mCameraFocusOffsetTarget = camera_offset_dir * rescale(fraction, 0.f, 1.f, max_zoom, min_zoom);
	}
	startCameraAnimation();
}


//-----------------------------------------------------------------------------
// cameraOrbitAround()
//-----------------------------------------------------------------------------
void LLAgentCamera::cameraOrbitAround(const F32 radians)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// do nothing for hud selection
	}
	else if (mFocusOnAvatar && (mCameraMode == CAMERA_MODE_THIRD_PERSON || mCameraMode == CAMERA_MODE_FOLLOW))
	{
		gAgent.yaw(radians);
	}
	else
	{
		mCameraFocusOffsetTarget.rotVec(radians, 0.f, 0.f, 1.f);
		
		cameraZoomIn(1.f);
	}
}


//-----------------------------------------------------------------------------
// cameraOrbitOver()
//-----------------------------------------------------------------------------
void LLAgentCamera::cameraOrbitOver(const F32 angle)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// do nothing for hud selection
	}
	else if (mFocusOnAvatar && mCameraMode == CAMERA_MODE_THIRD_PERSON)
	{
		gAgent.pitch(angle);
	}
	else
	{
		LLVector3 camera_offset_unit(mCameraFocusOffsetTarget);
		camera_offset_unit.normalize();

		F32 angle_from_up = acos( camera_offset_unit * gAgent.getReferenceUpVector() );

		LLVector3d left_axis;
		left_axis.setVec(LLViewerCamera::getInstance()->getLeftAxis());
		F32 new_angle = llclamp(angle_from_up - angle, 1.f * DEG_TO_RAD, 179.f * DEG_TO_RAD);
		mCameraFocusOffsetTarget.rotVec(angle_from_up - new_angle, left_axis);

		cameraZoomIn(1.f);
	}
}

//-----------------------------------------------------------------------------
// cameraZoomIn()
//-----------------------------------------------------------------------------
void LLAgentCamera::cameraZoomIn(const F32 fraction)
{
	if (gDisconnected)
	{
		return;
	}

	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// just update hud zoom level
		mHUDTargetZoom /= fraction;
		return;
	}


	LLVector3d	camera_offset(mCameraFocusOffsetTarget);
	LLVector3d	camera_offset_unit(mCameraFocusOffsetTarget);
	F32 min_zoom = LAND_MIN_ZOOM;
	F32 current_distance = (F32)camera_offset_unit.normalize();
	F32 new_distance = current_distance * fraction;

	// Don't move through focus point
	if (mFocusObject)
	{
		LLVector3 camera_offset_dir((F32)camera_offset_unit.mdV[VX], (F32)camera_offset_unit.mdV[VY], (F32)camera_offset_unit.mdV[VZ]);

		if (mFocusObject->isAvatar())
		{
			calcCameraMinDistance(min_zoom);
		}
		else
		{
			min_zoom = OBJECT_MIN_ZOOM;
		}
	}

	new_distance = llmax(new_distance, min_zoom); 

	// Don't zoom too far back
	const F32 DIST_FUDGE = 16.f; // meters
	F32 max_distance = llmin(mDrawDistance - DIST_FUDGE, 
							 LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE );

	if (new_distance > max_distance)
	{
		new_distance = max_distance;

		/*
		// Unless camera is unlocked
		if (!LLViewerCamera::sDisableCameraConstraints)
		{
			return;
		}
		*/
	}

	if(cameraCustomizeAvatar())
	{
		new_distance = llclamp( new_distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM );
	}

	mCameraFocusOffsetTarget = new_distance * camera_offset_unit;
}

//-----------------------------------------------------------------------------
// cameraOrbitIn()
//-----------------------------------------------------------------------------
void LLAgentCamera::cameraOrbitIn(const F32 meters)
{
	if (mFocusOnAvatar && mCameraMode == CAMERA_MODE_THIRD_PERSON)
	{
		F32 camera_offset_dist = llmax(0.001f, getCameraOffsetInitial().magVec() * gSavedSettings.getF32("CameraOffsetScale"));
		
		mCameraZoomFraction = (mTargetCameraDistance - meters) / camera_offset_dist;

		if (!gSavedSettings.getBOOL("FreezeTime") && mCameraZoomFraction < MIN_ZOOM_FRACTION && meters > 0.f)
		{
			// No need to animate, camera is already there.
			changeCameraToMouselook(FALSE);
		}

		mCameraZoomFraction = llclamp(mCameraZoomFraction, MIN_ZOOM_FRACTION, MAX_ZOOM_FRACTION);
	}
	else
	{
		LLVector3d	camera_offset(mCameraFocusOffsetTarget);
		LLVector3d	camera_offset_unit(mCameraFocusOffsetTarget);
		F32 current_distance = (F32)camera_offset_unit.normalize();
		F32 new_distance = current_distance - meters;
		F32 min_zoom = LAND_MIN_ZOOM;
		
		// Don't move through focus point
		if (mFocusObject.notNull())
		{
			if (mFocusObject->isAvatar())
			{
				min_zoom = AVATAR_MIN_ZOOM;
			}
			else
			{
				min_zoom = OBJECT_MIN_ZOOM;
			}
		}

		new_distance = llmax(new_distance, min_zoom);

		// Don't zoom too far back
		const F32 DIST_FUDGE = 16.f; // meters
		F32 max_distance = llmin(mDrawDistance - DIST_FUDGE, 
								 LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE );

		if (new_distance > max_distance)
		{
			// Unless camera is unlocked
			if (!gSavedSettings.getBOOL("DisableCameraConstraints"))
			{
				return;
			}
		}

		if( CAMERA_MODE_CUSTOMIZE_AVATAR == getCameraMode() )
		{
			new_distance = llclamp( new_distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM );
		}

		// Compute new camera offset
		mCameraFocusOffsetTarget = new_distance * camera_offset_unit;
		cameraZoomIn(1.f);
	}
}


//-----------------------------------------------------------------------------
// cameraPanIn()
//-----------------------------------------------------------------------------
void LLAgentCamera::cameraPanIn(F32 meters)
{
	LLVector3d at_axis;
	at_axis.setVec(LLViewerCamera::getInstance()->getAtAxis());

	mFocusTargetGlobal += meters * at_axis;
	mFocusGlobal = mFocusTargetGlobal;
	// don't enforce zoom constraints as this is the only way for users to get past them easily
	updateFocusOffset();
	// NOTE: panning movements expect the camera to move exactly with the focus target, not animated behind -Nyx
	mCameraSmoothingLastPositionGlobal = calcCameraPositionTargetGlobal();
}

//-----------------------------------------------------------------------------
// cameraPanLeft()
//-----------------------------------------------------------------------------
void LLAgentCamera::cameraPanLeft(F32 meters)
{
	LLVector3d left_axis;
	left_axis.setVec(LLViewerCamera::getInstance()->getLeftAxis());

	mFocusTargetGlobal += meters * left_axis;
	mFocusGlobal = mFocusTargetGlobal;

	// disable smoothing for camera pan, which causes some residents unhappiness
	mCameraSmoothingStop = true;
	
	cameraZoomIn(1.f);
	updateFocusOffset();
	// NOTE: panning movements expect the camera to move exactly with the focus target, not animated behind - Nyx
	mCameraSmoothingLastPositionGlobal = calcCameraPositionTargetGlobal();
}

//-----------------------------------------------------------------------------
// cameraPanUp()
//-----------------------------------------------------------------------------
void LLAgentCamera::cameraPanUp(F32 meters)
{
	LLVector3d up_axis;
	up_axis.setVec(LLViewerCamera::getInstance()->getUpAxis());

	mFocusTargetGlobal += meters * up_axis;
	mFocusGlobal = mFocusTargetGlobal;

	// disable smoothing for camera pan, which causes some residents unhappiness
	mCameraSmoothingStop = true;

	cameraZoomIn(1.f);
	updateFocusOffset();
	// NOTE: panning movements expect the camera to move exactly with the focus target, not animated behind -Nyx
	mCameraSmoothingLastPositionGlobal = calcCameraPositionTargetGlobal();
}

//-----------------------------------------------------------------------------
// updateLookAt()
//-----------------------------------------------------------------------------
void LLAgentCamera::updateLookAt(const S32 mouse_x, const S32 mouse_y)
{
	static LLVector3 last_at_axis;

	if (!isAgentAvatarValid()) return;

	LLQuaternion av_inv_rot = ~gAgentAvatarp->mRoot->getWorldRotation();
	LLVector3 root_at = LLVector3::x_axis * gAgentAvatarp->mRoot->getWorldRotation();

	if 	((gViewerWindow->getMouseVelocityStat()->getCurrent() < 0.01f) &&
		 (root_at * last_at_axis > 0.95f))
	{
		LLVector3 vel = gAgentAvatarp->getVelocity();
		if (vel.magVecSquared() > 4.f)
		{
			setLookAt(LOOKAT_TARGET_IDLE, gAgentAvatarp, vel * av_inv_rot);
		}
		else
		{
			// *FIX: rotate mframeagent by sit object's rotation?
			LLQuaternion look_rotation = gAgentAvatarp->isSitting() ? gAgentAvatarp->getRenderRotation() : gAgent.getFrameAgent().getQuaternion(); // use camera's current rotation
			LLVector3 look_offset = LLVector3(2.f, 0.f, 0.f) * look_rotation * av_inv_rot;
			setLookAt(LOOKAT_TARGET_IDLE, gAgentAvatarp, look_offset);
		}
		last_at_axis = root_at;
		return;
	}

	last_at_axis = root_at;
	
	if (CAMERA_MODE_CUSTOMIZE_AVATAR == getCameraMode())
	{
		setLookAt(LOOKAT_TARGET_NONE, gAgentAvatarp, LLVector3(-2.f, 0.f, 0.f));	
	}
	else
	{
		// Move head based on cursor position
		ELookAtType lookAtType = LOOKAT_TARGET_NONE;
		LLVector3 headLookAxis;
		LLCoordFrame frameCamera = *((LLCoordFrame*)LLViewerCamera::getInstance());

		if (cameraMouselook())
		{
			lookAtType = LOOKAT_TARGET_MOUSELOOK;
		}
		else if (cameraThirdPerson())
		{
			// range from -.5 to .5
			F32 x_from_center = 
				((F32) mouse_x / (F32) gViewerWindow->getWorldViewWidthScaled() ) - 0.5f;
			F32 y_from_center = 
				((F32) mouse_y / (F32) gViewerWindow->getWorldViewHeightScaled() ) - 0.5f;

			frameCamera.yaw( - x_from_center * gSavedSettings.getF32("YawFromMousePosition") * DEG_TO_RAD);
			frameCamera.pitch( - y_from_center * gSavedSettings.getF32("PitchFromMousePosition") * DEG_TO_RAD);
			lookAtType = LOOKAT_TARGET_FREELOOK;
		}

		headLookAxis = frameCamera.getAtAxis();
		// RN: we use world-space offset for mouselook and freelook
		//headLookAxis = headLookAxis * av_inv_rot;
		setLookAt(lookAtType, gAgentAvatarp, headLookAxis);
	}
}

//-----------------------------------------------------------------------------
// updateCamera()
//-----------------------------------------------------------------------------
void LLAgentCamera::updateCamera()
{
	static LLFastTimer::DeclareTimer ftm("Camera");
	LLFastTimer t(ftm);

	// - changed camera_skyward to the new global "mCameraUpVector"
	mCameraUpVector = LLVector3::z_axis;
	//LLVector3	camera_skyward(0.f, 0.f, 1.f);

	U32 camera_mode = mCameraAnimating ? mLastCameraMode : mCameraMode;

	validateFocusObject();

	if (isAgentAvatarValid() && 
		gAgentAvatarp->isSitting() &&
		camera_mode == CAMERA_MODE_MOUSELOOK)
	{
		//changed camera_skyward to the new global "mCameraUpVector"
		mCameraUpVector = mCameraUpVector * gAgentAvatarp->getRenderRotation();
	}

	if (cameraThirdPerson() && mFocusOnAvatar && LLFollowCamMgr::getActiveFollowCamParams())
	{
		changeCameraToFollow();
	}

	//NOTE - this needs to be integrated into a general upVector system here within llAgent. 
	if ( camera_mode == CAMERA_MODE_FOLLOW && mFocusOnAvatar )
	{
		mCameraUpVector = mFollowCam.getUpVector();
	}

	if (mSitCameraEnabled)
	{
		if (mSitCameraReferenceObject->isDead())
		{
			setSitCamera(LLUUID::null);
		}
	}

	// Update UI with our camera inputs
	LLFloaterCamera* camera_floater = LLFloaterReg::findTypedInstance<LLFloaterCamera>("camera");
	if (camera_floater)
	{
		camera_floater->mRotate->setToggleState(gAgentCamera.getOrbitRightKey() > 0.f,	// left
												gAgentCamera.getOrbitUpKey() > 0.f,		// top
												gAgentCamera.getOrbitLeftKey() > 0.f,	// right
												gAgentCamera.getOrbitDownKey() > 0.f);	// bottom
		
		camera_floater->mTrack->setToggleState(gAgentCamera.getPanLeftKey() > 0.f,		// left
											   gAgentCamera.getPanUpKey() > 0.f,			// top
											   gAgentCamera.getPanRightKey() > 0.f,		// right
											   gAgentCamera.getPanDownKey() > 0.f);		// bottom
	}

	// Handle camera movement based on keyboard.
	const F32 ORBIT_OVER_RATE = 90.f * DEG_TO_RAD;			// radians per second
	const F32 ORBIT_AROUND_RATE = 90.f * DEG_TO_RAD;		// radians per second
	const F32 PAN_RATE = 5.f;								// meters per second

	if (gAgentCamera.getOrbitUpKey() || gAgentCamera.getOrbitDownKey())
	{
		F32 input_rate = gAgentCamera.getOrbitUpKey() - gAgentCamera.getOrbitDownKey();
		cameraOrbitOver( input_rate * ORBIT_OVER_RATE / gFPSClamped );
	}

	if (gAgentCamera.getOrbitLeftKey() || gAgentCamera.getOrbitRightKey())
	{
		F32 input_rate = gAgentCamera.getOrbitLeftKey() - gAgentCamera.getOrbitRightKey();
		cameraOrbitAround(input_rate * ORBIT_AROUND_RATE / gFPSClamped);
	}

	if (gAgentCamera.getOrbitInKey() || gAgentCamera.getOrbitOutKey())
	{
		F32 input_rate = gAgentCamera.getOrbitInKey() - gAgentCamera.getOrbitOutKey();
		
		LLVector3d to_focus = gAgent.getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin()) - calcFocusPositionTargetGlobal();
		F32 distance_to_focus = (F32)to_focus.magVec();
		// Move at distance (in meters) meters per second
		cameraOrbitIn( input_rate * distance_to_focus / gFPSClamped );
	}

	if (gAgentCamera.getPanInKey() || gAgentCamera.getPanOutKey())
	{
		F32 input_rate = gAgentCamera.getPanInKey() - gAgentCamera.getPanOutKey();
		cameraPanIn(input_rate * PAN_RATE / gFPSClamped);
	}

	if (gAgentCamera.getPanRightKey() || gAgentCamera.getPanLeftKey())
	{
		F32 input_rate = gAgentCamera.getPanRightKey() - gAgentCamera.getPanLeftKey();
		cameraPanLeft(input_rate * -PAN_RATE / gFPSClamped );
	}

	if (gAgentCamera.getPanUpKey() || gAgentCamera.getPanDownKey())
	{
		F32 input_rate = gAgentCamera.getPanUpKey() - gAgentCamera.getPanDownKey();
		cameraPanUp(input_rate * PAN_RATE / gFPSClamped );
	}

	// Clear camera keyboard keys.
	gAgentCamera.clearOrbitKeys();
	gAgentCamera.clearPanKeys();

	// lerp camera focus offset
	mCameraFocusOffset = lerp(mCameraFocusOffset, mCameraFocusOffsetTarget, LLCriticalDamp::getInterpolant(CAMERA_FOCUS_HALF_LIFE));

	if ( mCameraMode == CAMERA_MODE_FOLLOW )
	{
		if (isAgentAvatarValid())
		{
			//--------------------------------------------------------------------------------
			// this is where the avatar's position and rotation are given to followCam, and 
			// where it is updated. All three of its attributes are updated: (1) position, 
			// (2) focus, and (3) upvector. They can then be queried elsewhere in llAgent.
			//--------------------------------------------------------------------------------
			// *TODO: use combined rotation of frameagent and sit object
			LLQuaternion avatarRotationForFollowCam = gAgentAvatarp->isSitting() ? gAgentAvatarp->getRenderRotation() : gAgent.getFrameAgent().getQuaternion();

			LLFollowCamParams* current_cam = LLFollowCamMgr::getActiveFollowCamParams();
			if (current_cam)
			{
				mFollowCam.copyParams(*current_cam);
				mFollowCam.setSubjectPositionAndRotation( gAgentAvatarp->getRenderPosition(), avatarRotationForFollowCam );
				mFollowCam.update();
				LLViewerJoystick::getInstance()->setCameraNeedsUpdate(true);
			}
			else
			{
				changeCameraToThirdPerson(TRUE);
			}
		}
	}

	BOOL hit_limit;
	LLVector3d camera_pos_global;
	LLVector3d camera_target_global = calcCameraPositionTargetGlobal(&hit_limit);
	mCameraVirtualPositionAgent = gAgent.getPosAgentFromGlobal(camera_target_global);
	LLVector3d focus_target_global = calcFocusPositionTargetGlobal();

	// perform field of view correction
	mCameraFOVZoomFactor = calcCameraFOVZoomFactor();
	camera_target_global = focus_target_global + (camera_target_global - focus_target_global) * (1.f + mCameraFOVZoomFactor);

	gAgent.setShowAvatar(TRUE); // can see avatar by default

	// Adjust position for animation
	if (mCameraAnimating)
	{
		F32 time = mAnimationTimer.getElapsedTimeF32();

		// yet another instance of critically damped motion, hooray!
		// F32 fraction_of_animation = 1.f - pow(2.f, -time / CAMERA_ZOOM_HALF_LIFE);

		// linear interpolation
		F32 fraction_of_animation = time / mAnimationDuration;

		BOOL isfirstPerson = mCameraMode == CAMERA_MODE_MOUSELOOK;
		BOOL wasfirstPerson = mLastCameraMode == CAMERA_MODE_MOUSELOOK;
		F32 fraction_animation_to_skip;

		if (mAnimationCameraStartGlobal == camera_target_global)
		{
			fraction_animation_to_skip = 0.f;
		}
		else
		{
			LLVector3d cam_delta = mAnimationCameraStartGlobal - camera_target_global;
			fraction_animation_to_skip = HEAD_BUFFER_SIZE / (F32)cam_delta.magVec();
		}
		F32 animation_start_fraction = (wasfirstPerson) ? fraction_animation_to_skip : 0.f;
		F32 animation_finish_fraction =  (isfirstPerson) ? (1.f - fraction_animation_to_skip) : 1.f;
	
		if (fraction_of_animation < animation_finish_fraction)
		{
			if (fraction_of_animation < animation_start_fraction || fraction_of_animation > animation_finish_fraction )
			{
				gAgent.setShowAvatar(FALSE);
			}

			// ...adjust position for animation
			F32 smooth_fraction_of_animation = llsmoothstep(0.0f, 1.0f, fraction_of_animation);
			camera_pos_global = lerp(mAnimationCameraStartGlobal, camera_target_global, smooth_fraction_of_animation);
			mFocusGlobal = lerp(mAnimationFocusStartGlobal, focus_target_global, smooth_fraction_of_animation);
		}
		else
		{
			// ...animation complete
			mCameraAnimating = FALSE;

			camera_pos_global = camera_target_global;
			mFocusGlobal = focus_target_global;

			gAgent.endAnimationUpdateUI();
			gAgent.setShowAvatar(TRUE);
		}

		if (isAgentAvatarValid() && (mCameraMode != CAMERA_MODE_MOUSELOOK))
		{
			gAgentAvatarp->updateAttachmentVisibility(mCameraMode);
		}
	}
	else 
	{
		camera_pos_global = camera_target_global;
		mFocusGlobal = focus_target_global;
		gAgent.setShowAvatar(TRUE);
	}

	// smoothing
	if (TRUE)
	{
		LLVector3d agent_pos = gAgent.getPositionGlobal();
		LLVector3d camera_pos_agent = camera_pos_global - agent_pos;
		// Sitting on what you're manipulating can cause camera jitter with smoothing. 
		// This turns off smoothing while editing. -MG
		bool in_build_mode = LLToolMgr::getInstance()->inBuildMode();
		mCameraSmoothingStop = mCameraSmoothingStop || in_build_mode;
		
		if (cameraThirdPerson() && !mCameraSmoothingStop)
		{
			const F32 SMOOTHING_HALF_LIFE = 0.02f;
			
			F32 smoothing = LLCriticalDamp::getInterpolant(gSavedSettings.getF32("CameraPositionSmoothing") * SMOOTHING_HALF_LIFE, FALSE);
					
			if (!mFocusObject)  // we differentiate on avatar mode 
			{
				// for avatar-relative focus, we smooth in avatar space -
				// the avatar moves too jerkily w/r/t global space to smooth there.

				LLVector3d delta = camera_pos_agent - mCameraSmoothingLastPositionAgent;
				if (delta.magVec() < MAX_CAMERA_SMOOTH_DISTANCE)  // only smooth over short distances please
				{
					camera_pos_agent = lerp(mCameraSmoothingLastPositionAgent, camera_pos_agent, smoothing);
					camera_pos_global = camera_pos_agent + agent_pos;
				}
			}
			else
			{
				LLVector3d delta = camera_pos_global - mCameraSmoothingLastPositionGlobal;
				if (delta.magVec() < MAX_CAMERA_SMOOTH_DISTANCE) // only smooth over short distances please
				{
					camera_pos_global = lerp(mCameraSmoothingLastPositionGlobal, camera_pos_global, smoothing);
				}
			}
		}
								 
		mCameraSmoothingLastPositionGlobal = camera_pos_global;
		mCameraSmoothingLastPositionAgent = camera_pos_agent;
		mCameraSmoothingStop = false;
	}

	
	mCameraCurrentFOVZoomFactor = lerp(mCameraCurrentFOVZoomFactor, mCameraFOVZoomFactor, LLCriticalDamp::getInterpolant(FOV_ZOOM_HALF_LIFE));

//	llinfos << "Current FOV Zoom: " << mCameraCurrentFOVZoomFactor << " Target FOV Zoom: " << mCameraFOVZoomFactor << " Object penetration: " << mFocusObjectDist << llendl;

	LLVector3 focus_agent = gAgent.getPosAgentFromGlobal(mFocusGlobal);
	
	mCameraPositionAgent = gAgent.getPosAgentFromGlobal(camera_pos_global);

	// Move the camera

	LLViewerCamera::getInstance()->updateCameraLocation(mCameraPositionAgent, mCameraUpVector, focus_agent);
	//LLViewerCamera::getInstance()->updateCameraLocation(mCameraPositionAgent, camera_skyward, focus_agent);
	
	// Change FOV
	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / (1.f + mCameraCurrentFOVZoomFactor));

	// follow camera when in customize mode
	if (cameraCustomizeAvatar())	
	{
		setLookAt(LOOKAT_TARGET_FOCUS, NULL, mCameraPositionAgent);
	}

	// update the travel distance stat
	// this isn't directly related to the camera
	// but this seemed like the best place to do this
	LLVector3d global_pos = gAgent.getPositionGlobal(); 
	if (!gAgent.getLastPositionGlobal().isExactlyZero())
	{
		LLVector3d delta = global_pos - gAgent.getLastPositionGlobal();
		gAgent.setDistanceTraveled(gAgent.getDistanceTraveled() + delta.magVec());
	}
	gAgent.setLastPositionGlobal(global_pos);
	
	if (LLVOAvatar::sVisibleInFirstPerson && isAgentAvatarValid() && !gAgentAvatarp->isSitting() && cameraMouselook())
	{
		LLVector3 head_pos = gAgentAvatarp->mHeadp->getWorldPosition() + 
			LLVector3(0.08f, 0.f, 0.05f) * gAgentAvatarp->mHeadp->getWorldRotation() + 
			LLVector3(0.1f, 0.f, 0.f) * gAgentAvatarp->mPelvisp->getWorldRotation();
		LLVector3 diff = mCameraPositionAgent - head_pos;
		diff = diff * ~gAgentAvatarp->mRoot->getWorldRotation();

		LLJoint* torso_joint = gAgentAvatarp->mTorsop;
		LLJoint* chest_joint = gAgentAvatarp->mChestp;
		LLVector3 torso_scale = torso_joint->getScale();
		LLVector3 chest_scale = chest_joint->getScale();

		// shorten avatar skeleton to avoid foot interpenetration
		if (!gAgentAvatarp->mInAir)
		{
			LLVector3 chest_offset = LLVector3(0.f, 0.f, chest_joint->getPosition().mV[VZ]) * torso_joint->getWorldRotation();
			F32 z_compensate = llclamp(-diff.mV[VZ], -0.2f, 1.f);
			F32 scale_factor = llclamp(1.f - ((z_compensate * 0.5f) / chest_offset.mV[VZ]), 0.5f, 1.2f);
			torso_joint->setScale(LLVector3(1.f, 1.f, scale_factor));

			LLJoint* neck_joint = gAgentAvatarp->mNeckp;
			LLVector3 neck_offset = LLVector3(0.f, 0.f, neck_joint->getPosition().mV[VZ]) * chest_joint->getWorldRotation();
			scale_factor = llclamp(1.f - ((z_compensate * 0.5f) / neck_offset.mV[VZ]), 0.5f, 1.2f);
			chest_joint->setScale(LLVector3(1.f, 1.f, scale_factor));
			diff.mV[VZ] = 0.f;
		}

		gAgentAvatarp->mPelvisp->setPosition(gAgentAvatarp->mPelvisp->getPosition() + diff);

		gAgentAvatarp->mRoot->updateWorldMatrixChildren();

		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
			 iter != gAgentAvatarp->mAttachmentPoints.end(); )
		{
			LLVOAvatar::attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachment = curiter->second;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				LLViewerObject *attached_object = (*attachment_iter);
				if (attached_object && !attached_object->isDead() && attached_object->mDrawable.notNull())
				{
					// clear any existing "early" movements of attachment
					attached_object->mDrawable->clearState(LLDrawable::EARLY_MOVE);
					gPipeline.updateMoveNormalAsync(attached_object->mDrawable);
					attached_object->updateText();
				}
			}
		}

		torso_joint->setScale(torso_scale);
		chest_joint->setScale(chest_scale);
	}
}

void LLAgentCamera::updateLastCamera()
{
	mLastCameraMode = mCameraMode;
}

void LLAgentCamera::updateFocusOffset()
{
	validateFocusObject();
	if (mFocusObject.notNull())
	{
		LLVector3d obj_pos = gAgent.getPosGlobalFromAgent(mFocusObject->getRenderPosition());
		mFocusObjectOffset.setVec(mFocusTargetGlobal - obj_pos);
	}
}

void LLAgentCamera::validateFocusObject()
{
	if (mFocusObject.notNull() && 
		mFocusObject->isDead())
	{
		mFocusObjectOffset.clearVec();
		clearFocusObject();
		mCameraFOVZoomFactor = 0.f;
	}
}

//-----------------------------------------------------------------------------
// calcFocusPositionTargetGlobal()
//-----------------------------------------------------------------------------
LLVector3d LLAgentCamera::calcFocusPositionTargetGlobal()
{
	if (mFocusObject.notNull() && mFocusObject->isDead())
	{
		clearFocusObject();
	}

	if (mCameraMode == CAMERA_MODE_FOLLOW && mFocusOnAvatar)
	{
		mFocusTargetGlobal = gAgent.getPosGlobalFromAgent(mFollowCam.getSimulatedFocus());
		return mFocusTargetGlobal;
	}
	else if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		LLVector3d at_axis(1.0, 0.0, 0.0);
		LLQuaternion agent_rot = gAgent.getFrameAgent().getQuaternion();
		if (isAgentAvatarValid() && gAgentAvatarp->getParent())
		{
			LLViewerObject* root_object = (LLViewerObject*)gAgentAvatarp->getRoot();
			if (!root_object->flagCameraDecoupled())
			{
				agent_rot *= ((LLViewerObject*)(gAgentAvatarp->getParent()))->getRenderRotation();
			}
		}
		at_axis = at_axis * agent_rot;
		mFocusTargetGlobal = calcCameraPositionTargetGlobal() + at_axis;
		return mFocusTargetGlobal;
	}
	else if (mCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		return mFocusTargetGlobal;
	}
	else if (!mFocusOnAvatar)
	{
		if (mFocusObject.notNull() && !mFocusObject->isDead() && mFocusObject->mDrawable.notNull())
		{
			LLDrawable* drawablep = mFocusObject->mDrawable;
			
			if (mTrackFocusObject &&
				drawablep && 
				drawablep->isActive())
			{
				if (!mFocusObject->isAvatar())
				{
					if (mFocusObject->isSelected())
					{
						gPipeline.updateMoveNormalAsync(drawablep);
					}
					else
					{
						if (drawablep->isState(LLDrawable::MOVE_UNDAMPED))
						{
							gPipeline.updateMoveNormalAsync(drawablep);
						}
						else
						{
							gPipeline.updateMoveDampedAsync(drawablep);
						}
					}
				}
			}
			// if not tracking object, update offset based on new object position
			else
			{
				updateFocusOffset();
			}
			LLVector3 focus_agent = mFocusObject->getRenderPosition() + mFocusObjectOffset;
			mFocusTargetGlobal.setVec(gAgent.getPosGlobalFromAgent(focus_agent));
		}
		return mFocusTargetGlobal;
	}
	else if (mSitCameraEnabled && isAgentAvatarValid() && gAgentAvatarp->isSitting() && mSitCameraReferenceObject.notNull())
	{
		// sit camera
		LLVector3 object_pos = mSitCameraReferenceObject->getRenderPosition();
		LLQuaternion object_rot = mSitCameraReferenceObject->getRenderRotation();

		LLVector3 target_pos = object_pos + (mSitCameraFocus * object_rot);
		return gAgent.getPosGlobalFromAgent(target_pos);
	}
	else
	{
		return gAgent.getPositionGlobal() + calcThirdPersonFocusOffset();
	}
}

LLVector3d LLAgentCamera::calcThirdPersonFocusOffset()
{
	// ...offset from avatar
	LLVector3d focus_offset;
	LLQuaternion agent_rot = gAgent.getFrameAgent().getQuaternion();
	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		agent_rot *= ((LLViewerObject*)(gAgentAvatarp->getParent()))->getRenderRotation();
	}

	focus_offset = convert_from_llsd<LLVector3d>(mFocusOffsetInitial[mCameraPreset]->get(), TYPE_VEC3D, "");
	return focus_offset * agent_rot;
}

void LLAgentCamera::setupSitCamera()
{
	// agent frame entering this function is in world coordinates
	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		LLQuaternion parent_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
		// slam agent coordinate frame to proper parent local version
		LLVector3 at_axis = gAgent.getFrameAgent().getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis * ~parent_rot);
	}
}

//-----------------------------------------------------------------------------
// getCameraPositionAgent()
//-----------------------------------------------------------------------------
const LLVector3 &LLAgentCamera::getCameraPositionAgent() const
{
	return LLViewerCamera::getInstance()->getOrigin();
}

//-----------------------------------------------------------------------------
// getCameraPositionGlobal()
//-----------------------------------------------------------------------------
LLVector3d LLAgentCamera::getCameraPositionGlobal() const
{
	return gAgent.getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin());
}

//-----------------------------------------------------------------------------
// calcCameraFOVZoomFactor()
//-----------------------------------------------------------------------------
F32	LLAgentCamera::calcCameraFOVZoomFactor()
{
	LLVector3 camera_offset_dir;
	camera_offset_dir.setVec(mCameraFocusOffset);

	if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		return 0.f;
	}
	else if (mFocusObject.notNull() && !mFocusObject->isAvatar() && !mFocusOnAvatar)
	{
		// don't FOV zoom on mostly transparent objects
		F32 obj_min_dist = 0.f;
		calcCameraMinDistance(obj_min_dist);
		F32 current_distance = llmax(0.001f, camera_offset_dir.magVec());

		mFocusObjectDist = obj_min_dist - current_distance;

		F32 new_fov_zoom = llclamp(mFocusObjectDist / current_distance, 0.f, 1000.f);
		return new_fov_zoom;
	}
	else // focusing on land or avatar
	{
		// keep old field of view until user changes focus explicitly
		return mCameraFOVZoomFactor;
		//return 0.f;
	}
}

//-----------------------------------------------------------------------------
// calcCameraPositionTargetGlobal()
//-----------------------------------------------------------------------------
LLVector3d LLAgentCamera::calcCameraPositionTargetGlobal(BOOL *hit_limit)
{
	// Compute base camera position and look-at points.
	F32			camera_land_height;
	LLVector3d	frame_center_global = !isAgentAvatarValid() ? 
		gAgent.getPositionGlobal() :
		gAgent.getPosGlobalFromAgent(gAgentAvatarp->mRoot->getWorldPosition());
	
	BOOL		isConstrained = FALSE;
	LLVector3d	head_offset;
	head_offset.setVec(mThirdPersonHeadOffset);

	LLVector3d camera_position_global;

	if (mCameraMode == CAMERA_MODE_FOLLOW && mFocusOnAvatar)
	{
		camera_position_global = gAgent.getPosGlobalFromAgent(mFollowCam.getSimulatedPosition());
	}
	else if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		if (!isAgentAvatarValid() || gAgentAvatarp->mDrawable.isNull())
		{
			llwarns << "Null avatar drawable!" << llendl;
			return LLVector3d::zero;
		}
		head_offset.clearVec();
		if (gAgentAvatarp->isSitting() && gAgentAvatarp->getParent())
		{
			gAgentAvatarp->updateHeadOffset();
			head_offset.mdV[VX] = gAgentAvatarp->mHeadOffset.mV[VX];
			head_offset.mdV[VY] = gAgentAvatarp->mHeadOffset.mV[VY];
			head_offset.mdV[VZ] = gAgentAvatarp->mHeadOffset.mV[VZ] + 0.1f;
			const LLMatrix4& mat = ((LLViewerObject*) gAgentAvatarp->getParent())->getRenderMatrix();
			camera_position_global = gAgent.getPosGlobalFromAgent
								((gAgentAvatarp->getPosition()+
								 LLVector3(head_offset)*gAgentAvatarp->getRotation()) * mat);
		}
		else
		{
			head_offset.mdV[VZ] = gAgentAvatarp->mHeadOffset.mV[VZ];
			if (gAgentAvatarp->isSitting())
			{
				head_offset.mdV[VZ] += 0.1;
			}
			camera_position_global = gAgent.getPosGlobalFromAgent(gAgentAvatarp->getRenderPosition());//frame_center_global;
			head_offset = head_offset * gAgentAvatarp->getRenderRotation();
			camera_position_global = camera_position_global + head_offset;
		}
	}
	else if (mCameraMode == CAMERA_MODE_THIRD_PERSON && mFocusOnAvatar)
	{
		LLVector3 local_camera_offset;
		F32 camera_distance = 0.f;

		if (mSitCameraEnabled 
			&& isAgentAvatarValid() 
			&& gAgentAvatarp->isSitting() 
			&& mSitCameraReferenceObject.notNull())
		{
			// sit camera
			LLVector3 object_pos = mSitCameraReferenceObject->getRenderPosition();
			LLQuaternion object_rot = mSitCameraReferenceObject->getRenderRotation();

			LLVector3 target_pos = object_pos + (mSitCameraPos * object_rot);

			camera_position_global = gAgent.getPosGlobalFromAgent(target_pos);
		}
		else
		{
			local_camera_offset = mCameraZoomFraction * getCameraOffsetInitial() * gSavedSettings.getF32("CameraOffsetScale");
			
			// are we sitting down?
			if (isAgentAvatarValid() && gAgentAvatarp->getParent())
			{
				LLQuaternion parent_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
				// slam agent coordinate frame to proper parent local version
				LLVector3 at_axis = gAgent.getFrameAgent().getAtAxis() * parent_rot;
				at_axis.mV[VZ] = 0.f;
				at_axis.normalize();
				gAgent.resetAxes(at_axis * ~parent_rot);

				local_camera_offset = local_camera_offset * gAgent.getFrameAgent().getQuaternion() * parent_rot;
			}
			else
			{
				local_camera_offset = gAgent.getFrameAgent().rotateToAbsolute( local_camera_offset );
			}

			if (!mCameraCollidePlane.isExactlyZero() && (!isAgentAvatarValid() || !gAgentAvatarp->isSitting()))
			{
				LLVector3 plane_normal;
				plane_normal.setVec(mCameraCollidePlane.mV);

				F32 offset_dot_norm = local_camera_offset * plane_normal;
				if (llabs(offset_dot_norm) < 0.001f)
				{
					offset_dot_norm = 0.001f;
				}
				
				camera_distance = local_camera_offset.normalize();

				F32 pos_dot_norm = gAgent.getPosAgentFromGlobal(frame_center_global + head_offset) * plane_normal;
				
				// if agent is outside the colliding half-plane
				if (pos_dot_norm > mCameraCollidePlane.mV[VW])
				{
					// check to see if camera is on the opposite side (inside) the half-plane
					if (offset_dot_norm + pos_dot_norm < mCameraCollidePlane.mV[VW])
					{
						// diminish offset by factor to push it back outside the half-plane
						camera_distance *= (pos_dot_norm - mCameraCollidePlane.mV[VW] - CAMERA_COLLIDE_EPSILON) / -offset_dot_norm;
					}
				}
				else
				{
					if (offset_dot_norm + pos_dot_norm > mCameraCollidePlane.mV[VW])
					{
						camera_distance *= (mCameraCollidePlane.mV[VW] - pos_dot_norm - CAMERA_COLLIDE_EPSILON) / offset_dot_norm;
					}
				}
			}
			else
			{
				camera_distance = local_camera_offset.normalize();
			}

			mTargetCameraDistance = llmax(camera_distance, MIN_CAMERA_DISTANCE);

			if (mTargetCameraDistance != mCurrentCameraDistance)
			{
				F32 camera_lerp_amt = LLCriticalDamp::getInterpolant(CAMERA_ZOOM_HALF_LIFE);

				mCurrentCameraDistance = lerp(mCurrentCameraDistance, mTargetCameraDistance, camera_lerp_amt);
			}

			// Make the camera distance current
			local_camera_offset *= mCurrentCameraDistance;

			// set the global camera position
			LLVector3d camera_offset;
			
			camera_offset.setVec( local_camera_offset );
			camera_position_global = frame_center_global + head_offset + camera_offset;

			if (isAgentAvatarValid())
			{
				LLVector3d camera_lag_d;
				F32 lag_interp = LLCriticalDamp::getInterpolant(CAMERA_LAG_HALF_LIFE);
				LLVector3 target_lag;
				LLVector3 vel = gAgent.getVelocity();

				// lag by appropriate amount for flying
				F32 time_in_air = gAgentAvatarp->mTimeInAir.getElapsedTimeF32();
				if(!mCameraAnimating && gAgentAvatarp->mInAir && time_in_air > GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME)
				{
					LLVector3 frame_at_axis = gAgent.getFrameAgent().getAtAxis();
					frame_at_axis -= projected_vec(frame_at_axis, gAgent.getReferenceUpVector());
					frame_at_axis.normalize();

					//transition smoothly in air mode, to avoid camera pop
					F32 u = (time_in_air - GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME) / GROUND_TO_AIR_CAMERA_TRANSITION_TIME;
					u = llclamp(u, 0.f, 1.f);

					lag_interp *= u;

					if (gViewerWindow->getLeftMouseDown() && gViewerWindow->getLastPick().mObjectID == gAgentAvatarp->getID())
					{
						// disable camera lag when using mouse-directed steering
						target_lag.clearVec();
					}
					else
					{
						target_lag = vel * gSavedSettings.getF32("DynamicCameraStrength") / 30.f;
					}

					mCameraLag = lerp(mCameraLag, target_lag, lag_interp);

					F32 lag_dist = mCameraLag.magVec();
					if (lag_dist > MAX_CAMERA_LAG)
					{
						mCameraLag = mCameraLag * MAX_CAMERA_LAG / lag_dist;
					}

					// clamp camera lag so that avatar is always in front
					F32 dot = (mCameraLag - (frame_at_axis * (MIN_CAMERA_LAG * u))) * frame_at_axis;
					if (dot < -(MIN_CAMERA_LAG * u))
					{
						mCameraLag -= (dot + (MIN_CAMERA_LAG * u)) * frame_at_axis;
					}
				}
				else
				{
					mCameraLag = lerp(mCameraLag, LLVector3::zero, LLCriticalDamp::getInterpolant(0.15f));
				}

				camera_lag_d.setVec(mCameraLag);
				camera_position_global = camera_position_global - camera_lag_d;
			}
		}
	}
	else
	{
		LLVector3d focusPosGlobal = calcFocusPositionTargetGlobal();
		// camera gets pushed out later wrt mCameraFOVZoomFactor...this is "raw" value
		camera_position_global = focusPosGlobal + mCameraFocusOffset;
	}

	if (!gSavedSettings.getBOOL("DisableCameraConstraints") && !gAgent.isGodlike())
	{
		LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosGlobal(camera_position_global);
		bool constrain = true;
		if(regionp && regionp->canManageEstate())
		{
			constrain = false;
		}
		if(constrain)
		{
			F32 max_dist = (CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode) ? APPEARANCE_MAX_ZOOM : mDrawDistance;

			LLVector3d camera_offset = camera_position_global - gAgent.getPositionGlobal();
			F32 camera_distance = (F32)camera_offset.magVec();

			if(camera_distance > max_dist)
			{
				camera_position_global = gAgent.getPositionGlobal() + (max_dist/camera_distance)*camera_offset;
				isConstrained = TRUE;
			}
		}

// JC - Could constrain camera based on parcel stuff here.
//			LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(camera_position_global);
//			
//			if (regionp && !regionp->mParcelOverlay->isBuildCameraAllowed(regionp->getPosRegionFromGlobal(camera_position_global)))
//			{
//				camera_position_global = last_position_global;
//
//				isConstrained = TRUE;
//			}
	}

	// Don't let camera go underground
	F32 camera_min_off_ground = getCameraMinOffGround();

	camera_land_height = LLWorld::getInstance()->resolveLandHeightGlobal(camera_position_global);

	if (camera_position_global.mdV[VZ] < camera_land_height + camera_min_off_ground)
	{
		camera_position_global.mdV[VZ] = camera_land_height + camera_min_off_ground;
		isConstrained = TRUE;
	}


	if (hit_limit)
	{
		*hit_limit = isConstrained;
	}

	return camera_position_global;
}


LLVector3 LLAgentCamera::getCameraOffsetInitial()
{
	return convert_from_llsd<LLVector3>(mCameraOffsetInitial[mCameraPreset]->get(), TYPE_VEC3, "");
}


//-----------------------------------------------------------------------------
// handleScrollWheel()
//-----------------------------------------------------------------------------
void LLAgentCamera::handleScrollWheel(S32 clicks)
{
	if (mCameraMode == CAMERA_MODE_FOLLOW && getFocusOnAvatar())
	{
		if (!mFollowCam.getPositionLocked()) // not if the followCam position is locked in place
		{
			mFollowCam.zoom(clicks); 
			if (mFollowCam.isZoomedToMinimumDistance())
			{
				changeCameraToMouselook(FALSE);
			}
		}
	}
	else
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
		const F32 ROOT_ROOT_TWO = sqrt(F_SQRT2);

		// Block if camera is animating
		if (mCameraAnimating)
		{
			return;
		}

		if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
		{
			F32 zoom_factor = (F32)pow(0.8, -clicks);
			cameraZoomIn(zoom_factor);
		}
		else if (mFocusOnAvatar && (mCameraMode == CAMERA_MODE_THIRD_PERSON))
		{
			F32 camera_offset_initial_mag = getCameraOffsetInitial().magVec();
			
			F32 current_zoom_fraction = mTargetCameraDistance / (camera_offset_initial_mag * gSavedSettings.getF32("CameraOffsetScale"));
			current_zoom_fraction *= 1.f - pow(ROOT_ROOT_TWO, clicks);
			
			cameraOrbitIn(current_zoom_fraction * camera_offset_initial_mag * gSavedSettings.getF32("CameraOffsetScale"));
		}
		else
		{
			F32 current_zoom_fraction = (F32)mCameraFocusOffsetTarget.magVec();
			cameraOrbitIn(current_zoom_fraction * (1.f - pow(ROOT_ROOT_TWO, clicks)));
		}
	}
}


//-----------------------------------------------------------------------------
// getCameraMinOffGround()
//-----------------------------------------------------------------------------
F32 LLAgentCamera::getCameraMinOffGround()
{
	if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		return 0.f;
	}
	else
	{
		if (gSavedSettings.getBOOL("DisableCameraConstraints"))
		{
			return -1000.f;
		}
		else
		{
			return 0.5f;
		}
	}
}


//-----------------------------------------------------------------------------
// resetCamera()
//-----------------------------------------------------------------------------
void LLAgentCamera::resetCamera()
{
	// Remove any pitch from the avatar
	LLVector3 at = gAgent.getFrameAgent().getAtAxis();
	at.mV[VZ] = 0.f;
	at.normalize();
	gAgent.resetAxes(at);
	// have to explicitly clear field of view zoom now
	mCameraFOVZoomFactor = 0.f;

	updateCamera();
}

//-----------------------------------------------------------------------------
// changeCameraToMouselook()
//-----------------------------------------------------------------------------
void LLAgentCamera::changeCameraToMouselook(BOOL animate)
{
	if (!gSavedSettings.getBOOL("EnableMouselook") 
		|| LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}
	
	// visibility changes at end of animation
	gViewerWindow->getWindow()->resetBusyCount();

	// Menus should not remain open on switching to mouselook...
	LLMenuGL::sMenuContainer->hideMenus();
	LLUI::clearPopups();

	// unpause avatar animation
	gAgent.unpauseAnimation();

	LLToolMgr::getInstance()->setCurrentToolset(gMouselookToolset);

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->stopMotion(ANIM_AGENT_BODY_NOISE);
		gAgentAvatarp->stopMotion(ANIM_AGENT_BREATHE_ROT);
	}

	//gViewerWindow->stopGrab();
	LLSelectMgr::getInstance()->deselectAll();
	gViewerWindow->hideCursor();
	gViewerWindow->moveCursorToCenter();

	if (mCameraMode != CAMERA_MODE_MOUSELOOK)
	{
		gFocusMgr.setKeyboardFocus(NULL);
		
		updateLastCamera();
		mCameraMode = CAMERA_MODE_MOUSELOOK;
		const U32 old_flags = gAgent.getControlFlags();
		gAgent.setControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (old_flags != gAgent.getControlFlags())
		{
			gAgent.setFlagsDirty();
		}

		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			mCameraAnimating = FALSE;
			gAgent.endAnimationUpdateUI();
		}
	}
}


//-----------------------------------------------------------------------------
// changeCameraToDefault()
//-----------------------------------------------------------------------------
void LLAgentCamera::changeCameraToDefault()
{
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}

	if (LLFollowCamMgr::getActiveFollowCamParams())
	{
		changeCameraToFollow();
	}
	else
	{
		changeCameraToThirdPerson();
	}
}


//-----------------------------------------------------------------------------
// changeCameraToFollow()
//-----------------------------------------------------------------------------
void LLAgentCamera::changeCameraToFollow(BOOL animate)
{
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}

	if(mCameraMode != CAMERA_MODE_FOLLOW)
	{
		if (mCameraMode == CAMERA_MODE_MOUSELOOK)
		{
			animate = FALSE;
		}
		startCameraAnimation();

		updateLastCamera();
		mCameraMode = CAMERA_MODE_FOLLOW;

		// bang-in the current focus, position, and up vector of the follow cam
		mFollowCam.reset(mCameraPositionAgent, LLViewerCamera::getInstance()->getPointOfInterest(), LLVector3::z_axis);
		
		if (gBasicToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		}

		if (isAgentAvatarValid())
		{
			gAgentAvatarp->mPelvisp->setPosition(LLVector3::zero);
			gAgentAvatarp->startMotion( ANIM_AGENT_BODY_NOISE );
			gAgentAvatarp->startMotion( ANIM_AGENT_BREATHE_ROT );
		}

		// unpause avatar animation
		gAgent.unpauseAnimation();

		gAgent.clearControlFlags(AGENT_CONTROL_MOUSELOOK);

		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			mCameraAnimating = FALSE;
			gAgent.endAnimationUpdateUI();
		}
	}
}

//-----------------------------------------------------------------------------
// changeCameraToThirdPerson()
//-----------------------------------------------------------------------------
void LLAgentCamera::changeCameraToThirdPerson(BOOL animate)
{
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		return;
	}

	gViewerWindow->getWindow()->resetBusyCount();

	mCameraZoomFraction = INITIAL_ZOOM_FRACTION;

	if (isAgentAvatarValid())
	{
		if (!gAgentAvatarp->isSitting())
		{
			gAgentAvatarp->mPelvisp->setPosition(LLVector3::zero);
		}
		gAgentAvatarp->startMotion(ANIM_AGENT_BODY_NOISE);
		gAgentAvatarp->startMotion(ANIM_AGENT_BREATHE_ROT);
	}

	LLVector3 at_axis;

	// unpause avatar animation
	gAgent.unpauseAnimation();

	if (mCameraMode != CAMERA_MODE_THIRD_PERSON)
	{
		if (gBasicToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		}

		mCameraLag.clearVec();
		if (mCameraMode == CAMERA_MODE_MOUSELOOK)
		{
			mCurrentCameraDistance = MIN_CAMERA_DISTANCE;
			mTargetCameraDistance = MIN_CAMERA_DISTANCE;
			animate = FALSE;
		}
		updateLastCamera();
		mCameraMode = CAMERA_MODE_THIRD_PERSON;
		gAgent.clearControlFlags(AGENT_CONTROL_MOUSELOOK);
	}

	// Remove any pitch from the avatar
	if (isAgentAvatarValid() && gAgentAvatarp->getParent())
	{
		LLQuaternion obj_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
		at_axis = LLViewerCamera::getInstance()->getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis * ~obj_rot);
	}
	else
	{
		at_axis = gAgent.getFrameAgent().getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis);
	}


	if (animate)
	{
		startCameraAnimation();
	}
	else
	{
		mCameraAnimating = FALSE;
		gAgent.endAnimationUpdateUI();
	}
}

//-----------------------------------------------------------------------------
// changeCameraToCustomizeAvatar()
//-----------------------------------------------------------------------------
void LLAgentCamera::changeCameraToCustomizeAvatar()
{
	if (LLViewerJoystick::getInstance()->getOverrideCamera() || !isAgentAvatarValid())
	{
		return;
	}

	gAgent.standUp(); // force stand up
	gViewerWindow->getWindow()->resetBusyCount();

	if (gFaceEditToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(gFaceEditToolset);
	}

	startCameraAnimation();

	if (mCameraMode != CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		updateLastCamera();
		mCameraMode = CAMERA_MODE_CUSTOMIZE_AVATAR;
		gAgent.clearControlFlags(AGENT_CONTROL_MOUSELOOK);

		gFocusMgr.setKeyboardFocus( NULL );
		gFocusMgr.setMouseCapture( NULL );

		// Remove any pitch or rotation from the avatar
		LLVector3 at = gAgent.getAtAxis();
		at.mV[VZ] = 0.f;
		at.normalize();
		gAgent.resetAxes(at);

		gAgent.sendAnimationRequest(ANIM_AGENT_CUSTOMIZE, ANIM_REQUEST_START);
		gAgent.setCustomAnim(TRUE);
		gAgentAvatarp->startMotion(ANIM_AGENT_CUSTOMIZE);
		LLMotion* turn_motion = gAgentAvatarp->findMotion(ANIM_AGENT_CUSTOMIZE);

		if (turn_motion)
		{
			// delay camera animation long enough to play through turn animation
			setAnimationDuration(turn_motion->getDuration() + CUSTOMIZE_AVATAR_CAMERA_ANIM_SLOP);
		}
	}

	LLVector3 agent_at = gAgent.getAtAxis();
	agent_at.mV[VZ] = 0.f;
	agent_at.normalize();

	// default focus point for customize avatar
	LLVector3 focus_target = isAgentAvatarValid() 
		? gAgentAvatarp->mHeadp->getWorldPosition()
		: gAgent.getPositionAgent();

	LLVector3d camera_offset(agent_at * -1.0);
	// push camera up and out from avatar
	camera_offset.mdV[VZ] = 0.1f; 
	camera_offset *= CUSTOMIZE_AVATAR_CAMERA_DEFAULT_DIST;
	LLVector3d focus_target_global = gAgent.getPosGlobalFromAgent(focus_target);
	setAnimationDuration(gSavedSettings.getF32("ZoomTime"));
	setCameraPosAndFocusGlobal(focus_target_global + camera_offset, focus_target_global, gAgent.getID());
}


void LLAgentCamera::switchCameraPreset(ECameraPreset preset)
{
	//zoom is supposed to be reset for the front and group views
	mCameraZoomFraction = 1.f;

	//focusing on avatar in that case means following him on movements
	mFocusOnAvatar = TRUE;

	mCameraPreset = preset;

	gSavedSettings.setU32("CameraPreset", mCameraPreset);
}


//
// Focus point management
//

void LLAgentCamera::setAnimationDuration(F32 duration)
{ 
	if (mCameraAnimating)
	{
		// do not cut any existing camera animation short
		F32 animation_left = llmax(0.f, mAnimationDuration - mAnimationTimer.getElapsedTimeF32());
		mAnimationDuration = llmax(duration, animation_left);
	}
	else
	{
		mAnimationDuration = duration; 
	}
}

//-----------------------------------------------------------------------------
// startCameraAnimation()
//-----------------------------------------------------------------------------
void LLAgentCamera::startCameraAnimation()
{
	mAnimationCameraStartGlobal = getCameraPositionGlobal();
	mAnimationFocusStartGlobal = mFocusGlobal;
	setAnimationDuration(gSavedSettings.getF32("ZoomTime"));
	mAnimationTimer.reset();
	mCameraAnimating = TRUE;
}

//-----------------------------------------------------------------------------
// stopCameraAnimation()
//-----------------------------------------------------------------------------
void LLAgentCamera::stopCameraAnimation()
{
	mCameraAnimating = FALSE;
}

void LLAgentCamera::clearFocusObject()
{
	if (mFocusObject.notNull())
	{
		startCameraAnimation();

		setFocusObject(NULL);
		mFocusObjectOffset.clearVec();
	}
}

void LLAgentCamera::setFocusObject(LLViewerObject* object)
{
	mFocusObject = object;
}

// Focus on a point, but try to keep camera position stable.
//-----------------------------------------------------------------------------
// setFocusGlobal()
//-----------------------------------------------------------------------------
void LLAgentCamera::setFocusGlobal(const LLPickInfo& pick)
{
	LLViewerObject* objectp = gObjectList.findObject(pick.mObjectID);

	if (objectp)
	{
		// focus on object plus designated offset
		// which may or may not be same as pick.mPosGlobal
		setFocusGlobal(objectp->getPositionGlobal() + LLVector3d(pick.mObjectOffset), pick.mObjectID);
	}
	else
	{
		// focus directly on point where user clicked
		setFocusGlobal(pick.mPosGlobal, pick.mObjectID);
	}
}


void LLAgentCamera::setFocusGlobal(const LLVector3d& focus, const LLUUID &object_id)
{
	setFocusObject(gObjectList.findObject(object_id));
	LLVector3d old_focus = mFocusTargetGlobal;
	LLViewerObject *focus_obj = mFocusObject;

	// if focus has changed
	if (old_focus != focus)
	{
		if (focus.isExactlyZero())
		{
			if (isAgentAvatarValid())
			{
				mFocusTargetGlobal = gAgent.getPosGlobalFromAgent(gAgentAvatarp->mHeadp->getWorldPosition());
			}
			else
			{
				mFocusTargetGlobal = gAgent.getPositionGlobal();
			}
			mCameraFocusOffsetTarget = getCameraPositionGlobal() - mFocusTargetGlobal;
			mCameraFocusOffset = mCameraFocusOffsetTarget;
			setLookAt(LOOKAT_TARGET_CLEAR);
		}
		else
		{
			mFocusTargetGlobal = focus;
			if (!focus_obj)
			{
				mCameraFOVZoomFactor = 0.f;
			}

			mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(mCameraVirtualPositionAgent) - mFocusTargetGlobal;

			startCameraAnimation();

			if (focus_obj)
			{
				if (focus_obj->isAvatar())
				{
					setLookAt(LOOKAT_TARGET_FOCUS, focus_obj);
				}
				else
				{
					setLookAt(LOOKAT_TARGET_FOCUS, focus_obj, (gAgent.getPosAgentFromGlobal(focus) - focus_obj->getRenderPosition()) * ~focus_obj->getRenderRotation());
				}
			}
			else
			{
				setLookAt(LOOKAT_TARGET_FOCUS, NULL, gAgent.getPosAgentFromGlobal(mFocusTargetGlobal));
			}
		}
	}
	else // focus == mFocusTargetGlobal
	{
		if (focus.isExactlyZero())
		{
			if (isAgentAvatarValid())
			{
				mFocusTargetGlobal = gAgent.getPosGlobalFromAgent(gAgentAvatarp->mHeadp->getWorldPosition());
			}
			else
			{
				mFocusTargetGlobal = gAgent.getPositionGlobal();
			}
		}
		mCameraFocusOffsetTarget = (getCameraPositionGlobal() - mFocusTargetGlobal) / (1.f + mCameraFOVZoomFactor);;
		mCameraFocusOffset = mCameraFocusOffsetTarget;
	}

	if (mFocusObject.notNull())
	{
		// for attachments, make offset relative to avatar, not the attachment
		if (mFocusObject->isAttachment())
		{
			while (mFocusObject.notNull() && !mFocusObject->isAvatar())
			{
				mFocusObject = (LLViewerObject*) mFocusObject->getParent();
			}
			setFocusObject((LLViewerObject*)mFocusObject);
		}
		updateFocusOffset();
	}
}

// Used for avatar customization
//-----------------------------------------------------------------------------
// setCameraPosAndFocusGlobal()
//-----------------------------------------------------------------------------
void LLAgentCamera::setCameraPosAndFocusGlobal(const LLVector3d& camera_pos, const LLVector3d& focus, const LLUUID &object_id)
{
	LLVector3d old_focus = mFocusTargetGlobal.isExactlyZero() ? focus : mFocusTargetGlobal;

	F64 focus_delta_squared = (old_focus - focus).magVecSquared();
	const F64 ANIM_EPSILON_SQUARED = 0.0001;
	if (focus_delta_squared > ANIM_EPSILON_SQUARED)
	{
		startCameraAnimation();
	}
	
	//LLViewerCamera::getInstance()->setOrigin( gAgent.getPosAgentFromGlobal( camera_pos ) );
	setFocusObject(gObjectList.findObject(object_id));
	mFocusTargetGlobal = focus;
	mCameraFocusOffsetTarget = camera_pos - focus;
	mCameraFocusOffset = mCameraFocusOffsetTarget;

	if (mFocusObject)
	{
		if (mFocusObject->isAvatar())
		{
			setLookAt(LOOKAT_TARGET_FOCUS, mFocusObject);
		}
		else
		{
			setLookAt(LOOKAT_TARGET_FOCUS, mFocusObject, (gAgent.getPosAgentFromGlobal(focus) - mFocusObject->getRenderPosition()) * ~mFocusObject->getRenderRotation());
		}
	}
	else
	{
		setLookAt(LOOKAT_TARGET_FOCUS, NULL, gAgent.getPosAgentFromGlobal(mFocusTargetGlobal));
	}

	if (mCameraAnimating)
	{
		const F64 ANIM_METERS_PER_SECOND = 10.0;
		const F64 MIN_ANIM_SECONDS = 0.5;
		const F64 MAX_ANIM_SECONDS = 10.0;
		F64 anim_duration = llmax( MIN_ANIM_SECONDS, sqrt(focus_delta_squared) / ANIM_METERS_PER_SECOND );
		anim_duration = llmin( anim_duration, MAX_ANIM_SECONDS );
		setAnimationDuration( (F32)anim_duration );
	}

	updateFocusOffset();
}

//-----------------------------------------------------------------------------
// setSitCamera()
//-----------------------------------------------------------------------------
void LLAgentCamera::setSitCamera(const LLUUID &object_id, const LLVector3 &camera_pos, const LLVector3 &camera_focus)
{
	BOOL camera_enabled = !object_id.isNull();

	if (camera_enabled)
	{
		LLViewerObject *reference_object = gObjectList.findObject(object_id);
		if (reference_object)
		{
			//convert to root object relative?
			mSitCameraPos = camera_pos;
			mSitCameraFocus = camera_focus;
			mSitCameraReferenceObject = reference_object;
			mSitCameraEnabled = TRUE;
		}
	}
	else
	{
		mSitCameraPos.clearVec();
		mSitCameraFocus.clearVec();
		mSitCameraReferenceObject = NULL;
		mSitCameraEnabled = FALSE;
	}
}

//-----------------------------------------------------------------------------
// setFocusOnAvatar()
//-----------------------------------------------------------------------------
void LLAgentCamera::setFocusOnAvatar(BOOL focus_on_avatar, BOOL animate)
{
	if (focus_on_avatar != mFocusOnAvatar)
	{
		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			stopCameraAnimation();
		}
	}
	
	//RN: when focused on the avatar, we're not "looking" at it
	// looking implies intent while focusing on avatar means
	// you're just walking around with a camera on you...eesh.
	if (!mFocusOnAvatar && focus_on_avatar)
	{
		setFocusGlobal(LLVector3d::zero);
		mCameraFOVZoomFactor = 0.f;
		if (mCameraMode == CAMERA_MODE_THIRD_PERSON)
		{
			LLVector3 at_axis;
			if (isAgentAvatarValid() && gAgentAvatarp->getParent())
			{
				LLQuaternion obj_rot = ((LLViewerObject*)gAgentAvatarp->getParent())->getRenderRotation();
				at_axis = LLViewerCamera::getInstance()->getAtAxis();
				at_axis.mV[VZ] = 0.f;
				at_axis.normalize();
				gAgent.resetAxes(at_axis * ~obj_rot);
			}
			else
			{
				at_axis = LLViewerCamera::getInstance()->getAtAxis();
				at_axis.mV[VZ] = 0.f;
				at_axis.normalize();
				gAgent.resetAxes(at_axis);
			}
		}
	}
	// unlocking camera from avatar
	else if (mFocusOnAvatar && !focus_on_avatar)
	{
		// keep camera focus point consistent, even though it is now unlocked
		setFocusGlobal(gAgent.getPositionGlobal() + calcThirdPersonFocusOffset(), gAgent.getID());
	}
	
	mFocusOnAvatar = focus_on_avatar;
}


BOOL LLAgentCamera::setLookAt(ELookAtType target_type, LLViewerObject *object, LLVector3 position)
{
	if(object && object->isAttachment())
	{
		LLViewerObject* parent = object;
		while(parent)
		{
			if (parent == gAgentAvatarp)
			{
				// looking at an attachment on ourselves, which we don't want to do
				object = gAgentAvatarp;
				position.clearVec();
			}
			parent = (LLViewerObject*)parent->getParent();
		}
	}
	if(!mLookAt || mLookAt->isDead())
	{
		mLookAt = (LLHUDEffectLookAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_LOOKAT);
		mLookAt->setSourceObject(gAgentAvatarp);
	}

	return mLookAt->setLookAt(target_type, object, position);
}

//-----------------------------------------------------------------------------
// lookAtLastChat()
//-----------------------------------------------------------------------------
void LLAgentCamera::lookAtLastChat()
{
	// Block if camera is animating or not in normal third person camera mode
	if (mCameraAnimating || !cameraThirdPerson())
	{
		return;
	}

	LLViewerObject *chatter = gObjectList.findObject(gAgent.getLastChatter());
	if (!chatter)
	{
		return;
	}

	LLVector3 delta_pos;
	if (chatter->isAvatar())
	{
		LLVOAvatar *chatter_av = (LLVOAvatar*)chatter;
		if (isAgentAvatarValid() && chatter_av->mHeadp)
		{
			delta_pos = chatter_av->mHeadp->getWorldPosition() - gAgentAvatarp->mHeadp->getWorldPosition();
		}
		else
		{
			delta_pos = chatter->getPositionAgent() - gAgent.getPositionAgent();
		}
		delta_pos.normalize();

		gAgent.setControlFlags(AGENT_CONTROL_STOP);

		changeCameraToThirdPerson();

		LLVector3 new_camera_pos = gAgentAvatarp->mHeadp->getWorldPosition();
		LLVector3 left = delta_pos % LLVector3::z_axis;
		left.normalize();
		LLVector3 up = left % delta_pos;
		up.normalize();
		new_camera_pos -= delta_pos * 0.4f;
		new_camera_pos += left * 0.3f;
		new_camera_pos += up * 0.2f;

		setFocusOnAvatar(FALSE, FALSE);

		if (chatter_av->mHeadp)
		{
			setFocusGlobal(gAgent.getPosGlobalFromAgent(chatter_av->mHeadp->getWorldPosition()), gAgent.getLastChatter());
			mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(new_camera_pos) - gAgent.getPosGlobalFromAgent(chatter_av->mHeadp->getWorldPosition());
		}
		else
		{
			setFocusGlobal(chatter->getPositionGlobal(), gAgent.getLastChatter());
			mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(new_camera_pos) - chatter->getPositionGlobal();
		}
	}
	else
	{
		delta_pos = chatter->getRenderPosition() - gAgent.getPositionAgent();
		delta_pos.normalize();

		gAgent.setControlFlags(AGENT_CONTROL_STOP);

		changeCameraToThirdPerson();

		LLVector3 new_camera_pos = gAgentAvatarp->mHeadp->getWorldPosition();
		LLVector3 left = delta_pos % LLVector3::z_axis;
		left.normalize();
		LLVector3 up = left % delta_pos;
		up.normalize();
		new_camera_pos -= delta_pos * 0.4f;
		new_camera_pos += left * 0.3f;
		new_camera_pos += up * 0.2f;

		setFocusOnAvatar(FALSE, FALSE);

		setFocusGlobal(chatter->getPositionGlobal(), gAgent.getLastChatter());
		mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(new_camera_pos) - chatter->getPositionGlobal();
	}
}

BOOL LLAgentCamera::setPointAt(EPointAtType target_type, LLViewerObject *object, LLVector3 position)
{
	// disallow pointing at attachments and avatars
	if (object && (object->isAttachment() || object->isAvatar()))
	{
		return FALSE;
	}
	if (!mPointAt || mPointAt->isDead())
	{
		mPointAt = (LLHUDEffectPointAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINTAT);
		mPointAt->setSourceObject(gAgentAvatarp);
	}
	return mPointAt->setPointAt(target_type, object, position);
}

ELookAtType LLAgentCamera::getLookAtType()
{
	if (mLookAt) 
	{
		return mLookAt->getLookAtType();
	}
	return LOOKAT_TARGET_NONE;
}

EPointAtType LLAgentCamera::getPointAtType()
{ 
	if (mPointAt) 
	{
		return mPointAt->getPointAtType();
	}
	return POINTAT_TARGET_NONE;
}

void LLAgentCamera::clearGeneralKeys()
{
	mAtKey 				= 0;
	mWalkKey 			= 0;
	mLeftKey 			= 0;
	mUpKey 				= 0;
	mYawKey 			= 0.f;
	mPitchKey 			= 0.f;
}

void LLAgentCamera::clearOrbitKeys()
{
	mOrbitLeftKey		= 0.f;
	mOrbitRightKey		= 0.f;
	mOrbitUpKey			= 0.f;
	mOrbitDownKey		= 0.f;
	mOrbitInKey			= 0.f;
	mOrbitOutKey		= 0.f;
}

void LLAgentCamera::clearPanKeys()
{
	mPanRightKey		= 0.f;
	mPanLeftKey			= 0.f;
	mPanUpKey			= 0.f;
	mPanDownKey			= 0.f;
	mPanInKey			= 0.f;
	mPanOutKey			= 0.f;
}

// static
S32 LLAgentCamera::directionToKey(S32 direction)
{
	if (direction > 0) return 1;
	if (direction < 0) return -1;
	return 0;
}


// EOF

