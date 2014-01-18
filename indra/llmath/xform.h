/** 
 * @file xform.h
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

#ifndef LL_XFORM_H
#define LL_XFORM_H

#include "v3math.h"
#include "m4math.h"
#include "llquaternion.h"

const F32 MAX_OBJECT_Z 		= 4096.f; // should match REGION_HEIGHT_METERS, Pre-havok4: 768.f
const F32 MIN_OBJECT_Z 		= -256.f;
const F32 DEFAULT_MAX_PRIM_SCALE = 64.f;
const F32 DEFAULT_MAX_PRIM_SCALE_NO_MESH = 10.f;
const F32 MIN_PRIM_SCALE = 0.01f;
const F32 MAX_PRIM_SCALE = 65536.f;	// something very high but not near FLT_MAX

class LLXform
{
protected:
	LLVector3	  mPosition;
	LLQuaternion  mRotation; 
	LLVector3	  mScale;
	
	//RN: TODO: move these world transform members to LLXformMatrix
	// as they are *never* updated or accessed in the base class
	LLVector3	  mWorldPosition;
	LLQuaternion  mWorldRotation;

	LLXform*      mParent;
	U32			  mChanged;

	BOOL		  mScaleChildOffset;

public:
	typedef enum e_changed_flags
	{
		UNCHANGED  	= 0x00,
		TRANSLATED 	= 0x01,
		ROTATED		= 0x02,
		SCALED		= 0x04,
		SHIFTED		= 0x08,
		GEOMETRY	= 0x10,
		TEXTURE		= 0x20,
		MOVED       = TRANSLATED|ROTATED|SCALED,
		SILHOUETTE	= 0x40,
		ALL_CHANGED = 0x7f
	}EChangedFlags;

	void init()
	{
		mParent  = NULL;
		mChanged = UNCHANGED;
		mPosition.setVec(0,0,0);
		mRotation.loadIdentity();
		mScale.   setVec(1,1,1);
		mWorldPosition.clearVec();
		mWorldRotation.loadIdentity();
		mScaleChildOffset = FALSE;
	}

	 LLXform();
	virtual ~LLXform();

	void getLocalMat4(LLMatrix4 &mat) const { mat.initAll(mScale, mRotation, mPosition); }

	inline BOOL setParent(LLXform *parent);

	inline void setPosition(const LLVector3& pos);
	inline void setPosition(const F32 x, const F32 y, const F32 z);
	inline void setPositionX(const F32 x);
	inline void setPositionY(const F32 y);
	inline void setPositionZ(const F32 z);
	inline void addPosition(const LLVector3& pos);


	inline void setScale(const LLVector3& scale);
	inline void setScale(const F32 x, const F32 y, const F32 z);
	inline void setRotation(const LLQuaternion& rot);
	inline void setRotation(const F32 x, const F32 y, const F32 z);
	inline void setRotation(const F32 x, const F32 y, const F32 z, const F32 s);

	// Above functions must be inline for speed, but also
	// need to emit warnings.  llwarns causes inline LLError::CallSite
	// static objects that make more work for the linker.
	// Avoid inline llwarns by calling this function.
	void warn(const char* const msg);
	
	void 		setChanged(const U32 bits)					{ mChanged |= bits; }
	BOOL		isChanged() const							{ return mChanged; }
	BOOL 		isChanged(const U32 bits) const				{ return mChanged & bits; }
	void 		clearChanged()								{ mChanged = 0; }
	void        clearChanged(U32 bits)                      { mChanged &= ~bits; }

	void		setScaleChildOffset(BOOL scale)				{ mScaleChildOffset = scale; }
	BOOL		getScaleChildOffset()						{ return mScaleChildOffset; }

	LLXform* getParent() const { return mParent; }
	LLXform* getRoot() const;
	virtual BOOL isRoot() const;
	virtual BOOL isRootEdit() const;

	const LLVector3&	getPosition()  const	    { return mPosition; }
	const LLVector3&	getScale() const			{ return mScale; }
	const LLQuaternion& getRotation() const			{ return mRotation; }
	const LLVector3&	getPositionW() const		{ return mWorldPosition; }
	const LLQuaternion& getWorldRotation() const	{ return mWorldRotation; }
	const LLVector3&	getWorldPosition() const	{ return mWorldPosition; }
};

class LLXformMatrix : public LLXform
{
public:
	LLXformMatrix() : LLXform() {};
	virtual ~LLXformMatrix();

	const LLMatrix4&    getWorldMatrix() const      { return mWorldMatrix; }
	void setWorldMatrix (const LLMatrix4& mat)   { mWorldMatrix = mat; }

	void init()
	{
		mWorldMatrix.setIdentity();
		mMin.clearVec();
		mMax.clearVec();

		LLXform::init();
	}

	void update();
	void updateMatrix(BOOL update_bounds = TRUE);
	void getMinMax(LLVector3& min,LLVector3& max) const;

protected:
	LLMatrix4	mWorldMatrix;
	LLVector3	mMin;
	LLVector3	mMax;

};

BOOL LLXform::setParent(LLXform* parent)
{
	// Validate and make sure we're not creating a loop
	if (parent == mParent)
	{
		return TRUE;
	}
	if (parent)
	{
		LLXform *cur_par = parent->mParent;
		while (cur_par)
		{
			if (cur_par == this)
			{
				//warn("LLXform::setParent Creating loop when setting parent!");
				return FALSE;
			}
			cur_par = cur_par->mParent;
		}
	}
	mParent = parent;
	return TRUE;
}

void LLXform::setPosition(const LLVector3& pos)			
{
	setChanged(TRANSLATED);
	if (pos.isFinite())
		mPosition = pos; 
	else
	{
		mPosition.clearVec();
		warn("Non Finite in LLXform::setPosition(LLVector3)");
	}
}

void LLXform::setPosition(const F32 x, const F32 y, const F32 z)
{
	setChanged(TRANSLATED);
	if (llfinite(x) && llfinite(y) && llfinite(z))
		mPosition.setVec(x,y,z); 
	else
	{
		mPosition.clearVec();
		warn("Non Finite in LLXform::setPosition(F32,F32,F32)");
	}
}

void LLXform::setPositionX(const F32 x)
{ 
	setChanged(TRANSLATED);
	if (llfinite(x))
		mPosition.mV[VX] = x; 
	else
	{
		mPosition.mV[VX] = 0.f;
		warn("Non Finite in LLXform::setPositionX");
	}
}

void LLXform::setPositionY(const F32 y)
{ 
	setChanged(TRANSLATED);
	if (llfinite(y))
		mPosition.mV[VY] = y; 
	else
	{
		mPosition.mV[VY] = 0.f;
		warn("Non Finite in LLXform::setPositionY");
	}
}

void LLXform::setPositionZ(const F32 z)
{ 
	setChanged(TRANSLATED);
	if (llfinite(z))
		mPosition.mV[VZ] = z; 
	else
	{
		mPosition.mV[VZ] = 0.f;
		warn("Non Finite in LLXform::setPositionZ");
	}
}

void LLXform::addPosition(const LLVector3& pos)
{ 
	setChanged(TRANSLATED);
	if (pos.isFinite())
		mPosition += pos; 
	else
		warn("Non Finite in LLXform::addPosition");
}

void LLXform::setScale(const LLVector3& scale)
{ 
	setChanged(SCALED);
	if (scale.isFinite())
		mScale = scale; 
	else
	{
		mScale.setVec(1.f, 1.f, 1.f);
		warn("Non Finite in LLXform::setScale");
	}
}
void LLXform::setScale(const F32 x, const F32 y, const F32 z)
{ 
	setChanged(SCALED);
	if (llfinite(x) && llfinite(y) && llfinite(z))
		mScale.setVec(x,y,z); 
	else
	{
		mScale.setVec(1.f, 1.f, 1.f);
		warn("Non Finite in LLXform::setScale");
	}
}
void LLXform::setRotation(const LLQuaternion& rot)
{ 
	setChanged(ROTATED);
	if (rot.isFinite())
		mRotation = rot; 
	else
	{
		mRotation.loadIdentity();
		warn("Non Finite in LLXform::setRotation");
	}
}
void LLXform::setRotation(const F32 x, const F32 y, const F32 z) 
{ 
	setChanged(ROTATED);
	if (llfinite(x) && llfinite(y) && llfinite(z))
	{
		mRotation.setQuat(x,y,z); 
	}
	else
	{
		mRotation.loadIdentity();
		warn("Non Finite in LLXform::setRotation");
	}
}
void LLXform::setRotation(const F32 x, const F32 y, const F32 z, const F32 s) 
{ 
	setChanged(ROTATED);
	if (llfinite(x) && llfinite(y) && llfinite(z) && llfinite(s))
	{
		mRotation.mQ[VX] = x; mRotation.mQ[VY] = y; mRotation.mQ[VZ] = z; mRotation.mQ[VS] = s; 
	}
	else
	{
		mRotation.loadIdentity();
		warn("Non Finite in LLXform::setRotation");
	}
}

#endif
