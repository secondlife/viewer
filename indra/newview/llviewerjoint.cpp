/** 
 * @file llviewerjoint.cpp
 * @brief Implementation of LLViewerJoint class
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#include "llviewerjoint.h"

#include "llgl.h"
#include "llrender.h"
#include "llmath.h"
#include "llglheaders.h"
#include "llrendersphere.h"
#include "llvoavatar.h"
#include "pipeline.h"

#define DEFAULT_LOD 0.0f

const S32 MIN_PIXEL_AREA_3PASS_HAIR = 64*64;

//-----------------------------------------------------------------------------
// Static Data
//-----------------------------------------------------------------------------
BOOL					LLViewerJoint::sDisableLOD = FALSE;

//-----------------------------------------------------------------------------
// LLViewerJoint()
// Class Constructor
//-----------------------------------------------------------------------------
LLViewerJoint::LLViewerJoint()
{
	mUpdateXform = TRUE;
	mValid = FALSE;
	mComponents = SC_JOINT | SC_BONE | SC_AXES;
	mMinPixelArea = DEFAULT_LOD;
	mPickName = PN_DEFAULT;
	mVisible = TRUE;
	mMeshID = 0;
}


//-----------------------------------------------------------------------------
// LLViewerJoint()
// Class Constructor
//-----------------------------------------------------------------------------
LLViewerJoint::LLViewerJoint(const std::string &name, LLJoint *parent) :
	LLJoint(name, parent)
{
	mValid = FALSE;
	mComponents = SC_JOINT | SC_BONE | SC_AXES;
	mMinPixelArea = DEFAULT_LOD;
	mPickName = PN_DEFAULT;
}


//-----------------------------------------------------------------------------
// ~LLViewerJoint()
// Class Destructor
//-----------------------------------------------------------------------------
LLViewerJoint::~LLViewerJoint()
{
}


//--------------------------------------------------------------------
// setValid()
//--------------------------------------------------------------------
void LLViewerJoint::setValid( BOOL valid, BOOL recursive )
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
			LLViewerJoint* joint = (LLViewerJoint*)(*iter);
			joint->setValid(valid, TRUE);
		}
	}

}

//--------------------------------------------------------------------
// renderSkeleton()
// DEBUG (UNUSED)
//--------------------------------------------------------------------
// void LLViewerJoint::renderSkeleton(BOOL recursive)
// {
// 	F32 nc = 0.57735f;

// 	//----------------------------------------------------------------
// 	// push matrix stack
// 	//----------------------------------------------------------------
// 	glPushMatrix();

// 	//----------------------------------------------------------------
// 	// render the bone to my parent
// 	//----------------------------------------------------------------
// 	if (mComponents & SC_BONE)
// 	{
// 		drawBone();
// 	}

// 	//----------------------------------------------------------------
// 	// offset to joint position and 
// 	// rotate to our orientation
// 	//----------------------------------------------------------------
// 	glLoadIdentity();
// 	glMultMatrixf( &getWorldMatrix().mMatrix[0][0] );

// 	//----------------------------------------------------------------
// 	// render joint axes
// 	//----------------------------------------------------------------
// 	if (mComponents & SC_AXES)
// 	{
// 		gGL.begin(LLRender::LINES);
// 		gGL.color3f( 1.0f, 0.0f, 0.0f );
// 		gGL.vertex3f( 0.0f,            0.0f, 0.0f );
// 		gGL.vertex3f( 0.1f, 0.0f, 0.0f );

// 		gGL.color3f( 0.0f, 1.0f, 0.0f );
// 		gGL.vertex3f( 0.0f, 0.0f,            0.0f );
// 		gGL.vertex3f( 0.0f, 0.1f, 0.0f );

// 		gGL.color3f( 0.0f, 0.0f, 1.0f );
// 		gGL.vertex3f( 0.0f, 0.0f, 0.0f );
// 		gGL.vertex3f( 0.0f, 0.0f, 0.1f );
// 		gGL.end();
// 	}

// 	//----------------------------------------------------------------
// 	// render the joint graphic
// 	//----------------------------------------------------------------
// 	if (mComponents & SC_JOINT)
// 	{
// 		gGL.color3f( 1.0f, 1.0f, 0.0f );

// 		gGL.begin(LLRender::TRIANGLES);

// 		// joint top half
// 		glNormal3f(nc, nc, nc);
// 		gGL.vertex3f(0.0f,             0.0f, 0.05f);
// 		gGL.vertex3f(0.05f,       0.0f,       0.0f);
// 		gGL.vertex3f(0.0f,       0.05f,       0.0f);

// 		glNormal3f(-nc, nc, nc);
// 		gGL.vertex3f(0.0f,             0.0f, 0.05f);
// 		gGL.vertex3f(0.0f,       0.05f,       0.0f);
// 		gGL.vertex3f(-0.05f,      0.0f,       0.0f);
		
// 		glNormal3f(-nc, -nc, nc);
// 		gGL.vertex3f(0.0f,             0.0f, 0.05f);
// 		gGL.vertex3f(-0.05f,      0.0f,      0.0f);
// 		gGL.vertex3f(0.0f,      -0.05f,      0.0f);

// 		glNormal3f(nc, -nc, nc);
// 		gGL.vertex3f(0.0f,              0.0f, 0.05f);
// 		gGL.vertex3f(0.0f,       -0.05f,       0.0f);
// 		gGL.vertex3f(0.05f,        0.0f,       0.0f);
		
// 		// joint bottom half
// 		glNormal3f(nc, nc, -nc);
// 		gGL.vertex3f(0.0f,             0.0f, -0.05f);
// 		gGL.vertex3f(0.0f,       0.05f,        0.0f);
// 		gGL.vertex3f(0.05f,       0.0f,        0.0f);

// 		glNormal3f(-nc, nc, -nc);
// 		gGL.vertex3f(0.0f,             0.0f, -0.05f);
// 		gGL.vertex3f(-0.05f,      0.0f,        0.0f);
// 		gGL.vertex3f(0.0f,       0.05f,        0.0f);
		
// 		glNormal3f(-nc, -nc, -nc);
// 		gGL.vertex3f(0.0f,              0.0f, -0.05f);
// 		gGL.vertex3f(0.0f,       -0.05f,        0.0f);
// 		gGL.vertex3f(-0.05f,       0.0f,        0.0f);

// 		glNormal3f(nc, -nc, -nc);
// 		gGL.vertex3f(0.0f,             0.0f,  -0.05f);
// 		gGL.vertex3f(0.05f,       0.0f,         0.0f);
// 		gGL.vertex3f(0.0f,      -0.05f,         0.0f);
		
// 		gGL.end();
// 	}

// 	//----------------------------------------------------------------
// 	// render children
// 	//----------------------------------------------------------------
// 	if (recursive)
// 	{
// 		for (child_list_t::iterator iter = mChildren.begin();
// 			 iter != mChildren.end(); ++iter)
// 		{
// 			LLViewerJoint* joint = (LLViewerJoint*)(*iter);
// 			joint->renderSkeleton();
// 		}
// 	}

// 	//----------------------------------------------------------------
// 	// pop matrix stack
// 	//----------------------------------------------------------------
// 	glPopMatrix();
// }


//--------------------------------------------------------------------
// render()
//--------------------------------------------------------------------
U32 LLViewerJoint::render( F32 pixelArea, BOOL first_pass, BOOL is_dummy )
{
	stop_glerror();

	U32 triangle_count = 0;

	//----------------------------------------------------------------
	// ignore invisible objects
	//----------------------------------------------------------------
	if ( mValid )
	{


		//----------------------------------------------------------------
		// if object is transparent, defer it, otherwise
		// give the joint subclass a chance to draw itself
		//----------------------------------------------------------------
		if ( gRenderForSelect || is_dummy )
		{
			triangle_count += drawShape( pixelArea, first_pass, is_dummy );
		}
		else if (LLPipeline::sShadowRender)
		{
			triangle_count += drawShape(pixelArea, first_pass, is_dummy );
		}
		else if ( isTransparent() && !LLPipeline::sReflectionRender)
		{
			// Hair and Skirt
			if ((pixelArea > MIN_PIXEL_AREA_3PASS_HAIR))
			{
				// render all three passes
				LLGLDisable cull(GL_CULL_FACE);
				// first pass renders without writing to the z buffer
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape( pixelArea, first_pass, is_dummy );
				}
				// second pass writes to z buffer only
				gGL.setColorMask(false, false);
				{
					triangle_count += drawShape( pixelArea, FALSE, is_dummy  );
				}
				// third past respects z buffer and writes color
				gGL.setColorMask(true, false);
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape( pixelArea, FALSE, is_dummy  );
				}
			}
			else
			{
				// Render Inside (no Z buffer write)
				glCullFace(GL_FRONT);
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					triangle_count += drawShape( pixelArea, first_pass, is_dummy  );
				}
				// Render Outside (write to the Z buffer)
				glCullFace(GL_BACK);
				{
					triangle_count += drawShape( pixelArea, FALSE, is_dummy  );
				}
			}
		}
		else
		{
			// set up render state
			triangle_count += drawShape( pixelArea, first_pass );
		}
	}

	//----------------------------------------------------------------
	// render children
	//----------------------------------------------------------------
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLViewerJoint* joint = (LLViewerJoint*)(*iter);
		F32 jointLOD = joint->getLOD();
		if (pixelArea >= jointLOD || sDisableLOD)
		{
			triangle_count += joint->render( pixelArea, TRUE, is_dummy );

			if (jointLOD != DEFAULT_LOD)
			{
				break;
			}
		}
	}

	return triangle_count;
}


//--------------------------------------------------------------------
// drawBone()
// DEBUG (UNUSED)
//--------------------------------------------------------------------
// void LLViewerJoint::drawBone()
// {
// 	if ( mParent == NULL )
// 		return;

// 	F32 boneSize = 0.02f;

// 	// rotate to point to child (bone direction)
// 	glPushMatrix();

// 	LLVector3 boneX = getPosition();
// 	F32 length = boneX.normVec();

// 	LLVector3 boneZ(1.0f, 0.0f, 1.0f);
	
// 	LLVector3 boneY = boneZ % boneX;
// 	boneY.normVec();

// 	boneZ = boneX % boneY;

// 	LLMatrix4 rotateMat;
// 	rotateMat.setFwdRow( boneX );
// 	rotateMat.setLeftRow( boneY );
// 	rotateMat.setUpRow( boneZ );
// 	glMultMatrixf( &rotateMat.mMatrix[0][0] );

// 	// render the bone
// 	gGL.color3f( 0.5f, 0.5f, 0.0f );

// 	gGL.begin(LLRender::TRIANGLES);

// 	gGL.vertex3f( length,     0.0f,       0.0f);
// 	gGL.vertex3f( 0.0f,       boneSize,  0.0f);
// 	gGL.vertex3f( 0.0f,       0.0f,       boneSize);

// 	gGL.vertex3f( length,     0.0f,        0.0f);
// 	gGL.vertex3f( 0.0f,       0.0f,        -boneSize);
// 	gGL.vertex3f( 0.0f,       boneSize,   0.0f);

// 	gGL.vertex3f( length,     0.0f,        0.0f);
// 	gGL.vertex3f( 0.0f,       -boneSize,  0.0f);
// 	gGL.vertex3f( 0.0f,       0.0f,        -boneSize);

// 	gGL.vertex3f( length,     0.0f,        0.0f);
// 	gGL.vertex3f( 0.0f,       0.0f,        boneSize);
// 	gGL.vertex3f( 0.0f,       -boneSize,  0.0f);

// 	gGL.end();

// 	// restore matrix
// 	glPopMatrix();
// }

//--------------------------------------------------------------------
// isTransparent()
//--------------------------------------------------------------------
BOOL LLViewerJoint::isTransparent()
{
	return FALSE;
}

//--------------------------------------------------------------------
// drawShape()
//--------------------------------------------------------------------
U32 LLViewerJoint::drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy )
{
	return 0;
}

//--------------------------------------------------------------------
// setSkeletonComponents()
//--------------------------------------------------------------------
void LLViewerJoint::setSkeletonComponents( U32 comp, BOOL recursive )
{
	mComponents = comp;
	if (recursive)
	{
		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLViewerJoint* joint = (LLViewerJoint*)(*iter);
			joint->setSkeletonComponents(comp, recursive);
		}
	}
}

void LLViewerJoint::updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area)
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLViewerJoint* joint = (LLViewerJoint*)(*iter);
		joint->updateFaceSizes(num_vertices, num_indices, pixel_area);
	}
}

void LLViewerJoint::updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind)
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLViewerJoint* joint = (LLViewerJoint*)(*iter);
		joint->updateFaceData(face, pixel_area, damp_wind);
	}
}

void LLViewerJoint::updateJointGeometry()
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLViewerJoint* joint = (LLViewerJoint*)(*iter);
		joint->updateJointGeometry();
	}
}


BOOL LLViewerJoint::updateLOD(F32 pixel_area, BOOL activate)
{
	BOOL lod_changed = FALSE;
	BOOL found_lod = FALSE;

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLViewerJoint* joint = (LLViewerJoint*)(*iter);
		F32 jointLOD = joint->getLOD();
		
		if (found_lod || jointLOD == DEFAULT_LOD)
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

void LLViewerJoint::dump()
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLViewerJoint* joint = (LLViewerJoint*)(*iter);
		joint->dump();
	}
}

void LLViewerJoint::setVisible(BOOL visible, BOOL recursive)
{
	mVisible = visible;

	if (recursive)
	{
		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLViewerJoint* joint = (LLViewerJoint*)(*iter);
			joint->setVisible(visible, recursive);
		}
	}
}


void LLViewerJoint::setMeshesToChildren()
{
	removeAllChildren();
	for (std::vector<LLViewerJointMesh*>::iterator iter = mMeshParts.begin();
		iter != mMeshParts.end(); iter++)
	{
		addChild((LLViewerJointMesh *) *iter);
	}
}
//-----------------------------------------------------------------------------
// LLViewerJointCollisionVolume()
//-----------------------------------------------------------------------------

LLViewerJointCollisionVolume::LLViewerJointCollisionVolume()
{
	mUpdateXform = FALSE;
}

LLViewerJointCollisionVolume::LLViewerJointCollisionVolume(const std::string &name, LLJoint *parent) : LLViewerJoint(name, parent)
{
	
}

void LLViewerJointCollisionVolume::renderCollision()
{
	updateWorldMatrix();
	
	gGL.pushMatrix();
	glMultMatrixf( &mXform.getWorldMatrix().mMatrix[0][0] );

	gGL.color3f( 0.f, 0.f, 1.f );
	
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

LLVector3 LLViewerJointCollisionVolume::getVolumePos(LLVector3 &offset)
{
	mUpdateXform = TRUE;
	
	LLVector3 result = offset;
	result.scaleVec(getScale());
	result.rotVec(getWorldRotation());
	result += getWorldPosition();

	return result;
}

// End
