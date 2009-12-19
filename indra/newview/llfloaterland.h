/** 
 * @file llfloaterland.h
 * @author James Cook
 * @brief "About Land" floater, allowing display and editing of land parcel properties.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llpointer.h"	// LLPointer<>
//#include "llviewertexturelist.h"
#include "llsafehandle.h"

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
class LLUIImage;
class LLParcelSelection;

class LLPanelLandGeneral;
class LLPanelLandObjects;
class LLPanelLandOptions;
class LLPanelLandAudio;
class LLPanelLandMedia;
class LLPanelLandAccess;
class LLPanelLandBan;
class LLPanelLandRenters;
class LLPanelLandCovenant;
class LLParcel;

class LLFloaterLand
:	public LLFloater
{
	friend class LLFloaterReg;
public:
	static void refreshAll();

	static LLPanelLandObjects* getCurrentPanelLandObjects();
	static LLPanelLandCovenant* getCurrentPanelLandCovenant();
	
	LLParcel* getCurrentSelectedParcel();
	
	virtual void onOpen(const LLSD& key);
	virtual BOOL postBuild();

private:
	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterLand(const LLSD& seed);
	virtual ~LLFloaterLand();
		
	void onVisibilityChange(const LLSD& visible);

protected:

	/*virtual*/ void refresh();

	static void* createPanelLandGeneral(void* data);
	static void* createPanelLandCovenant(void* data);
	static void* createPanelLandObjects(void* data);
	static void* createPanelLandOptions(void* data);
	static void* createPanelLandAudio(void* data);
	static void* createPanelLandMedia(void* data);
	static void* createPanelLandAccess(void* data);
	static void* createPanelLandBan(void* data);


protected:
	static LLParcelSelectionObserver* sObserver;
	static S32 sLastTab;

	LLTabContainer*			mTabLand;
	LLPanelLandGeneral*		mPanelGeneral;
	LLPanelLandObjects*		mPanelObjects;
	LLPanelLandOptions*		mPanelOptions;
	LLPanelLandAudio*		mPanelAudio;
	LLPanelLandMedia*		mPanelMedia;
	LLPanelLandAccess*		mPanelAccess;
	LLPanelLandCovenant*	mPanelCovenant;

	LLSafeHandle<LLParcelSelection>	mParcel;

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
	LLPanelLandGeneral(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandGeneral();
	/*virtual*/ void refresh();
	void refreshNames();
	virtual void draw();

	void setGroup(const LLUUID& group_id);
	static void onClickProfile(void*);
		   void onClickSetGroup();
	static BOOL enableDeedToGroup(void*);
	static void onClickDeed(void*);
	static void onClickBuyLand(void* data);
	static void onClickScriptLimits(void* data);
	static void onClickRelease(void*);
	static void onClickReclaim(void*);
	static void onClickBuyPass(void* deselect_when_done);
	static BOOL enableBuyPass(void*);
	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void finalizeCommit(void * userdata);
	static void onForSaleChange(LLUICtrl *ctrl, void * userdata);
	static void finalizeSetSellChange(void * userdata);
	static void onSalePriceChange(LLUICtrl *ctrl, void * userdata);

	static bool cbBuyPass(const LLSD& notification, const LLSD& response);

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
	
	LLTextBox*		mContentRating;
	LLTextBox*		mLandType;

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
	LLButton*		mBtnScriptLimits;
	LLButton*		mBtnBuyGroupLand;

	// these buttons share the same location, but
	// reclaim is in exactly the same visual place,
	// ond is only shown for estate owners on their
	// estate since they cannot release land.
	LLButton*		mBtnReleaseLand;
	LLButton*		mBtnReclaimLand;

	LLButton*		mBtnBuyPass;
	LLButton* mBtnStartAuction;

	LLSafeHandle<LLParcelSelection>&	mParcel;

	static LLHandle<LLFloater> sBuyPassDialogHandle;
};

class LLPanelLandObjects
:	public LLPanel
{
public:
	LLPanelLandObjects(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandObjects();
	/*virtual*/ void refresh();
	virtual void draw();

	bool callbackReturnOwnerObjects(const LLSD& notification, const LLSD& response);
	bool callbackReturnGroupObjects(const LLSD& notification, const LLSD& response);
	bool callbackReturnOtherObjects(const LLSD& notification, const LLSD& response);
	bool callbackReturnOwnerList(const LLSD& notification, const LLSD& response);

	static void clickShowCore(LLPanelLandObjects* panelp, S32 return_type, uuid_list_t* list = 0);
	static void onClickShowOwnerObjects(void*);
	static void onClickShowGroupObjects(void*);
	static void onClickShowOtherObjects(void*);

	static void onClickReturnOwnerObjects(void*);
	static void onClickReturnGroupObjects(void*);
	static void onClickReturnOtherObjects(void*);
	static void onClickReturnOwnerList(void*);
	static void onClickRefresh(void*);

	static void onDoubleClickOwner(void*);	

	static void onCommitList(LLUICtrl* ctrl, void* data);
	static void onLostFocus(LLFocusableElement* caller, void* user_data);
	static void onCommitClean(LLUICtrl* caller, void* user_data);
	static void processParcelObjectOwnersReply(LLMessageSystem *msg, void **);
	
	virtual BOOL postBuild();

protected:

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
	LLNameListCtrl	*mOwnerList;

	LLPointer<LLUIImage>	mIconAvatarOnline;
	LLPointer<LLUIImage>	mIconAvatarOffline;
	LLPointer<LLUIImage>	mIconGroup;

	BOOL			mFirstReply;

	uuid_list_t		mSelectedOwners;
	std::string		mSelectedName;
	S32				mSelectedCount;
	BOOL			mSelectedIsGroup;

	LLSafeHandle<LLParcelSelection>&	mParcel;
};


class LLPanelLandOptions
:	public LLPanel
{
public:
	LLPanelLandOptions(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandOptions();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void refresh();

private:
	// Refresh the "show in search" checkbox and category selector.
	void refreshSearch();

	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onClickSet(void* userdata);
	static void onClickClear(void* userdata);

private:
	LLCheckBoxCtrl*	mCheckEditObjects;
	LLCheckBoxCtrl*	mCheckEditGroupObjects;
	LLCheckBoxCtrl*	mCheckAllObjectEntry;
	LLCheckBoxCtrl*	mCheckGroupObjectEntry;
	LLCheckBoxCtrl*	mCheckEditLand;
	LLCheckBoxCtrl*	mCheckSafe;
	LLCheckBoxCtrl*	mCheckFly;
	LLCheckBoxCtrl*	mCheckGroupScripts;
	LLCheckBoxCtrl*	mCheckOtherScripts;

	LLCheckBoxCtrl*	mCheckShowDirectory;
	LLComboBox*		mCategoryCombo;
	LLComboBox*		mLandingTypeCombo;

	LLTextureCtrl*	mSnapshotCtrl;

	LLTextBox*		mLocationText;
	LLButton*		mSetBtn;
	LLButton*		mClearBtn;

	LLCheckBoxCtrl		*mMatureCtrl;
	LLCheckBoxCtrl		*mPushRestrictionCtrl;

	LLSafeHandle<LLParcelSelection>&	mParcel;
};


class LLPanelLandAccess
:	public LLPanel
{
public:
	LLPanelLandAccess(LLSafeHandle<LLParcelSelection>& parcelp);
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

	LLSafeHandle<LLParcelSelection>&	mParcel;
};


class LLPanelLandCovenant
:	public LLPanel
{
public:
	LLPanelLandCovenant(LLSafeHandle<LLParcelSelection>& parcelp);
	virtual ~LLPanelLandCovenant();
	virtual BOOL postBuild();
	void refresh();
	static void updateCovenantText(const std::string& string);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerName(const std::string& name);

protected:
	LLSafeHandle<LLParcelSelection>&	mParcel;
};

#endif
