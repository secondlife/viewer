/** 
 * @file llfloaterbuyland.cpp
 * @brief LLFloaterBuyLand class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
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
#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llframetimer.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llslurl.h"
#include "llstatusbar.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewchildren.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llweb.h"
#include "llwindow.h"
#include "llworld.h"
#include "llxmlrpctransaction.h"
#include "llviewernetwork.h"
#include "roles_constants.h"

// NOTE: This is duplicated in lldatamoney.cpp ...
const F32 GROUP_LAND_BONUS_FACTOR = 1.1f;
const F64 CURRENCY_ESTIMATE_FREQUENCY = 0.5;
	// how long of a pause in typing a currency buy amount before an
	// estimate is fetched from the server

class LLFloaterBuyLandUI
:	public LLFloater
{
public:
	LLFloaterBuyLandUI(const LLSD& key);
	virtual ~LLFloaterBuyLandUI();
	
	/*virtual*/ void onClose(bool app_quitting);

	// Left padding for maturity rating icon.
	static const S32 ICON_PAD = 2;

private:
	class SelectionObserver : public LLParcelObserver
	{
	public:
		SelectionObserver(LLFloaterBuyLandUI* floater) : mFloater(floater) {}
		virtual void changed();
	private:
		LLFloaterBuyLandUI* mFloater;
	};
	
private:
	SelectionObserver mParcelSelectionObserver;
	LLViewerRegion*	mRegion;
	LLParcelSelectionHandle mParcel;
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
	
public:
	void setForGroup(bool is_for_group);
	void setParcel(LLViewerRegion* region, LLParcelSelectionHandle parcel);
		
	void updateAgentInfo();
	void updateParcelInfo();
	void updateCovenantInfo();
	static void onChangeAgreeCovenant(LLUICtrl* ctrl, void* user_data);
	void updateFloaterCovenantText(const std::string& string, const LLUUID &asset_id);
	void updateFloaterEstateName(const std::string& name);
	void updateFloaterLastModified(const std::string& text);
	void updateFloaterEstateOwnerName(const std::string& name);
	void updateWebSiteInfo();
	void finishWebSiteInfo();
	
	void runWebSitePrep(const std::string& password);
	void finishWebSitePrep();
	void sendBuyLand();

	void updateNames();
	// Name cache callback
	void updateGroupName(const LLUUID& id,
						 const std::string& name,
						 bool is_group);
	
	void refreshUI();
	
	void startTransaction(TransactionType type, const LLXMLRPCValue& params);
	bool checkTransaction();
	
	void tellUserError(const std::string& message, const std::string& uri);

	virtual BOOL postBuild();
	
	void startBuyPreConfirm();
	void startBuyPostConfirm(const std::string& password);

	void onClickBuy();
	void onClickCancel();
	 void onClickErrorWeb();
	
	virtual void draw();
	virtual BOOL canClose();

	void onVisibilityChange ( const LLSD& new_visibility );
	
};

// static
void LLFloaterBuyLand::buyLand(
	LLViewerRegion* region, LLParcelSelectionHandle parcel, bool is_for_group)
{
	if(is_for_group && !gAgent.hasPowerInActiveGroup(GP_LAND_DEED))
	{
		LLNotificationsUtil::add("OnlyOfficerCanBuyLand");
		return;
	}

	LLFloaterBuyLandUI* ui = LLFloaterReg::showTypedInstance<LLFloaterBuyLandUI>("buy_land");
	if (ui)
	{
		ui->setForGroup(is_for_group);
		ui->setParcel(region, parcel);
	}
}

// static
void LLFloaterBuyLand::updateCovenantText(const std::string& string, const LLUUID &asset_id)
{
	LLFloaterBuyLandUI* floater = LLFloaterReg::findTypedInstance<LLFloaterBuyLandUI>("buy_land");
	if (floater)
	{
		floater->updateFloaterCovenantText(string, asset_id);
	}
}

// static
void LLFloaterBuyLand::updateEstateName(const std::string& name)
{
	LLFloaterBuyLandUI* floater = LLFloaterReg::findTypedInstance<LLFloaterBuyLandUI>("buy_land");
	if (floater)
	{
		floater->updateFloaterEstateName(name);
	}
}

// static
void LLFloaterBuyLand::updateLastModified(const std::string& text)
{
	LLFloaterBuyLandUI* floater = LLFloaterReg::findTypedInstance<LLFloaterBuyLandUI>("buy_land");
	if (floater)
	{
		floater->updateFloaterLastModified(text);
	}
}

// static
void LLFloaterBuyLand::updateEstateOwnerName(const std::string& name)
{
	LLFloaterBuyLandUI* floater = LLFloaterReg::findTypedInstance<LLFloaterBuyLandUI>("buy_land");
	if (floater)
	{
		floater->updateFloaterEstateOwnerName(name);
	}
}

// static
LLFloater* LLFloaterBuyLand::buildFloater(const LLSD& key)
{
	LLFloaterBuyLandUI* floater = new LLFloaterBuyLandUI(key);
	return floater;
}

//----------------------------------------------------------------------------
// LLFloaterBuyLandUI
//----------------------------------------------------------------------------

#if LL_WINDOWS
// passing 'this' during construction generates a warning. The callee
// only uses the pointer to hold a reference to 'this' which is
// already valid, so this call does the correct thing. Disable the
// warning so that we can compile without generating a warning.
#pragma warning(disable : 4355)
#endif 
LLFloaterBuyLandUI::LLFloaterBuyLandUI(const LLSD& key)
:	LLFloater(LLSD()),
	mParcelSelectionObserver(this),
	mParcel(0),
	mBought(false),
	mParcelValid(false), mSiteValid(false),
	mChildren(*this), mCurrency(*this), mTransaction(0),
	mParcelBuyInfo(0)
{
	LLViewerParcelMgr::getInstance()->addObserver(&mParcelSelectionObserver);
	
}

LLFloaterBuyLandUI::~LLFloaterBuyLandUI()
{
	LLViewerParcelMgr::getInstance()->removeObserver(&mParcelSelectionObserver);
	LLViewerParcelMgr::getInstance()->deleteParcelBuy(&mParcelBuyInfo);
	
	delete mTransaction;
}

// virtual
void LLFloaterBuyLandUI::onClose(bool app_quitting)
{
	// This object holds onto observer, transactions, and parcel state.
	// Despite being single_instance, destroy it to call destructors and clean
	// everything up.
	setVisible(FALSE);
	destroy();
}

void LLFloaterBuyLandUI::SelectionObserver::changed()
{
	if (LLViewerParcelMgr::getInstance()->selectionEmpty())
	{
		mFloater->closeFloater();
	}
	else
	{
		mFloater->setParcel(LLViewerParcelMgr::getInstance()->getSelectionRegion(),
							LLViewerParcelMgr::getInstance()->getParcelSelection());
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
	LLParcel* parcel = mParcel->getParcel();
	mParcelValid = parcel && mRegion;
	mParcelIsForSale = false;
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
		mCannotBuyReason = getString("no_land_selected");
		return;
	}
	
	if (mParcel->getMultipleOwners())
	{
		mCannotBuyReason = getString("multiple_parcels_selected");
		return;
	}

	const LLUUID& parcelOwner = parcel->getOwnerID();
	
	mIsClaim = parcel->isPublic();
	if (!mIsClaim)
	{
		mParcelActualArea = parcel->getArea();
		mParcelIsForSale = parcel->getForSale();
		mParcelIsGroupLand = parcel->getIsGroupOwned();
		mParcelPrice = mParcelIsForSale ? parcel->getSalePrice() : 0;
		
		if (mParcelIsGroupLand)
		{
			LLUUID group_id = parcel->getGroupID();
			mParcelGroupContribution = gAgent.getGroupContribution(group_id);
		}
	}
	else
	{
		mParcelActualArea = mParcel->getClaimableArea();
		mParcelIsForSale = true;
		mParcelPrice = mParcelActualArea * parcel->getClaimPricePerMeter();
	}

	mParcelBillableArea =
		llround(mRegion->getBillableFactor() * mParcelActualArea);

 	mParcelSupportedObjects = llround(
		parcel->getMaxPrimCapacity() * parcel->getParcelPrimBonus()); 
 	// Can't have more than region max tasks, regardless of parcel 
 	// object bonus factor. 
 	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion(); 
 	if(region) 
 	{ 
		S32 max_tasks_per_region = (S32)region->getMaxTasks(); 
		mParcelSupportedObjects = llmin(
			mParcelSupportedObjects, max_tasks_per_region); 
 	} 

	mParcelSoldWithObjects = parcel->getSellWithObjects();

	
	LLVector3 center = parcel->getCenterpoint();
	mParcelLocation = llformat("%s %d,%d",
				mRegion->getName().c_str(),
				(int)center[VX], (int)center[VY]
				);
	
	mParcelSnapshot = parcel->getSnapshotID();
	
	updateNames();
	
	bool haveEnoughCash = mParcelPrice <= mAgentCashBalance;
	S32 cashBuy = haveEnoughCash ? 0 : (mParcelPrice - mAgentCashBalance);
	mCurrency.setAmount(cashBuy, true);
	mCurrency.setZeroMessage(haveEnoughCash ? getString("none_needed") : LLStringUtil::null);

	// checks that we can buy the land

	if(mIsForGroup && !gAgent.hasPowerInActiveGroup(GP_LAND_DEED))
	{
		mCannotBuyReason = getString("cant_buy_for_group");
		return;
	}

	if (!mIsClaim)
	{
		const LLUUID& authorizedBuyer = parcel->getAuthorizedBuyerID();
		const LLUUID buyer = gAgent.getID();
		const LLUUID newOwner = mIsForGroup ? gAgent.getGroupID() : buyer;

		if (!mParcelIsForSale
			|| (mParcelPrice == 0  &&  authorizedBuyer.isNull()))
		{
			
			mCannotBuyReason = getString("parcel_not_for_sale");
			return;
		}

		if (parcelOwner == newOwner)
		{
			if (mIsForGroup)
			{
				mCannotBuyReason = getString("group_already_owns");
			}
			else
			{
				mCannotBuyReason = getString("you_already_own");
			}
			return;
		}

		if (!authorizedBuyer.isNull() && buyer != authorizedBuyer)
		{
			// Maybe the parcel is set for sale to a group we are in.
			bool authorized_group =
				gAgent.hasPowerInGroup(authorizedBuyer,GP_LAND_DEED)
				&& gAgent.hasPowerInGroup(authorizedBuyer,GP_LAND_SET_SALE_INFO);

			if (!authorized_group)
			{
				mCannotBuyReason = getString("set_to_sell_to_other");
				return;
			}
		}
	}
	else
	{
		if (mParcelActualArea == 0)
		{
			mCannotBuyReason = getString("no_public_land");
			return;
		}

		if (mParcel->hasOthersSelected())
		{
			// Policy: Must not have someone else's land selected
			mCannotBuyReason = getString("not_owned_by_you");
			return;
		}
	}

	mCanBuy = true;
}

void LLFloaterBuyLandUI::updateCovenantInfo()
{
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if(!region) return;

	U8 sim_access = region->getSimAccess();
	std::string rating = LLViewerRegion::accessToString(sim_access);
	
	LLTextBox* region_name = getChild<LLTextBox>("region_name_text");
	if (region_name)
	{
		std::string region_name_txt = region->getName() + " ("+rating +")";
		region_name->setText(region_name_txt);

		LLIconCtrl* rating_icon = getChild<LLIconCtrl>("rating_icon");
		LLRect rect = rating_icon->getRect();
		S32 region_name_width = llmin(region_name->getRect().getWidth(), region_name->getTextBoundingRect().getWidth());
		S32 icon_left_pad = region_name->getRect().mLeft + region_name_width + ICON_PAD;
		region_name->setToolTip(region_name->getText());
		rating_icon->setRect(rect.setOriginAndSize(icon_left_pad, rect.mBottom, rect.getWidth(), rect.getHeight()));

		switch(sim_access)
		{
		case SIM_ACCESS_PG:
			rating_icon->setValue(getString("icon_PG"));
			break;

		case SIM_ACCESS_ADULT:
			rating_icon->setValue(getString("icon_R"));
			break;

		default:
			rating_icon->setValue(getString("icon_M"));
		}
	}

	LLTextBox* region_type = getChild<LLTextBox>("region_type_text");
	if (region_type)
	{
		region_type->setText(region->getLocalizedSimProductName());
		region_type->setToolTip(region->getLocalizedSimProductName());
	}
	
	LLTextBox* resellable_clause = getChild<LLTextBox>("resellable_clause");
	if (resellable_clause)
	{
		if (region->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
		{
			resellable_clause->setText(getString("can_not_resell"));
		}
		else
		{
			resellable_clause->setText(getString("can_resell"));
		}
	}
	
	LLTextBox* changeable_clause = getChild<LLTextBox>("changeable_clause");
	if (changeable_clause)
	{
		if (region->getRegionFlags() & REGION_FLAGS_ALLOW_PARCEL_CHANGES)
		{
			changeable_clause->setText(getString("can_change"));
		}
		else
		{
			changeable_clause->setText(getString("can_not_change"));
		}
	}

	LLCheckBoxCtrl* check = getChild<LLCheckBoxCtrl>("agree_covenant");
	if(check)
	{
		check->set(false);
		check->setEnabled(true);
		check->setCommitCallback(onChangeAgreeCovenant, this);
	}

	LLTextBox* box = getChild<LLTextBox>("covenant_text");
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

void LLFloaterBuyLandUI::updateFloaterCovenantText(const std::string &string, const LLUUID& asset_id)
{
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("covenant_editor");
	editor->setText(string);

	LLCheckBoxCtrl* check = getChild<LLCheckBoxCtrl>("agree_covenant");
	LLTextBox* box = getChild<LLTextBox>("covenant_text");
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

void LLFloaterBuyLandUI::updateFloaterEstateName(const std::string& name)
{
	LLTextBox* box = getChild<LLTextBox>("estate_name_text");
	box->setText(name);
	box->setToolTip(name);
}

void LLFloaterBuyLandUI::updateFloaterLastModified(const std::string& text)
{
	LLTextBox* editor = getChild<LLTextBox>("covenant_timestamp_text");
	if (editor) editor->setText(text);
}

void LLFloaterBuyLandUI::updateFloaterEstateOwnerName(const std::string& name)
{
	LLTextBox* box = getChild<LLTextBox>("estate_owner_text");
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
	keywordArgs.appendString("language", LLUI::getLanguage());
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
	if (currency["estimatedCost"].isValid())
	{
		mCurrency.setUSDEstimate(currency["estimatedCost"].asInt());
	}
	if (currency["estimatedLocalCost"].isValid())
	{
		mCurrency.setLocalEstimate(currency["estimatedLocalCost"].asString());
	}

	mSiteConfirm = result["confirm"].asString();
}

void LLFloaterBuyLandUI::runWebSitePrep(const std::string& password)
{
	if (!mCanBuy)
	{
		return;
	}
	
	BOOL remove_contribution = getChild<LLUICtrl>("remove_contribution")->getValue().asBoolean();
	mParcelBuyInfo = LLViewerParcelMgr::getInstance()->setupParcelBuy(gAgent.getID(), gAgent.getSessionID(),
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
		LLComboBox* levels = getChild<LLComboBox>( "account_level");
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
	keywordArgs.appendString("language", LLUI::getLanguage());
	keywordArgs.appendString("levelId", newLevel);
	keywordArgs.appendInt("billableArea",
		mIsForGroup ? 0 : mParcelBillableArea);
	keywordArgs.appendInt("currencyBuy", mCurrency.getAmount());
	keywordArgs.appendInt("estimatedCost", mCurrency.getUSDEstimate());
	keywordArgs.appendString("estimatedLocalCost", mCurrency.getLocalEstimate());
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
		LLViewerParcelMgr::getInstance()->sendParcelBuy(mParcelBuyInfo);
		LLViewerParcelMgr::getInstance()->deleteParcelBuy(&mParcelBuyInfo);
		mBought = true;
	}
}

void LLFloaterBuyLandUI::updateNames()
{
	LLParcel* parcelp = mParcel->getParcel();

	if (!parcelp)
	{
		mParcelSellerName = LLStringUtil::null;
		return;
	}
	
	if (mIsClaim)
	{
		mParcelSellerName = "Linden Lab";
	}
	else if (parcelp->getIsGroupOwned())
	{
		gCacheName->getGroup(parcelp->getGroupID(),
			boost::bind(&LLFloaterBuyLandUI::updateGroupName, this,
				_1, _2, _3));
	}
	else
	{
		mParcelSellerName = LLSLURL("agent", parcelp->getOwnerID(), "completename").getSLURLString();
	}
}

void LLFloaterBuyLandUI::updateGroupName(const LLUUID& id,
						 const std::string& name,
						 bool is_group)
{
	LLParcel* parcelp = mParcel->getParcel();
	if (parcelp
		&& parcelp->getGroupID() == id)
	{
		// request is current
		mParcelSellerName = name;
	}
}

void LLFloaterBuyLandUI::startTransaction(TransactionType type, const LLXMLRPCValue& params)
{
	delete mTransaction;
	mTransaction = NULL;

	mTransactionType = type;

	// Select a URI and method appropriate for the transaction type.
	static std::string transaction_uri;
	if (transaction_uri.empty())
	{
		transaction_uri = LLGridManager::getInstance()->getHelperURI() + "landtool.php";
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
	mCannotBuyReason = getString("fetching_error");
	mCannotBuyReason += message;
	mCannotBuyURI = uri;
}


// virtual
BOOL LLFloaterBuyLandUI::postBuild()
{
	setVisibleCallback(boost::bind(&LLFloaterBuyLandUI::onVisibilityChange, this, _2));
	
	mCurrency.prepare();
	
	getChild<LLUICtrl>("buy_btn")->setCommitCallback( boost::bind(&LLFloaterBuyLandUI::onClickBuy, this));
	getChild<LLUICtrl>("cancel_btn")->setCommitCallback( boost::bind(&LLFloaterBuyLandUI::onClickCancel, this));
	getChild<LLUICtrl>("error_web")->setCommitCallback( boost::bind(&LLFloaterBuyLandUI::onClickErrorWeb, this));

	center();
	
	return TRUE;
}

void LLFloaterBuyLandUI::setParcel(LLViewerRegion* region, LLParcelSelectionHandle parcel)
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
		closeFloater();
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

// virtual
BOOL LLFloaterBuyLandUI::canClose()
{
	bool can_close = (mTransaction ? FALSE : TRUE) && mCurrency.canCancel();
	if (!can_close)
	{
		// explain to user why they can't do this, see DEV-9605
		LLNotificationsUtil::add("CannotCloseFloaterBuyLand");
	}
	return can_close;
}

void LLFloaterBuyLandUI::onVisibilityChange ( const LLSD& new_visibility )
{
	if (new_visibility.asBoolean())
	{
		refreshUI();
	}
}

void LLFloaterBuyLandUI::refreshUI()
{
	// section zero: title area
	{
		LLTextureCtrl* snapshot = getChild<LLTextureCtrl>("info_image");
		if (snapshot)
		{
			snapshot->setImageAssetID(
				mParcelValid ? mParcelSnapshot : LLUUID::null);
		}
		
		if (mParcelValid)
		{
			getChild<LLUICtrl>("info_parcel")->setValue(mParcelLocation);

			LLStringUtil::format_map_t string_args;
			string_args["[AMOUNT]"] = llformat("%d", mParcelActualArea);
			string_args["[AMOUNT2]"] = llformat("%d", mParcelSupportedObjects);
		
			getChild<LLUICtrl>("info_size")->setValue(getString("meters_supports_object", string_args));

			F32 cost_per_sqm = 0.0f;
			if (mParcelActualArea > 0)
			{
				cost_per_sqm = (F32)mParcelPrice / (F32)mParcelActualArea;
			}

			LLStringUtil::format_map_t info_price_args;
			info_price_args["[PRICE]"] = llformat("%d", mParcelPrice);
			info_price_args["[PRICE_PER_SQM]"] = llformat("%.1f", cost_per_sqm);
			if (mParcelSoldWithObjects)
			{
				info_price_args["[SOLD_WITH_OBJECTS]"] = getString("sold_with_objects");
			}
			else
			{
				info_price_args["[SOLD_WITH_OBJECTS]"] = getString("sold_without_objects");
			}
			getChild<LLUICtrl>("info_price")->setValue(getString("info_price_string", info_price_args));
			getChildView("info_price")->setVisible( mParcelIsForSale);
		}
		else
		{
			getChild<LLUICtrl>("info_parcel")->setValue(getString("no_parcel_selected"));
			getChild<LLUICtrl>("info_size")->setValue(LLStringUtil::null);
			getChild<LLUICtrl>("info_price")->setValue(LLStringUtil::null);
		}
		
		getChild<LLUICtrl>("info_action")->setValue(
			mCanBuy
				?
					mIsForGroup
						? getString("buying_for_group")//"Buying land for group:"
						: getString("buying_will")//"Buying this land will:"
				: 
					mCannotBuyIsError
						? getString("cannot_buy_now")//"Cannot buy now:"
						: getString("not_for_sale")//"Not for sale:"

			);
	}
	
	bool showingError = !mCanBuy || !mSiteValid;
	
	// error section
	if (showingError)
	{
		mChildren.setBadge(std::string("step_error"),
			mCannotBuyIsError
				? LLViewChildren::BADGE_ERROR
				: LLViewChildren::BADGE_WARN);
		
		LLTextBox* message = getChild<LLTextBox>("error_message");
		if (message)
		{
			message->setVisible(true);
			message->setValue(LLSD(!mCanBuy ? mCannotBuyReason : "(waiting for data)"));
		}

		getChildView("error_web")->setVisible(mCannotBuyIsError && !mCannotBuyURI.empty());
	}
	else
	{
		getChildView("step_error")->setVisible(FALSE);
		getChildView("error_message")->setVisible(FALSE);
		getChildView("error_web")->setVisible(FALSE);
	}
	
	
	// section one: account
	if (!showingError)
	{
		mChildren.setBadge(std::string("step_1"),
			mSiteMembershipUpgrade
				? LLViewChildren::BADGE_NOTE
				: LLViewChildren::BADGE_OK);
		getChild<LLUICtrl>("account_action")->setValue(mSiteMembershipAction);
		getChild<LLUICtrl>("account_reason")->setValue( 
			mSiteMembershipUpgrade
				?	getString("must_upgrade")
				:	getString("cant_own_land")
			);
		
		LLComboBox* levels = getChild<LLComboBox>( "account_level");
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

		getChildView("step_1")->setVisible(TRUE);
		getChildView("account_action")->setVisible(TRUE);
		getChildView("account_reason")->setVisible(TRUE);
	}
	else
	{
		getChildView("step_1")->setVisible(FALSE);
		getChildView("account_action")->setVisible(FALSE);
		getChildView("account_reason")->setVisible(FALSE);
		getChildView("account_level")->setVisible(FALSE);
	}
	
	// section two: land use fees
	if (!showingError)
	{
		mChildren.setBadge(std::string("step_2"),
			mSiteLandUseUpgrade
				? LLViewChildren::BADGE_NOTE
				: LLViewChildren::BADGE_OK);
		getChild<LLUICtrl>("land_use_action")->setValue(mSiteLandUseAction);
		
		std::string message;
		
		if (mIsForGroup)
		{
			LLStringUtil::format_map_t string_args;
			string_args["[GROUP]"] = std::string(gAgent.getGroupName());

			message += getString("insufficient_land_credits", string_args);
				
		}
		else
		{
			LLStringUtil::format_map_t string_args;
			string_args["[BUYER]"] = llformat("%d", mAgentCommittedTier);
			message += getString("land_holdings", string_args);
		}
		
		if (!mParcelValid)
		{
			message += LLTrans::getString("sentences_separator") + getString("no_parcel_selected");
		}
		else if (mParcelBillableArea == mParcelActualArea)
		{
			LLStringUtil::format_map_t string_args;
			string_args["[AMOUNT]"] = llformat("%d ", mParcelActualArea);
			message += LLTrans::getString("sentences_separator") + getString("parcel_meters", string_args);
		}
		else
		{

			if (mParcelBillableArea > mParcelActualArea)
			{	
				LLStringUtil::format_map_t string_args;
				string_args["[AMOUNT]"] = llformat("%d ", mParcelBillableArea);
				message += LLTrans::getString("sentences_separator") + getString("premium_land", string_args);
			}
			else
			{
				LLStringUtil::format_map_t string_args;
				string_args["[AMOUNT]"] = llformat("%d ", mParcelBillableArea);
				message += LLTrans::getString("sentences_separator") + getString("discounted_land", string_args);
			}
		}

		getChild<LLUICtrl>("land_use_reason")->setValue(message);

		getChildView("step_2")->setVisible(TRUE);
		getChildView("land_use_action")->setVisible(TRUE);
		getChildView("land_use_reason")->setVisible(TRUE);
	}
	else
	{
		getChildView("step_2")->setVisible(FALSE);
		getChildView("land_use_action")->setVisible(FALSE);
		getChildView("land_use_reason")->setVisible(FALSE);
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
		mChildren.setBadge(std::string("step_3"),
			!willHaveEnough
				? LLViewChildren::BADGE_WARN
				: mCurrency.getAmount() > 0
					? LLViewChildren::BADGE_NOTE
					: LLViewChildren::BADGE_OK);
			
		LLStringUtil::format_map_t string_args;
		string_args["[AMOUNT]"] = llformat("%d", mParcelPrice);
		string_args["[SELLER]"] = mParcelSellerName;
		getChild<LLUICtrl>("purchase_action")->setValue(getString("pay_to_for_land", string_args));
		getChildView("purchase_action")->setVisible( mParcelValid);
		
		std::string reasonString;

		if (haveEnough)
		{
			LLStringUtil::format_map_t string_args;
			string_args["[AMOUNT]"] = llformat("%d", mAgentCashBalance);

			getChild<LLUICtrl>("currency_reason")->setValue(getString("have_enough_lindens", string_args));
		}
		else
		{
			LLStringUtil::format_map_t string_args;
			string_args["[AMOUNT]"] = llformat("%d", mAgentCashBalance);
			string_args["[AMOUNT2]"] = llformat("%d", mParcelPrice - mAgentCashBalance);
			
			getChild<LLUICtrl>("currency_reason")->setValue(getString("not_enough_lindens", string_args));

			getChild<LLUICtrl>("currency_est")->setTextArg("[LOCAL_AMOUNT]", mCurrency.getLocalEstimate());
		}
		
		if (willHaveEnough)
		{
			LLStringUtil::format_map_t string_args;
			string_args["[AMOUNT]"] = llformat("%d", finalBalance);

			getChild<LLUICtrl>("currency_balance")->setValue(getString("balance_left", string_args));

		}
		else
		{
			LLStringUtil::format_map_t string_args;
			string_args["[AMOUNT]"] = llformat("%d", mParcelPrice - mAgentCashBalance);
	
			getChild<LLUICtrl>("currency_balance")->setValue(getString("balance_needed", string_args));
			
		}

		getChild<LLUICtrl>("remove_contribution")->setValue(LLSD(groupContributionEnough));
		getChildView("remove_contribution")->setEnabled(groupContributionEnough);
		bool showRemoveContribution = mParcelIsGroupLand
							&& (mParcelGroupContribution > 0);
		getChildView("remove_contribution")->setLabelArg("[AMOUNT]",
							llformat("%d", minContribution));
		getChildView("remove_contribution")->setVisible( showRemoveContribution);

		getChildView("step_3")->setVisible(TRUE);
		getChildView("purchase_action")->setVisible(TRUE);
		getChildView("currency_reason")->setVisible(TRUE);
		getChildView("currency_balance")->setVisible(TRUE);
	}
	else
	{
		getChildView("step_3")->setVisible(FALSE);
		getChildView("purchase_action")->setVisible(FALSE);
		getChildView("currency_reason")->setVisible(FALSE);
		getChildView("currency_balance")->setVisible(FALSE);
		getChildView("remove_group_donation")->setVisible(FALSE);
	}


	bool agrees_to_covenant = false;
	LLCheckBoxCtrl* check = getChild<LLCheckBoxCtrl>("agree_covenant");
	if (check)
	{
	    agrees_to_covenant = check->get();
	}

	getChildView("buy_btn")->setEnabled(mCanBuy  &&  mSiteValid  &&  willHaveEnough  &&  !mTransaction && agrees_to_covenant);
}

void LLFloaterBuyLandUI::startBuyPreConfirm()
{
	std::string action;
	
	if (mSiteMembershipUpgrade)
	{
		action += mSiteMembershipAction;
		action += "\n";

		LLComboBox* levels = getChild<LLComboBox>( "account_level");
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
		LLStringUtil::format_map_t string_args;
		string_args["[AMOUNT]"] = llformat("%d", mCurrency.getAmount());
		string_args["[LOCAL_AMOUNT]"] = mCurrency.getLocalEstimate();
		
		action += getString("buy_for_US", string_args);
	}

	LLStringUtil::format_map_t string_args;
	string_args["[AMOUNT]"] = llformat("%d", mParcelPrice);
	string_args["[SELLER]"] = mParcelSellerName;
	action += getString("pay_to_for_land", string_args);
		
	
	LLConfirmationManager::confirm(mSiteConfirm,
		action,
		*this,
		&LLFloaterBuyLandUI::startBuyPostConfirm);
}

void LLFloaterBuyLandUI::startBuyPostConfirm(const std::string& password)
{
	runWebSitePrep(password);
	
	mCanBuy = false;
	mCannotBuyReason = getString("processing");
	refreshUI();
}


void LLFloaterBuyLandUI::onClickBuy()
{
	startBuyPreConfirm();
}

void LLFloaterBuyLandUI::onClickCancel()
{
	closeFloater();
}

void LLFloaterBuyLandUI::onClickErrorWeb()
{
	LLWeb::loadURLExternal(mCannotBuyURI);
	closeFloater();
}



