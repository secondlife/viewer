/** 
 * @file llviewerjointattachment.h
 * @brief Implementation of LLViewerJointAttachment class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERJOINTATTACHMENT_H
#define LL_LLVIEWERJOINTATTACHMENT_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerjoint.h"
#include "llstring.h"
#include "lluuid.h"

class LLDrawable;
class LLViewerObject;

//-----------------------------------------------------------------------------
// class LLViewerJointAttachment
//-----------------------------------------------------------------------------
class LLViewerJointAttachment :
	public LLViewerJoint
{
public:
	LLViewerJointAttachment();
	virtual ~LLViewerJointAttachment();

	//virtual U32 render( F32 pixelArea );	// Returns triangle count

	// Returns true if this object is transparent.
	// This is used to determine in which order to draw objects.
	/*virtual*/ BOOL isTransparent();

	// Draws the shape attached to a joint.
	// Called by render().
	/*virtual*/ U32 drawShape( F32 pixelArea, BOOL first_pass );
	
	/*virtual*/ BOOL updateLOD(F32 pixel_area, BOOL activate);

	//
	// accessors
	//

	void setJoint (LLJoint* joint) { mJoint = joint; }
	void setPieSlice(S32 pie_slice) { mPieSlice = pie_slice; }	
	void setVisibleInFirstPerson(BOOL visibility) { mVisibleInFirst = visibility; }
	BOOL getVisibleInFirstPerson() { return mVisibleInFirst; }
	void setGroup(S32 group) { mGroup = group; }
	void setOriginalPosition(LLVector3 &position);
	void setAttachmentVisibility(BOOL visible);
	void setIsHUDAttachment(BOOL is_hud) { mIsHUDAttachment = is_hud; }
	BOOL getIsHUDAttachment() { return mIsHUDAttachment; }

	BOOL isAnimatable() { return FALSE; }

	S32 getGroup() { return mGroup; }
	S32 getPieSlice() { return mPieSlice; }
	BOOL getAttachmentDirty() { return mAttachmentDirty && mAttachedObject.notNull(); }
	LLViewerObject *getObject() { return mAttachedObject; }
	S32	getNumObjects() { return (mAttachedObject ? 1 : 0); }
	const LLUUID& getItemID() { return mItemID; }

	//
	// unique methods
	//
	BOOL addObject(LLViewerObject* object);
	void removeObject(LLViewerObject *object);

	void lazyAttach();
	void setupDrawable(LLDrawable* drawable);
	void clampObjectPosition();

protected:
	void calcLOD();
	
protected:
	LLJoint*		mJoint;
	LLPointer<LLViewerObject>	mAttachedObject;
	BOOL			mAttachmentDirty;	// does attachment drawable need to be fixed up?
	BOOL			mVisibleInFirst;
	LLVector3		mOriginalPos;
	S32				mGroup;
	BOOL			mIsHUDAttachment;
	S32				mPieSlice;
	LLUUID			mItemID;			// Inventory item id of the attached item (null if not in inventory)
};

#endif // LL_LLVIEWERJOINTATTACHMENT_H
