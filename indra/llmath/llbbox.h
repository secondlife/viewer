/** 
 * @file llbbox.h
 * @brief General purpose bounding box class
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

	const LLVector3&	getMinLocal() const					{ return mMinLocal; }
	void				setMinLocal( const LLVector3& min )	{ mMinLocal = min; }

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
