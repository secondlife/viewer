/** 
 * @file llscrollingpanelparam.cpp
 * @brief UI panel for a list of visual param panels
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llscrollingpanelparam.h"
#include "llviewerjointmesh.h"
#include "llviewervisualparam.h"
#include "llwearable.h"
#include "llviewervisualparam.h"
#include "lltoolmorph.h"
#include "lltrans.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llviewborder.h"
#include "llvoavatarself.h"

// Constants for LLPanelVisualParam
const F32 LLScrollingPanelParam::PARAM_STEP_TIME_THRESHOLD = 0.25f;

const S32 LLScrollingPanelParam::PARAM_HINT_WIDTH = 128;
const S32 LLScrollingPanelParam::PARAM_HINT_HEIGHT = 128;

// LLScrollingPanelParam
//static
S32 LLScrollingPanelParam::sUpdateDelayFrames = 0;

LLScrollingPanelParam::LLScrollingPanelParam( const LLPanel::Params& panel_params,
					      LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLWearable* wearable, LLJoint* jointp, BOOL use_hints )
    : LLScrollingPanelParamBase( panel_params, mesh, param, allow_modify, wearable, jointp, use_hints)
{
	// *HACK To avoid hard coding texture position, lets use border's position for texture. 
	LLViewBorder* left_border = getChild<LLViewBorder>("left_border");

	static LLUICachedControl<S32> slider_ctrl_height ("UISliderctrlHeight", 0);
	S32 pos_x = left_border->getRect().mLeft + left_border->getBorderWidth();
	S32 pos_y = left_border->getRect().mBottom + left_border->getBorderWidth();
	F32 min_weight = param->getMinWeight();
	F32 max_weight = param->getMaxWeight();

	mHintMin = new LLVisualParamHint( pos_x, pos_y, PARAM_HINT_WIDTH, PARAM_HINT_HEIGHT, mesh, (LLViewerVisualParam*) wearable->getVisualParam(param->getID()), wearable,  min_weight, jointp);
	pos_x = getChild<LLViewBorder>("right_border")->getRect().mLeft + left_border->getBorderWidth();
	mHintMax = new LLVisualParamHint( pos_x, pos_y, PARAM_HINT_WIDTH, PARAM_HINT_HEIGHT, mesh, (LLViewerVisualParam*) wearable->getVisualParam(param->getID()), wearable, max_weight, jointp );
	
	mHintMin->setAllowsUpdates( FALSE );
	mHintMax->setAllowsUpdates( FALSE );

	std::string min_name = LLTrans::getString(param->getMinDisplayName());
	std::string max_name = LLTrans::getString(param->getMaxDisplayName());
	getChild<LLUICtrl>("min param text")->setValue(min_name);
	getChild<LLUICtrl>("max param text")->setValue(max_name);

	LLButton* less = getChild<LLButton>("less");
	if (less)
	{
		less->setMouseDownCallback( LLScrollingPanelParam::onHintMinMouseDown, this );
		less->setMouseUpCallback( LLScrollingPanelParam::onHintMinMouseUp, this );
		less->setHeldDownCallback( LLScrollingPanelParam::onHintMinHeldDown, this );
		less->setHeldDownDelay( PARAM_STEP_TIME_THRESHOLD );
	}

	LLButton* more = getChild<LLButton>("more");
	if (more)
	{
		more->setMouseDownCallback( LLScrollingPanelParam::onHintMaxMouseDown, this );
		more->setMouseUpCallback( LLScrollingPanelParam::onHintMaxMouseUp, this );
		more->setHeldDownCallback( LLScrollingPanelParam::onHintMaxHeldDown, this );
		more->setHeldDownDelay( PARAM_STEP_TIME_THRESHOLD );
	}

	setVisible(FALSE);
	setBorderVisible( FALSE );
}

LLScrollingPanelParam::~LLScrollingPanelParam()
{
}
void LLScrollingPanelParam::updatePanel(BOOL allow_modify)
{
	if (!mWearable)
	{
		// not editing a wearable just now, no update necessary
		return;
	}
	LLScrollingPanelParamBase::updatePanel(allow_modify);

	mHintMin->requestUpdate( sUpdateDelayFrames++ );
	mHintMax->requestUpdate( sUpdateDelayFrames++ );
	getChildView("less")->setEnabled(mAllowModify);
	getChildView("more")->setEnabled(mAllowModify);
}

void LLScrollingPanelParam::setVisible( BOOL visible )
{
	if( getVisible() != visible )
	{
		LLPanel::setVisible( visible );
		if (mHintMin)
			mHintMin->setAllowsUpdates( visible );
		if (mHintMax)
			mHintMax->setAllowsUpdates( visible );

		if( visible )
		{
			if (mHintMin)
				mHintMin->setUpdateDelayFrames( sUpdateDelayFrames++ );
			if (mHintMax)
				mHintMax->setUpdateDelayFrames( sUpdateDelayFrames++ );
		}
	}
}

void LLScrollingPanelParam::draw()
{
	if( !mWearable )
	{
		return;
	}
	
	getChildView("less")->setVisible( mHintMin->getVisible());
	getChildView("more")->setVisible( mHintMax->getVisible());

	// hide borders if texture has been loaded
	getChildView("left_border")->setVisible( !mHintMin->getVisible());
	getChildView("right_border")->setVisible( !mHintMax->getVisible());

	// Draw all the children except for the labels
	getChildView("min param text")->setVisible( FALSE );
	getChildView("max param text")->setVisible( FALSE );
	LLPanel::draw();
	
	// If we're in a focused floater, don't apply the floater's alpha to visual param hint,
	// making its behavior similar to texture controls'.
	F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();

	// Draw the hints over the "less" and "more" buttons.
	gGL.pushUIMatrix();
	{
		const LLRect& r = mHintMin->getRect();
		gGL.translateUI((F32)r.mLeft, (F32)r.mBottom, 0.f);
		mHintMin->draw(alpha);
	}
	gGL.popUIMatrix();

	gGL.pushUIMatrix();
	{
		const LLRect& r = mHintMax->getRect();
		gGL.translateUI((F32)r.mLeft, (F32)r.mBottom, 0.f);
		mHintMax->draw(alpha);
	}
	gGL.popUIMatrix();


	// Draw labels on top of the buttons
	getChildView("min param text")->setVisible( TRUE );
	drawChild(getChild<LLView>("min param text"));

	getChildView("max param text")->setVisible( TRUE );
	drawChild(getChild<LLView>("max param text"));
}

// static
void LLScrollingPanelParam::onSliderMouseDown(LLUICtrl* ctrl, void* userdata)
{
}

// static
void LLScrollingPanelParam::onSliderMouseUp(LLUICtrl* ctrl, void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	LLVisualParamHint::requestHintUpdates( self->mHintMin, self->mHintMax );
}

// static
void LLScrollingPanelParam::onHintMinMouseDown( void* userdata )
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	self->onHintMouseDown( self->mHintMin );
}

// static
void LLScrollingPanelParam::onHintMaxMouseDown( void* userdata )
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	self->onHintMouseDown( self->mHintMax );
}


void LLScrollingPanelParam::onHintMouseDown( LLVisualParamHint* hint )
{
	// morph towards this result
	F32 current_weight = mWearable->getVisualParamWeight( hint->getVisualParam()->getID() );

	// if we have maxed out on this morph, we shouldn't be able to click it
	if( hint->getVisualParamWeight() != current_weight )
	{
		mMouseDownTimer.reset();
		mLastHeldTime = 0.f;
	}
}

// static
void LLScrollingPanelParam::onHintMinHeldDown( void* userdata )
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	self->onHintHeldDown( self->mHintMin );
}

// static
void LLScrollingPanelParam::onHintMaxHeldDown( void* userdata )
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	self->onHintHeldDown( self->mHintMax );
}
	
void LLScrollingPanelParam::onHintHeldDown( LLVisualParamHint* hint )
{
	F32 current_weight = mWearable->getVisualParamWeight( hint->getVisualParam()->getID() );

	if (current_weight != hint->getVisualParamWeight() )
	{
		const F32 FULL_BLEND_TIME = 2.f;
		F32 elapsed_time = mMouseDownTimer.getElapsedTimeF32() - mLastHeldTime;
		mLastHeldTime += elapsed_time;

		F32 new_weight;
		if (current_weight > hint->getVisualParamWeight() )
		{
			new_weight = current_weight - (elapsed_time / FULL_BLEND_TIME);
		}
		else
		{
			new_weight = current_weight + (elapsed_time / FULL_BLEND_TIME);
		}

		// Make sure we're not taking the slider out of bounds
		// (this is where some simple UI limits are stored)
		F32 new_percent = weightToPercent(new_weight);
		LLSliderCtrl* slider = getChild<LLSliderCtrl>("param slider");
		if (slider)
		{
			if (slider->getMinValue() < new_percent
				&& new_percent < slider->getMaxValue())
			{
				mWearable->setVisualParamWeight( hint->getVisualParam()->getID(), new_weight, FALSE);
				mWearable->writeToAvatar(gAgentAvatarp);
				gAgentAvatarp->updateVisualParams();

				slider->setValue( weightToPercent( new_weight ) );
			}
		}
	}
}

// static
void LLScrollingPanelParam::onHintMinMouseUp( void* userdata )
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;

	F32 elapsed_time = self->mMouseDownTimer.getElapsedTimeF32();

	LLVisualParamHint* hint = self->mHintMin;

	if (elapsed_time < PARAM_STEP_TIME_THRESHOLD)
	{
		// step in direction
		F32 current_weight = self->mWearable->getVisualParamWeight( hint->getVisualParam()->getID() );
		F32 range = self->mHintMax->getVisualParamWeight() - self->mHintMin->getVisualParamWeight();
		// step a fraction in the negative directiona
		F32 new_weight = current_weight - (range / 10.f);
		F32 new_percent = self->weightToPercent(new_weight);
		LLSliderCtrl* slider = self->getChild<LLSliderCtrl>("param slider");
		if (slider)
		{
			if (slider->getMinValue() < new_percent
				&& new_percent < slider->getMaxValue())
			{
				self->mWearable->setVisualParamWeight(hint->getVisualParam()->getID(), new_weight, FALSE);
				self->mWearable->writeToAvatar(gAgentAvatarp);
				slider->setValue( self->weightToPercent( new_weight ) );
			}
		}
	}

	LLVisualParamHint::requestHintUpdates( self->mHintMin, self->mHintMax );
}

void LLScrollingPanelParam::onHintMaxMouseUp( void* userdata )
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;

	F32 elapsed_time = self->mMouseDownTimer.getElapsedTimeF32();

	if (isAgentAvatarValid())
	{
		LLVisualParamHint* hint = self->mHintMax;

		if (elapsed_time < PARAM_STEP_TIME_THRESHOLD)
		{
			// step in direction
			F32 current_weight = self->mWearable->getVisualParamWeight( hint->getVisualParam()->getID() );
			F32 range = self->mHintMax->getVisualParamWeight() - self->mHintMin->getVisualParamWeight();
			// step a fraction in the negative direction
			F32 new_weight = current_weight + (range / 10.f);
			F32 new_percent = self->weightToPercent(new_weight);
			LLSliderCtrl* slider = self->getChild<LLSliderCtrl>("param slider");
			if (slider)
			{
				if (slider->getMinValue() < new_percent
					&& new_percent < slider->getMaxValue())
				{
					self->mWearable->setVisualParamWeight(hint->getVisualParam()->getID(), new_weight, FALSE);
					self->mWearable->writeToAvatar(gAgentAvatarp);
					slider->setValue( self->weightToPercent( new_weight ) );
				}
			}
		}
	}

	LLVisualParamHint::requestHintUpdates( self->mHintMin, self->mHintMax );
}


F32 LLScrollingPanelParam::weightToPercent( F32 weight )
{
	LLViewerVisualParam* param = mParam;
	return (weight - param->getMinWeight()) /  (param->getMaxWeight() - param->getMinWeight()) * 100.f;
}

F32 LLScrollingPanelParam::percentToWeight( F32 percent )
{
	LLViewerVisualParam* param = mParam;
	return percent / 100.f * (param->getMaxWeight() - param->getMinWeight()) + param->getMinWeight();
}
