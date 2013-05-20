/** 
 * @file lltoolmorph.h
 * @brief A tool to select object faces.
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

#ifndef LL_LLTOOLMORPH_H
#define LL_LLTOOLMORPH_H

#include "lltool.h"
#include "m4math.h"
#include "v2math.h"
#include "lldynamictexture.h"
#include "llundo.h"
#include "lltextbox.h"
#include "llstrider.h"
#include "llviewervisualparam.h"
#include "llframetimer.h"
#include "llviewertexture.h"

class LLViewerJointMesh;
class LLPolyMesh;
class LLViewerObject;
class LLJoint;

//-----------------------------------------------------------------------------
// LLVisualParamHint
//-----------------------------------------------------------------------------
class LLVisualParamHint : public LLViewerDynamicTexture
{
protected:
	virtual ~LLVisualParamHint();

public:
	LLVisualParamHint(
		S32 pos_x, S32 pos_y,
		S32 width, S32 height, 
		LLViewerJointMesh *mesh, 
		LLViewerVisualParam *param,
		LLWearable *wearable,
		F32 param_weight, 
		LLJoint* jointp);	

	/*virtual*/ S8 getType() const ;

	BOOL					needsRender();
	void					preRender(BOOL clear_depth);
	BOOL					render();
	void					requestUpdate( S32 delay_frames ) {mNeedsUpdate = TRUE; mDelayFrames = delay_frames; }
	void					setUpdateDelayFrames( S32 delay_frames ) { mDelayFrames = delay_frames; }
	void					draw(F32 alpha);
	
	LLViewerVisualParam*	getVisualParam() { return mVisualParam; }
	F32						getVisualParamWeight() { return mVisualParamWeight; }
	BOOL					getVisible() { return mIsVisible; }

	void					setAllowsUpdates( BOOL b ) { mAllowsUpdates = b; }

	const LLRect&			getRect()	{ return mRect; }

	// Requests updates for all instances (excluding two possible exceptions)  Grungy but efficient.
	static void				requestHintUpdates( LLVisualParamHint* exception1 = NULL, LLVisualParamHint* exception2 = NULL );

protected:
	BOOL					mNeedsUpdate;		// does this texture need to be re-rendered?
	BOOL					mIsVisible;			// is this distortion hint visible?
	LLViewerJointMesh*		mJointMesh;			// mesh that this distortion applies to
	LLViewerVisualParam*	mVisualParam;		// visual param applied by this hint
	LLWearable*				mWearablePtr;		// wearable we're editing
	F32						mVisualParamWeight;		// weight for this visual parameter
	BOOL					mAllowsUpdates;		// updates are blocked unless this is true
	S32						mDelayFrames;		// updates are blocked for this many frames
	LLRect					mRect;
	F32						mLastParamWeight;
	LLJoint*				mCamTargetJoint;	// joint to target with preview camera

	LLUIImagePtr mBackgroundp;

	typedef std::set< LLVisualParamHint* > instance_list_t;
	static instance_list_t sInstances;
};

// this class resets avatar data at the end of an update cycle
class LLVisualParamReset : public LLViewerDynamicTexture
{
protected:
	/*virtual */ ~LLVisualParamReset(){}
public:
	LLVisualParamReset();
	/*virtual */ BOOL render();
	/*virtual*/ S8 getType() const ;

	static BOOL sDirty;
};

#endif

