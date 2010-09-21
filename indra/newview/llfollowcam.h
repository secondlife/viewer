/** 
 * @file llfollowcam.h
 * @author Jeffrey Ventrella
 * @brief LLFollowCam class definition
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

//--------------------------------------------------------------------
// FollowCam
//
// The FollowCam controls three dynamic variables which determine 
// a camera orientation and position for a "loose" third-person view
// (orientation being derived from a combination of focus and up 
// vector). It is good for fast-moving vehicles that change 
// acceleration a lot, but it can also be general-purpose, like for 
// avatar navigation. It has a handful of parameters allowing it to 
// be tweaked to assume different styles of tracking objects.
//
//--------------------------------------------------------------------
#ifndef LL_FOLLOWCAM_H
#define LL_FOLLOWCAM_H

#include "llcoordframe.h"
#include "indra_constants.h"
#include "llmath.h"
#include "lltimer.h"
#include "llquaternion.h"
#include "llcriticaldamp.h"
#include <map>
#include <vector>

class LLFollowCamParams
{
public:
	LLFollowCamParams();
	virtual ~LLFollowCamParams();
	
	//--------------------------------------
	// setty setty set set
	//--------------------------------------
	virtual void setPositionLag			( F32 );
	virtual void setFocusLag			( F32 );
	virtual void setFocusThreshold		( F32 );
	virtual void setPositionThreshold	( F32 );
	virtual void setDistance			( F32 );
	virtual void setPitch				( F32 );
	virtual void setFocusOffset			( const LLVector3& );
	virtual void setBehindnessAngle		( F32 );
	virtual void setBehindnessLag		( F32 );
	virtual void setPosition			( const LLVector3& );
	virtual void setFocus				( const LLVector3& );
	virtual void setPositionLocked		( bool );
	virtual void setFocusLocked			( bool );


	//--------------------------------------
	// getty getty get get
	//--------------------------------------
	virtual F32			getPositionLag() const;
	virtual F32			getFocusLag() const;
	virtual F32			getPositionThreshold() const;
	virtual F32			getFocusThreshold() const;
	virtual F32			getDistance() const;
	virtual F32			getPitch() const;
	virtual LLVector3	getFocusOffset() const;
	virtual F32			getBehindnessAngle() const;
	virtual F32			getBehindnessLag() const;
	virtual LLVector3	getPosition() const;
	virtual LLVector3	getFocus() const;
	virtual bool		getFocusLocked() const;
	virtual bool		getPositionLocked() const;
	virtual bool		getUseFocus() const { return mUseFocus; }
	virtual bool		getUsePosition() const { return mUsePosition; }

protected:
	F32		mPositionLag;
	F32		mFocusLag;
	F32		mFocusThreshold;
	F32		mPositionThreshold;
	F32		mDistance;	
	F32		mPitch;
	LLVector3	mFocusOffset;
	F32		mBehindnessMaxAngle;
	F32		mBehindnessLag;
	F32		mMaxCameraDistantFromSubject;

	bool			mPositionLocked;
	bool			mFocusLocked;
	bool			mUsePosition; // specific camera point specified by script
	bool			mUseFocus; // specific focus point specified by script
	LLVector3		mPosition;			// where the camera is (in world-space)
	LLVector3		mFocus;				// what the camera is aimed at (in world-space)
};

class LLFollowCam : public LLFollowCamParams
{
public:
	//--------------------
	// Contructor
	//--------------------
	LLFollowCam();

	//--------------------
	// Destructor
	//--------------------
	virtual ~LLFollowCam();

	//---------------------------------------------------------------------------------------
	// The following methods must be called every time step. However, if you know for 
	// sure that your  subject matter (what the camera is looking at) is not moving, 
	// then you can get away with not calling "update" But keep in mind that "update" 
	// may still be needed after the subject matter has stopped moving because the 
	// camera may still need to animate itself catching up to its ideal resting place. 
	//---------------------------------------------------------------------------------------
	void setSubjectPositionAndRotation	( const LLVector3 p, const LLQuaternion r );
	void update();

	// initialize from another instance of llfollowcamparams
	void copyParams(LLFollowCamParams& params);

	//-----------------------------------------------------------------------------------
	// this is how to bang the followCam into a specific configuration. Keep in mind 
	// that it will immediately try to adjust these values according to its attributes. 
	//-----------------------------------------------------------------------------------
	void reset( const LLVector3 position, const LLVector3 focus, const LLVector3 upVector );

	void setMaxCameraDistantFromSubject	( F32 m ); // this should be determined by llAgent
	bool isZoomedToMinimumDistance();
	LLVector3	getUpVector();
	void zoom( S32 );

	// overrides for setters and getters
	virtual void setPitch( F32 );
	virtual void setDistance( F32 );
	virtual void setPosition(const LLVector3& pos);
	virtual void setFocus(const LLVector3& focus);
	virtual void setPositionLocked		( bool );
	virtual void setFocusLocked			( bool );

	LLVector3	getSimulatedPosition() const;
	LLVector3	getSimulatedFocus() const;

	//------------------------------------------
	// protected members of FollowCam
	//------------------------------------------
protected:
	F32		mPitchCos;					// derived from mPitch
	F32		mPitchSin;					// derived from mPitch
	LLGlobalVec		mSimulatedPositionGlobal;		// where the camera is (global coordinates), simulated
	LLGlobalVec		mSimulatedFocusGlobal;		// what the camera is aimed at (global coordinates), simulated
	F32				mSimulatedDistance;

	//---------------------
	// dynamic variables
	//---------------------
	bool			mZoomedToMinimumDistance;
	LLFrameTimer	mTimer;
	LLVector3		mSubjectPosition;	// this is the position of what I'm looking at
	LLQuaternion	mSubjectRotation;	// this is the rotation of what I'm looking at
	LLVector3		mUpVector;			// the camera's up vector in world-space (determines roll)
	LLVector3		mRelativeFocus;
	LLVector3		mRelativePos;

	bool mPitchSineAndCosineNeedToBeUpdated;

	//------------------------------------------
	// protected methods of FollowCam
	//------------------------------------------
protected:
		void	calculatePitchSineAndCosine();
		BOOL	updateBehindnessConstraint(LLVector3 focus, LLVector3& cam_position);

};// end of FollowCam class


class LLFollowCamMgr
{
public:
	static void cleanupClass			( );
	
	static void setPositionLag			( const LLUUID& source, F32 lag);
	static void setFocusLag				( const LLUUID& source, F32 lag);
	static void setFocusThreshold		( const LLUUID& source, F32 threshold);
	static void setPositionThreshold	( const LLUUID& source, F32 threshold);
	static void setDistance				( const LLUUID& source, F32 distance);
	static void setPitch				( const LLUUID& source, F32 pitch);
	static void setFocusOffset			( const LLUUID& source, const LLVector3& offset);
	static void setBehindnessAngle		( const LLUUID& source, F32 angle);
	static void setBehindnessLag		( const LLUUID& source, F32 lag);
	static void setPosition				( const LLUUID& source, const LLVector3 position);
	static void setFocus				( const LLUUID& source, const LLVector3 focus);
	static void setPositionLocked		( const LLUUID& source, bool locked);
	static void setFocusLocked			( const LLUUID& source, bool locked );

	static void setCameraActive			( const LLUUID& source, bool active );

	static LLFollowCamParams* getActiveFollowCamParams();
	static LLFollowCamParams* getParamsForID(const LLUUID& source);
	static void removeFollowCamParams(const LLUUID& source);
	static bool isScriptedCameraSource(const LLUUID& source);
	static void dump();

protected:

	typedef std::map<LLUUID, LLFollowCamParams*> param_map_t;
	static param_map_t sParamMap;

	typedef std::vector<LLFollowCamParams*> param_stack_t;
	static param_stack_t sParamStack;
};

#endif //LL_FOLLOWCAM_H
