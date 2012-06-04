/** 
 * @file llfloatersellland.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llfloatersellland.h"

#include "llavatarnamecache.h"
#include "llfloateravatarpicker.h"
#include "llfloaterreg.h"
#include "llfloaterland.h"
#include "lllineeditor.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llselectmgr.h"
#include "lltexturectrl.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "lltrans.h"

class LLAvatarName;

// defined in llfloaterland.cpp
void send_parcel_select_objects(S32 parcel_local_id, U32 return_type,
								uuid_list_t* return_ids = NULL);

enum Badge { BADGE_OK, BADGE_NOTE, BADGE_WARN, BADGE_ERROR };

class LLFloaterSellLandUI
:	public LLFloater
{
public:
	LLFloaterSellLandUI(const LLSD& key);
	virtual ~LLFloaterSellLandUI();
	/*virtual*/ void onClose(bool app_quitting);
	
private:
	class SelectionObserver : public LLParcelObserver
	{
	public:
		SelectionObserver(LLFloaterSellLandUI* floater) : mFloater(floater) {}
		virtual void changed();
	private:
		LLFloaterSellLandUI* mFloater;
	};
	
private:
	LLViewerRegion*	mRegion;
	LLParcelSelectionHandle	mParcelSelection;
	bool					mParcelIsForSale;
	bool					mSellToBuyer;
	bool					mChoseSellTo;
	S32						mParcelPrice;
	S32						mParcelActualArea;
	LLUUID					mParcelSnapshot;
	LLUUID					mAuthorizedBuyer;
	bool					mParcelSoldWithObjects;
	SelectionObserver 		mParcelSelectionObserver;
	
	void updateParcelInfo();
	void refreshUI();
	void setBadge(const char* id, Badge badge);

	static void onChangeValue(LLUICtrl *ctrl, void *userdata);
	void doSelectAgent();
	static void doCancel(void *userdata);
	static void doSellLand(void *userdata);
	bool onConfirmSale(const LLSD& notification, const LLSD& response);
	static void doShowObjects(void *userdata);

	void callbackAvatarPick(const uuid_vec_t& ids, const std::vector<LLAvatarName> names);

	void onBuyerNameCache(const LLAvatarName& av_name);

public:
	virtual BOOL postBuild();
	
	bool setParcel(LLViewerRegion* region, LLParcelSelectionHandle parcel);
	static bool callbackHighlightTransferable(const LLSD& notification, const LLSD& response);
};

// static
void LLFloaterSellLand::sellLand(
	LLViewerRegion* region, LLParcelSelectionHandle parcel)
{
	LLFloaterSellLandUI* ui = LLFloaterReg::showTypedInstance<LLFloaterSellLandUI>("sell_land");
	ui->setParcel(region, parcel);
}

// static
LLFloater* LLFloaterSellLand::buildFloater(const LLSD& key)
{
	LLFloaterSellLandUI* floater = new LLFloaterSellLandUI(key);
	return floater;
}

#if LL_WINDOWS
// passing 'this' during construction generates a warning. The callee
// only uses the pointer to hold a reference to 'this' which is
// already valid, so this call does the correct thing. Disable the
// warning so that we can compile without generating a warning.
#pragma warning(disable : 4355)
#endif 
LLFloaterSellLandUI::LLFloaterSellLandUI(const LLSD& key)
:	LLFloater(key),
	mParcelSelectionObserver(this),
	mRegion(0)
{
	LLViewerParcelMgr::getInstance()->addObserver(&mParcelSelectionObserver);
}

LLFloaterSellLandUI::~LLFloaterSellLandUI()
{
	LLViewerParcelMgr::getInstance()->removeObserver(&mParcelSelectionObserver);
}

// Because we are single_instance, we are not destroyed on close.
void LLFloaterSellLandUI::onClose(bool app_quitting)
{
	// Must release parcel selection to allow land to deselect, see EXT-803
	mParcelSelection = NULL;
}

void LLFloaterSellLandUI::SelectionObserver::changed()
{
	if (LLViewerParcelMgr::getInstance()->selectionEmpty())
	{
		mFloater->closeFloater();
	}
	else if (mFloater->getVisible()) // only update selection if sell land ui in use
	{
		mFloater->setParcel(LLViewerParcelMgr::getInstance()->getSelectionRegion(),
							LLViewerParcelMgr::getInstance()->getParcelSelection());
	}
}

BOOL LLFloaterSellLandUI::postBuild()
{
	childSetCommitCallback("sell_to", onChangeValue, this);
	childSetCommitCallback("price", onChangeValue, this);
	getChild<LLLineEditor>("price")->setPrevalidate(LLTextValidate::validateNonNegativeS32);
	childSetCommitCallback("sell_objects", onChangeValue, this);
	childSetAction("sell_to_select_agent", boost::bind( &LLFloaterSellLandUI::doSelectAgent, this));
	childSetAction("cancel_btn", doCancel, this);
	childSetAction("sell_btn", doSellLand, this);
	childSetAction("show_objects", doShowObjects, this);
	center();
	getChild<LLUICtrl>("profile_scroll")->setTabStop(true);
	return TRUE;
}

bool LLFloaterSellLandUI::setParcel(LLViewerRegion* region, LLParcelSelectionHandle parcel)
{
	if (!parcel->getParcel())
	{
		return false;
	}

	mRegion = region;
	mParcelSelection = parcel;
	mChoseSellTo = false;


	updateParcelInfo();
	refreshUI();

	return true;
}

void LLFloaterSellLandUI::updateParcelInfo()
{
	LLParcel* parcelp = mParcelSelection->getParcel();
	if (!parcelp) return;

	mParcelActualArea = parcelp->getArea();
	mParcelIsForSale = parcelp->getForSale();
	if (mParcelIsForSale)
	{
		mChoseSellTo = true;
	}
	mParcelPrice = mParcelIsForSale ? parcelp->getSalePrice() : 0;
	mParcelSoldWithObjects = parcelp->getSellWithObjects();
	if (mParcelIsForSale)
	{
		getChild<LLUICtrl>("price")->setValue(mParcelPrice);
		if (mParcelSoldWithObjects)
		{
			getChild<LLUICtrl>("sell_objects")->setValue("yes");
		}
		else
		{
			getChild<LLUICtrl>("sell_objects")->setValue("no");
		}
	}
	else
	{
		getChild<LLUICtrl>("price")->setValue("");
		getChild<LLUICtrl>("sell_objects")->setValue("none");
	}

	mParcelSnapshot = parcelp->getSnapshotID();

	mAuthorizedBuyer = parcelp->getAuthorizedBuyerID();
	mSellToBuyer = mAuthorizedBuyer.notNull();

	if(mSellToBuyer)
	{
		LLAvatarNameCache::get(mAuthorizedBuyer, 
			boost::bind(&LLFloaterSellLandUI::onBuyerNameCache, this, _2));
	}
}

void LLFloaterSellLandUI::onBuyerNameCache(const LLAvatarName& av_name)
{
	getChild<LLUICtrl>("sell_to_agent")->setValue(av_name.getCompleteName());
	getChild<LLUICtrl>("sell_to_agent")->setToolTip(av_name.mUsername);
}

void LLFloaterSellLandUI::setBadge(const char* id, Badge badge)
{
	static std::string badgeOK("badge_ok.j2c");
	static std::string badgeNote("badge_note.j2c");
	static std::string badgeWarn("badge_warn.j2c");
	static std::string badgeError("badge_error.j2c");
	
	std::string badgeName;
	switch (badge)
	{
		default:
		case BADGE_OK:		badgeName = badgeOK;	break;
		case BADGE_NOTE:	badgeName = badgeNote;	break;
		case BADGE_WARN:	badgeName = badgeWarn;	break;
		case BADGE_ERROR:	badgeName = badgeError;	break;
	}
	
	getChild<LLUICtrl>(id)->setValue(badgeName);
}

void LLFloaterSellLandUI::refreshUI()
{
	LLParcel* parcelp = mParcelSelection->getParcel();
	if (!parcelp) return;

	LLTextureCtrl* snapshot = getChild<LLTextureCtrl>("info_image");
	snapshot->setImageAssetID(mParcelSnapshot);

	getChild<LLUICtrl>("info_parcel")->setValue(parcelp->getName());
	getChild<LLUICtrl>("info_size")->setTextArg("[AREA]", llformat("%d", mParcelActualArea));

	std::string price_str = getChild<LLUICtrl>("price")->getValue().asString();
	bool valid_price = false;
	valid_price = (price_str != "") && LLTextValidate::validateNonNegativeS32(utf8str_to_wstring(price_str));

	if (valid_price && mParcelActualArea > 0)
	{
		F32 per_meter_price = 0;
		per_meter_price = F32(mParcelPrice) / F32(mParcelActualArea);
		getChild<LLUICtrl>("price_per_m")->setTextArg("[PER_METER]", llformat("%0.2f", per_meter_price));
		getChildView("price_per_m")->setVisible(TRUE);

		setBadge("step_price", BADGE_OK);
	}
	else
	{
		getChildView("price_per_m")->setVisible(FALSE);

		if ("" == price_str)
		{
			setBadge("step_price", BADGE_NOTE);
		}
		else
		{
			setBadge("step_price", BADGE_ERROR);
		}
	}

	if (mSellToBuyer)
	{
		getChild<LLUICtrl>("sell_to")->setValue("user");
		getChildView("sell_to_agent")->setVisible(TRUE);
		getChildView("sell_to_select_agent")->setVisible(TRUE);
	}
	else
	{
		if (mChoseSellTo)
		{
			getChild<LLUICtrl>("sell_to")->setValue("anyone");
		}
		else
		{
			getChild<LLUICtrl>("sell_to")->setValue("select");
		}
		getChildView("sell_to_agent")->setVisible(FALSE);
		getChildView("sell_to_select_agent")->setVisible(FALSE);
	}

	// Must select Sell To: Anybody, or User (with a specified username)
	std::string sell_to = getChild<LLUICtrl>("sell_to")->getValue().asString();
	bool valid_sell_to = "select" != sell_to &&
		("user" != sell_to || mAuthorizedBuyer.notNull());

	if (!valid_sell_to)
	{
		setBadge("step_sell_to", BADGE_NOTE);
	}
	else
	{
		setBadge("step_sell_to", BADGE_OK);
	}

	bool valid_sell_objects = ("none" != getChild<LLUICtrl>("sell_objects")->getValue().asString());

	if (!valid_sell_objects)
	{
		setBadge("step_sell_objects", BADGE_NOTE);
	}
	else
	{
		setBadge("step_sell_objects", BADGE_OK);
	}

	if (valid_sell_to && valid_price && valid_sell_objects)
	{
		getChildView("sell_btn")->setEnabled(TRUE);
	}
	else
	{
		getChildView("sell_btn")->setEnabled(FALSE);
	}
}

// static
void LLFloaterSellLandUI::onChangeValue(LLUICtrl *ctrl, void *userdata)
{
	LLFloaterSellLandUI *self = (LLFloaterSellLandUI *)userdata;

	std::string sell_to = self->getChild<LLUICtrl>("sell_to")->getValue().asString();

	if (sell_to == "user")
	{
		self->mChoseSellTo = true;
		self->mSellToBuyer = true;
		if (self->mAuthorizedBuyer.isNull())
		{
			self->doSelectAgent();
		}
	}
	else if (sell_to == "anyone")
	{
		self->mChoseSellTo = true;
		self->mSellToBuyer = false;
	}

	self->mParcelPrice = self->getChild<LLUICtrl>("price")->getValue();

	if ("yes" == self->getChild<LLUICtrl>("sell_objects")->getValue().asString())
	{
		self->mParcelSoldWithObjects = true;
	}
	else
	{
		self->mParcelSoldWithObjects = false;
	}

	self->refreshUI();
}

void LLFloaterSellLandUI::doSelectAgent()
{
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterSellLandUI::callbackAvatarPick, this, _1, _2), FALSE, TRUE);
	// grandparent is a floater, in order to set up dependency
	if (picker)
	{
		addDependentFloater(picker);
	}
}

void LLFloaterSellLandUI::callbackAvatarPick(const uuid_vec_t& ids, const std::vector<LLAvatarName> names)
{	
	LLParcel* parcel = mParcelSelection->getParcel();

	if (names.empty() || ids.empty()) return;
	
	LLUUID id = ids[0];
	parcel->setAuthorizedBuyerID(id);

	mAuthorizedBuyer = ids[0];

	getChild<LLUICtrl>("sell_to_agent")->setValue(names[0].getCompleteName());

	refreshUI();
}

// static
void LLFloaterSellLandUI::doCancel(void *userdata)
{
	LLFloaterSellLandUI* self = (LLFloaterSellLandUI*)userdata;
	self->closeFloater();
}

// static
void LLFloaterSellLandUI::doShowObjects(void *userdata)
{
	LLFloaterSellLandUI* self = (LLFloaterSellLandUI*)userdata;
	LLParcel* parcel = self->mParcelSelection->getParcel();
	if (!parcel) return;

	send_parcel_select_objects(parcel->getLocalID(), RT_SELL);

	// we shouldn't pass callback functor since it is registered in LLFunctorRegistration
	LLNotificationsUtil::add("TransferObjectsHighlighted",
						LLSD(), LLSD());
}

static LLNotificationFunctorRegistration tr("TransferObjectsHighlighted", &LLFloaterSellLandUI::callbackHighlightTransferable);

// static
bool LLFloaterSellLandUI::callbackHighlightTransferable(const LLSD& notification, const LLSD& data)
{
	LLSelectMgr::getInstance()->unhighlightAll();
	return false;
}

// static
void LLFloaterSellLandUI::doSellLand(void *userdata)
{
	LLFloaterSellLandUI* self = (LLFloaterSellLandUI*)userdata;

	LLParcel* parcel = self->mParcelSelection->getParcel();

	// Do a confirmation
	S32 sale_price = self->getChild<LLUICtrl>("price")->getValue();
	S32 area = parcel->getArea();
	std::string authorizedBuyerName = LLTrans::getString("Anyone");
	bool sell_to_anyone = true;
	if ("user" == self->getChild<LLUICtrl>("sell_to")->getValue().asString())
	{
		authorizedBuyerName = self->getChild<LLUICtrl>("sell_to_agent")->getValue().asString();
		sell_to_anyone = false;
	}

	// must sell to someone if indicating sale to anyone
	if (!parcel->getForSale() 
		&& (sale_price == 0) 
		&& sell_to_anyone)
	{
		LLNotificationsUtil::add("SalePriceRestriction");
		return;
	}

	LLSD args;
	args["LAND_SIZE"] = llformat("%d",area);
	args["SALE_PRICE"] = llformat("%d",sale_price);
	args["NAME"] = authorizedBuyerName;

	LLNotification::Params params("ConfirmLandSaleChange");
	params.substitutions(args)
		.functor.function(boost::bind(&LLFloaterSellLandUI::onConfirmSale, self, _1, _2));

	if (sell_to_anyone)
	{
		params.name("ConfirmLandSaleToAnyoneChange");
	}
	
	if (parcel->getForSale())
	{
		// parcel already for sale, so ignore this question
		LLNotifications::instance().forceResponse(params, -1);
	}
	else
	{
		// ask away
		LLNotifications::instance().add(params);
	}

}

bool LLFloaterSellLandUI::onConfirmSale(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)
	{
		return false;
	}
	S32  sale_price	= getChild<LLUICtrl>("price")->getValue();

	// Valid extracted data
	if (sale_price < 0)
	{
		// TomY TODO: Throw an error
		return false;
	}

	LLParcel* parcel = mParcelSelection->getParcel();
	if (!parcel) return false;

	// can_agent_modify_parcel deprecated by GROUPS
// 	if (!can_agent_modify_parcel(parcel))
// 	{
// 		close();
// 		return;
// 	}

	parcel->setParcelFlag(PF_FOR_SALE, TRUE);
	parcel->setSalePrice(sale_price);
	bool sell_with_objects = false;
	if ("yes" == getChild<LLUICtrl>("sell_objects")->getValue().asString())
	{
		sell_with_objects = true;
	}
	parcel->setSellWithObjects(sell_with_objects);
	if ("user" == getChild<LLUICtrl>("sell_to")->getValue().asString())
	{
		parcel->setAuthorizedBuyerID(mAuthorizedBuyer);
	}
	else
	{
		parcel->setAuthorizedBuyerID(LLUUID::null);
	}

	// Send update to server
	LLViewerParcelMgr::getInstance()->sendParcelPropertiesUpdate( parcel );

	closeFloater();
	return false;
}
