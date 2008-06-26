/** 
 * @file llpanelpick.h
 * @brief LLPanelPick class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

// Display of a "Top Pick" used both for the global top picks in the 
// Find directory, and also for each individual user's picks in their
// profile.

#ifndef LL_LLPANELPICK_H
#define LL_LLPANELPICK_H

#include "llpanel.h"
#include "v3dmath.h"
#include "lluuid.h"

class LLButton;
class LLCheckBoxCtrl;
class LLIconCtrl;
class LLLineEditor;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLUICtrl;
class LLMessageSystem;

class LLPanelPick : public LLPanel
{
public:
    LLPanelPick(BOOL top_pick);
    /*virtual*/ ~LLPanelPick();

	void reset();

    /*virtual*/ BOOL postBuild();

    /*virtual*/ void draw();

	/*virtual*/ void refresh();

	// Setup a new pick, including creating an id, giving a sane
	// initial position, etc.
	void initNewPick();

	// We need to know the creator id so the database knows which partition
	// to query for the pick data.
	void setPickID(const LLUUID& pick_id, const LLUUID& creator_id);

	// Schedules the panel to request data
	// from the server next time it is drawn.
	void markForServerRequest();

	std::string getPickName();
	const LLUUID& getPickID() const { return mPickID; }
	const LLUUID& getPickCreatorID() const { return mCreatorID; }

    void sendPickInfoRequest();
	void sendPickInfoUpdate();

    static void processPickInfoReply(LLMessageSystem* msg, void**);

protected:
    static void onClickTeleport(void* data);
    static void onClickMap(void* data);
    //static void onClickLandmark(void* data);
    static void onClickSet(void* data);

	static void onCommitAny(LLUICtrl* ctrl, void* data);

protected:
	BOOL mTopPick;
    LLUUID mPickID;
	LLUUID mCreatorID;
	LLUUID mParcelID;

	// Data will be requested on first draw
	BOOL mDataRequested;
	BOOL mDataReceived;

	std::string mSimName;
    LLVector3d mPosGlobal;

    LLTextureCtrl*	mSnapshotCtrl;
    LLLineEditor*	mNameEditor;
    LLTextEditor*	mDescEditor;
    LLLineEditor*	mLocationEditor;

    LLButton*    mTeleportBtn;
    LLButton*    mMapBtn;

    LLTextBox*    mSortOrderText;
    LLLineEditor* mSortOrderEditor;
    LLCheckBoxCtrl* mEnabledCheck;
    LLButton*    mSetBtn;

    typedef std::list<LLPanelPick*> panel_list_t;
	static panel_list_t sAllPanels;
};

#endif // LL_LLPANELPICK_H
