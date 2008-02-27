/** 
 * @file llsprite.cpp
 * @brief LLSprite class implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

/* -*- c++ -*-
 *	Notes:
 *		PR - Should add a creator that can take a pointer rather than handle for streaming 
 *		object textures.
 *		PR - Need to add support for lit/non-lit conditions, set normals?
 */

#include "llviewerprecompiledheaders.h"

#include <llglheaders.h>

#include "llsprite.h"

#include "math.h"

#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"

LLVector3 LLSprite::sCameraUp(0.0f,0.0f,1.0f);
LLVector3 LLSprite::sCameraRight(1.0f,0.0f,0.0f);
LLVector3 LLSprite::sCameraPosition(0.f, 0.f, 0.f);
LLVector3 LLSprite::sNormal(0.0f,0.0f,0.0f);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// A simple initialization
LLSprite::LLSprite(const LLUUID &image_uuid)
{
	mImageID = image_uuid;
	mImagep = NULL;

	setSize(1.0f, 1.0f);
	setPosition(LLVector3(0.0f, 0.0f, 0.0f));
	mTexMode = GL_REPLACE;
	mColor.setVec(0.5f, 0.5f, 0.5f, 1.0f);
	mFollow = TRUE;
	mUseCameraUp = TRUE;
}

LLSprite::LLSprite(const LLUUID &image_uuid, const F32 width, const F32 height, const BOOL b_usemipmap)
{
	mImageID = image_uuid;
	mImagep = NULL;

	setSize(width,height);
	setPosition(LLVector3(0.0f, 0.0f, 0.0f));
	mTexMode = GL_REPLACE;
	mColor.setVec(0.5f, 0.5f, 0.5f, 1.0f);
	mFollow = TRUE;
	mUseCameraUp = TRUE;
}

//////////////////////////////////////////////////////////////////////
LLSprite::~LLSprite()
{
}

void LLSprite::updateFace(LLFace &face)
{
	LLViewerCamera &camera = *gCamera;

	// First, figure out how many vertices/indices we need.
	U32 num_vertices, num_indices;
	U32 vertex_count = 0;
	
	// Get the total number of vertices and indices
	if (mFollow)
	{
		num_vertices = 4;
		num_indices = 6;
	}
	else
	{
		num_vertices = 4;
		num_indices = 12;
	}

	face.setSize(num_vertices, num_indices);
	
	if (mFollow) 
	{
		sCameraUp = camera.getUpAxis();
		sCameraRight = -camera.getLeftAxis();
		sCameraPosition = camera.getOrigin();
		sNormal = -camera.getAtAxis();
		if (mUseCameraUp)
		{
			// these need to live here because the height/width may change between render calls
			mScaledUp = sCameraUp;
			mScaledRight = sCameraRight;

			mScaledUp *= mHeightDiv2;
			mScaledRight *= mWidthDiv2;

			mA = mPosition + mScaledRight + mScaledUp;
			mB = mPosition - mScaledRight + mScaledUp;
			mC = mPosition - mScaledRight - mScaledUp;
			mD = mPosition + mScaledRight - mScaledUp;
		}
		else
		{
			// The up vector is perpendicular to the camera vector...
			LLVector3 camera_vec = mPosition - sCameraPosition;
			mScaledRight = camera_vec % LLVector3(0.f, 0.f, 1.f);
			mScaledUp = -(camera_vec % mScaledRight);
			mScaledUp.normVec();
			mScaledRight.normVec();
			mScaledUp *= mHeightDiv2;
			mScaledRight *= mWidthDiv2;

			mA = mPosition + mScaledRight + mScaledUp;
			mB = mPosition - mScaledRight + mScaledUp;
			mC = mPosition - mScaledRight - mScaledUp;
			mD = mPosition + mScaledRight - mScaledUp;
		}
	}
	else
	{
		// this is equivalent to how it was done before. . . 
		// we need to establish a way to 
		// identify the orientation of a particular sprite rather than
		// just banging it in on the x,z plane if it's not following the camera.

		LLVector3 x_axis;
		LLVector3 y_axis;

		F32 dot = sNormal * LLVector3(0.f, 1.f, 0.f);
		if (dot == 1.f || dot == -1.f)
		{
			x_axis.setVec(1.f, 0.f, 0.f);
			y_axis.setVec(0.f, 1.f, 0.f);
		}
		else
		{
			x_axis = sNormal % LLVector3(0.f, -1.f, 0.f);
			x_axis.normVec();

			y_axis = sNormal % x_axis;
		}

		LLQuaternion yaw_rot(mYaw, sNormal);

		// rotate axes by specified yaw
		x_axis = x_axis * yaw_rot;
		y_axis = y_axis * yaw_rot;

		// rescale axes by width and height of sprite
		x_axis = x_axis * mWidthDiv2;
		y_axis = y_axis *  mHeightDiv2;

		mA = -x_axis + y_axis;
		mB = x_axis + y_axis;
		mC = x_axis - y_axis;
		mD = -x_axis - y_axis;

		mA += mPosition;
		mB += mPosition;
		mC += mPosition;
		mD += mPosition;
	}

	face.setFaceColor(mColor);

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> tex_coordsp;
	LLStrider<U16> indicesp;
	U16 index_offset;

	// Setup face
	if (face.mVertexBuffer.isNull())
	{	
		face.mVertexBuffer = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | 
												LLVertexBuffer::MAP_TEXCOORD,
												GL_STREAM_DRAW_ARB);
		face.mVertexBuffer->allocateBuffer(4, 12, TRUE);
		face.setGeomIndex(0);
		face.setIndicesIndex(0);
	}
		
	index_offset = face.getGeometry(verticesp,normalsp,tex_coordsp, indicesp);

	*tex_coordsp = LLVector2(0.f, 0.f);
	*verticesp = mC;
	tex_coordsp++;
	verticesp++;
	vertex_count++;

	*tex_coordsp = LLVector2(0.f, 1.f);
	*verticesp = mB;
	tex_coordsp++;
	verticesp++;
	vertex_count++;

	*tex_coordsp = LLVector2(1.f, 1.f);
	*verticesp = mA;
	tex_coordsp++;
	verticesp++;
	vertex_count++;

	*tex_coordsp = LLVector2(1.f, 0.0f);
	*verticesp = mD;
	tex_coordsp++;
	verticesp++;
	vertex_count++;

	// Generate indices, since they're easy.
	// Just a series of quads.
	*indicesp++ = index_offset;
	*indicesp++ = 2 + index_offset;
	*indicesp++ = 1 + index_offset;

	*indicesp++ = index_offset;
	*indicesp++ = 3 + index_offset;
	*indicesp++ = 2 + index_offset;

	if (!mFollow)
	{
		*indicesp++ = 0 + index_offset;
		*indicesp++ = 1 + index_offset;
		*indicesp++ = 2 + index_offset;
		*indicesp++ = 0 + index_offset;
		*indicesp++ = 2 + index_offset;
		*indicesp++ = 3 + index_offset;
	}

	face.mVertexBuffer->setBuffer(0);
	face.mCenterAgent = mPosition;
}

void LLSprite::setPosition(const LLVector3 &position) 
{
	mPosition = position; 
}


void LLSprite::setPitch(const F32 pitch) 
{
	mPitch = pitch; 
}


void LLSprite::setSize(const F32 width, const F32 height) 
{
	mWidth = width; 
	mHeight = height;
	mWidthDiv2 = width/2.0f;
	mHeightDiv2 = height/2.0f;
}

void LLSprite::setYaw(F32 yaw) 
{
	mYaw = yaw; 
}

void LLSprite::setFollow(const BOOL follow)
{
	mFollow = follow; 
}

void LLSprite::setUseCameraUp(const BOOL use_up)
{
	mUseCameraUp = use_up; 
}

void LLSprite::setTexMode(const LLGLenum mode) 
{
	mTexMode = mode; 
}

void LLSprite::setColor(const LLColor4 &color)
{
	mColor = color; 
}

void LLSprite::setColor(const F32 r, const F32 g, const F32 b, const F32 a)
{
	mColor.setVec(r, g, b, a);
}










