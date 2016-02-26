/** 
 * @file LLAvatarJointMesh.cpp
 * @brief Implementation of LLAvatarJointMesh class
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
#include "linden_common.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llavatarjointmesh.h"
#include "llavatarappearance.h"
#include "lltexlayer.h"
#include "llmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "llmatrix4a.h"


// Utility functions added with Bento to simplify handling of extra
// spine joints, or other new joints internal to the original
// skeleton, and unknown to the system avatar.

//-----------------------------------------------------------------------------
// getBaseSkeletonAncestor()
//-----------------------------------------------------------------------------
LLAvatarJoint *getBaseSkeletonAncestor(LLAvatarJoint* joint)
{
    LLJoint *ancestor = joint->getParent();
    while (ancestor->getParent() && (ancestor->getSupport() != LLJoint::SUPPORT_BASE))
    {
        LL_DEBUGS("Avatar") << "skipping non-base ancestor " << ancestor->getName() << LL_ENDL;
        ancestor = ancestor->getParent();
    }
    return (LLAvatarJoint*) ancestor;
}

//-----------------------------------------------------------------------------
// totalSkinOffset()
//-----------------------------------------------------------------------------
LLVector3 totalSkinOffset(LLAvatarJoint *joint)
{
    LLVector3 totalOffset;
    while (joint)
    {
		if (joint->getSupport() == LLJoint::SUPPORT_BASE)
		{
			totalOffset += joint->getSkinOffset();
		}
		joint = (LLAvatarJoint*)joint->getParent();
    }
    return totalOffset;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLAvatarJointMesh::LLSkinJoint
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::LLSkinJoint()
{
	mJoint       = NULL;
}

//-----------------------------------------------------------------------------
// ~LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::~LLSkinJoint()
{
	mJoint = NULL;
}


//-----------------------------------------------------------------------------
// LLSkinJoint::setupSkinJoint()
//-----------------------------------------------------------------------------
BOOL LLSkinJoint::setupSkinJoint( LLAvatarJoint *joint)
{
	// find the named joint
	mJoint = joint;
	if ( !mJoint )
	{
		LL_INFOS() << "Can't find joint" << LL_ENDL;
	}

	// compute the inverse root skin matrix
	mRootToJointSkinOffset = totalSkinOffset(joint);
    mRootToJointSkinOffset = -mRootToJointSkinOffset;

	//mRootToParentJointSkinOffset = totalSkinOffset((LLAvatarJoint*)joint->getParent());
	mRootToParentJointSkinOffset = totalSkinOffset(getBaseSkeletonAncestor(joint));
	mRootToParentJointSkinOffset = -mRootToParentJointSkinOffset;

	return TRUE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLAvatarJointMesh
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

BOOL LLAvatarJointMesh::sPipelineRender = FALSE;
U32 LLAvatarJointMesh::sClothingMaskImageName = 0;
LLColor4 LLAvatarJointMesh::sClothingInnerColor;

//-----------------------------------------------------------------------------
// LLAvatarJointMesh()
//-----------------------------------------------------------------------------
LLAvatarJointMesh::LLAvatarJointMesh()
	:
	mTexture( NULL ),
	mLayerSet( NULL ),
	mTestImageName( 0 ),
	mFaceIndexCount(0)
{

	mColor[0] = 1.0f;
	mColor[1] = 1.0f;
	mColor[2] = 1.0f;
	mColor[3] = 1.0f;
	mShiny = 0.0f;
	mCullBackFaces = TRUE;

	mMesh = NULL;

	mNumSkinJoints = 0;
	mSkinJoints = NULL;

	mFace = NULL;

	mMeshID = 0;
	mUpdateXform = FALSE;

	mValid = FALSE;

	mIsTransparent = FALSE;
}


//-----------------------------------------------------------------------------
// ~LLAvatarJointMesh()
// Class Destructor
//-----------------------------------------------------------------------------
LLAvatarJointMesh::~LLAvatarJointMesh()
{
	mMesh = NULL;
	mTexture = NULL;
	freeSkinData();
}


//-----------------------------------------------------------------------------
// LLAvatarJointMesh::allocateSkinData()
//-----------------------------------------------------------------------------
BOOL LLAvatarJointMesh::allocateSkinData( U32 numSkinJoints )
{
	mSkinJoints = new LLSkinJoint[ numSkinJoints ];
	mNumSkinJoints = numSkinJoints;
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLAvatarJointMesh::freeSkinData()
//-----------------------------------------------------------------------------
void LLAvatarJointMesh::freeSkinData()
{
	mNumSkinJoints = 0;
	delete [] mSkinJoints;
	mSkinJoints = NULL;
}

//--------------------------------------------------------------------
// LLAvatarJointMesh::getColor()
//--------------------------------------------------------------------
void LLAvatarJointMesh::getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha )
{
	*red   = mColor[0];
	*green = mColor[1];
	*blue  = mColor[2];
	*alpha = mColor[3];
}

//--------------------------------------------------------------------
// LLAvatarJointMesh::setColor()
//--------------------------------------------------------------------
void LLAvatarJointMesh::setColor( F32 red, F32 green, F32 blue, F32 alpha )
{
	mColor[0] = red;
	mColor[1] = green;
	mColor[2] = blue;
	mColor[3] = alpha;
}

void LLAvatarJointMesh::setColor( const LLColor4& color )
{
	mColor = color;
}


//--------------------------------------------------------------------
// LLAvatarJointMesh::getTexture()
//--------------------------------------------------------------------
//LLViewerTexture *LLAvatarJointMesh::getTexture()
//{
//	return mTexture;
//}

//--------------------------------------------------------------------
// LLAvatarJointMesh::setTexture()
//--------------------------------------------------------------------
void LLAvatarJointMesh::setTexture( LLGLTexture *texture )
{
	mTexture = texture;

	// texture and dynamic_texture are mutually exclusive
	if( texture )
	{
		mLayerSet = NULL;
		//texture->bindTexture(0);
		//texture->setClamp(TRUE, TRUE);
	}
}


BOOL LLAvatarJointMesh::hasGLTexture() const
{
	return mTexture.notNull() && mTexture->hasGLTexture();
}

//--------------------------------------------------------------------
// LLAvatarJointMesh::setLayerSet()
// Sets the shape texture (takes precedence over normal texture)
//--------------------------------------------------------------------
void LLAvatarJointMesh::setLayerSet( LLTexLayerSet* layer_set )
{
	mLayerSet = layer_set;
	
	// texture and dynamic_texture are mutually exclusive
	if( layer_set )
	{
		mTexture = NULL;
	}
}

BOOL LLAvatarJointMesh::hasComposite() const
{
	return (mLayerSet && mLayerSet->hasComposite());
}


//--------------------------------------------------------------------
// LLAvatarJointMesh::getMesh()
//--------------------------------------------------------------------
LLPolyMesh *LLAvatarJointMesh::getMesh()
{
	return mMesh;
}

//-----------------------------------------------------------------------------
// LLAvatarJointMesh::setMesh()
//-----------------------------------------------------------------------------
void LLAvatarJointMesh::setMesh( LLPolyMesh *mesh )
{
	// set the mesh pointer
	mMesh = mesh;

	// release any existing skin joints
	freeSkinData();

	if ( mMesh == NULL )
	{
		return;
	}

	// acquire the transform from the mesh object
    // SL-315
	setPosition( mMesh->getPosition() );
	setRotation( mMesh->getRotation() );
	setScale( mMesh->getScale() );

	// create skin joints if necessary
	if ( mMesh->hasWeights() && !mMesh->isLOD())
	{
		U32 numJointNames = mMesh->getNumJointNames();
		
		allocateSkinData( numJointNames );
		std::string *jointNames = mMesh->getJointNames();

		U32 jn;
		for (jn = 0; jn < numJointNames; jn++)
		{
			//LL_INFOS() << "Setting up joint " << jointNames[jn] << LL_ENDL;
			LLAvatarJoint* joint = (LLAvatarJoint*)(getRoot()->findJoint(jointNames[jn]) );
			mSkinJoints[jn].setupSkinJoint( joint );
		}
	}

	// setup joint array
	if (!mMesh->isLOD())
	{
		setupJoint((LLAvatarJoint*)getRoot());
        LL_DEBUGS("Avatar") << getName() << " joint render entries: " << mMesh->mJointRenderData.size() << LL_ENDL;
	}

}

//-----------------------------------------------------------------------------
// setupJoint()
//-----------------------------------------------------------------------------
void LLAvatarJointMesh::setupJoint(LLAvatarJoint* current_joint)
{
	U32 sj;

	for (sj=0; sj<mNumSkinJoints; sj++)
	{
		LLSkinJoint &js = mSkinJoints[sj];

		if (js.mJoint != current_joint)
		{
			continue;
		}

		// we've found a skinjoint for this joint..
        LL_DEBUGS("Avatar") << "Mesh: " << getName() << " joint " << current_joint->getName() << " matches skinjoint " << sj << LL_ENDL;

		// is the last joint in the array our parent?

        std::vector<LLJointRenderData*>	&jrd = mMesh->mJointRenderData;

        // SL-287 - need to update this so the results are the same if
        // additional extended-skeleton joints lie between this joint
        // and the original parent.
        LLJoint *ancestor = getBaseSkeletonAncestor(current_joint);
		if(jrd.size() && jrd.back()->mWorldMatrix == &ancestor->getWorldMatrix())
		{
			// ...then just add ourselves
			LLAvatarJoint* jointp = js.mJoint;
			jrd.push_back(new LLJointRenderData(&jointp->getWorldMatrix(), &js));
			LL_DEBUGS("Avatar") << "add joint[" << (jrd.size()-1) << "] = " << js.mJoint->getName() << LL_ENDL;
		}
		// otherwise add our ancestor and ourselves
		else
		{
			jrd.push_back(new LLJointRenderData(&ancestor->getWorldMatrix(), NULL));
			LL_DEBUGS("Avatar") << "add2 ancestor joint[" << (jrd.size()-1) << "] = " << ancestor->getName() << LL_ENDL;
			jrd.push_back(new LLJointRenderData(&current_joint->getWorldMatrix(), &js));
            LL_DEBUGS("Avatar") << "add2 joint[" << (jrd.size()-1) << "] = " << current_joint->getName() << LL_ENDL;
		}
	}

	// depth-first traversal
	for (LLJoint::child_list_t::iterator iter = current_joint->mChildren.begin();
		 iter != current_joint->mChildren.end(); ++iter)
	{
		LLAvatarJoint* child_joint = (LLAvatarJoint*)(*iter);
		setupJoint(child_joint);
	}
}


// End
