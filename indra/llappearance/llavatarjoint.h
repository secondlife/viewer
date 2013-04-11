/** 
 * @file llavatarjoint.h
 * @brief Implementation of LLAvatarJoint class
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

#ifndef LL_LLAVATARJOINT_H
#define LL_LLAVATARJOINT_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "lljoint.h"
#include "lljointpickname.h"

class LLFace;
class LLAvatarJointMesh;

extern const F32 DEFAULT_AVATAR_JOINT_LOD;

//-----------------------------------------------------------------------------
// class LLViewerJoint
//-----------------------------------------------------------------------------
class LLAvatarJoint :
	public LLJoint
{
public:
	LLAvatarJoint();
	LLAvatarJoint(S32 joint_num);
	// *TODO: Only used for LLVOAvatarSelf::mScreenp.  *DOES NOT INITIALIZE mResetAfterRestoreOldXform*
	LLAvatarJoint(const std::string &name, LLJoint *parent = NULL);
	virtual ~LLAvatarJoint();

	// Gets the validity of this joint
	BOOL getValid() { return mValid; }

	// Sets the validity of this joint
	virtual void setValid( BOOL valid, BOOL recursive=FALSE );

	// Returns true if this object is transparent.
	// This is used to determine in which order to draw objects.
	virtual BOOL isTransparent() { return mIsTransparent; }

	// Returns true if this object should inherit scale modifiers from its immediate parent
	virtual BOOL inheritScale() { return FALSE; }

	enum Components
	{
		SC_BONE		= 1,
		SC_JOINT	= 2,
		SC_AXES		= 4
	};

	// Selects which skeleton components to draw
	void setSkeletonComponents( U32 comp, BOOL recursive = TRUE );

	// Returns which skeleton components are enables for drawing
	U32 getSkeletonComponents() { return mComponents; }

	// Sets the level of detail for this node as a minimum
	// pixel area threshold.  If the current pixel area for this
	// object is less than the specified threshold, the node is
	// not traversed.  In addition, if a value is specified (not
	// default of 0.0), and the pixel area is larger than the
	// specified minimum, the node is rendered, but no other siblings
	// of this node under the same parent will be.
	F32 getLOD() { return mMinPixelArea; }
	void setLOD( F32 pixelArea ) { mMinPixelArea = pixelArea; }

	void setPickName(LLJointPickName name) { mPickName = name; }
	LLJointPickName getPickName() { return mPickName; }

	void setVisible( BOOL visible, BOOL recursive );

	// Takes meshes in mMeshParts and sets each one as a child joint
	void setMeshesToChildren();

	// LLViewerJoint interface
	virtual U32 render( F32 pixelArea, BOOL first_pass = TRUE, BOOL is_dummy = FALSE ) = 0;
	virtual void updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area);
	virtual void updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind = FALSE, bool terse_update = false);
	virtual BOOL updateLOD(F32 pixel_area, BOOL activate);
	virtual void updateJointGeometry();
	virtual void dump();
	

public:
	static BOOL	sDisableLOD;
	avatar_joint_mesh_list_t mMeshParts; //LLViewerJointMesh*
	void setMeshID( S32 id ) {mMeshID = id;}

protected:
	void init();

	BOOL		mValid;
	BOOL		mIsTransparent;
	U32			mComponents;
	F32			mMinPixelArea;
	LLJointPickName	mPickName;
	BOOL		mVisible;
	S32			mMeshID;
};

class LLAvatarJointCollisionVolume : public LLAvatarJoint
{
public:
	LLAvatarJointCollisionVolume();
	virtual ~LLAvatarJointCollisionVolume() {};

	/*virtual*/ BOOL inheritScale() { return TRUE; }
	/*virtual*/ U32 render( F32 pixelArea, BOOL first_pass = TRUE, BOOL is_dummy = FALSE );

	void renderCollision();

	LLVector3 getVolumePos(LLVector3 &offset);
};

#endif // LL_LLAVATARJOINT_H


