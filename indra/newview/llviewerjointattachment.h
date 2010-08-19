/** 
 * @file llviewerjointattachment.h
 * @brief Implementation of LLViewerJointAttachment class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
