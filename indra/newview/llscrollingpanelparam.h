/** 
 * @file llscrollingpanelparam.h
 * @brief the scrolling panel containing a list of visual param 
 *  	  panels
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

#ifndef LL_SCROLLINGPANELPARAM_H
#define LL_SCROLLINGPANELPARAM_H

#include "llscrollingpanelparambase.h"

class LLViewerJointMesh;
class LLViewerVisualParam;
class LLWearable;
class LLVisualParamHint;
class LLViewerVisualParam;
class LLJoint;

class LLScrollingPanelParam : public LLScrollingPanelParamBase
{
public:
	LLScrollingPanelParam( const LLPanel::Params& panel_params,
			       LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLWearable* wearable, LLJoint* jointp, BOOL use_hints = TRUE );
	virtual ~LLScrollingPanelParam();

	virtual void		draw();
	virtual void		setVisible( BOOL visible );
	virtual void		updatePanel(BOOL allow_modify);

	static void			onSliderMouseDown(LLUICtrl* ctrl, void* userdata);
	static void			onSliderMouseUp(LLUICtrl* ctrl, void* userdata);

	static void			onHintMinMouseDown(void* userdata);
	static void			onHintMinHeldDown(void* userdata);
	static void			onHintMaxMouseDown(void* userdata);
	static void			onHintMaxHeldDown(void* userdata);
	static void			onHintMinMouseUp(void* userdata);
	static void			onHintMaxMouseUp(void* userdata);

	void				onHintMouseDown( LLVisualParamHint* hint );
	void				onHintHeldDown( LLVisualParamHint* hint );

	F32					weightToPercent( F32 weight );
	F32					percentToWeight( F32 percent );

public:
	// Constants for LLPanelVisualParam
	const static F32 PARAM_STEP_TIME_THRESHOLD;
	
	const static S32 PARAM_HINT_WIDTH;
	const static S32 PARAM_HINT_HEIGHT;

public:
	LLPointer<LLVisualParamHint>	mHintMin;
	LLPointer<LLVisualParamHint>	mHintMax;
	static S32 			sUpdateDelayFrames;
	
protected:
	LLTimer				mMouseDownTimer;	// timer for how long mouse has been held down on a hint.
	F32					mLastHeldTime;
	BOOL mAllowModify;
}; 

#endif
