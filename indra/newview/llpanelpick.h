/** 
 * @file llpanelpick.h
 * @brief LLPanelPick class definition
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	void refresh();

	// Setup a new pick, including creating an id, giving a sane
	// initial position, etc.
	void initNewPick();

	void setPickID(const LLUUID& id);

	// Schedules the panel to request data
	// from the server next time it is drawn.
	void markForServerRequest();

	std::string getPickName();
	const LLUUID& getPickID() const { return mPickID; }

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

	LLString mSimName;
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
