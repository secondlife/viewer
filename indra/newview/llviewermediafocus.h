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
class LLPanelMediaHUD;

class LLViewerMediaFocus : 
	public LLFocusableElement, 
	public LLSingleton<LLViewerMediaFocus>
{
public:
	LLViewerMediaFocus();
	~LLViewerMediaFocus();
	
	static void cleanupClass();

	void setFocusFace(BOOL b, LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl);
	void clearFocus() { setFocusFace(false, NULL, 0, NULL); }
	/*virtual*/ bool	getFocus();
	/*virtual*/ // void onNavigateComplete( const EventType& event_in );

	/*virtual*/ BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/ BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

	LLUUID getSelectedUUID();
	LLObjectSelectionHandle getSelection() { return mFocus; }

	void update();

	void setCameraZoom(F32 padding_factor);
	void setMouseOverFlag(bool b, viewer_media_t media_impl = NULL);
	bool getMouseOverFlag() { return mMouseOverFlag; }
	void setPickInfo(LLPickInfo pick_info) { mPickInfo = pick_info; }
	F32 getBBoxAspectRatio(const LLBBox& bbox, const LLVector3& normal, F32* height, F32* width, F32* depth);

protected:
	/*virtual*/ void	onFocusReceived();
	/*virtual*/ void	onFocusLost();

private:
	LLObjectSelectionHandle mFocus;
	std::string mLastURL;
	bool mMouseOverFlag;
	LLPickInfo mPickInfo;
	LLHandle<LLPanelMediaHUD> mMediaHUD;
	LLUUID mObjectID;
	viewer_media_t mMediaImpl;
};


#endif // LL_VIEWERMEDIAFOCUS_H
