/** 
 * @file llfloatersellland.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersellland.h"

#include "llfloateravatarpicker.h"
#include "llfloater.h"
#include "llfloaterland.h"
#include "lllineeditor.h"
#include "llnotify.h"
#include "llparcel.h"
#include "llselectmgr.h"
#include "lltexturectrl.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"

// defined in llfloaterland.cpp
void send_parcel_select_objects(S32 parcel_local_id, S32 return_type,
								uuid_list_t* return_ids = NULL);

enum Badge { BADGE_OK, BADGE_NOTE, BADGE_WARN, BADGE_ERROR };

class LLFloaterSellLandUI
:	public LLFloater
{
private:
	LLFloaterSellLandUI();
	virtual ~LLFloaterSellLandUI();

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
	
	void updateParcelInfo();
	void refreshUI();
	void setBadge(const char* id, Badge badge);

	static LLFloaterSellLandUI* sInstance;

	static void onChangeValue(LLUICtrl *ctrl, void *userdata);
	static void doSelectAgent(void *userdata);
	static void doCancel(void *userdata);
	static void doSellLand(void *userdata);
	static void onConfirmSale(S32 option, void *userdata);
	static void doShowObjects(void *userdata);
	static void callbackHighlightTransferable(S32 option, void* userdata);

	static void callbackAvatarPick(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data);

public:
	virtual BOOL postBuild();
	virtual void onClose(bool app_quitting);
	
	static LLFloaterSellLandUI* soleInstance(bool createIfNeeded);

	bool setParcel(LLViewerRegion* region, LLParcelSelectionHandle parcel);

private:
	class SelectionObserver : public LLParcelObserver
	{
	public:
		virtual void changed();
	};
};

// static
void LLFloaterSellLand::sellLand(
	LLViewerRegion* region, LLParcelSelectionHandle parcel)
{
	LLFloaterSellLandUI* ui = LLFloaterSellLandUI::soleInstance(true);
	if (ui->setParcel(region, parcel))
	{
		ui->open();		/* Flawfinder: ignore */
	}
}

// static
LLFloaterSellLandUI* LLFloaterSellLandUI::sInstance = NULL;

// static
LLFloaterSellLandUI* LLFloaterSellLandUI::soleInstance(bool createIfNeeded)
{
	if (!sInstance  &&  createIfNeeded)
	{
		sInstance = new LLFloaterSellLandUI();

		gUICtrlFactory->buildFloater(sInstance, "floater_sell_land.xml");
		sInstance->center();
	}
	

	static SelectionObserver* parcelSelectionObserver = NULL;
	if (!parcelSelectionObserver)
	{
		parcelSelectionObserver = new SelectionObserver;
		gParcelMgr->addObserver(parcelSelectionObserver);
	}

	return sInstance;
}

LLFloaterSellLandUI::LLFloaterSellLandUI()
:	LLFloater("Sell Land"),
	mRegion(0)
{
}

LLFloaterSellLandUI::~LLFloaterSellLandUI()
{
	if (sInstance == this)
	{
		sInstance = NULL;
	}
}

void LLFloaterSellLandUI::SelectionObserver::changed()
{
	LLFloaterSellLandUI* ui = LLFloaterSellLandUI::soleInstance(false);
	if (ui)
	{
		if (gParcelMgr->selectionEmpty())
		{
			ui->close();
		}
		else {
			ui->setParcel(
				gParcelMgr->getSelectionRegion(),
				gParcelMgr->getParcelSelection());
		}
	}
}

void LLFloaterSellLandUI::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
	destroy();
}

BOOL LLFloaterSellLandUI::postBuild()
{
	childSetCommitCallback("sell_to", onChangeValue, this);
	childSetCommitCallback("price", onChangeValue, this);
	childSetPrevalidate("price", LLLineEditor::prevalidateNonNegativeS32);
	childSetCommitCallback("sell_objects", onChangeValue, this);
	childSetAction("sell_to_select_agent", doSelectAgent, this);
	childSetAction("cancel_btn", doCancel, this);
	childSetAction("sell_btn", doSellLand, this);
	childSetAction("show_objects", doShowObjects, this);
	return TRUE;
}

bool LLFloaterSellLandUI::setParcel(LLViewerRegion* region, LLParcelSelectionHandle parcel)
{
	if (!parcel->getParcel()) // || !can_agent_modify_parcel(parcel)) // can_agent_modify_parcel was deprecated by GROUPS
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
		LLString name;
		char firstname[MAX_STRING];		/* Flawfinder: ignore */
		char lastname[MAX_STRING];		/* Flawfinder: ignore */
		gCacheName->getName(mAuthorizedBuyer, firstname, lastname);
		name.assign(firstname);
		name.append(" ");
		name.append(lastname);

		childSetText("sell_to_agent", name);
	}
}

void LLFloaterSellLandUI::setBadge(const char* id, Badge badge)
{
	static LLUUID badgeOK(gViewerArt.getString("badge_ok.tga"));
	static LLUUID badgeNote(gViewerArt.getString("badge_note.tga"));
	static LLUUID badgeWarn(gViewerArt.getString("badge_warn.tga"));
	static LLUUID badgeError(gViewerArt.getString("badge_error.tga"));
	
	LLUUID badgeUUID;
	switch (badge)
	{
		default:
		case BADGE_OK:		badgeUUID = badgeOK;	break;
		case BADGE_NOTE:	badgeUUID = badgeNote;	break;
		case BADGE_WARN:	badgeUUID = badgeWarn;	break;
		case BADGE_ERROR:	badgeUUID = badgeError;	break;
	}
	
	childSetValue(id, badgeUUID);
}

void LLFloaterSellLandUI::refreshUI()
{
	LLParcel* parcelp = mParcelSelection->getParcel();
	if (!parcelp) return;

	LLTextureCtrl* snapshot = LLViewerUICtrlFactory::getTexturePickerByName(this, "info_image");
	if (snapshot)
	{
		snapshot->setImageAssetID(mParcelSnapshot);
	}

	childSetText("info_parcel", parcelp->getName());
	childSetTextArg("info_size", "[AREA]", llformat("%d", mParcelActualArea));

	LLString price_str = childGetValue("price").asString();
	bool valid_price = false;
	valid_price = (price_str != "") && LLLineEditor::prevalidateNonNegativeS32(utf8str_to_wstring(price_str));

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
	LLString sell_to = childGetValue("sell_to").asString();
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

	LLString sell_to = self->childGetValue("sell_to").asString();

	if (sell_to == "user")
	{
		self->mChoseSellTo = true;
		self->mSellToBuyer = true;
		if (self->mAuthorizedBuyer.isNull())
		{
			doSelectAgent(self);
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

// static
void LLFloaterSellLandUI::doSelectAgent(void *userdata)
{
	LLFloaterSellLandUI* floaterp = (LLFloaterSellLandUI*)userdata;
	// grandparent is a floater, in order to set up dependency
	floaterp->addDependentFloater(LLFloaterAvatarPicker::show(callbackAvatarPick, floaterp, FALSE, TRUE));
}

// static
void LLFloaterSellLandUI::callbackAvatarPick(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data)
{	
	LLFloaterSellLandUI* floaterp = (LLFloaterSellLandUI*)data;
	LLParcel* parcel = floaterp->mParcelSelection->getParcel();

	if (names.empty() || ids.empty()) return;
	
	LLUUID id = ids[0];
	parcel->setAuthorizedBuyerID(id);

	floaterp->mAuthorizedBuyer = ids[0];

	floaterp->childSetText("sell_to_agent", names[0]);

	floaterp->refreshUI();
}

// static
void LLFloaterSellLandUI::doCancel(void *userdata)
{
	LLFloaterSellLandUI* self = (LLFloaterSellLandUI*)userdata;
	self->close();
}

// static
void LLFloaterSellLandUI::doShowObjects(void *userdata)
{
	LLFloaterSellLandUI* self = (LLFloaterSellLandUI*)userdata;
	LLParcel* parcel = self->mParcelSelection->getParcel();
	if (!parcel) return;

	send_parcel_select_objects(parcel->getLocalID(), RT_SELL);

	LLNotifyBox::showXml("TransferObjectsHighlighted",
						 callbackHighlightTransferable,
						 userdata);
}

// static
void LLFloaterSellLandUI::callbackHighlightTransferable(S32 option, void* userdata)
{
	gSelectMgr->unhighlightAll();
}

// static
void LLFloaterSellLandUI::doSellLand(void *userdata)
{
	LLFloaterSellLandUI* self = (LLFloaterSellLandUI*)userdata;

	LLParcel* parcel = self->mParcelSelection->getParcel();

	// Do a confirmation
	if (!parcel->getForSale())
	{
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
		if ((sale_price == 0) && sell_to_anyone)
		{
			gViewerWindow->alertXml("SalePriceRestriction");
			return;
		}

		LLStringBase<char>::format_map_t args;
		args["[LAND_SIZE]"] = llformat("%d",area);
		args["[SALE_PRICE]"] = llformat("%d",sale_price);
		args["[NAME]"] = authorizedBuyerName;

		gViewerWindow->alertXml("ConfirmLandSaleChange", args, onConfirmSale, self);
	}
	else
	{
		onConfirmSale(-1, self);
	}
}

// static
void LLFloaterSellLandUI::onConfirmSale(S32 option, void *userdata)
{
	if (option != 0)
	{
		return;
	}
	LLFloaterSellLandUI* self = (LLFloaterSellLandUI*)userdata;
	S32  sale_price	= self->childGetValue("price");

	// Valid extracted data
	if (sale_price < 0)
	{
		// TomY TODO: Throw an error
		return;
	}

	LLParcel* parcel = self->mParcelSelection->getParcel();
	if (!parcel) return;

	// can_agent_modify_parcel deprecated by GROUPS
// 	if (!can_agent_modify_parcel(parcel))
// 	{
// 		self->close();
// 		return;
// 	}

	parcel->setParcelFlag(PF_FOR_SALE, TRUE);
	parcel->setSalePrice(sale_price);
	bool sell_with_objects = false;
	if ("yes" == self->childGetValue("sell_objects").asString())
	{
		sell_with_objects = true;
	}
	parcel->setSellWithObjects(sell_with_objects);
	if ("user" == self->childGetValue("sell_to").asString())
	{
		parcel->setAuthorizedBuyerID(self->mAuthorizedBuyer);
	}
	else
	{
		parcel->setAuthorizedBuyerID(LLUUID::null);
	}

	// Send update to server
	gParcelMgr->sendParcelPropertiesUpdate( parcel );

	self->close();
}
