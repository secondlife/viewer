/** 
 * @file llflexibleobject.h
 * @author JJ Ventrella, Andrew Meadows, Tom Yedwab
 * @brief Flexible object definition
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

/**
 * This is for specifying objects in the world that are animated and 
 * rendered locally - on the viewer. Flexible Objects are linear arrays
 * of positions, which stay at a fixed distance from each other. One 
 * position is fixed as an "anchor" and is attached to some other object 
 * in the world, determined by the server. All the other positions are 
 * updated according to local physics. 
 */

#ifndef LL_LLFLEXIBLEOBJECT_H
#define LL_LLFLEXIBLEOBJECT_H

#include "llprimitive.h"
#include "llvovolume.h"
#include "llwind.h"

// 10 ms for the whole thing!
const F32	FLEXIBLE_OBJECT_TIMESLICE		= 0.003f;
const U32	FLEXIBLE_OBJECT_MAX_LOD			= 10;

// See llprimitive.h for LLFlexibleObjectData and DEFAULT/MIN/MAX values 

//-------------------------------------------------------------------

struct LLFlexibleObjectSection
{
	// Input parameters
	LLVector2		mScale;
	LLQuaternion	mAxisRotation;
	// Simulated state
	LLVector3		mPosition;
	LLVector3		mVelocity;
	LLVector3		mDirection;
	LLQuaternion	mRotation;
	// Derivatives (Not all currently used, will come back with LLVolume changes to automagically generate normals)
	LLVector3		mdPosition;
	//LLMatrix4		mRotScale;
	//LLMatrix4		mdRotScale;
};

//---------------------------------------------------------
// The LLVolumeImplFlexible class 
//---------------------------------------------------------
class LLVolumeImplFlexible : public LLVolumeInterface
{
	public:
		LLVolumeImplFlexible(LLViewerObject* volume, LLFlexibleObjectData* attributes);

		// Implements LLVolumeInterface
		U32 getID() const { return mID; }
		LLVector3 getFramePosition() const;
		LLQuaternion getFrameRotation() const;
		LLVolumeInterfaceType getInterfaceType() const		{ return INTERFACE_FLEXIBLE; }
		BOOL doIdleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
		BOOL doUpdateGeometry(LLDrawable *drawable);
		LLVector3 getPivotPosition() const;
		void onSetVolume(const LLVolumeParams &volume_params, const S32 detail);
		void onSetScale(const LLVector3 &scale, BOOL damped);
		void onParameterChanged(U16 param_type, LLNetworkData *data, BOOL in_use, bool local_origin);
		void onShift(const LLVector3 &shift_vector);
		bool isVolumeUnique() const { return true; }
		bool isVolumeGlobal() const { return true; }
		bool isActive() const { return true; }
		const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const;
		void updateRelativeXform();
		void doFlexibleUpdate(); // Called to update the simulation
		void doFlexibleRebuild(); // Called to rebuild the geometry
		void preRebuild();

		//void				setAttributes( LLFlexibleObjectData );
		void				setParentPositionAndRotationDirectly( LLVector3 p, LLQuaternion r );
		void				setUsingCollisionSphere( bool u );
		void				setCollisionSphere( LLVector3 position, F32 radius );
		void				setRenderingCollisionSphere( bool r);

		LLVector3			getEndPosition();
		LLQuaternion		getEndRotation();
		LLVector3			getNodePosition( int nodeIndex );
		LLVector3			getAnchorPosition() const;

	private:
		//--------------------------------------
		// private members
		//--------------------------------------
	    // Backlink only; don't make this an LLPointer.
		LLViewerObject*				mVO;
		LLTimer						mTimer;
		LLVector3					mAnchorPosition;
		LLVector3					mParentPosition;
		LLQuaternion				mParentRotation;
		LLQuaternion				mLastFrameRotation;
		LLQuaternion				mLastSegmentRotation;
		BOOL						mInitialized;
		BOOL						mUpdated;
		LLFlexibleObjectData*		mAttributes;
		LLFlexibleObjectSection		mSection	[ (1<<FLEXIBLE_OBJECT_MAX_SECTIONS)+1 ];
		S32							mInitializedRes;
		S32							mSimulateRes;
		S32							mRenderRes;
		U32							mFrameNum;
		LLVector3					mCollisionSpherePosition;
		F32							mCollisionSphereRadius;
		U32							mID;

		//--------------------------------------
		// private methods
		//--------------------------------------
		void setAttributesOfAllSections	(LLVector3* inScale = NULL);

		void remapSections(LLFlexibleObjectSection *source, S32 source_sections,
										 LLFlexibleObjectSection *dest, S32 dest_sections);
		
public:
		// Global setting for update rate
		static F32					sUpdateFactor;

};// end of class definition


#endif // LL_LLFLEXIBLEOBJECT_H
