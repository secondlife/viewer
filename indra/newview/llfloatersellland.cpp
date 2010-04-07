/** 
 * @file llfloatersellland.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersellland.h"

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
	static bool callbackHighlightTransferable(const LLSD& notification, const LLSD& response);

	void callbackAvatarPick(const std::vector<std::string>& names, const uuid_vec_t& ids);

public:
	virtual BOOL postBuild();
	
	bool setParcel(LLViewerRegion* region, LLParcelSelectionHandle parcel);
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
	childSetPrevalidate("price", LLTextValidate::validateNonNegativeS32);
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
		childSetValue("price", mParcelPrice);
		if (mParcelSoldWithObjects)
		{
			childSetValue("sell_objects", "yes");
		}
		else
		{
			childSetValue("sell_objects", "no");
		}
	}
	else
	{
		childSetValue("price", "");
		childSetValue("sell_objects", "none");
	}

	mParcelSnapshot = parcelp->getSnapshotID();

	mAuthorizedBuyer = parcelp->getAuthorizedBuyerID();
	mSellToBuyer = mAuthorizedBuyer.notNull();

	if(mSellToBuyer)
	{
		std::string name;
		gCacheName->getFullName(mAuthorizedBuyer, name);
		childSetText("sell_to_agent", name);
	}
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
	
	childSetValue(id, badgeName);
}

void LLFloaterSellLandUI::refreshUI()
{
	LLParcel* parcelp = mParcelSelection->getParcel();
	if (!parcelp) return;

	LLTextureCtrl* snapshot = getChild<LLTextureCtrl>("info_image");
	snapshot->setImageAssetID(mParcelSnapshot);

	childSetText("info_parcel", parcelp->getName());
	childSetTextArg("info_size", "[AREA]", llformat("%d", mParcelActualArea));

	std::string price_str = childGetValue("price").asString();
	bool valid_price = false;
	valid_price = (price_str != "") && LLTextValidate::validateNonNegativeS32(utf8str_to_wstring(price_str));

	if (valid_price && mParcelActualArea > 0)
	{
		F32 per_meter_price = 0;
		per_meter_price = F32(mParcelPrice) / F32(mParcelActualArea);
		childSetTextArg("price_per_m", "[PER_METER]", llformat("%0.2f", per_meter_price));
		childShow("price_per_m");

		setBadge("step_price", BADGE_OK);
	}
	else
	{
		childHide("price_per_m");

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
		childSetValue("sell_to", "user");
		childShow("sell_to_agent");
		childShow("sell_to_select_agent");
	}
	else
	{
		if (mChoseSellTo)
		{
			childSetValue("sell_to", "anyone");
		}
		else
		{
			childSetValue("sell_to", "select");
		}
		childHide("sell_to_agent");
		childHide("sell_to_select_agent");
	}

	// Must select Sell To: Anybody, or User (with a specified username)
	std::string sell_to = childGetValue("sell_to").asString();
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

	bool valid_sell_objects = ("none" != childGetValue("sell_objects").asString());

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
		childEnable("sell_btn");
	}
	else
	{
		childDisable("sell_btn");
	}
}

// static
void LLFloaterSellLandUI::onChangeValue(LLUICtrl *ctrl, void *userdata)
{
	LLFloaterSellLandUI *self = (LLFloaterSellLandUI *)userdata;

	std::string sell_to = self->childGetValue("sell_to").asString();

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

	self->mParcelPrice = self->childGetValue("price");

	if ("yes" == self->childGetValue("sell_objects").asString())
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
	// grandparent is a floater, in order to set up dependency
	addDependentFloater(LLFloaterAvatarPicker::show(boost::bind(&LLFloaterSellLandUI::callbackAvatarPick, this, _1, _2), FALSE, TRUE));
}

void LLFloaterSellLandUI::callbackAvatarPick(const std::vector<std::string>& names, const uuid_vec_t& ids)
{	
	LLParcel* parcel = mParcelSelection->getParcel();

	if (names.empty() || ids.empty()) return;
	
	LLUUID id = ids[0];
	parcel->setAuthorizedBuyerID(id);

	mAuthorizedBuyer = ids[0];

	childSetText("sell_to_agent", names[0]);

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

	LLNotificationsUtil::add("TransferObjectsHighlighted",
						LLSD(), LLSD(),
						&LLFloaterSellLandUI::callbackHighlightTransferable);
}

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
	S32 sale_price = self->childGetValue("price");
	S32 area = parcel->getArea();
	std::string authorizedBuyerName = "Anyone";
	bool sell_to_anyone = true;
	if ("user" == self->childGetValue("sell_to").asString())
	{
		authorizedBuyerName = self->childGetText("sell_to_agent");
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
	S32  sale_price	= childGetValue("price");

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
	if ("yes" == childGetValue("sell_objects").asString())
	{
		sell_with_objects = true;
	}
	parcel->setSellWithObjects(sell_with_objects);
	if ("user" == childGetValue("sell_to").asString())
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
