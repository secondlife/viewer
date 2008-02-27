/** 
 * @file llhudicon.cpp
 * @brief LLHUDIcon class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#include "llhudicon.h"

#include "llgl.h"
#include "llglimmediate.h"

#include "llviewerobject.h"
#include "lldrawable.h"
#include "llviewercamera.h"
#include "llviewerimage.h"
#include "llviewerwindow.h"

//-----------------------------------------------------------------------------
// Local consts
//-----------------------------------------------------------------------------
const F32 ANIM_TIME = 0.4f;
const F32 DIST_START_FADE = 15.f;
const F32 DIST_END_FADE = 30.f;
const F32 MAX_VISIBLE_TIME = 15.f;
const F32 FADE_OUT_TIME = 1.f;

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------
static F32 calc_bouncy_animation(F32 x)
{
	return -(cosf(x * F_PI * 2.5f - F_PI_BY_TWO))*(0.4f + x * -0.1f) + x * 1.3f;
}


//-----------------------------------------------------------------------------
// static declarations
//-----------------------------------------------------------------------------
LLHUDIcon::icon_instance_t LLHUDIcon::sIconInstances;

LLHUDIcon::LLHUDIcon(const U8 type) :
			LLHUDObject(type),
			mImagep(NULL),
			mPickID(0),
			mScale(0.1f),
			mHidden(FALSE)
{
	sIconInstances.push_back(this);
}

LLHUDIcon::~LLHUDIcon()
{
	mImagep = NULL;
}

void LLHUDIcon::renderIcon(BOOL for_select)
{
	LLGLSUIDefault texture_state;
	LLGLDepthTest gls_depth(GL_TRUE);
	if (for_select)
	{
		LLViewerImage::unbindTexture(0);
	}
	
	if (mHidden)
		return;

	if (mSourceObject.isNull() || mImagep.isNull())
	{
		markDead();
		return;
	}

	LLVector3 obj_position = mSourceObject->getRenderPosition();

	// put icon above object, and in front
	// RN: don't use drawable radius, it's fricking HUGE
	LLVector3 icon_relative_pos = (gCamera->getUpAxis() * ~mSourceObject->getRenderRotation());
	icon_relative_pos.abs();

	F32 distance_scale = llmin(mSourceObject->getScale().mV[VX] / icon_relative_pos.mV[VX], 
		mSourceObject->getScale().mV[VY] / icon_relative_pos.mV[VY], 
		mSourceObject->getScale().mV[VZ] / icon_relative_pos.mV[VZ]);
	F32 up_distance = 0.5f * distance_scale;
	LLVector3 icon_position = obj_position + (up_distance * gCamera->getUpAxis()) * 1.2f;

	LLVector3 icon_to_cam = gCamera->getOrigin() - icon_position;
	icon_to_cam.normVec();

	icon_position += icon_to_cam * mSourceObject->mDrawable->getRadius() * 1.1f;

	mDistance = dist_vec(icon_position, gCamera->getOrigin());

	F32 alpha_factor = for_select ? 1.f : clamp_rescale(mDistance, DIST_START_FADE, DIST_END_FADE, 1.f, 0.f);

	LLVector3 x_pixel_vec;
	LLVector3 y_pixel_vec;
	
	gCamera->getPixelVectors(icon_position, y_pixel_vec, x_pixel_vec);

	F32 scale_factor = 1.f;
	if (mAnimTimer.getElapsedTimeF32() < ANIM_TIME)
	{
		scale_factor = llmax(0.f, calc_bouncy_animation(mAnimTimer.getElapsedTimeF32() / ANIM_TIME));
	}

	F32 time_elapsed = mLifeTimer.getElapsedTimeF32();
	if (time_elapsed > MAX_VISIBLE_TIME)
	{
		markDead();
		return;
	}
	
	if (time_elapsed > MAX_VISIBLE_TIME - FADE_OUT_TIME)
	{
		alpha_factor *= clamp_rescale(time_elapsed, MAX_VISIBLE_TIME - FADE_OUT_TIME, MAX_VISIBLE_TIME, 1.f, 0.f);
	}

	F32 image_aspect = (F32)mImagep->mFullWidth / (F32)mImagep->mFullHeight;
	LLVector3 x_scale = image_aspect * (F32)gViewerWindow->getWindowHeight() * mScale * scale_factor * x_pixel_vec;
	LLVector3 y_scale = (F32)gViewerWindow->getWindowHeight() * mScale * scale_factor * y_pixel_vec;

	LLVector3 lower_left = icon_position - (x_scale * 0.5f);
	LLVector3 lower_right = icon_position + (x_scale * 0.5f);
	LLVector3 upper_left = icon_position - (x_scale * 0.5f) + y_scale;
	LLVector3 upper_right = icon_position + (x_scale * 0.5f) + y_scale;

	if (for_select)
	{
		// set color to unique color id for picking
		LLColor4U coloru((U8)(mPickID >> 16), (U8)(mPickID >> 8), (U8)mPickID);
		gGL.color4ubv(coloru.mV);
	}
	else
	{
		LLColor4 icon_color = LLColor4::white;
		icon_color.mV[VALPHA] = alpha_factor;
		gGL.color4fv(icon_color.mV);
		LLViewerImage::bindTexture(mImagep);
	}

	gGL.begin(GL_QUADS);
	{
		gGL.texCoord2f(0.f, 1.f);
		gGL.vertex3fv(upper_left.mV);
		gGL.texCoord2f(0.f, 0.f);
		gGL.vertex3fv(lower_left.mV);
		gGL.texCoord2f(1.f, 0.f);
		gGL.vertex3fv(lower_right.mV);
		gGL.texCoord2f(1.f, 1.f);
		gGL.vertex3fv(upper_right.mV);
	}
	gGL.end();
}

void LLHUDIcon::setImage(LLViewerImage* imagep)
{
	mImagep = imagep;
	mImagep->setClamp(TRUE, TRUE);
}

void LLHUDIcon::setScale(F32 fraction_of_fov)
{
	mScale = fraction_of_fov;
}

void LLHUDIcon::markDead()
{
	if (mSourceObject)
	{
		mSourceObject->clearIcon();
	}
	LLHUDObject::markDead();
}

void LLHUDIcon::render()
{
	renderIcon(FALSE);
}

void LLHUDIcon::renderForSelect()
{
	renderIcon(TRUE);
}


//static
S32 LLHUDIcon::generatePickIDs(S32 start_id, S32 step_size)
{
	S32 cur_id = start_id;
	icon_instance_t::iterator icon_it;

	for(icon_it = sIconInstances.begin(); icon_it != sIconInstances.end(); ++icon_it)
	{
		(*icon_it)->mPickID = cur_id;
		cur_id += step_size;
	}

	return cur_id;
}

//static
LLHUDIcon* LLHUDIcon::handlePick(S32 pick_id)
{
	icon_instance_t::iterator icon_it;

	for(icon_it = sIconInstances.begin(); icon_it != sIconInstances.end(); ++icon_it)
	{
		if (pick_id == (*icon_it)->mPickID)
		{
			return *icon_it;
		}
	}

	return NULL;
}

 //static
void LLHUDIcon::updateAll()
{
	cleanupDeadIcons();
}

//static
BOOL LLHUDIcon::iconsNearby()
{
	return !sIconInstances.empty();
}

//static
void LLHUDIcon::cleanupDeadIcons()
{
	icon_instance_t::iterator icon_it;

	icon_instance_t icons_to_erase;
	for(icon_it = sIconInstances.begin(); icon_it != sIconInstances.end(); ++icon_it)
	{
		if ((*icon_it)->mDead)
		{
			icons_to_erase.push_back(*icon_it);
		}
	}

	for(icon_it = icons_to_erase.begin(); icon_it != icons_to_erase.end(); ++icon_it)
	{
		icon_instance_t::iterator found_it = std::find(sIconInstances.begin(), sIconInstances.end(), *icon_it);
		if (found_it != sIconInstances.end())
		{
			sIconInstances.erase(found_it);
		}
	}
}

//static
S32 LLHUDIcon::getNumInstances()
{
	return (S32)sIconInstances.size();
}
