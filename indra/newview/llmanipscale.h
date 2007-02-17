/** 
 * @file llmanipscale.h
 * @brief LLManipScale class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

class LLToolComposite;
class LLColor4;

typedef	enum e_scale_manipulator_type
{
	SCALE_MANIP_CORNER,
	SCALE_MANIP_FACE
} EScaleManipulatorType;


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


	LLManipScale( LLToolComposite* composite );
	~LLManipScale();

	virtual BOOL	handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL	handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL	handleHover( S32 x, S32 y, MASK mask );
	virtual void	render();
	virtual void	handleSelect();
	virtual void	handleDeselect();

	BOOL			handleMouseDownOnPart(S32 x, S32 y, MASK mask);
	EManipPart		getHighlightedPart() { return mHighlightedPart; }
	virtual void	highlightManipulators(S32 x, S32 y);	// decided which manipulator, if any, should be highlighted by mouse hover

	static void		setUniform( BOOL b );
	static BOOL		getUniform();
	static void		setStretchTextures( BOOL b );
	static BOOL		getStretchTextures();
	static void		setShowAxes( BOOL b );
	static BOOL		getShowAxes();

private:
	void			renderCorners( const LLBBox& local_bbox );
	void			renderFaces( const LLBBox& local_bbox );
	void			renderEdges( const LLBBox& local_bbox );
	void			renderBoxHandle( F32 x, F32 y, F32 z );
	void			renderAxisHandle( const LLVector3& start, const LLVector3& end );
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

	BOOL			isSelectionScalable();

	void			stretchFace( const LLVector3& drag_start_agent, const LLVector3& drag_delta_agent);

	void			adjustTextureRepeats();		// Adjusts texture coords based on mSavedScale and current scale, only works for boxes

	void			updateSnapGuides(const LLBBox& bbox);
private:

	F32				mBoxHandleSize;		// The size of the handles at the corners of the bounding box
	F32				mScaledBoxHandleSize; // handle size after scaling for selection feedback
	EManipPart		mManipPart;
	LLVector3d		mDragStartPointGlobal;
	LLVector3d		mDragStartCenterGlobal;	// The center of the bounding box of all selected objects at time of drag start
	LLVector3d		mDragPointGlobal;
	LLVector3d 		mDragFarHitGlobal;
	EManipPart		mHighlightedPart;
	S32				mLastMouseX;
	S32				mLastMouseY;
	BOOL			mSendUpdateOnMouseUp;
	U32 			mLastUpdateFlags;
	LLLinkedList<ManipulatorHandle>		mProjectedManipulators;
	LLVector4		mManipulatorVertices[14];
	F32				mScaleSnapUnit1;  // size of snap multiples for axis 1
	F32				mScaleSnapUnit2;  // size of snap multiples for axis 2
	LLVector3		mScalePlaneNormal1; // normal of plane in which scale occurs that most faces camera
	LLVector3		mScalePlaneNormal2; // normal of plane in which scale occurs that most faces camera
	LLVector3		mSnapGuideDir1;
	LLVector3		mSnapGuideDir2;
	LLVector3		mSnapDir1;
	LLVector3		mSnapDir2;
	F32				mSnapRegimeOffset;
	F32				mSnapGuideLength;
	LLVector3		mScaleCenter;
	LLVector3		mScaleDir;
	F32				mScaleSnapValue;
	BOOL			mInSnapRegime;
	F32*			mManipulatorScales;
};

#endif  // LL_MANIPSCALE_H
