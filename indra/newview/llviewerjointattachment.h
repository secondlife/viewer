/** 
 * @file llviewerjointattachment.h
 * @brief Implementation of LLViewerJointAttachment class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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
	LLViewerObject *getObject() { return mAttachedObject; }
	S32	getNumObjects() { return (mAttachedObject ? 1 : 0); }
	const LLUUID& getItemID() { return mItemID; }

	//
	// unique methods
	//
	BOOL addObject(LLViewerObject* object);
	void removeObject(LLViewerObject *object);

	void setupDrawable(LLDrawable* drawable);
	void clampObjectPosition();

protected:
	void calcLOD();
	
protected:
	// Backlink only; don't make this an LLPointer.
	LLViewerObject*	mAttachedObject;
	BOOL			mVisibleInFirst;
	LLVector3		mOriginalPos;
	S32				mGroup;
	BOOL			mIsHUDAttachment;
	S32				mPieSlice;
	LLUUID			mItemID;			// Inventory item id of the attached item (null if not in inventory)
};

#endif // LL_LLVIEWERJOINTATTACHMENT_H
