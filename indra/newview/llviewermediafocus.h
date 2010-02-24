/** 
 * @file llpanelmsgs.h
 * @brief Message popup preferences panel
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#ifndef LL_VIEWERMEDIAFOCUS_H
#define LL_VIEWERMEDIAFOCUS_H

// includes for LLViewerMediaFocus
#include "llfocusmgr.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llselectmgr.h"

class LLViewerMediaImpl;
class LLPanelPrimMediaControls;

class LLViewerMediaFocus : 
	public LLFocusableElement, 
	public LLSingleton<LLViewerMediaFocus>
{
public:
	LLViewerMediaFocus();
	~LLViewerMediaFocus();
	
	// Set/clear the face that has media focus (takes keyboard input and has the full set of controls)
	void setFocusFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);
	void clearFocus();
	
	// Set/clear the face that has "media hover" (has the mimimal set of controls to zoom in or pop out into a media browser).
	// If a media face has focus, the media hover will be ignored.
	void setHoverFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);
	void clearHover();
	
	/*virtual*/ bool	getFocus();
	/*virtual*/ BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/ BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

	void update();
	
	static void setCameraZoom(LLViewerObject* object, LLVector3 normal, F32 padding_factor, bool zoom_in_only = false);
	static F32 getBBoxAspectRatio(const LLBBox& bbox, const LLVector3& normal, F32* height, F32* width, F32* depth);

	bool isFocusedOnFace(LLPointer<LLViewerObject> objectp, S32 face);
	bool isHoveringOverFace(LLPointer<LLViewerObject> objectp, S32 face);
	
	// These look up (by uuid) and return the values that were set with setFocusFace.  They will return null if the objects have been destroyed.
	LLViewerMediaImpl* getFocusedMediaImpl();
	LLViewerObject* getFocusedObject();
	S32 getFocusedFace() { return mFocusedObjectFace; }
	LLUUID getFocusedObjectID() { return mFocusedObjectID; }
	
	// These look up (by uuid) and return the values that were set with setHoverFace.  They will return null if the objects have been destroyed.
	LLViewerMediaImpl* getHoverMediaImpl();
	LLViewerObject* getHoverObject();
	S32 getHoverFace() { return mHoverObjectFace; }
	
	// Try to focus/zoom on the specified media (if it's on an object in world).
	void focusZoomOnMedia(LLUUID media_id);
	// Are we zoomed in?
	bool isZoomed() const;
	void unZoom();
	
	// Return the ID of the media instance the controls are currently attached to (either focus or hover).
	LLUUID getControlsMediaID();

protected:
	/*virtual*/ void	onFocusReceived();
	/*virtual*/ void	onFocusLost();

private:
	
	LLHandle<LLPanelPrimMediaControls> mMediaControls;
	LLObjectSelectionHandle mSelection;
	
	LLUUID mFocusedObjectID;
	S32 mFocusedObjectFace;
	LLUUID mFocusedImplID;
	LLVector3 mFocusedObjectNormal;
	
	LLUUID mHoverObjectID;
	S32 mHoverObjectFace;
	LLUUID mHoverImplID;
	LLVector3 mHoverObjectNormal;
};


#endif // LL_VIEWERMEDIAFOCUS_H
