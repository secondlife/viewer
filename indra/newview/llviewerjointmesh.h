/** 
 * @file llviewerjointmesh.h
 * @brief Implementation of LLViewerJointMesh class
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
	
	static void updateVectorize(); // Update globals when settings variables change
	
private:
	// Avatar vertex skinning is a significant performance issue on computers
	// with avatar vertex programs turned off (for example, most Macs).  We
	// therefore have custom versions that use SIMD instructions.
	//
	// These functions require compiler options for SSE2, SSE, or neither, and
	// hence are contained in separate individual .cpp files.  JC
	static void updateGeometryOriginal(LLFace* face, LLPolyMesh* mesh);
	// generic vector code, used for Altivec
	static void updateGeometryVectorized(LLFace* face, LLPolyMesh* mesh);
	static void updateGeometrySSE(LLFace* face, LLPolyMesh* mesh);
	static void updateGeometrySSE2(LLFace* face, LLPolyMesh* mesh);

	// Use a fuction pointer to indicate which version we are running.
	static void (*sUpdateGeometryFunc)(LLFace* face, LLPolyMesh* mesh);

private:
	// Allocate skin data
	BOOL allocateSkinData( U32 numSkinJoints );

	// Free skin data
	void freeSkinData();
};

#endif // LL_LLVIEWERJOINTMESH_H
