/** 
 * @file llpanelplace.h
 * @brief Display of a place in the Find directory.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELPLACE_H
#define LL_LLPANELPLACE_H

#include "llpanel.h"

#include "v3dmath.h"
#include "lluuid.h"

class LLButton;
class LLTextBox;
class LLLineEditor;
class LLTextEditor;
class LLTextureCtrl;
class LLMessageSystem;

class LLPanelPlace : public LLPanel
{
public:
	LLPanelPlace();
	/*virtual*/ ~LLPanelPlace();

	/*virtual*/ BOOL postBuild();


	void setParcelID(const LLUUID& parcel_id);

	void sendParcelInfoRequest();

	static void processParcelInfoReply(LLMessageSystem* msg, void**);

protected:
	static void onClickTeleport(void* data);
	static void onClickMap(void* data);
	//static void onClickLandmark(void* data);
	static void onClickAuction(void* data);

	// Go to auction web page if user clicked OK
	static void callbackAuctionWebPage(S32 option, void* data);

protected:
	LLUUID			mParcelID;
	LLUUID			mRequestedID;
	LLVector3d		mPosGlobal;
	// Zero if this is not an auction
	S32				mAuctionID;

	LLTextureCtrl* mSnapshotCtrl;

	LLLineEditor* mNameEditor;
	LLTextEditor* mDescEditor;
	LLLineEditor* mInfoEditor;
	LLLineEditor* mLocationEditor;

	LLButton*	mTeleportBtn;
	LLButton*	mMapBtn;
	//LLButton*	mLandmarkBtn;
	LLButton*	mAuctionBtn;

	typedef std::list<LLPanelPlace*> panel_list_t;
	static panel_list_t sAllPanels;
};

#endif // LL_LLPANELPLACE_H
