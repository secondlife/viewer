/** 
 * @file llfloaterbuyland.cpp
 * @brief LLFloaterBuyLand class implementation
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuyland.h"

// viewer includes
#include "llagent.h"
#include "llbutton.h"
#include "llcachename.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llconfirmationmanager.h"
#include "llcurrencyuimanager.h"
#include "llfloater.h"
#include "llfloatertools.h"
#include "llframetimer.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llnotify.h"
#include "llparcel.h"
#include "llstatusbar.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "llviewchildren.h"
#include "llviewercontrol.h"
#include "llvieweruictrlfactory.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llweb.h"
#include "llwindow.h"
#include "llworld.h"
#include "llxmlrpctransaction.h"
#include "viewer.h"
#include "roles_constants.h"

// NOTE: This is duplicated in lldatamoney.cpp ...
const F32 GROUP_LAND_BONUS_FACTOR = 1.1f;
const F64 CURRENCY_ESTIMATE_FREQUENCY = 0.5;
	// how long of a pause in typing a currency buy amount before an
	// esimate is fetched from the server

class LLFloaterBuyLandUI
:	public LLFloater
{
private:
	LLFloaterBuyLandUI();
	virtual ~LLFloaterBuyLandUI();

	LLViewerRegion*	mRegion;
	LLParcel*		mParcel;
	bool			mIsClaim;
	bool			mIsForGroup;

	bool			mCanBuy;
	bool			mCannotBuyIsError;
	std::string		mCannotBuyReason;
	std::string		mCannotBuyURI;
	
	bool			mBought;
	
	// information about the agent
	S32				mAgentCommittedTier;
	S32				mAgentCashBalance;
	bool			mAgentHasNeverOwnedLand;
	
	// information about the parcel
	bool			mParcelValid;
	bool			mParcelIsForSale;
	bool			mParcelIsFirstLand;
	bool			mParcelIsGroupLand;
	S32				mParcelGroupContribution;
	S32				mParcelPrice;
	S32				mParcelActualArea;
	S32				mParcelBillableArea;
	S32				mParcelSupportedObjects;
	bool			mParcelSoldWithObjects;
	std::string		mParcelLocation;
	LLUUID			mParcelSnapshot;
	std::string		mParcelSellerName;
	
	// user's choices
	S32				mUserPlanChoice;
	
	// from website
	bool			mSiteValid;
	bool			mSiteMembershipUpgrade;
	std::string		mSiteMembershipAction;
	std::vector<std::string>
					mSiteMembershipPlanIDs;
	std::vector<std::string>
					mSiteMembershipPlanNames;
	bool			mSiteLandUseUpgrade;
	std::string		mSiteLandUseAction;
	std::string		mSiteConfirm;
	
	// values in current Preflight transaction... used to avoid extra
	// preflights when the parcel manager goes update crazy
	S32				mPreflightAskBillableArea;
	S32				mPreflightAskCurrencyBuy;

	LLViewChildren		mChildren;
	LLCurrencyUIManager	mCurrency;
	
	enum TransactionType
	{
		TransactionPreflight,
		TransactionCurrency,
		TransactionBuy
	};
	LLXMLRPCTransaction* mTransaction;
	TransactionType		 mTransactionType;
	
	LLViewerParcelMgr::ParcelBuyInfo*	mParcelBuyInfo;
	
	static LLFloaterBuyLandUI* sInstance;
	
public:
	static LLFloaterBuyLandUI* soleInstance(bool createIfNeeded);

	void setForGroup(bool is_for_group);
	void setParcel(LLViewerRegion* region, LLParcel* parcel);
		
	void updateAgentInfo();
	void updateParcelInfo();
	void updateCovenantInfo();
	static void onChangeAgreeCovenant(LLUICtrl* ctrl, void* user_data);
	void updateCovenantText(const std::string& string, const LLUUID &asset_id);
	void updateEstateName(const std::string& name);
	void updateLastModified(const std::string& text);
	void updateEstateOwnerName(const std::string& name);
	void updateWebSiteInfo();
	void finishWebSiteInfo();
	
	void runWebSitePrep(const std::string& password);
	void finishWebSitePrep();
	void sendBuyLand();

	void updateNames();
	
	void refreshUI();
	
	void startTransaction(TransactionType type, LLXMLRPCValue params);
	bool checkTransaction();
	
	void tellUserError(const std::string& message, const std::string& uri);

	virtual BOOL postBuild();
	
	void startBuyPreConfirm();
	void startBuyPostConfirm(const std::string& password);

	static void onClickBuy(void* data);
	static void onClickCancel(void* data);
	static void onClickErrorWeb(void* data);
	
	virtual void draw();
	virtual BOOL canClose();
	virtual void onClose(bool app_quitting);
	
private:
	class SelectionObserver : public LLParcelObserver
	{
	public:
		virtual void changed();
	};
};

static void cacheNameUpdateRefreshesBuyLand(const LLUUID&,
	const char*, const char*, BOOL, void* data)
{
	LLFloaterBuyLandUI* ui = LLFloaterBuyLandUI::soleInstance(false);
	if (ui)
	{
		ui->updateNames();
	}
}

// static
void LLFloaterBuyLand::buyLand(
	LLViewerRegion* region, LLParcel* parcel, bool is_for_group)
{
	if(is_for_group && !gAgent.hasPowerInActiveGroup(GP_LAND_DEED))
	{
		gViewerWindow->alertXml("OnlyOfficerCanBuyLand");
		return;
	}

	LLFloaterBuyLandUI* ui = LLFloaterBuyLandUI::soleInstance(true);
	ui->setForGroup(is_for_group);
	ui->setParcel(region, parcel);
	ui->open();	/*Flawfinder: ignore*/
}

// static
void LLFloaterBuyLand::updateCovenantText(const std::string& string, const LLUUID &asset_id)
{
	LLFloaterBuyLandUI* floater = LLFloaterBuyLandUI::soleInstance(FALSE);
	if (floater)
	{
		floater->updateCovenantText(string, asset_id);
	}
}

// static
void LLFloaterBuyLand::updateEstateName(const std::string& name)
{
	LLFloaterBuyLandUI* floater = LLFloaterBuyLandUI::soleInstance(FALSE);
	if (floater)
	{
		floater->updateEstateName(name);
	}
}

// static
void LLFloaterBuyLand::updateLastModified(const std::string& text)
{
	LLFloaterBuyLandUI* floater = LLFloaterBuyLandUI::soleInstance(FALSE);
	if (floater)
	{
		floater->updateLastModified(text);
	}
}

// static
void LLFloaterBuyLand::updateEstateOwnerName(const std::string& name)
{
	LLFloaterBuyLandUI* floater = LLFloaterBuyLandUI::soleInstance(FALSE);
	if (floater)
	{
		floater->updateEstateOwnerName(name);
	}
}

// static
BOOL LLFloaterBuyLand::isOpen()
{
	LLFloaterBuyLandUI* floater = LLFloaterBuyLandUI::soleInstance(FALSE);
	if (floater)
	{
		return floater->getVisible();
	}
	return FALSE;
}

// static
LLFloaterBuyLandUI* LLFloaterBuyLandUI::sInstance = NULL;

// static
LLFloaterBuyLandUI* LLFloaterBuyLandUI::soleInstance(bool createIfNeeded)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	if (createIfNeeded)
	{
		delete sInstance;
		sInstance = NULL;
	}
#endif
	if (!sInstance  &&  createIfNeeded)
	{
		sInstance = new LLFloaterBuyLandUI();

		gUICtrlFactory->buildFloater(sInstance, "floater_buy_land.xml");
		sInstance->center();

		static bool observingCacheName = false;
		if (!observingCacheName)
		{
			gCacheName->addObserver(cacheNameUpdateRefreshesBuyLand);
			observingCacheName = true;
		}

		static SelectionObserver* parcelSelectionObserver = NULL;
		if (!parcelSelectionObserver)
		{
			parcelSelectionObserver = new SelectionObserver;
			gParcelMgr->addObserver(parcelSelectionObserver);
		}
	}
	
	return sInstance;
}


#if LL_WINDOWS
// passing 'this' during construction generates a warning. The callee
// only uses the pointer to hold a reference to 'this' which is
// already valid, so this call does the correct thing. Disable the
// warning so that we can compile without generating a warning.
#pragma warning(disable : 4355)
#endif 
LLFloaterBuyLandUI::LLFloaterBuyLandUI()
:	LLFloater("Buy Land"),
	mParcel(0),
	mBought(false),
	mParcelValid(false), mSiteValid(false),
	mChildren(*this), mCurrency(*this), mTransaction(0),
	mParcelBuyInfo(0)
{
}

LLFloaterBuyLandUI::~LLFloaterBuyLandUI()
{
	delete mTransaction;

	gParcelMgr->deleteParcelBuy(mParcelBuyInfo);
	
	if (sInstance == this)
	{
		sInstance = NULL;
	}
}

void LLFloaterBuyLandUI::SelectionObserver::changed()
{
	LLFloaterBuyLandUI* ui = LLFloaterBuyLandUI::soleInstance(false);
	if (ui)
	{
		if (gParcelMgr->selectionEmpty())
		{
			ui->close();
		}
		else {
			ui->setParcel(
				gParcelMgr->getSelectionRegion(),
				gParcelMgr->getSelectedParcel());
		}
	}
}


void LLFloaterBuyLandUI::updateAgentInfo()
{
	mAgentCommittedTier = gStatusBar->getSquareMetersCommitted();
	mAgentCashBalance = gStatusBar->getBalance();

	// *TODO: This is an approximation, we should send this value down
	// to the viewer. See SL-10728 for details.
	mAgentHasNeverOwnedLand = mAgentCommittedTier == 0;
}

void LLFloaterBuyLandUI::updateParcelInfo()
{
	mParcelValid = mParcel && mRegion;
	mParcelIsForSale = false;
	mParcelIsFirstLand = false;
	mParcelIsGroupLand = false;
	mParcelGroupContribution = 0;
	mParcelPrice = 0;
	mParcelActualArea = 0;
	mParcelBillableArea = 0;
	mParcelSupportedObjects = 0;
	mParcelSoldWithObjects = false;
	mParcelLocation = "";
	mParcelSnapshot.setNull();
	mParcelSellerName = "";
	
	mCanBuy = false;
	mCannotBuyIsError = false;
	
	if (!mParcelValid)
	{
		mCannotBuyReason= childGetText("no_land_selected");
		return;
	}
	
	if (gParcelMgr->getMultipleOwners())
	{
		mCannotBuyReason = childGetText("multiple_parcels_selected");
		return;
	}

	
	const LLUUID& parcelOwner = mParcel->getOwnerID();
	
	mIsClaim = mParcel->isPublic();
	if (!mIsClaim)
	{
		mParcelActualArea = mParcel->getArea();
		mParcelIsForSale = mParcel->getForSale();
		mParcelIsFirstLand = mParcel->getReservedForNewbie();
		mParcelIsGroupLand = mParcel->getIsGroupOwned();
		mParcelPrice = mParcelIsForSale ? mParcel->getSalePrice() : 0;
		
		if (mParcelIsGroupLand)
		{
			LLUUID group_id = mParcel->getGroupID();
			mParcelGroupContribution = gAgent.getGroupContribution(group_id);
		}
	}
	else
	{
		mParcelActualArea = gParcelMgr->getClaimableArea();
		mParcelIsForSale = true;
		mParcelPrice = mParcelActualArea * mParcel->getClaimPricePerMeter();
	}

	mParcelBillableArea =
		llround(mRegion->getBillableFactor() * mParcelActualArea);

 	mParcelSupportedObjects = llround(
		mParcel->getMaxPrimCapacity() * mParcel->getParcelPrimBonus()); 
 	// Can't have more than region max tasks, regardless of parcel 
 	// object bonus factor. 
 	LLViewerRegion* region = gParcelMgr->getSelectionRegion(); 
 	if(region) 
 	{ 
		S32 max_tasks_per_region = (S32)region->getMaxTasks(); 
		mParcelSupportedObjects = llmin(
			mParcelSupportedObjects, max_tasks_per_region); 
 	} 

	mParcelSoldWithObjects = mParcel->getSellWithObjects();
	
	LLVector3 center = mParcel->getCenterpoint();
	mParcelLocation = llformat("%s %d,%d",
				mRegion->getName().c_str(),
				(int)center[VX], (int)center[VY]
				);
	
	mParcelSnapshot = mParcel->getSnapshotID();
	
	updateNames();
	
	bool haveEnoughCash = mParcelPrice <= mAgentCashBalance;
	S32 cashBuy = haveEnoughCash ? 0 : (mParcelPrice - mAgentCashBalance);
	mCurrency.setAmount(cashBuy, true);
	mCurrency.setZeroMessage(haveEnoughCash ? childGetText("none_needed") : "");

	// checks that we can buy the land

	if(mIsForGroup && !gAgent.hasPowerInActiveGroup(GP_LAND_DEED))
	{
		mCannotBuyReason = childGetText("cant_buy_for_group");
		return;
	}

	if (!mIsClaim)
	{
		const LLUUID& authorizedBuyer = mParcel->getAuthorizedBuyerID();
		const LLUUID buyer = gAgent.getID();
		const LLUUID newOwner = mIsForGroup ? gAgent.getGroupID() : buyer;

		if (!mParcelIsForSale
			|| (mParcelPrice == 0  &&  authorizedBuyer.isNull()))
		{
			
			mCannotBuyReason = childGetText("parcel_not_for_sale");
			return;
		}

		if (parcelOwner == newOwner)
		{
			if (mIsForGroup)
			{
				mCannotBuyReason = childGetText("group_already_owns");
			}
			else
			{
				mCannotBuyReason = childGetText("you_already_own");
			}
			return;
		}

		if (!authorizedBuyer.isNull()  &&  buyer != authorizedBuyer)
		{
			mCannotBuyReason = childGetText("set_to_sell_to_other");
			return;
		}
	}
	else
	{
		if (mParcelActualArea == 0)
		{
			mCannotBuyReason = childGetText("no_public_land");
			return;
		}

		if (gParcelMgr->hasOthersSelected())
		{
			// Policy: Must not have someone else's land selected
			mCannotBuyReason = childGetText("not_owned_by_you");
			return;
		}
	}

	/*
	if ((mRegion->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
		&& !gAgent.isGodlike())
	{
		mCannotBuyReason = llformat(
				"The region %s does not allow transfer of land.",
				mRegion->getName().c_str() );
		return;
	}
	*/
	
	if (mParcel->getReservedForNewbie())
	{
		if (mIsForGroup)
		{
			mCannotBuyReason = childGetText("first_time_group");
			return;
		}
		
		if (gStatusBar->getSquareMetersCommitted() > 0)
		{
			mCannotBuyReason == childGetText("first_time");
			return;
		}
		
		// *TODO: There should be a check based on the database value
		// indra.user.ever_owned_land, only that value never makes it
		// to the viewer, see SL-10728
	}

	mCanBuy = true;
}

void LLFloaterBuyLandUI::updateCovenantInfo()
{
	LLViewerRegion* region = gParcelMgr->getSelectionRegion();
	if(!region) return;

	LLTextBox* region_name = (LLTextBox*)getChildByName("region_name_text");
	if (region_name)
	{
		region_name->setText(region->getName());
	}

	LLTextBox* resellable_clause = (LLTextBox*)getChildByName("resellable_clause");
	if (resellable_clause)
	{
		if (region->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
		{
			resellable_clause->setText(childGetText("can_not_resell"));
		}
		else
		{
			resellable_clause->setText(childGetText("can_resell"));
		}
	}
	
	LLTextBox* changeable_clause = (LLTextBox*)getChildByName("changeable_clause");
	if (changeable_clause)
	{
		if (region->getRegionFlags() & REGION_FLAGS_ALLOW_PARCEL_CHANGES)
		{
			changeable_clause->setText(childGetText("can_change"));
		}
		else
		{
			changeable_clause->setText(childGetText("can_not_change"));
		}
	}

	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)getChildByName("agree_covenant");
	if(check)
	{
		check->set(false);
		check->setEnabled(true);
		check->setCallbackUserData(this);
		check->setCommitCallback(onChangeAgreeCovenant);
	}

	LLTextBox* box = (LLTextBox*)getChildByName("covenant_text");
	if(box)
	{
		box->setVisible(FALSE);
	}
	
	// send EstateCovenantInfo message
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessage("EstateCovenantRequest");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->sendReliable(region->getHost());
}

// static
void LLFloaterBuyLandUI::onChangeAgreeCovenant(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterBuyLandUI* self = (LLFloaterBuyLandUI*)user_data;
	if(self)
	{
		self->refreshUI();
	}
}

void LLFloaterBuyLandUI::updateCovenantText(const std::string &string, const LLUUID& asset_id)
{
	LLViewerTextEditor* editor = (LLViewerTextEditor*)getChildByName("covenant_editor");
	if (editor)
	{
		editor->setHandleEditKeysDirectly(FALSE);
		editor->setText(string);

		LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)getChildByName("agree_covenant");
		LLTextBox* box = (LLTextBox*)getChildByName("covenant_text");
		if(check && box)
		{
			if (asset_id.isNull())
			{
				check->set(true);
				check->setEnabled(false);
				refreshUI();

				// remove the line stating that you must agree
				box->setVisible(FALSE);
			}
			else
			{
				check->setEnabled(true);

				// remove the line stating that you must agree
				box->setVisible(TRUE);
			}
		}
	}
}

void LLFloaterBuyLandUI::updateEstateName(const std::string& name)
{
	LLTextBox* box = (LLTextBox*)getChildByName("estate_name_text");
	if (box) box->setText(name);
}

void LLFloaterBuyLandUI::updateLastModified(const std::string& text)
{
	LLTextBox* editor = (LLTextBox*)getChildByName("covenant_timestamp_text");
	if (editor) editor->setText(text);
}

void LLFloaterBuyLandUI::updateEstateOwnerName(const std::string& name)
{
	LLTextBox* box = (LLTextBox*)getChildByName("estate_owner_text");
	if (box) box->setText(name);
}

void LLFloaterBuyLandUI::updateWebSiteInfo()
{
	S32 askBillableArea = mIsForGroup ? 0 : mParcelBillableArea;
	S32 askCurrencyBuy = mCurrency.getAmount();
	
	if (mTransaction && mTransactionType == TransactionPreflight
	&&  mPreflightAskBillableArea == askBillableArea
	&&  mPreflightAskCurrencyBuy == askCurrencyBuy)
	{
		return;
	}
	
	mPreflightAskBillableArea = askBillableArea;
	mPreflightAskCurrencyBuy = askCurrencyBuy;
	
#if 0
	// enable this code if you want the details to blank while we're talking
	// to the web site... it's kind of jarring
	mSiteValid = false;
	mSiteMembershipUpgrade = false;
	mSiteMembershipAction = "(waiting)";
	mSiteMembershipPlanIDs.clear();
	mSiteMembershipPlanNames.clear();
	mSiteLandUseUpgrade = false;
	mSiteLandUseAction = "(waiting)";
	mSiteCurrencyEstimated = false;
	mSiteCurrencyEstimatedCost = 0;
#endif
	
	LLXMLRPCValue keywordArgs = LLXMLRPCValue::createStruct();
	keywordArgs.appendString("agentId", gAgent.getID().asString());
	keywordArgs.appendString(
		"secureSessionId",
		gAgent.getSecureSessionID().asString());
	keywordArgs.appendInt("billableArea", mPreflightAskBillableArea);
	keywordArgs.appendInt("currencyBuy", mPreflightAskCurrencyBuy);
	
	LLXMLRPCValue params = LLXMLRPCValue::createArray();
	params.append(keywordArgs);
	
	startTransaction(TransactionPreflight, params);
}

void LLFloaterBuyLandUI::finishWebSiteInfo()
{
	
	LLXMLRPCValue result = mTransaction->responseValue();

	mSiteValid = result["success"].asBool();
	if (!mSiteValid)
	{
		tellUserError(
			result["errorMessage"].asString(),
			result["errorURI"].asString()
		);
		return;
	}
	
	LLXMLRPCValue membership = result["membership"];
	mSiteMembershipUpgrade = membership["upgrade"].asBool();
	mSiteMembershipAction = membership["action"].asString();
	mSiteMembershipPlanIDs.clear();
	mSiteMembershipPlanNames.clear();
	LLXMLRPCValue levels = membership["levels"];
	for (LLXMLRPCValue level = levels.rewind();
		level.isValid();
		level = levels.next())
	{
		mSiteMembershipPlanIDs.push_back(level["id"].asString());
		mSiteMembershipPlanNames.push_back(level["description"].asString());
	}
	mUserPlanChoice = 0;

	LLXMLRPCValue landUse = result["landUse"];
	mSiteLandUseUpgrade = landUse["upgrade"].asBool();
	mSiteLandUseAction = landUse["action"].asString();

	LLXMLRPCValue currency = result["currency"];
	mCurrency.setEstimate(currency["estimatedCost"].asInt());

	mSiteConfirm = result["confirm"].asString();
}

void LLFloaterBuyLandUI::runWebSitePrep(const std::string& password)
{
	if (!mCanBuy)
	{
		return;
	}
	
	BOOL remove_contribution = childGetValue("remove_contribution").asBoolean();
	mParcelBuyInfo = gParcelMgr->setupParcelBuy(gAgent.getID(), gAgent.getSessionID(),
						gAgent.getGroupID(), mIsForGroup, mIsClaim, remove_contribution);

	if (mParcelBuyInfo
		&& !mSiteMembershipUpgrade
		&& !mSiteLandUseUpgrade
		&& mCurrency.getAmount() == 0
		&& mSiteConfirm != "password")
	{
		sendBuyLand();
		return;
	}


	std::string newLevel = "noChange";
	
	if (mSiteMembershipUpgrade)
	{
		LLComboBox* levels = LLUICtrlFactory::getComboBoxByName(this, "account_level");
		if (levels)
		{
			mUserPlanChoice = levels->getCurrentIndex();
			newLevel = mSiteMembershipPlanIDs[mUserPlanChoice];
		}
	}
	
	LLXMLRPCValue keywordArgs = LLXMLRPCValue::createStruct();
	keywordArgs.appendString("agentId", gAgent.getID().asString());
	keywordArgs.appendString(
		"secureSessionId",
		gAgent.getSecureSessionID().asString());
	keywordArgs.appendString("levelId", newLevel);
	keywordArgs.appendInt("billableArea",
		mIsForGroup ? 0 : mParcelBillableArea);
	keywordArgs.appendInt("currencyBuy", mCurrency.getAmount());
	keywordArgs.appendInt("estimatedCost", mCurrency.getEstimate());
	keywordArgs.appendString("confirm", mSiteConfirm);
	if (!password.empty())
	{
		keywordArgs.appendString("password", password);
	}
	
	LLXMLRPCValue params = LLXMLRPCValue::createArray();
	params.append(keywordArgs);
	
	startTransaction(TransactionBuy, params);
}

void LLFloaterBuyLandUI::finishWebSitePrep()
{
	LLXMLRPCValue result = mTransaction->responseValue();

	bool success = result["success"].asBool();
	if (!success)
	{
		tellUserError(
			result["errorMessage"].asString(),
			result["errorURI"].asString()
		);
		return;
	}
	
	sendBuyLand();
}

void LLFloaterBuyLandUI::sendBuyLand()
{
	if (mParcelBuyInfo)
	{
		gParcelMgr->sendParcelBuy(mParcelBuyInfo);
		gParcelMgr->deleteParcelBuy(mParcelBuyInfo);
		mBought = true;
	}
}

void LLFloaterBuyLandUI::updateNames()
{
	if (!mParcelValid)
	{
		mParcelSellerName = "";
		return;
	}
	
	if (mIsClaim)
	{
		mParcelSellerName = "Linden Lab";
	}
	else if (mParcel->getIsGroupOwned())
	{
		char groupName[DB_LAST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
		
		gCacheName->getGroupName(mParcel->getGroupID(), &groupName[0]);
		mParcelSellerName = groupName;
	}
	else
	{
		char firstName[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
		char lastName[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
		
		gCacheName->getName(mParcel->getOwnerID(), firstName, lastName);
		mParcelSellerName = llformat("%s %s", firstName, lastName);
	}
}


void LLFloaterBuyLandUI::startTransaction(TransactionType type,
										  LLXMLRPCValue params)
{
	delete mTransaction;
	mTransaction = NULL;

	mTransactionType = type;

	// Select a URI and method appropriate for the transaction type.
	static std::string transaction_uri;
	if (transaction_uri.empty())
	{
		transaction_uri = getHelperURI() + "landtool.php";
	}
	
	const char* method;
	switch (mTransactionType)
	{
		case TransactionPreflight:
			method = "preflightBuyLandPrep";
			break;
		case TransactionBuy:
			method = "buyLandPrep";
			break;
		default:
			llwarns << "LLFloaterBuyLandUI: Unknown transaction type!" << llendl;
			return;
	}

	mTransaction = new LLXMLRPCTransaction(
		transaction_uri,
		method,
		params,
		false /* don't use gzip */
		);
}

bool LLFloaterBuyLandUI::checkTransaction()
{
	if (!mTransaction)
	{
		return false;
	}
	
	if (!mTransaction->process())
	{
		return false;
	}

	if (mTransaction->status(NULL) != LLXMLRPCTransaction::StatusComplete)
	{
		tellUserError(mTransaction->statusMessage(), mTransaction->statusURI());
	}
	else {
		switch (mTransactionType)
		{	
			case TransactionPreflight:	finishWebSiteInfo();	break;
			case TransactionBuy:		finishWebSitePrep();	break;
			default: ;
		}
	}
	
	delete mTransaction;
	mTransaction = NULL;
	
	return true;
}

void LLFloaterBuyLandUI::tellUserError(
	const std::string& message, const std::string& uri)
{
	mCanBuy = false;
	mCannotBuyIsError = true;
	mCannotBuyReason = childGetText("fetching_error");
	mCannotBuyReason += message;
	mCannotBuyURI = uri;
}


// virtual
BOOL LLFloaterBuyLandUI::postBuild()
{
	mCurrency.prepare();
	
	childSetAction("buy_btn", onClickBuy, this);
	childSetAction("cancel_btn", onClickCancel, this);
	childSetAction("error_web", onClickErrorWeb, this);
	
	return TRUE;
}

void LLFloaterBuyLandUI::setParcel(LLViewerRegion* region, LLParcel* parcel)
{
	if (mTransaction &&  mTransactionType == TransactionBuy)
	{
		// the user is buying, don't change the selection
		return;
	}
	
	mRegion = region;
	mParcel = parcel;

	updateAgentInfo();
	updateParcelInfo();
	updateCovenantInfo();
	if (mCanBuy)
	{
		updateWebSiteInfo();
	}
	refreshUI();
}

void LLFloaterBuyLandUI::setForGroup(bool forGroup)
{
	mIsForGroup = forGroup;
}

void LLFloaterBuyLandUI::draw()
{
	LLFloater::draw();
	
	bool needsUpdate = false;
	needsUpdate |= checkTransaction();
	needsUpdate |= mCurrency.process();
	
	if (mBought)
	{
		close();
	}
	else if (needsUpdate)
	{
		if (mCanBuy && mCurrency.hasError())
		{
			tellUserError(mCurrency.errorMessage(), mCurrency.errorURI());
		}
		
		refreshUI();
	}
}

BOOL LLFloaterBuyLandUI::canClose()
{
	return (mTransaction ? FALSE : TRUE) && mCurrency.canCancel();
}

void LLFloaterBuyLandUI::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
	delete this;
}


void LLFloaterBuyLandUI::refreshUI()
{
	// section zero: title area
	{
		LLTextureCtrl* snapshot = LLViewerUICtrlFactory::getTexturePickerByName(this, "info_image");
		if (snapshot)
		{
			snapshot->setImageAssetID(
				mParcelValid ? mParcelSnapshot : LLUUID::null);
		}
		
		
			
		if (mParcelValid)
		{
			childSetText("info_parcel", mParcelLocation);

			
			childSetTextArg("meters_supports_object", "[AMOUNT]",  llformat("%d", mParcelActualArea));
			childSetTextArg("meters_supports_object", "[AMOUNT2]",  llformat("%d", mParcelSupportedObjects));
			
			childSetText("info_size", childGetText("meters_supports_object"));


			childSetText("info_price",
				llformat(
					"L$ %d%s",
					mParcelPrice,
					mParcelSoldWithObjects
						? "\nsold with objects"
						: ""));
			childSetVisible("info_price", mParcelIsForSale);
		}
		else
		{
			childSetText("info_parcel", "(no parcel selected)");
			childSetText("info_size", "");
			childSetText("info_price", "");
		}
		
		childSetText("info_action",
			mCanBuy
				?
					mIsForGroup
						? childGetText("buying_for_group")//"Buying land for group:"
						: childGetText("buying_will")//"Buying this land will:"
				: 
					mCannotBuyIsError
						? childGetText("cannot_buy_now")//"Cannot buy now:"
						: childGetText("not_for_sale")//"Not for sale:"

			);
	}
	
	bool showingError = !mCanBuy || !mSiteValid;
	
	// error section
	if (showingError)
	{
		mChildren.setBadge("step_error",
			mCannotBuyIsError
				? LLViewChildren::BADGE_ERROR
				: LLViewChildren::BADGE_WARN);
		
		LLTextBox* message = LLUICtrlFactory::getTextBoxByName(this, "error_message");
		if (message)
		{
			message->setVisible(true);
			message->setWrappedText(
				!mCanBuy ? mCannotBuyReason : "(waiting for data)"
				);
		}

		childSetVisible("error_web", 
				mCannotBuyIsError && !mCannotBuyURI.empty());
	}
	else
	{
		childHide("step_error");
		childHide("error_message");
		childHide("error_web");
	}
	
	
	// section one: account
	if (!showingError)
	{
		mChildren.setBadge("step_1",
			mSiteMembershipUpgrade
				? LLViewChildren::BADGE_NOTE
				: LLViewChildren::BADGE_OK);
		childSetText("account_action", mSiteMembershipAction);
		childSetText("account_reason", 
			mSiteMembershipUpgrade
				?	childGetText("must_upgrade")
				:	childGetText("cant_own_land")
			);
		
		LLComboBox* levels = LLUICtrlFactory::getComboBoxByName(this, "account_level");
		if (levels)
		{
			levels->setVisible(mSiteMembershipUpgrade);
			
			levels->removeall();
			for(std::vector<std::string>::const_iterator i
					= mSiteMembershipPlanNames.begin();
				i != mSiteMembershipPlanNames.end();
				++i)
			{
				levels->add(*i);
			}
			
			levels->setCurrentByIndex(mUserPlanChoice);
		}

		childShow("step_1");
		childShow("account_action");
		childShow("account_reason");
	}
	else
	{
		childHide("step_1");
		childHide("account_action");
		childHide("account_reason");
		childHide("account_level");
	}
	
	// section two: land use fees
	if (!showingError)
	{
		mChildren.setBadge("step_2",
			mSiteLandUseUpgrade
				? LLViewChildren::BADGE_NOTE
				: LLViewChildren::BADGE_OK);
		childSetText("land_use_action", mSiteLandUseAction);
		
		std::string message;
		
		if (mIsForGroup)
		{
			childSetTextArg("insufficient_land_credits", "[GROUP]", gAgent.mGroupName);


			message += childGetText("insufficient_land_credits");
				
		}
		else if (mAgentHasNeverOwnedLand)
		{
			if (mParcelIsFirstLand)
			{
				message += childGetText("first_purchase");
			}
			else
			{
				message += childGetText("first_time_but_not_first_land");
			}
		}
		else
		{
		
		childSetTextArg("land_holdings", "[BUYER]", llformat("%d", mAgentCommittedTier));
		message += childGetText("land_holdings");

		}
		
		if (!mParcelValid)
		{
			message += "(no parcel selected)";
		}
		else if (mParcelBillableArea == mParcelActualArea)
		{
			childSetTextArg("parcel_meters", "[AMOUNT]", llformat("%d", mParcelActualArea));
			message += childGetText("parcel_meters");
		}
		else
		{

			if (mParcelBillableArea > mParcelActualArea)
			{	
				childSetTextArg("premium_land", "[AMOUNT]", llformat("%d", mParcelBillableArea) );
				message += childGetText("premium_land");
			}
			else
			{
				childSetTextArg("discounted_land", "[AMOUNT]", llformat("%d", mParcelBillableArea) );
				message += childGetText("discounted_land");
			}
		}

		childSetWrappedText("land_use_reason", message);

		childShow("step_2");
		childShow("land_use_action");
		childShow("land_use_reason");
	}
	else
	{
		childHide("step_2");
		childHide("land_use_action");
		childHide("land_use_reason");
	}
	
	// section three: purchase & currency
	S32 finalBalance = mAgentCashBalance + mCurrency.getAmount() - mParcelPrice;
	bool willHaveEnough = finalBalance >= 0;
	bool haveEnough = mAgentCashBalance >= mParcelPrice;
	S32 minContribution = llceil((F32)mParcelBillableArea / GROUP_LAND_BONUS_FACTOR);
	bool groupContributionEnough = mParcelGroupContribution >= minContribution;
	
	mCurrency.updateUI(!showingError  &&  !haveEnough);

	if (!showingError)
	{
		mChildren.setBadge("step_3",
			!willHaveEnough
				? LLViewChildren::BADGE_WARN
				: mCurrency.getAmount() > 0
					? LLViewChildren::BADGE_NOTE
					: LLViewChildren::BADGE_OK);
			
		childSetText("purchase_action",
			llformat(
				"Pay L$ %d to %s for this land",
				mParcelPrice,
				mParcelSellerName.c_str()
				));
		childSetVisible("purchase_action", mParcelValid);
		
		std::string reasonString;

		if (haveEnough)
		{
			
			childSetTextArg("have_enough_lindens", "[AMOUNT]", llformat("%d", mAgentCashBalance));
			childSetText("currency_reason", childGetText("have_enough_lindens"));
		}
		else
		{
			childSetTextArg("not_enough_lindens", "[AMOUNT]", llformat("%d", mAgentCashBalance));
			childSetTextArg("not_enough_lindens", "[AMOUNT2]", llformat("%d", mParcelPrice - mAgentCashBalance));
			
			childSetText("currency_reason", childGetText("not_enough_lindens"));

			childSetTextArg("currency_est", "[AMOUNT2]", llformat("%#.2f", mCurrency.getEstimate() / 100.0));
			
		}
		
		if (willHaveEnough)
		{

			childSetTextArg("balance_left", "[AMOUNT]", llformat("%d", finalBalance));
			
			childSetText("currency_balance", childGetText("balance_left"));

		}
		else
		{
	
			childSetTextArg("balance_needed", "[AMOUNT]", llformat("%d", mParcelPrice - mAgentCashBalance));
		
			childSetText("currency_balance", childGetText("balance_needed"));
			
		}

		//remove_contribution not in XML - ?!
		childSetValue("remove_contribution", LLSD(groupContributionEnough));
		childSetEnabled("remove_contribution", groupContributionEnough);
		bool showRemoveContribution = mParcelIsGroupLand
							&& (mParcelGroupContribution > 0);
		childSetText("remove_contribution",
			llformat("Remove %d square meters of contribution from group",
				minContribution));
		childSetVisible("remove_contribution", showRemoveContribution);

		childShow("step_3");
		childShow("purchase_action");
		childShow("currency_reason");
		childShow("currency_balance");
	}
	else
	{
		childHide("step_3");
		childHide("purchase_action");
		childHide("currency_reason");
		childHide("currency_balance");
		childHide("remove_group_donation");
	}


	bool agrees_to_covenant = false;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)getChildByName("agree_covenant");
	if (check)
	{
	    agrees_to_covenant = check->get();
	}

	childSetEnabled("buy_btn",
		mCanBuy  &&  mSiteValid  &&  willHaveEnough  &&  !mTransaction && agrees_to_covenant);
}

void LLFloaterBuyLandUI::startBuyPreConfirm()
{
	std::string action;
	
	if (mSiteMembershipUpgrade)
	{
		action += mSiteMembershipAction;
		action += "\n";

		LLComboBox* levels = LLUICtrlFactory::getComboBoxByName(this, "account_level");
		if (levels)
		{
			action += " * ";
			action += mSiteMembershipPlanNames[levels->getCurrentIndex()];
			action += "\n";
		}
	}
	if (mSiteLandUseUpgrade)
	{
		action += mSiteLandUseAction;
		action += "\n";
	}
	if (mCurrency.getAmount() > 0)
	{
		
		childSetTextArg("buy_for_US", "[AMOUNT]", llformat("%d", mCurrency.getAmount()));
		childSetTextArg("buy_for_US", "[AMOUNT2]", llformat("%#.2f", mCurrency.getEstimate() / 100.0));

		action += childGetText("buy_for_US");
	}

	childSetTextArg("pay_to_for_land", "[AMOUNT]", llformat("%d", mParcelPrice));
	childSetTextArg("pay_to_for_land", "[SELLER]", mParcelSellerName.c_str());

	action += childGetText("pay_to_for_land");
		
	
	LLConfirmationManager::confirm(mSiteConfirm,
		action,
		*this,
		&LLFloaterBuyLandUI::startBuyPostConfirm);
}

void LLFloaterBuyLandUI::startBuyPostConfirm(const std::string& password)
{
	runWebSitePrep(password);
	
	mCanBuy = false;
	mCannotBuyReason = childGetText("processing");
	refreshUI();
}


// static
void LLFloaterBuyLandUI::onClickBuy(void* data)
{
	LLFloaterBuyLandUI* self = (LLFloaterBuyLandUI*)data;
	self->startBuyPreConfirm();
}

// static
void LLFloaterBuyLandUI::onClickCancel(void* data)
{
	LLFloaterBuyLandUI* self = (LLFloaterBuyLandUI*)data;
	self->close();
}

// static
void LLFloaterBuyLandUI::onClickErrorWeb(void* data)
{
	LLFloaterBuyLandUI* self = (LLFloaterBuyLandUI*)data;
	LLWeb::loadURLExternal(self->mCannotBuyURI);
	self->close();
}

