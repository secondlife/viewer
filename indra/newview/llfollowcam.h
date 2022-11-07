/** 
 * @file llfollowcam.h
 * @author Jeffrey Ventrella
 * @brief LLFollowCam class definition
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
    virtual void setPositionLag         ( F32 );
    virtual void setFocusLag            ( F32 );
    virtual void setFocusThreshold      ( F32 );
    virtual void setPositionThreshold   ( F32 );
    virtual void setDistance            ( F32 );
    virtual void setPitch               ( F32 );
    virtual void setFocusOffset         ( const LLVector3& );
    virtual void setBehindnessAngle     ( F32 );
    virtual void setBehindnessLag       ( F32 );
    virtual void setPosition            ( const LLVector3& );
    virtual void setFocus               ( const LLVector3& );
    virtual void setPositionLocked      ( bool );
    virtual void setFocusLocked         ( bool );


    //--------------------------------------
    // getty getty get get
    //--------------------------------------
    virtual F32         getPositionLag() const;
    virtual F32         getFocusLag() const;
    virtual F32         getPositionThreshold() const;
    virtual F32         getFocusThreshold() const;
    virtual F32         getDistance() const;
    virtual F32         getPitch() const;
    virtual LLVector3   getFocusOffset() const;
    virtual F32         getBehindnessAngle() const;
    virtual F32         getBehindnessLag() const;
    virtual LLVector3   getPosition() const;
    virtual LLVector3   getFocus() const;
    virtual bool        getFocusLocked() const;
    virtual bool        getPositionLocked() const;
    virtual bool        getUseFocus() const { return mUseFocus; }
    virtual bool        getUsePosition() const { return mUsePosition; }

protected:
    F32     mPositionLag;
    F32     mFocusLag;
    F32     mFocusThreshold;
    F32     mPositionThreshold;
    F32     mDistance;  
    F32     mPitch;
    LLVector3   mFocusOffset;
    F32     mBehindnessMaxAngle;
    F32     mBehindnessLag;
    F32     mMaxCameraDistantFromSubject;

    bool            mPositionLocked;
    bool            mFocusLocked;
    bool            mUsePosition; // specific camera point specified by script
    bool            mUseFocus; // specific focus point specified by script
    LLVector3       mPosition;          // where the camera is (in world-space)
    LLVector3       mFocus;             // what the camera is aimed at (in world-space)
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
    void setSubjectPositionAndRotation  ( const LLVector3 p, const LLQuaternion r );
    void update();

    // initialize from another instance of llfollowcamparams
    void copyParams(LLFollowCamParams& params);

    //-----------------------------------------------------------------------------------
    // this is how to bang the followCam into a specific configuration. Keep in mind 
    // that it will immediately try to adjust these values according to its attributes. 
    //-----------------------------------------------------------------------------------
    void reset( const LLVector3 position, const LLVector3 focus, const LLVector3 upVector );

    void setMaxCameraDistantFromSubject ( F32 m ); // this should be determined by llAgent
    bool isZoomedToMinimumDistance();
    LLVector3   getUpVector();
    void zoom( S32 );

    // overrides for setters and getters
    virtual void setPitch( F32 );
    virtual void setDistance( F32 );
    virtual void setPosition(const LLVector3& pos);
    virtual void setFocus(const LLVector3& focus);
    virtual void setPositionLocked      ( bool );
    virtual void setFocusLocked         ( bool );

    LLVector3   getSimulatedPosition() const;
    LLVector3   getSimulatedFocus() const;

    //------------------------------------------
    // protected members of FollowCam
    //------------------------------------------
protected:
    F32     mPitchCos;                  // derived from mPitch
    F32     mPitchSin;                  // derived from mPitch
    LLGlobalVec     mSimulatedPositionGlobal;       // where the camera is (global coordinates), simulated
    LLGlobalVec     mSimulatedFocusGlobal;      // what the camera is aimed at (global coordinates), simulated
    F32             mSimulatedDistance;

    //---------------------
    // dynamic variables
    //---------------------
    bool            mZoomedToMinimumDistance;
    LLFrameTimer    mTimer;
    LLVector3       mSubjectPosition;   // this is the position of what I'm looking at
    LLQuaternion    mSubjectRotation;   // this is the rotation of what I'm looking at
    LLVector3       mUpVector;          // the camera's up vector in world-space (determines roll)
    LLVector3       mRelativeFocus;
    LLVector3       mRelativePos;

    bool mPitchSineAndCosineNeedToBeUpdated;

    //------------------------------------------
    // protected methods of FollowCam
    //------------------------------------------
protected:
        void    calculatePitchSineAndCosine();
        BOOL    updateBehindnessConstraint(LLVector3 focus, LLVector3& cam_position);

};// end of FollowCam class


class LLFollowCamMgr : public LLSingleton<LLFollowCamMgr>
{
    LLSINGLETON(LLFollowCamMgr);
    ~LLFollowCamMgr();
public: 
    void setPositionLag         ( const LLUUID& source, F32 lag);
    void setFocusLag                ( const LLUUID& source, F32 lag);
    void setFocusThreshold      ( const LLUUID& source, F32 threshold);
    void setPositionThreshold   ( const LLUUID& source, F32 threshold);
    void setDistance                ( const LLUUID& source, F32 distance);
    void setPitch               ( const LLUUID& source, F32 pitch);
    void setFocusOffset         ( const LLUUID& source, const LLVector3& offset);
    void setBehindnessAngle     ( const LLUUID& source, F32 angle);
    void setBehindnessLag       ( const LLUUID& source, F32 lag);
    void setPosition                ( const LLUUID& source, const LLVector3 position);
    void setFocus               ( const LLUUID& source, const LLVector3 focus);
    void setPositionLocked      ( const LLUUID& source, bool locked);
    void setFocusLocked         ( const LLUUID& source, bool locked );

    void setCameraActive            ( const LLUUID& source, bool active );

    LLFollowCamParams* getActiveFollowCamParams();
    LLFollowCamParams* getParamsForID(const LLUUID& source);
    void removeFollowCamParams(const LLUUID& source);
    bool isScriptedCameraSource(const LLUUID& source);
    void dump();

protected:

    typedef std::map<LLUUID, LLFollowCamParams*> param_map_t;
    param_map_t mParamMap;

    typedef std::vector<LLFollowCamParams*> param_stack_t;
    param_stack_t mParamStack;
};

#endif //LL_FOLLOWCAM_H
