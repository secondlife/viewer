/** 
 * @file llmaniptranslate.h
 * @brief LLManipTranslate class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMANIPTRANSLATE_H
#define LL_LLMANIPTRANSLATE_H

#include "llmanip.h"
#include "lltimer.h"
#include "v4math.h"
#include "linked_lists.h"
#include "llquaternion.h"

class LLManipTranslate : public LLManip
{
public:
	class ManipulatorHandle
	{
	public:
		LLVector3	mStartPosition;
		LLVector3	mEndPosition;
		EManipPart	mManipID;
		F32			mHotSpotRadius;

		ManipulatorHandle(LLVector3 start_pos, LLVector3 end_pos, EManipPart id, F32 radius):mStartPosition(start_pos), mEndPosition(end_pos), mManipID(id), mHotSpotRadius(radius){}
	};


	LLManipTranslate( LLToolComposite* composite );
	virtual ~LLManipTranslate();

	static	void	restoreGL();
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual void	render();
	virtual void	handleSelect();
	virtual void	handleDeselect();

	EManipPart		getHighlightedPart() { return mHighlightedPart; }
	virtual void	highlightManipulators(S32 x, S32 y);
	/*virtual*/ BOOL	handleMouseDownOnPart(S32 x, S32 y, MASK mask);

protected:
	enum EHandleType {
		HANDLE_CONE,
		HANDLE_BOX,
		HANDLE_SPHERE 
	};

	void		renderArrow(S32 which_arrow, S32 selected_arrow, F32 box_size, F32 arrow_size, F32 handle_size, BOOL reverse_direction);
	void		renderTranslationHandles();
	void		renderText();
	void		renderSnapGuides();
	void		renderGrid(F32 x, F32 y, F32 size, F32 r, F32 g, F32 b, F32 a);
	void		renderGridVert(F32 x_trans, F32 y_trans, F32 r, F32 g, F32 b, F32 alpha);
	F32			getMinGridScale();

private:
	S32			mLastHoverMouseX;
	S32			mLastHoverMouseY;
	BOOL		mSendUpdateOnMouseUp;
	BOOL		mMouseOutsideSlop;		// true after mouse goes outside slop region
	BOOL		mCopyMadeThisDrag;
	S32			mMouseDownX;
	S32			mMouseDownY;
	F32			mAxisArrowLength;		// pixels
	F32			mConeSize;				// meters, world space
	F32			mArrowLengthMeters;		// meters
	F32			mGridSizeMeters;
	F32			mPlaneManipOffsetMeters;
	EManipPart	mManipPart;
	LLVector3	mManipNormal;
	LLVector3d	mDragCursorStartGlobal;
	LLVector3d	mDragSelectionStartGlobal;
	LLTimer		mUpdateTimer;
	LLLinkedList<ManipulatorHandle>		mProjectedManipulators;
	LLVector4	mManipulatorVertices[18];
	EManipPart	mHighlightedPart;
	F32			mSnapOffsetMeters;
	LLVector3	mSnapOffsetAxis;
	LLQuaternion mGridRotation;
	LLVector3	mGridOrigin;
	LLVector3	mGridScale;
	F32			mSubdivisions;
	BOOL		mInSnapRegime;
	BOOL		mSnapped;
	LLVector3	mArrowScales;
	LLVector3	mPlaneScales;
	LLVector4	mPlaneManipPositions;
};

#endif
