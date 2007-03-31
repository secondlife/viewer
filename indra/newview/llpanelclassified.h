/** 
 * @file llpanelclassified.h
 * @brief LLPanelClassified class definition
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Display of a classified used both for the global view in the
// Find directory, and also for each individual user's classified in their
// profile.

#ifndef LL_LLPANELCLASSIFIED_H
#define LL_LLPANELCLASSIFIED_H

#include "llpanel.h"
#include "llclassifiedinfo.h"
#include "v3dmath.h"
#include "lluuid.h"
#include "llfloater.h"
//#include "llrect.h"

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLIconCtrl;
class LLLineEditor;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLUICtrl;
class LLMessageSystem;

class LLPanelClassified : public LLPanel
{
public:
    LLPanelClassified(BOOL in_finder);
    /*virtual*/ ~LLPanelClassified();

	void reset();

    /*virtual*/ BOOL postBuild();

    /*virtual*/ void draw();

	void refresh();

	void apply();

	// Setup a new classified, including creating an id, giving a sane
	// initial position, etc.
	void initNewClassified();

	void setClassifiedID(const LLUUID& id);
	static void setClickThrough(const LLUUID& classified_id,
								S32 teleport, S32 map, S32 profile);

	// check that the title is valid (E.G. starts with a number or letter)
	BOOL titleIsValid();

	// Schedules the panel to request data
	// from the server next time it is drawn.
	void markForServerRequest();

	std::string getClassifiedName();
	const LLUUID& getClassifiedID() const { return mClassifiedID; }

    void sendClassifiedInfoRequest();
	void sendClassifiedInfoUpdate();

    static void processClassifiedInfoReply(LLMessageSystem* msg, void**);

	static void callbackGotPriceForListing(S32 option, LLString text, void* data);
	static void callbackConfirmPublish(S32 option, void* data);

protected:
	static void onClickUpdate(void* data);
    static void onClickTeleport(void* data);
    static void onClickMap(void* data);
	static void onClickProfile(void* data);
    static void onClickSet(void* data);

	static void onFocusReceived(LLUICtrl* ctrl, void* data);
	static void onCommitAny(LLUICtrl* ctrl, void* data);

	void sendClassifiedClickMessage(const char* type);

protected:
	BOOL mInFinder;
    LLUUID mClassifiedID;
    LLUUID mRequestedID;
	LLUUID mCreatorID;
	LLUUID mParcelID;
	S32 mPriceForListing;

	// Data will be requested on first draw
	BOOL mDataRequested;
	BOOL mEnableCommit;

	// For avatar panel classifieds only, has the user been charged
	// yet for this classified?  That is, have they saved once?
	BOOL mPaidFor;

	LLString mSimName;
    LLVector3d mPosGlobal;

    LLTextureCtrl*	mSnapshotCtrl;
    LLLineEditor*	mNameEditor;
	LLLineEditor*	mDateEditor;
    LLTextEditor*	mDescEditor;
    LLLineEditor*	mLocationEditor;
	LLComboBox*		mCategoryCombo;

	LLButton*    mUpdateBtn;
    LLButton*    mTeleportBtn;
    LLButton*    mMapBtn;
	LLButton*	 mProfileBtn;

	LLTextBox*		mInfoText;
	LLCheckBoxCtrl* mMatureCheck;
	LLCheckBoxCtrl* mAutoRenewCheck;
    LLButton*		mSetBtn;
	LLTextBox*		mClickThroughText;

	LLRect		mSnapshotSize;
	typedef std::list<LLPanelClassified*> panel_list_t;
	static panel_list_t sAllPanels;
};


class LLFloaterPriceForListing
: public LLFloater
{
public:
	LLFloaterPriceForListing();
	virtual ~LLFloaterPriceForListing();
	virtual BOOL postBuild();

	static void show( void (*callback)(S32 option, LLString value, void* userdata), void* userdata );

private:
	static void onClickSetPrice(void*);
	static void onClickCancel(void*);
	static void buttonCore(S32 button, void* data);

private:
	void (*mCallback)(S32 option, LLString, void*);
	void* mUserData;
};


#endif // LL_LLPANELCLASSIFIED_H
