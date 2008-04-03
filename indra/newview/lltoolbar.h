/** 
 * @file lltoolbar.h
 * @brief Large friendly buttons at bottom of screen.
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

#ifndef LL_LLTOOLBAR_H
#define LL_LLTOOLBAR_H

#include "llpanel.h"

#include "llframetimer.h"

// "Constants" loaded from settings.xml at start time
extern S32 TOOL_BAR_HEIGHT;

#if LL_DARWIN
	class LLFakeResizeHandle;
#endif // LL_DARWIN

class LLToolBar
:	public LLPanel
{
public:
	LLToolBar();
	~LLToolBar();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 LLString& tooltip_msg);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	static void toggle(void*);
	static BOOL visible(void*);

	// Move buttons to appropriate locations based on rect.
	void layoutButtons();

	// Per-frame refresh call
	void refresh();

	// callbacks
	static void onClickCommunicate(LLUICtrl*, void*);
	static void onClickChat(void* data);
	static void onClickAppearance(void* data);
	static void onClickClothing(void* data);
	static void onClickFly(void*);
	static void onClickSit(void*);
	static void onClickSnapshot(void* data);
	static void onClickDirectory(void* data);
	static void onClickBuild(void* data);
	static void onClickRadar(void* data);
	static void onClickMap(void* data);
	static void onClickInventory(void* data);

	static F32 sInventoryAutoOpenTime;

private:
	void updateCommunicateList();


private:
	BOOL		mInventoryAutoOpen;
	LLFrameTimer mInventoryAutoOpenTimer;
	S32			mNumUnreadIMs;
#if LL_DARWIN
	LLFakeResizeHandle *mResizeHandle;
#endif // LL_DARWIN
};

extern LLToolBar *gToolBar;

#endif
