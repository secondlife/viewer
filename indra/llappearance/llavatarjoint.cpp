/** 
 * @file llavatarjoint.cpp
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llavatarjoint.h"

#include "llgl.h"
#include "llrender.h"
#include "llmath.h"
#include "llglheaders.h"
#include "llavatarappearance.h"

const F32 DEFAULT_AVATAR_JOINT_LOD = 0.0f;

//-----------------------------------------------------------------------------
// Static Data
//-----------------------------------------------------------------------------
BOOL					LLAvatarJoint::sDisableLOD = FALSE;

//-----------------------------------------------------------------------------
// LLAvatarJoint()
// Class Constructors
//-----------------------------------------------------------------------------
LLAvatarJoint::LLAvatarJoint() :
	LLJoint()
{
	init();
}

LLAvatarJoint::LLAvatarJoint(const std::string &name, LLJoint *parent) :
	LLJoint(name, parent)
{
	init();
}

LLAvatarJoint::LLAvatarJoint(S32 joint_num) :
	LLJoint(joint_num)
{
	init();
}


void LLAvatarJoint::init()
{
	mValid = FALSE;
	mComponents = SC_JOINT | SC_BONE | SC_AXES;
	mMinPixelArea = DEFAULT_AVATAR_JOINT_LOD;
	mPickName = PN_DEFAULT;
	mVisible = TRUE;
	mMeshID = 0;
	mIsTransparent = FALSE;
}


//-----------------------------------------------------------------------------
// ~LLAvatarJoint()
// Class Destructor
//-----------------------------------------------------------------------------
LLAvatarJoint::~LLAvatarJoint()
{
}


//--------------------------------------------------------------------
// setValid()
//--------------------------------------------------------------------
void LLAvatarJoint::setValid( BOOL valid, BOOL recursive )
{
	//----------------------------------------------------------------
	// set visibility for this joint
	//----------------------------------------------------------------
	mValid = valid;
	
	//----------------------------------------------------------------
	// set visibility for children
	//----------------------------------------------------------------
	if (recursive)
	{
		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLAvatarJoint* joint = (LLAvatarJoint*)(*iter);
			joint->setValid(valid, TRUE);
		}
	}

}

//--------------------------------------------------------------------
// setSkeletonComponents()
//--------------------------------------------------------------------
void LLAvatarJoint::setSkeletonComponents( U32 comp, BOOL recursive )
{
	mComponents = comp;
	if (recursive)
	{
		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLAvatarJoint* joint = dynamic_cast<LLAvatarJoint*>(*iter);
			joint->setSkeletonComponents(comp, recursive);
		}
	}
}

void LLAvatarJoint::setVisible(BOOL visible, BOOL recursive)
{
	mVisible = visible;

	if (recursive)
	{
		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLAvatarJoint* joint = (LLAvatarJoint*)(*iter);
			joint->setVisible(visible, recursive);
		}
	}
}

void LLAvatarJoint::updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area)
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLAvatarJoint* joint = dynamic_cast<LLAvatarJoint*>(*iter);
		joint->updateFaceSizes(num_vertices, num_indices, pixel_area);
	}
}

void LLAvatarJoint::updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind, bool terse_update)
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLAvatarJoint* joint = dynamic_cast<LLAvatarJoint*>(*iter);
		joint->updateFaceData(face, pixel_area, damp_wind, terse_update);
	}
}

void LLAvatarJoint::updateJointGeometry()
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLAvatarJoint* joint = dynamic_cast<LLAvatarJoint*>(*iter);
		joint->updateJointGeometry();
	}
}


BOOL LLAvatarJoint::updateLOD(F32 pixel_area, BOOL activate)
{
	BOOL lod_changed = FALSE;
	BOOL found_lod = FALSE;

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLAvatarJoint* joint = dynamic_cast<LLAvatarJoint*>(*iter);
		F32 jointLOD = joint->getLOD();
		
		if (found_lod || jointLOD == DEFAULT_AVATAR_JOINT_LOD)
		{
			// we've already found a joint to enable, so enable the rest as alternatives
			lod_changed |= joint->updateLOD(pixel_area, TRUE);
		}
		else
		{
			if (pixel_area >= jointLOD || sDisableLOD)
			{
				lod_changed |= joint->updateLOD(pixel_area, TRUE);
				found_lod = TRUE;
			}
			else
			{
				lod_changed |= joint->updateLOD(pixel_area, FALSE);
			}
		}
	}
	return lod_changed;
}

void LLAvatarJoint::dump()
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLAvatarJoint* joint = dynamic_cast<LLAvatarJoint*>(*iter);
		joint->dump();
	}
}


void LLAvatarJoint::setMeshesToChildren()
{
	removeAllChildren();
	for (avatar_joint_mesh_list_t::iterator iter = mMeshParts.begin();
		iter != mMeshParts.end(); iter++)
	{
		addChild((*iter));
	}
}
//-----------------------------------------------------------------------------
// LLAvatarJointCollisionVolume()
//-----------------------------------------------------------------------------

LLAvatarJointCollisionVolume::LLAvatarJointCollisionVolume()
{
	mUpdateXform = FALSE;
}

/*virtual*/
U32 LLAvatarJointCollisionVolume::render( F32 pixelArea, BOOL first_pass, BOOL is_dummy )
{
	llerrs << "Cannot call render() on LLAvatarJointCollisionVolume" << llendl;
	return 0;
}

LLVector3 LLAvatarJointCollisionVolume::getVolumePos(LLVector3 &offset)
{
	mUpdateXform = TRUE;
	
	LLVector3 result = offset;
	result.scaleVec(getScale());
	result.rotVec(getWorldRotation());
	result += getWorldPosition();

	return result;
}

void LLAvatarJointCollisionVolume::renderCollision()
{
	updateWorldMatrix();
	
	gGL.pushMatrix();
	gGL.multMatrix( &mXform.getWorldMatrix().mMatrix[0][0] );

	gGL.diffuseColor3f( 0.f, 0.f, 1.f );
	
	gGL.begin(LLRender::LINES);
	
	LLVector3 v[] = 
	{
		LLVector3(1,0,0),
		LLVector3(-1,0,0),
		LLVector3(0,1,0),
		LLVector3(0,-1,0),

		LLVector3(0,0,-1),
		LLVector3(0,0,1),
	};

	//sides
	gGL.vertex3fv(v[0].mV); 
	gGL.vertex3fv(v[2].mV);

	gGL.vertex3fv(v[0].mV); 
	gGL.vertex3fv(v[3].mV);

	gGL.vertex3fv(v[1].mV); 
	gGL.vertex3fv(v[2].mV);

	gGL.vertex3fv(v[1].mV); 
	gGL.vertex3fv(v[3].mV);


	//top
	gGL.vertex3fv(v[0].mV); 
	gGL.vertex3fv(v[4].mV);

	gGL.vertex3fv(v[1].mV); 
	gGL.vertex3fv(v[4].mV);

	gGL.vertex3fv(v[2].mV); 
	gGL.vertex3fv(v[4].mV);

	gGL.vertex3fv(v[3].mV); 
	gGL.vertex3fv(v[4].mV);


	//bottom
	gGL.vertex3fv(v[0].mV); 
	gGL.vertex3fv(v[5].mV);

	gGL.vertex3fv(v[1].mV); 
	gGL.vertex3fv(v[5].mV);

	gGL.vertex3fv(v[2].mV); 
	gGL.vertex3fv(v[5].mV);

	gGL.vertex3fv(v[3].mV); 
	gGL.vertex3fv(v[5].mV);

	gGL.end();

	gGL.popMatrix();
}


// End
