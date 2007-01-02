/** 
 * @file llfloaterland.h
 * @author James Cook
 * @brief "About Land" floater, allowing display and editing of land parcel properties.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERLAND_H
#define LL_LLFLOATERLAND_H

#include <set>
#include <vector>

#include "llfloater.h"
#include "llviewerimagelist.h"

typedef std::set<LLUUID, lluuid_less> uuid_list_t;
const F32 CACHE_REFRESH_TIME	= 2.5f;

class LLTextBox;
class LLCheckBoxCtrl;
class LLComboBox;
class LLButton;
class LLNameListCtrl;
class LLSpinCtrl;
class LLLineEditor;
class LLRadioGroup;
class LLParcelSelectionObserver;
class LLTabContainer;
class LLTextureCtrl;
class LLViewerTextEditor;

class LLPanelLandGeneral;
class LLPanelLandObjects;
class LLPanelLandOptions;
class LLPanelLandMedia;
class LLPanelLandAccess;
class LLPanelLandBan;
class LLPanelLandRenters;
class LLPanelLandCovenant;

class LLFloaterLand
:	public LLFloater
{
public:
	// Call show() to open a land floater.
	// Will query the viewer parcel manager to see what is selected.
	static void show();
	static BOOL floaterVisible() { return sInstance && sInstance->getVisible(); }
	static void refreshAll();

	static LLPanelLandObjects* getCurrentPanelLandObjects();
	static LLPanelLandCovenant* getCurrentPanelLandCovenant();

	// Returns TRUE, but does some early prep work for closing the window.
	virtual BOOL canClose();

	// Destroys itself on close.
	virtual void onClose(bool app_quitting);

protected:
	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterLand();
	virtual ~LLFloaterLand();

	void refresh();


	static void* createPanelLandGeneral(void* data);
	static void* createPanelLandCovenant(void* data);
	static void* createPanelLandObjects(void* data);
	static void* createPanelLandOptions(void* data);
	static void* createPanelLandMedia(void* data);
	static void* createPanelLandAccess(void* data);
	static void* createPanelLandBan(void* data);


protected:
	static LLFloaterLand* sInstance;
	static LLParcelSelectionObserver* sObserver;
	static S32 sLastTab;

	LLTabContainer*			mTabLand;
	LLPanelLandGeneral*		mPanelGeneral;
	LLPanelLandObjects*		mPanelObjects;
	LLPanelLandOptions*		mPanelOptions;
	LLPanelLandMedia*		mPanelMedia;
	LLPanelLandAccess*		mPanelAccess;
	LLPanelLandBan*			mPanelBan;
	LLPanelLandCovenant*	mPanelCovenant;
	LLPanelLandRenters*		mPanelRenters;

public:
	// When closing the dialog, we want to deselect the land.  But when
	// we send an update to the simulator, it usually replies with the
	// parcel information, causing the land to be reselected.  This allows
	// us to suppress that behavior.
	static BOOL sRequestReplyOnUpdate;
};


class LLPanelLandGeneral
:	public LLPanel
{
public:
	LLPanelLandGeneral();
	virtual ~LLPanelLandGeneral();
	void refresh();
	void refreshNames();
	virtual void draw();

	void setGroup(const LLUUID& group_id);
	static void onClickProfile(void*);
	static void onClickSetGroup(void*);
	static void cbGroupID(LLUUID group_id, void* userdata);
	static BOOL enableDeedToGroup(void*);
	static void onClickDeed(void*);
	static void onClickBuyLand(void* data);
	static void onClickRelease(void*);
	static void onClickReclaim(void*);
	static void onClickBuyPass(void* deselect_when_done);
	static BOOL enableBuyPass(void*);
	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void finalizeCommit(void * userdata);
	static void onForSaleChange(LLUICtrl *ctrl, void * userdata);
	static void finalizeSetSellChange(void * userdata);
	static void onSalePriceChange(LLUICtrl *ctrl, void * userdata);

	static void cbBuyPass(S32 option, void*);

	static void onClickSellLand(void* data);
	static void onClickStopSellLand(void* data);
	static void onClickSet(void* data);
	static void onClickClear(void* data);
	static void onClickShow(void* data);
	static void callbackAvatarPick(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data);
	static void finalizeAvatarPick(void* data);
	static void callbackHighlightTransferable(S32 option, void* userdata);
	static void onClickStartAuction(void*);
	// sale change confirmed when "is for sale", "sale price", "sell to whom" fields are changed
	static void confirmSaleChange(S32 landSize, S32 salePrice, std::string authorizedName, void(*callback)(void*), void* userdata);
	static void callbackConfirmSaleChange(S32 option, void* userdata);

	virtual BOOL postBuild();

protected:
	BOOL			mUncheckedSell; // True only when verifying land information when land is for sale on sale info change
	
	LLTextBox*		mLabelName;
	LLLineEditor*	mEditName;
	LLTextBox*		mLabelDesc;
	LLLineEditor*	mEditDesc;

	LLTextBox*		mTextSalePending;

 	LLButton*		mBtnDeedToGroup;
 	LLButton*		mBtnSetGroup;

	LLTextBox*		mTextOwnerLabel;
	LLTextBox*		mTextOwner;
	LLButton*		mBtnProfile;

	LLTextBox*		mTextGroup;
	LLTextBox*		mTextGroupLabel;
	LLTextBox*		mTextClaimDateLabel;
	LLTextBox*		mTextClaimDate;

	LLTextBox*		mTextPriceLabel;
	LLTextBox*		mTextPrice;

	LLCheckBoxCtrl* mCheckDeedToGroup;
	LLCheckBoxCtrl* mCheckContributeWithDeed;

	LLTextBox*		mSaleInfoForSale1;
	LLTextBox*		mSaleInfoForSale2;
	LLTextBox*		mSaleInfoForSaleObjects;
	LLTextBox*		mSaleInfoForSaleNoObjects;
	LLTextBox*		mSaleInfoNotForSale;
	LLButton*		mBtnSellLand;
	LLButton*		mBtnStopSellLand;

	LLTextBox*		mTextDwell;

	LLButton*		mBtnBuyLand;
	LLButton*		mBtnBuyGroupLand;

	// these buttons share the same location, but
	// reclaim is in exactly the same visual place,
	// ond is only shown for estate owners on their
	// estate since they cannot release land.
	LLButton*		mBtnReleaseLand;
	LLButton*		mBtnReclaimLand;

	LLButton*		mBtnBuyPass;
	LLButton* mBtnStartAuction;
};

class LLPanelLandObjects
:	public LLPanel
{
public:
	LLPanelLandObjects();
	virtual ~LLPanelLandObjects();
	void refresh();
	virtual void draw();

	static void callbackReturnOwnerObjects(S32, void*);
	static void callbackReturnGroupObjects(S32, void*);
	static void callbackReturnOtherObjects(S32, void*);
	static void callbackReturnOwnerList(S32, void*);

	static void clickShowCore(S32 return_type, uuid_list_t* list = 0);
	static void onClickShowOwnerObjects(void*);
	static void onClickShowGroupObjects(void*);
	static void onClickShowOtherObjects(void*);

	static void onClickReturnOwnerObjects(void*);
	static void onClickReturnGroupObjects(void*);
	static void onClickReturnOtherObjects(void*);
	static void onClickReturnOwnerList(void*);
	static void onClickRefresh(void*);
	static void onClickType(void*);
	static void onClickName(void*);
	static void onClickDesc(void*);

	static void onDoubleClickOwner(void*);	

	static void onCommitList(LLUICtrl* ctrl, void* data);
	static void onLostFocus(LLLineEditor* caller, void* user_data);
	
	static void processParcelObjectOwnersReply(LLMessageSystem *msg, void **);
	
	virtual BOOL postBuild();

protected:
	void sortBtnCore(S32 column);

	LLTextBox		*mSWTotalObjectsLabel;
	LLTextBox		*mSWTotalObjects;

	LLTextBox		*mParcelObjectBonus;

	LLTextBox		*mObjectContributionLabel;
	LLTextBox		*mObjectContribution;
	LLTextBox		*mTotalObjectsLabel;
	LLTextBox		*mTotalObjects;

	LLTextBox		*mOwnerObjectsLabel;
	LLTextBox		*mOwnerObjects;
	LLButton		*mBtnShowOwnerObjects;
	LLButton		*mBtnReturnOwnerObjects;

	LLTextBox		*mGroupObjectsLabel;
	LLTextBox		*mGroupObjects;
	LLButton		*mBtnShowGroupObjects;
	LLButton		*mBtnReturnGroupObjects;

	LLTextBox		*mOtherObjectsLabel;
	LLTextBox		*mOtherObjects;
	LLButton		*mBtnShowOtherObjects;
	LLButton		*mBtnReturnOtherObjects;

	LLTextBox		*mSelectedObjectsLabel;
	LLTextBox		*mSelectedObjects;

	LLTextBox		*mCleanOtherObjectsLabel;
	LLLineEditor	*mCleanOtherObjectsTime;
	S32				mOtherTime;

	LLTextBox		*mOwnerListText;
	LLButton		*mBtnRefresh;
	LLButton		*mBtnReturnOwnerList;
	LLButton        *mBtnType;			// column 0
	LLButton        *mBtnName;			// column 2
	LLButton        *mBtnDescription;	// column 3
	LLNameListCtrl	*mOwnerList;

	LLPointer<LLViewerImage>	mIconAvatarOnline;
	LLPointer<LLViewerImage>	mIconAvatarOffline;
	LLPointer<LLViewerImage>	mIconGroup;

	U32             mCurrentSortColumn;
	BOOL            mCurrentSortAscending;
	S32             mColWidth[12];

	BOOL			mFirstReply;

	uuid_list_t		mSelectedOwners;
	LLString		mSelectedName;
	S32				mSelectedCount;
	BOOL			mSelectedIsGroup;
};


class LLPanelLandOptions
:	public LLPanel
{
public:
	LLPanelLandOptions();
	virtual ~LLPanelLandOptions();
	void refresh();


	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickSet(void* userdata);
	static void onClickClear(void* userdata);
	static void onClickPublishHelp(void*);

	virtual BOOL postBuild();

protected:
	LLCheckBoxCtrl*	mCheckEditObjects;
	LLCheckBoxCtrl*	mCheckEditGroupObjects;
	LLCheckBoxCtrl*	mCheckAllObjectEntry;
	LLCheckBoxCtrl*	mCheckGroupObjectEntry;
	LLCheckBoxCtrl*	mCheckEditLand;
	LLCheckBoxCtrl*	mCheckSafe;
	LLCheckBoxCtrl*	mCheckFly;
	LLCheckBoxCtrl*	mCheckGroupScripts;
	LLCheckBoxCtrl*	mCheckOtherScripts;
	LLCheckBoxCtrl*	mCheckLandmark;

	LLCheckBoxCtrl*	mCheckShowDirectory;
	LLComboBox*		mCategoryCombo;
	LLComboBox*		mLandingTypeCombo;

	LLTextureCtrl*	mSnapshotCtrl;

	LLTextBox*		mLocationText;
	LLButton*		mSetBtn;
	LLButton*		mClearBtn;

	LLCheckBoxCtrl		*mAllowPublishCtrl;
	LLCheckBoxCtrl		*mMatureCtrl;
	LLCheckBoxCtrl		*mPushRestrictionCtrl;
	LLButton			*mPublishHelpButton;
};


class LLPanelLandMedia
:	public LLPanel
{
public:
	LLPanelLandMedia();
	virtual ~LLPanelLandMedia();
	void refresh();

	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickStopMedia ( void* data );
	static void onClickStartMedia ( void* data );

	virtual BOOL postBuild();

protected:
	LLCheckBoxCtrl* mCheckSoundLocal;
	LLLineEditor*	mMusicURLEdit;
	LLLineEditor*	mMediaURLEdit;
	LLTextureCtrl*	mMediaTextureCtrl;
	LLCheckBoxCtrl*	mMediaAutoScaleCheck;
	//LLButton*		mMediaStopButton;
	//LLButton*		mMediaStartButton;
};



class LLPanelLandAccess
:	public LLPanel
{
public:
	LLPanelLandAccess();
	virtual ~LLPanelLandAccess();
	void refresh();
	void refreshNames();
	virtual void draw();

	void addAvatar(LLUUID id);

	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickAdd(void*);
	static void onClickRemove(void*);
	static void callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata);
	static void onAccessLevelChange(LLUICtrl* ctrl, void* userdata);

	virtual BOOL postBuild();

protected:
	LLTextBox*			mLabelTitle;

	LLCheckBoxCtrl*		mCheckGroup;

	LLCheckBoxCtrl*		mCheckAccess;
	LLNameListCtrl*		mListAccess;
	LLButton*			mBtnAddAccess;
	LLButton*			mBtnRemoveAccess;

	LLCheckBoxCtrl*		mCheckPass;
	LLSpinCtrl*			mSpinPrice;
	LLSpinCtrl*			mSpinHours;

	LLCheckBoxCtrl*		mCheckIdentified;
	LLCheckBoxCtrl*		mCheckTransacted;
	LLRadioGroup*		mCheckStatusLevel;

};


class LLPanelLandBan
:	public LLPanel
{
public:
	LLPanelLandBan();
	virtual ~LLPanelLandBan();
	void refresh();

	void addAvatar(LLUUID id);

	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickAdd(void*);
	static void onClickRemove(void*);
	static void callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata);

	virtual BOOL postBuild();

protected:
	LLTextBox*			mLabelTitle;

	LLCheckBoxCtrl*		mCheck;
	LLNameListCtrl*		mList;
	LLButton*			mBtnAdd;
	LLButton*			mBtnRemove;
	LLCheckBoxCtrl*		mCheckDenyAnonymous;
	LLCheckBoxCtrl*		mCheckDenyIdentified;
	LLCheckBoxCtrl*		mCheckDenyTransacted;
};


class LLPanelLandRenters
:	public LLPanel
{
public:
	LLPanelLandRenters();
	virtual ~LLPanelLandRenters();
	void refresh();

	static void onClickAdd(void*);
	static void onClickRemove(void*);

protected:
	LLCheckBoxCtrl*		mCheckRenters;
	LLNameListCtrl*		mListRenters;
	LLButton*			mBtnAddRenter;
	LLButton*			mBtnRemoveRenter;
};

class LLPanelLandCovenant
:	public LLPanel
{
public:
	LLPanelLandCovenant();
	virtual ~LLPanelLandCovenant();
	virtual BOOL postBuild();
	void refresh();
	static void updateCovenantText(const std::string& string);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerName(const std::string& name);
};

#endif
