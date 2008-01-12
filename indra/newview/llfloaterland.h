/** 
 * @file llfloaterland.h
 * @author James Cook
 * @brief "About Land" floater, allowing display and editing of land parcel properties.
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

#ifndef LL_LLFLOATERLAND_H
#define LL_LLFLOATERLAND_H

#include <set>
#include <vector>

#include "llfloater.h"
#include "llviewerimagelist.h"

typedef std::set<LLUUID, lluuid_less> uuid_list_t;
const F32 CACHE_REFRESH_TIME	= 2.5f;

class LLButton;
class LLCheckBoxCtrl;
class LLRadioGroup;
class LLComboBox;
class LLNameListCtrl;
class LLSpinCtrl;
class LLLineEditor;
class LLRadioGroup;
class LLParcelSelectionObserver;
class LLTabContainer;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLViewerTextEditor;
class LLParcelSelection;

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
	LLPanelLandCovenant*	mPanelCovenant;

	LLHandle<LLParcelSelection>	mParcel;

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
	LLPanelLandGeneral(LLHandle<LLParcelSelection>& parcelp);
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
	static BOOL buyPassDialogVisible();

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
	LLTextEditor*	mEditDesc;

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

	LLHandle<LLParcelSelection>&	mParcel;

	static LLViewHandle sBuyPassDialogHandle;
};

class LLPanelLandObjects
:	public LLPanel
{
public:
	LLPanelLandObjects(LLHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandObjects();
	void refresh();
	virtual void draw();

	static void callbackReturnOwnerObjects(S32, void*);
	static void callbackReturnGroupObjects(S32, void*);
	static void callbackReturnOtherObjects(S32, void*);
	static void callbackReturnOwnerList(S32, void*);

	static void clickShowCore(LLPanelLandObjects* panelp, S32 return_type, uuid_list_t* list = 0);
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
	static void onLostFocus(LLFocusableElement* caller, void* user_data);
	static void onCommitClean(LLUICtrl* caller, void* user_data);
	static void processParcelObjectOwnersReply(LLMessageSystem *msg, void **);
	
	virtual BOOL postBuild();

protected:
	void sortBtnCore(S32 column);

	LLTextBox		*mParcelObjectBonus;
	LLTextBox		*mSWTotalObjects;
	LLTextBox		*mObjectContribution;
	LLTextBox		*mTotalObjects;
	LLTextBox		*mOwnerObjects;
	LLButton		*mBtnShowOwnerObjects;
	LLButton		*mBtnReturnOwnerObjects;
	LLTextBox		*mGroupObjects;
	LLButton		*mBtnShowGroupObjects;
	LLButton		*mBtnReturnGroupObjects;
	LLTextBox		*mOtherObjects;
	LLButton		*mBtnShowOtherObjects;
	LLButton		*mBtnReturnOtherObjects;
	LLTextBox		*mSelectedObjects;
	LLLineEditor	*mCleanOtherObjectsTime;
	S32				mOtherTime;
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

	LLHandle<LLParcelSelection>&	mParcel;
};


class LLPanelLandOptions
:	public LLPanel
{
public:
	LLPanelLandOptions(LLHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandOptions();
	void refresh();


	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickSet(void* userdata);
	static void onClickClear(void* userdata);
	static void onClickPublishHelp(void*);

	virtual BOOL postBuild();
	virtual void draw();

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

	LLCheckBoxCtrl		*mMatureCtrl;
	LLCheckBoxCtrl		*mPushRestrictionCtrl;
	LLButton			*mPublishHelpButton;

	LLHandle<LLParcelSelection>&	mParcel;
};


class LLPanelLandMedia
:	public LLPanel
{
public:
	LLPanelLandMedia(LLHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandMedia();
	void refresh();

	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickStopMedia ( void* data );
	static void onClickStartMedia ( void* data );

	virtual BOOL postBuild();

protected:
	LLCheckBoxCtrl* mCheckSoundLocal;
	LLRadioGroup*	mRadioVoiceChat;
	LLLineEditor*	mMusicURLEdit;
	LLLineEditor*	mMediaURLEdit;
	LLTextureCtrl*	mMediaTextureCtrl;
	LLCheckBoxCtrl*	mMediaAutoScaleCheck;
	//LLButton*		mMediaStopButton;
	//LLButton*		mMediaStartButton;

	LLHandle<LLParcelSelection>&	mParcel;
};



class LLPanelLandAccess
:	public LLPanel
{
public:
	LLPanelLandAccess(LLHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandAccess();
	void refresh();
	void refresh_ui();
	void refreshNames();
	virtual void draw();

	static void onCommitPublicAccess(LLUICtrl* ctrl, void *userdata);
	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickAddAccess(void*);
	static void callbackAvatarCBAccess(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata);
	static void onClickRemoveAccess(void*);
	static void onClickAddBanned(void*);
	static void callbackAvatarCBBanned(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata);
	static void onClickRemoveBanned(void*);

	virtual BOOL postBuild();

protected:
	LLNameListCtrl*		mListAccess;
	LLNameListCtrl*		mListBanned;

	LLHandle<LLParcelSelection>&	mParcel;
};


class LLPanelLandCovenant
:	public LLPanel
{
public:
	LLPanelLandCovenant(LLHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandCovenant();
	virtual BOOL postBuild();
	void refresh();
	static void updateCovenantText(const std::string& string);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerName(const std::string& name);

protected:
	LLHandle<LLParcelSelection>&	mParcel;
};

#endif
