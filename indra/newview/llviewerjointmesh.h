/** 
 * @file llviewerjointmesh.h
 * @brief Implementation of LLViewerJointMesh class
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

#ifndef LL_LLVIEWERJOINTMESH_H
#define LL_LLVIEWERJOINTMESH_H

#include "llviewerjoint.h"
#include "llviewertexture.h"
#include "llpolymesh.h"
#include "v4color.h"

class LLDrawable;
class LLFace;
class LLCharacter;
class LLTexLayerSet;

typedef enum e_avatar_render_pass
{
	AVATAR_RENDER_PASS_SINGLE,
	AVATAR_RENDER_PASS_CLOTHING_INNER,
	AVATAR_RENDER_PASS_CLOTHING_OUTER
} EAvatarRenderPass;

class LLSkinJoint
{
public:
	LLSkinJoint();
	~LLSkinJoint();
	BOOL setupSkinJoint( LLViewerJoint *joint);

	LLViewerJoint	*mJoint;
	LLVector3		mRootToJointSkinOffset;
	LLVector3		mRootToParentJointSkinOffset;
};

//-----------------------------------------------------------------------------
// class LLViewerJointMesh
//-----------------------------------------------------------------------------
class LLViewerJointMesh : public LLViewerJoint
{
	friend class LLVOAvatar;
protected:
	LLColor4					mColor;			// color value
// 	LLColor4					mSpecular;		// specular color (always white for now)
	F32							mShiny;			// shiny value
	LLPointer<LLViewerTexture>	mTexture;		// ptr to a global texture
	LLTexLayerSet*				mLayerSet;		// ptr to a layer set owned by the avatar
	U32 						mTestImageName;		// handle to a temporary texture for previewing uploads
	LLPolyMesh*					mMesh;			// ptr to a global polymesh
	BOOL						mCullBackFaces;	// true by default
	LLFace*						mFace;			// ptr to a face w/ AGP copy of mesh

	U32							mFaceIndexCount;
	BOOL						mIsTransparent;

	U32							mNumSkinJoints;
	LLSkinJoint*				mSkinJoints;
	S32							mMeshID;

public:
	static BOOL					sPipelineRender;
	//RN: this is here for testing purposes
	static U32					sClothingMaskImageName;
	static EAvatarRenderPass	sRenderPass;
	static LLColor4				sClothingInnerColor;

public:
	// Constructor
	LLViewerJointMesh();

	// Destructor
	virtual ~LLViewerJointMesh();

	// Gets the shape color
	void getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha );

	// Sets the shape color
	void setColor( F32 red, F32 green, F32 blue, F32 alpha );

	// Sets the shininess
	void setSpecular( const LLColor4& color, F32 shiny ) { /*mSpecular = color;*/ mShiny = shiny; };

	// Sets the shape texture
	void setTexture( LLViewerTexture *texture );

	void setTestTexture( U32 name ) { mTestImageName = name; }

	// Sets layer set responsible for a dynamic shape texture (takes precedence over normal texture)
	void setLayerSet( LLTexLayerSet* layer_set );

	// Gets the poly mesh
	LLPolyMesh *getMesh();

	// Sets the poly mesh
	void setMesh( LLPolyMesh *mesh );

	// Sets up joint matrix data for rendering
	void setupJoint(LLViewerJoint* current_joint);

	// Render time method to upload batches of joint matrices
	void uploadJointMatrices();

	// Sets ID for picking
	void setMeshID( S32 id ) {mMeshID = id;}

	// Gets ID for picking
	S32 getMeshID() { return mMeshID; }	

	// overloaded from base class
	/*virtual*/ void drawBone();
	/*virtual*/ BOOL isTransparent();
	/*virtual*/ U32 drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy );

	/*virtual*/ void updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area);
	/*virtual*/ void updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind = FALSE, bool terse_update = false);
	/*virtual*/ BOOL updateLOD(F32 pixel_area, BOOL activate);
	/*virtual*/ void updateJointGeometry();
	/*virtual*/ void dump();

	void setIsTransparent(BOOL is_transparent) { mIsTransparent = is_transparent; }

	/*virtual*/ BOOL isAnimatable() const { return FALSE; }
	
private:

	//copy mesh into given face's vertex buffer, applying current animation pose
	static void updateGeometry(LLFace* face, LLPolyMesh* mesh);

private:
	// Allocate skin data
	BOOL allocateSkinData( U32 numSkinJoints );

	// Free skin data
	void freeSkinData();
};

#endif // LL_LLVIEWERJOINTMESH_H
