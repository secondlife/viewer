/** 
 * @file lltoolmorph.h
 * @brief A tool to select object faces.
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
		F32 param_weight);	

	/*virtual*/ S8 getType() const ;

	BOOL					needsRender();
	void					preRender(BOOL clear_depth);
	BOOL					render();
	void					requestUpdate( S32 delay_frames ) {mNeedsUpdate = TRUE; mDelayFrames = delay_frames; }
	void					setUpdateDelayFrames( S32 delay_frames ) { mDelayFrames = delay_frames; }
	void					draw();
	
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
	F32						mVisualParamWeight;		// weight for this visual parameter
	BOOL					mAllowsUpdates;		// updates are blocked unless this is true
	S32						mDelayFrames;		// updates are blocked for this many frames
	LLRect					mRect;
	F32						mLastParamWeight;

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

