/** 
 * @file llmanipscale.h
 * @brief LLManipScale class definition
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

#ifndef LL_MANIPSCALE_H
#define LL_MANIPSCALE_H

// llmanipscale.h
//
// copyright 2001-2002, linden research inc


#include "lltool.h"
#include "v3math.h"
#include "v4math.h"
#include "llmanip.h"
#include "llviewerobject.h"
#include "llbbox.h"


F32 get_default_max_prim_scale(bool is_flora = false);

class LLToolComposite;
class LLColor4;

typedef	enum e_scale_manipulator_type
{
	SCALE_MANIP_CORNER,
	SCALE_MANIP_FACE
} EScaleManipulatorType;

typedef	enum e_snap_regimes
{
	SNAP_REGIME_NONE = 0, //!< The cursor is not in either of the snap regimes.
	SNAP_REGIME_UPPER = 0x1, //!< The cursor is, non-exclusively, in the first of the snap regimes. Prefer to treat as bitmask.
	SNAP_REGIME_LOWER = 0x2 //!< The cursor is, non-exclusively, in the second of the snap regimes. Prefer to treat as bitmask.
} ESnapRegimes;


class LLManipScale : public LLManip
{
public:
	class ManipulatorHandle
	{
	public:
		LLVector3	mPosition;
		EManipPart	mManipID;
		EScaleManipulatorType			mType;

		ManipulatorHandle(LLVector3 pos, EManipPart id, EScaleManipulatorType type):mPosition(pos), mManipID(id), mType(type){}
	};
	static const S32 NUM_MANIPULATORS = 14;

	LLManipScale( LLToolComposite* composite );
	~LLManipScale();

	virtual BOOL	handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL	handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL	handleHover( S32 x, S32 y, MASK mask );
	virtual void	render();
	virtual void	handleSelect();

	virtual BOOL	handleMouseDownOnPart(S32 x, S32 y, MASK mask);
	virtual void	highlightManipulators(S32 x, S32 y);	// decided which manipulator, if any, should be highlighted by mouse hover
	virtual BOOL	canAffectSelection();

	static void		setUniform( BOOL b );
	static BOOL		getUniform();
	static void		setStretchTextures( BOOL b );
	static BOOL		getStretchTextures();
	static void		setShowAxes( BOOL b );
	static BOOL		getShowAxes();

private:
	void			renderCorners( const LLBBox& local_bbox );
	void			renderFaces( const LLBBox& local_bbox );
	void			renderBoxHandle( F32 x, F32 y, F32 z );
	void			renderAxisHandle( U32 part, const LLVector3& start, const LLVector3& end );
	void			renderGuidelinesPart( const LLBBox& local_bbox );
	void			renderSnapGuides( const LLBBox& local_bbox );

	void			revert();

	inline void		conditionalHighlight( U32 part, const LLColor4* highlight = NULL, const LLColor4* normal = NULL );

	void			drag( S32 x, S32 y );
	void			dragFace( S32 x, S32 y );
	void			dragCorner( S32 x, S32 y );

	void			sendUpdates( BOOL send_position_update, BOOL send_scale_update, BOOL corner = FALSE);

	LLVector3		faceToUnitVector( S32 part ) const;
	LLVector3		cornerToUnitVector( S32 part ) const;
	LLVector3		edgeToUnitVector( S32 part ) const;
	LLVector3		partToUnitVector( S32 part ) const;
	LLVector3		unitVectorToLocalBBoxExtent( const LLVector3& v, const LLBBox& bbox ) const;
	F32				partToMaxScale( S32 part, const LLBBox& bbox ) const;
	F32				partToMinScale( S32 part, const LLBBox& bbox ) const;
	LLVector3		nearestAxis( const LLVector3& v ) const;

	void			stretchFace( const LLVector3& drag_start_agent, const LLVector3& drag_delta_agent);

	void			adjustTextureRepeats();		// Adjusts texture coords based on mSavedScale and current scale, only works for boxes

	void			updateSnapGuides(const LLBBox& bbox);
private:

	struct compare_manipulators
	{
		bool operator() (const ManipulatorHandle* const a, const ManipulatorHandle* const b) const
		{
			if (a->mType != b->mType)
				return a->mType < b->mType;
			else if (a->mPosition.mV[VZ] != b->mPosition.mV[VZ])
				return a->mPosition.mV[VZ] < b->mPosition.mV[VZ];
			else
				return a->mManipID < b->mManipID;
		}
	};


	F32				mScaledBoxHandleSize; //!< Handle size after scaling for selection feedback.
	LLVector3d		mDragStartPointGlobal;
	LLVector3d		mDragStartCenterGlobal; //!< The center of the bounding box of all selected objects at time of drag start.
	LLVector3d		mDragPointGlobal;
	LLVector3d 		mDragFarHitGlobal;
	S32				mLastMouseX;
	S32				mLastMouseY;
	BOOL			mSendUpdateOnMouseUp;
	U32 			mLastUpdateFlags;
	typedef std::set<ManipulatorHandle*, compare_manipulators> manipulator_list_t;
	manipulator_list_t mProjectedManipulators;
	LLVector4		mManipulatorVertices[14];
	F32				mScaleSnapUnit1; //!< Size of snap multiples for the upper scale.
	F32				mScaleSnapUnit2; //!< Size of snap multiples for the lower scale.
	LLVector3		mScalePlaneNormal1; //!< Normal of plane in which scale occurs that most faces camera.
	LLVector3		mScalePlaneNormal2; //!< Normal of plane in which scale occurs that most faces camera.
	LLVector3		mSnapGuideDir1; //!< The direction in which the upper snap guide tick marks face.
	LLVector3		mSnapGuideDir2; //!< The direction in which the lower snap guide tick marks face.
	LLVector3		mSnapDir1; //!< The direction in which the upper snap guides face.
	LLVector3		mSnapDir2; //!< The direction in which the lower snap guides face.
	F32				mSnapRegimeOffset; //!< How far off the scale axis centerline the mouse can be before it exits/enters the snap regime.
	F32				mTickPixelSpacing1; //!< The pixel spacing between snap guide tick marks for the upper scale.
	F32				mTickPixelSpacing2; //!< The pixel spacing between snap guide tick marks for the lower scale.
	F32				mSnapGuideLength;
	LLVector3		mScaleCenter; //!< The location of the origin of the scaling operation.
	LLVector3		mScaleDir; //!< The direction of the scaling action.  In face-dragging this is aligned with one of the cardinal axis relative to the prim, but in corner-dragging this is along the diagonal.
	F32				mScaleSnappedValue; //!< The distance of the current position nearest the mouse location, measured along mScaleDir.  Is measured either from the center or from the far face/corner depending upon whether uniform scaling is true or false respectively.
	ESnapRegimes	mSnapRegime; //<! Which, if any, snap regime the cursor is currently residing in.
	F32				mManipulatorScales[NUM_MANIPULATORS];
	F32				mBoxHandleSize[NUM_MANIPULATORS];		// The size of the handles at the corners of the bounding box
	S32				mFirstClickX;
	S32				mFirstClickY;
	bool			mIsFirstClick;
};

#endif  // LL_MANIPSCALE_H
