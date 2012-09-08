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
#include "llrendersphere.h"
#include "llavatarappearance.h"
//#include "pipeline.h"

#define DEFAULT_LOD 0.0f

const S32 MIN_PIXEL_AREA_3PASS_HAIR = 64*64;

//-----------------------------------------------------------------------------
// Static Data
//-----------------------------------------------------------------------------
BOOL					LLAvatarJoint::sDisableLOD = FALSE;

//-----------------------------------------------------------------------------
// LLAvatarJoint()
// Class Constructor
//-----------------------------------------------------------------------------
LLAvatarJoint::LLAvatarJoint()
	:       LLJoint()
{
	init();
}


//-----------------------------------------------------------------------------
// LLAvatarJoint()
// Class Constructor
//-----------------------------------------------------------------------------
LLAvatarJoint::LLAvatarJoint(const std::string &name, LLJoint *parent)
	:	LLJoint(name, parent)
{
	init();
}


void LLAvatarJoint::init()
{
	mValid = FALSE;
	mComponents = SC_JOINT | SC_BONE | SC_AXES;
	mMinPixelArea = DEFAULT_LOD;
	mPickName = PN_DEFAULT;
	mVisible = TRUE;
	mMeshID = 0;
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
// isTransparent()
//--------------------------------------------------------------------
BOOL LLAvatarJoint::isTransparent()
{
	return FALSE;
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
			LLAvatarJoint* joint = (LLAvatarJoint*)(*iter);
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


void LLAvatarJoint::setMeshesToChildren()
{
	removeAllChildren();
	for (std::vector<LLAvatarJointMesh*>::iterator iter = mMeshParts.begin();
		iter != mMeshParts.end(); iter++)
	{
		addChild((LLAvatarJoint*) *iter);
	}
}
//-----------------------------------------------------------------------------
// LLAvatarJointCollisionVolume()
//-----------------------------------------------------------------------------

LLAvatarJointCollisionVolume::LLAvatarJointCollisionVolume()
{
	mUpdateXform = FALSE;
}

LLAvatarJointCollisionVolume::LLAvatarJointCollisionVolume(const std::string &name, LLJoint *parent) : LLAvatarJoint(name, parent)
{
	
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
