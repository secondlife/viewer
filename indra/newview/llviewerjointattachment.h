/** 
 * @file llviewerjointattachment.h
 * @brief Implementation of LLViewerJointAttachment class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
	/*virtual*/ U32 drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy );
	
	/*virtual*/ BOOL updateLOD(F32 pixel_area, BOOL activate);

	//
	// accessors
	//

	void setPieSlice(S32 pie_slice) { mPieSlice = pie_slice; }	
	void setVisibleInFirstPerson(BOOL visibility) { mVisibleInFirst = visibility; }
	BOOL getVisibleInFirstPerson() const { return mVisibleInFirst; }
	void setGroup(S32 group) { mGroup = group; }
	void setOriginalPosition(LLVector3 &position);
	void setAttachmentVisibility(BOOL visible);
	void setIsHUDAttachment(BOOL is_hud) { mIsHUDAttachment = is_hud; }
	BOOL getIsHUDAttachment() const { return mIsHUDAttachment; }

	BOOL isAnimatable() const { return FALSE; }

	S32 getGroup() const { return mGroup; }
	S32 getPieSlice() const { return mPieSlice; }
	S32	getNumObjects() const { return mAttachedObjects.size(); }

	void clampObjectPosition();

	//
	// unique methods
	//
	BOOL addObject(LLViewerObject* object);
	void removeObject(LLViewerObject *object);

	// 
	// attachments operations
	//
	BOOL isObjectAttached(const LLViewerObject *viewer_object) const;
	const LLViewerObject *getAttachedObject(const LLUUID &object_id) const;
	LLViewerObject *getAttachedObject(const LLUUID &object_id);

	// list of attachments for this joint
	typedef std::vector<LLViewerObject *> attachedobjs_vec_t;
	attachedobjs_vec_t mAttachedObjects;

protected:
	void calcLOD();
	void setupDrawable(LLViewerObject *object);

private:
	BOOL			mVisibleInFirst;
	LLVector3		mOriginalPos;
	S32				mGroup;
	BOOL			mIsHUDAttachment;
	S32				mPieSlice;
};

#endif // LL_LLVIEWERJOINTATTACHMENT_H
