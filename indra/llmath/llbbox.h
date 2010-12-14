/** 
 * @file llbbox.h
 * @brief General purpose bounding box class
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

#ifndef LL_BBOX_H
#define LL_BBOX_H

#include "v3math.h"
#include "llquaternion.h"

// Note: "local space" for an LLBBox is defined relative to agent space in terms of
// a translation followed by a rotation.  There is no scale term since the LLBBox's min and 
// max are not necessarily symetrical and define their own extents.

class LLBBox
{
public:
	LLBBox() {mEmpty = TRUE;}
	LLBBox( const LLVector3& pos_agent,
		const LLQuaternion& rot,
		const LLVector3& min_local,
		const LLVector3& max_local )
		:
		mMinLocal( min_local ), mMaxLocal( max_local ), mPosAgent(pos_agent), mRotation( rot), mEmpty( TRUE )
		{}

	// Default copy constructor is OK.

	const LLVector3&	getPositionAgent() const			{ return mPosAgent; }
	const LLQuaternion&	getRotation() const					{ return mRotation; }

	LLVector3           getMinAgent() const;
	const LLVector3&	getMinLocal() const					{ return mMinLocal; }
	void				setMinLocal( const LLVector3& min )	{ mMinLocal = min; }

	LLVector3           getMaxAgent() const;
	const LLVector3&	getMaxLocal() const					{ return mMaxLocal; }
	void				setMaxLocal( const LLVector3& max )	{ mMaxLocal = max; }

	LLVector3			getCenterLocal() const				{ return (mMaxLocal - mMinLocal) * 0.5f + mMinLocal; }
	LLVector3			getCenterAgent() const				{ return localToAgent( getCenterLocal() ); }

	LLVector3			getExtentLocal() const				{ return mMaxLocal - mMinLocal; }

	BOOL				containsPointLocal(const LLVector3& p) const;
	BOOL				containsPointAgent(const LLVector3& p) const;

	void				addPointAgent(LLVector3 p);
	void				addBBoxAgent(const LLBBox& b);
	
	void				addPointLocal(const LLVector3& p);
	void				addBBoxLocal(const LLBBox& b) {	addPointLocal( b.mMinLocal ); addPointLocal( b.mMaxLocal ); }

	void				expand( F32 delta );

	LLVector3			localToAgent( const LLVector3& v ) const;
	LLVector3			agentToLocal( const LLVector3& v ) const;

	// Changes rotation but not position
	LLVector3			localToAgentBasis(const LLVector3& v) const;
	LLVector3			agentToLocalBasis(const LLVector3& v) const;

	// Get the smallest possible axis aligned bbox that contains this bbox
	LLBBox              getAxisAligned() const;

	// Increases the size to contain other_box
	void 				join(const LLBBox& other_box);


//	friend LLBBox operator*(const LLBBox& a, const LLMatrix4& b);

private:
	LLVector3			mMinLocal;
	LLVector3			mMaxLocal;
	LLVector3			mPosAgent;  // Position relative to Agent's Region
	LLQuaternion		mRotation;
	BOOL				mEmpty;		// Nothing has been added to this bbox yet
};

//LLBBox operator*(const LLBBox &a, const LLMatrix4 &b);


#endif  // LL_BBOX_H
