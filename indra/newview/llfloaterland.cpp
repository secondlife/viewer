/** 
 * @file llfloaterland.cpp
 * @brief "About Land" floater, allowing display and editing of land parcel properties.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include <sstream>
#include <time.h>

#include "llfloaterland.h"

#include "llcachename.h"
#include "llfocusmgr.h"
#include "llparcel.h"
#include "message.h"

#include "llagent.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterauction.h"
#include "llfloateravatarinfo.h"
#include "llfloatergroups.h"
#include "llfloatergroupinfo.h"
#include "lllineeditor.h"
#include "llnamelistctrl.h"
#include "llnotify.h"
#include "llradiogroup.h"
#include "llscrolllistctrl.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lluiconstants.h"
#include "llvieweruictrlfactory.h"
#include "llviewermessage.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llmediaengine.h"
#include "llviewercontrol.h"
#include "roles_constants.h"

static const S32 EDIT_HEIGHT = 16;
static const S32 LEFT = HPAD;
static const S32 BOTTOM = VPAD;
static const S32 RULER0  = LEFT;
static const S32 RULER05 = RULER0 + 24;
static const S32 RULER1  = RULER05 + 16;
static const S32 RULER15 = RULER1 + 20;
static const S32 RULER2  = RULER1 + 32;
static const S32 RULER205= RULER2 + 32;
static const S32 RULER20 = RULER2 + 64;
static const S32 RULER21 = RULER20 + 16;
static const S32 RULER22 = RULER21 + 32;
static const S32 RULER225 = RULER20 + 64;
static const S32 RULER23 = RULER22 + 64;
static const S32 RULER24 = RULER23 + 26;
static const S32 RULER3  = RULER2 + 102;
static const S32 RULER4  = RULER3 + 8;
static const S32 RULER5  = RULER4 + 50;
static const S32 RULER6  = RULER5 + 52;
static const S32 RULER7  = RULER6 + 24;
static const S32 RIGHT  = LEFT + 278;
static const S32 FAR_RIGHT  = LEFT + 324 + 40;

static const char PRICE[] = "Price:";
static const char NO_PRICE[] = "";
static const char AREA[] = "Area:";

static const char OWNER_ONLINE[] 	= "0";
static const char OWNER_OFFLINE[]	= "1";
static const char OWNER_GROUP[] 	= "2";

static const char NEED_TIER_TO_MODIFY_STRING[] = "You must approve your purchase to modify this land.";

static const char WEB_PAGE[] = "Web page";
static const char QUICKTIME_MOVIE[] = "QuickTime movie";
static const char RAW_HTML[] = "Raw HTML";

// constants used in callbacks below - syntactic sugar.
static const BOOL BUY_GROUP_LAND = TRUE;
static const BOOL BUY_PERSONAL_LAND = FALSE;

// Statics
LLFloaterLand* LLFloaterLand::sInstance = NULL;
LLParcelSelectionObserver* LLFloaterLand::sObserver = NULL;
S32 LLFloaterLand::sLastTab = 0;

LLViewHandle LLPanelLandGeneral::sBuyPassDialogHandle;

// Local classes
class LLParcelSelectionObserver : public LLParcelObserver
{
public:
	virtual void changed() { LLFloaterLand::refreshAll(); }
};

//---------------------------------------------------------------------------
// LLFloaterLand
//---------------------------------------------------------------------------

void send_parcel_select_objects(S32 parcel_local_id, S32 return_type,
								uuid_list_t* return_ids = NULL)
{
	LLMessageSystem *msg = gMessageSystem;

	LLViewerRegion* region = gParcelMgr->getSelectionRegion();
	if (!region) return;

	// Since new highlight will be coming in, drop any highlights
	// that exist right now.
	gSelectMgr->unhighlightAll();

	msg->newMessageFast(_PREHASH_ParcelSelectObjects);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_LocalID, parcel_local_id);
	msg->addS32Fast(_PREHASH_ReturnType, return_type);

	// Throw all return ids into the packet.
	// TODO: Check for too many ids.
	if (return_ids)
	{
		uuid_list_t::iterator end = return_ids->end();
		for (uuid_list_t::iterator it = return_ids->begin();
			 it != end;
			 ++it)
		{
			msg->nextBlockFast(_PREHASH_ReturnIDs);
			msg->addUUIDFast(_PREHASH_ReturnID, (*it));
		}
	}
	else
	{
		// Put in a null key so that the message is complete.
		msg->nextBlockFast(_PREHASH_ReturnIDs);
		msg->addUUIDFast(_PREHASH_ReturnID, LLUUID::null);
	}

	msg->sendReliable(region->getHost());
}


// static
void LLFloaterLand::show()
{
	if (!sInstance)
	{
		sInstance = new LLFloaterLand();

		// Select tab from last view
		sInstance->mTabLand->selectTab(sLastTab);

		sObserver = new LLParcelSelectionObserver();
		gParcelMgr->addObserver( sObserver );
	}

	sInstance->open();	/*Flawfinder: ignore*/

	// Done automatically when the selected parcel's properties arrive
	// (and hence we have the local id).
	// gParcelMgr->sendParcelAccessListRequest(AL_ACCESS | AL_BAN | AL_RENTER);

	// If we've already got the parcel data, fill the
	// floater with it.
	sInstance->mParcel = gParcelMgr->getFloatingParcelSelection();
	if (sInstance->mParcel->getParcel())
	{
		sInstance->refresh();
	}
}

//static
LLPanelLandObjects* LLFloaterLand::getCurrentPanelLandObjects()
{
	if (!sInstance)
	{
		return NULL;
	}

	return sInstance->mPanelObjects;
}

//static
LLPanelLandCovenant* LLFloaterLand::getCurrentPanelLandCovenant()
{
	if (!sInstance)
	{
		return NULL;
	}

	return sInstance->mPanelCovenant;
}

// static
void LLFloaterLand::refreshAll()
{
	if (sInstance)
	{
		sInstance->refresh();
	}
}

// virtual
void LLFloaterLand::onClose(bool app_quitting)
{
	gParcelMgr->removeObserver( sObserver );
	delete sObserver;
	sObserver = NULL;

	// Might have been showing owned objects
	gSelectMgr->unhighlightAll();

	// Save which panel we had open
	sLastTab = mTabLand->getCurrentPanelIndex();

	destroy();
}


LLFloaterLand::LLFloaterLand()
:	LLFloater("floaterland", "FloaterLandRect5", "About Land")
{


	std::map<LLString, LLCallbackMap> factory_map;
	factory_map["land_general_panel"] = LLCallbackMap(createPanelLandGeneral, this);

	
	factory_map["land_covenant_panel"] = LLCallbackMap(createPanelLandCovenant, this);
	factory_map["land_objects_panel"] = LLCallbackMap(createPanelLandObjects, this);
	factory_map["land_options_panel"] = LLCallbackMap(createPanelLandOptions, this);
	factory_map["land_media_panel"] =	LLCallbackMap(createPanelLandMedia, this);
	factory_map["land_access_panel"] =	LLCallbackMap(createPanelLandAccess, this);
	factory_map["land_ban_panel"] =		LLCallbackMap(createPanelLandBan, this);

	gUICtrlFactory->buildFloater(this, "floater_about_land.xml", &factory_map);


	LLTabContainerCommon* tab = LLUICtrlFactory::getTabContainerByName(this, "landtab");

	mTabLand = (LLTabContainer*) tab;


	if (tab)
	{
		tab->selectTab(sLastTab);
	}
}


// virtual
LLFloaterLand::~LLFloaterLand()
{
	sInstance = NULL;
}


// public
void LLFloaterLand::refresh()
{
	mPanelGeneral->refresh();
	mPanelObjects->refresh();
	mPanelOptions->refresh();
	mPanelMedia->refresh();
	mPanelAccess->refresh();
	mPanelBan->refresh();
}



void* LLFloaterLand::createPanelLandGeneral(void* data)
{
	LLFloaterLand* self = (LLFloaterLand*)data;
	self->mPanelGeneral = new LLPanelLandGeneral(self->mParcel);
	return self->mPanelGeneral;
}

// static


void* LLFloaterLand::createPanelLandCovenant(void* data)
{
	LLFloaterLand* self = (LLFloaterLand*)data;
	self->mPanelCovenant = new LLPanelLandCovenant(self->mParcel);
	return self->mPanelCovenant;
}


// static
void* LLFloaterLand::createPanelLandObjects(void* data)
{
	LLFloaterLand* self = (LLFloaterLand*)data;
	self->mPanelObjects = new LLPanelLandObjects(self->mParcel);
	return self->mPanelObjects;
}

// static
void* LLFloaterLand::createPanelLandOptions(void* data)
{
	LLFloaterLand* self = (LLFloaterLand*)data;
	self->mPanelOptions = new LLPanelLandOptions(self->mParcel);
	return self->mPanelOptions;
}

// static
void* LLFloaterLand::createPanelLandMedia(void* data)
{
	LLFloaterLand* self = (LLFloaterLand*)data;
	self->mPanelMedia = new LLPanelLandMedia(self->mParcel);
	return self->mPanelMedia;
}

// static
void* LLFloaterLand::createPanelLandAccess(void* data)
{
	LLFloaterLand* self = (LLFloaterLand*)data;
	self->mPanelAccess = new LLPanelLandAccess(self->mParcel);
	return self->mPanelAccess;
}

// static
void* LLFloaterLand::createPanelLandBan(void* data)
{
	LLFloaterLand* self = (LLFloaterLand*)data;
	self->mPanelBan = new LLPanelLandBan(self->mParcel);
	return self->mPanelBan;
}


//---------------------------------------------------------------------------
// LLPanelLandGeneral
//---------------------------------------------------------------------------


LLPanelLandGeneral::LLPanelLandGeneral(LLParcelSelectionHandle& parcel)
:	LLPanel("land_general_panel"),
	mParcel(parcel),
	mUncheckedSell(FALSE)
{
}

BOOL LLPanelLandGeneral::postBuild()
{

	mEditName = LLUICtrlFactory::getLineEditorByName(this, "Name");
	mEditName->setCommitCallback(onCommitAny);	
	childSetPrevalidate("Name", LLLineEditor::prevalidatePrintableNotPipe);
	childSetUserData("Name", this);


	mEditDesc = LLUICtrlFactory::getLineEditorByName(this, "Description");
	mEditDesc->setCommitCallback(onCommitAny);	
	childSetPrevalidate("Description", LLLineEditor::prevalidatePrintableNotPipe);
	childSetUserData("Description", this);

	
	mTextSalePending = LLUICtrlFactory::getTextBoxByName(this, "SalePending");
	mTextOwnerLabel = LLUICtrlFactory::getTextBoxByName(this, "Owner:");
	mTextOwner = LLUICtrlFactory::getTextBoxByName(this, "OwnerText");

	
	mBtnProfile = LLUICtrlFactory::getButtonByName(this, "Profile...");
	mBtnProfile->setClickedCallback(onClickProfile, this);

	
	mTextGroupLabel = LLUICtrlFactory::getTextBoxByName(this, "Group:");
	mTextGroup = LLUICtrlFactory::getTextBoxByName(this, "GroupText");

	
	mBtnSetGroup = LLUICtrlFactory::getButtonByName(this, "Set...");
	mBtnSetGroup->setClickedCallback(onClickSetGroup, this);

	

	mCheckDeedToGroup = LLUICtrlFactory::getCheckBoxByName(this, "check deed");
	childSetCommitCallback("check deed", onCommitAny, this);

	
	mBtnDeedToGroup = LLUICtrlFactory::getButtonByName(this, "Deed...");
	mBtnDeedToGroup->setClickedCallback(onClickDeed, this);

	
	mCheckContributeWithDeed = LLUICtrlFactory::getCheckBoxByName(this, "check contib");
	childSetCommitCallback("check contib", onCommitAny, this);

	
	
	mSaleInfoNotForSale = LLUICtrlFactory::getTextBoxByName(this, "Not for sale.");
	
	mSaleInfoForSale1 = LLUICtrlFactory::getTextBoxByName(this, "For Sale: Price L$[PRICE].");

	
	mBtnSellLand = LLUICtrlFactory::getButtonByName(this, "Sell Land...");
	mBtnSellLand->setClickedCallback(onClickSellLand, this);
	
	mSaleInfoForSale2 = LLUICtrlFactory::getTextBoxByName(this, "For sale to");
	
	mSaleInfoForSaleObjects = LLUICtrlFactory::getTextBoxByName(this, "Sell with landowners objects in parcel.");
	
	mSaleInfoForSaleNoObjects = LLUICtrlFactory::getTextBoxByName(this, "Selling with no objects in parcel.");

	
	mBtnStopSellLand = LLUICtrlFactory::getButtonByName(this, "Cancel Land Sale");
	mBtnStopSellLand->setClickedCallback(onClickStopSellLand, this);

	
	mTextClaimDateLabel = LLUICtrlFactory::getTextBoxByName(this, "Claimed:");
	mTextClaimDate = LLUICtrlFactory::getTextBoxByName(this, "DateClaimText");

	
	mTextPriceLabel = LLUICtrlFactory::getTextBoxByName(this, "PriceLabel");
	mTextPrice = LLUICtrlFactory::getTextBoxByName(this, "PriceText");

	
	mTextDwell = LLUICtrlFactory::getTextBoxByName(this, "DwellText");

	
	mBtnBuyLand = LLUICtrlFactory::getButtonByName(this, "Buy Land...");
	mBtnBuyLand->setClickedCallback(onClickBuyLand, (void*)&BUY_PERSONAL_LAND);
	
	mBtnBuyGroupLand = LLUICtrlFactory::getButtonByName(this, "Buy For Group...");
	mBtnBuyGroupLand->setClickedCallback(onClickBuyLand, (void*)&BUY_GROUP_LAND);
	
	
	mBtnBuyPass = LLUICtrlFactory::getButtonByName(this, "Buy Pass...");
	mBtnBuyPass->setClickedCallback(onClickBuyPass, this);

	mBtnReleaseLand = LLUICtrlFactory::getButtonByName(this, "Abandon Land...");
	mBtnReleaseLand->setClickedCallback(onClickRelease, NULL);

	mBtnReclaimLand = LLUICtrlFactory::getButtonByName(this, "Reclaim Land...");
	mBtnReclaimLand->setClickedCallback(onClickReclaim, NULL);
	
	mBtnStartAuction = LLUICtrlFactory::getButtonByName(this, "Linden Sale...");
	mBtnStartAuction->setClickedCallback(onClickStartAuction, NULL);

	return TRUE;
}


// virtual
LLPanelLandGeneral::~LLPanelLandGeneral()
{ }


// public
void LLPanelLandGeneral::refresh()
{
	mBtnStartAuction->setVisible(gAgent.isGodlike());

	LLParcel *parcel = mParcel->getParcel();
	bool region_owner = false;
	LLViewerRegion* regionp = gParcelMgr->getSelectionRegion();
	if(regionp && (regionp->getOwner() == gAgent.getID()))
	{
		region_owner = true;
		mBtnReleaseLand->setVisible(FALSE);
		mBtnReclaimLand->setVisible(TRUE);
	}
	else
	{
		mBtnReleaseLand->setVisible(TRUE);
		mBtnReclaimLand->setVisible(FALSE);
	}
	if (!parcel)
	{
		// nothing selected, disable panel
		mEditName->setEnabled(FALSE);
		mEditName->setText("");

		mEditDesc->setEnabled(FALSE);
		mEditDesc->setText("");

		mTextSalePending->setText("");
		mTextSalePending->setEnabled(FALSE);

		mBtnDeedToGroup->setEnabled(FALSE);
		mBtnSetGroup->setEnabled(FALSE);
		mBtnStartAuction->setEnabled(FALSE);

		mCheckDeedToGroup	->set(FALSE);
		mCheckDeedToGroup	->setEnabled(FALSE);
		mCheckContributeWithDeed->set(FALSE);
		mCheckContributeWithDeed->setEnabled(FALSE);

		mTextOwner->setText("");
		mBtnProfile->setLabelSelected("Profile...");
		mBtnProfile->setLabelUnselected("Profile...");
		mBtnProfile->setEnabled(FALSE);

		mTextClaimDate->setText("");
		mTextGroup->setText("");
		mTextPrice->setText("");

		mSaleInfoForSale1->setVisible(FALSE);
		mSaleInfoForSale2->setVisible(FALSE);
		mSaleInfoForSaleObjects->setVisible(FALSE);
		mSaleInfoForSaleNoObjects->setVisible(FALSE);
		mSaleInfoNotForSale->setVisible(FALSE);
		mBtnSellLand->setVisible(FALSE);
		mBtnStopSellLand->setVisible(FALSE);

		mTextPriceLabel->setText(NO_PRICE);
		mTextDwell->setText("");

		mBtnBuyLand->setEnabled(FALSE);
		mBtnBuyGroupLand->setEnabled(FALSE);
		mBtnReleaseLand->setEnabled(FALSE);
		mBtnReclaimLand->setEnabled(FALSE);
		mBtnBuyPass->setEnabled(FALSE);
	}
	else
	{
		// something selected, hooray!
		BOOL is_leased = (LLParcel::OS_LEASED == parcel->getOwnershipStatus());
		BOOL region_xfer = FALSE;
		if(regionp
		   && !(regionp->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL))
		{
			region_xfer = TRUE;
		}

		// estate owner/manager cannot edit other parts of the parcel
		BOOL estate_manager_sellable = !parcel->getAuctionID()
										&& gAgent.canManageEstate()
										// estate manager/owner can only sell parcels owned by estate owner
										&& regionp
										&& (parcel->getOwnerID() == regionp->getOwner());
		BOOL owner_sellable = region_xfer && !parcel->getAuctionID()
							&& LLViewerParcelMgr::isParcelModifiableByAgent(
										parcel, GP_LAND_SET_SALE_INFO);
		BOOL can_be_sold = owner_sellable || estate_manager_sellable;

		const LLUUID &owner_id = parcel->getOwnerID();
		BOOL is_public = parcel->isPublic();

		// Is it owned?
		if (is_public)
		{
			mTextSalePending->setText("");
			mTextSalePending->setEnabled(FALSE);
			mTextOwner->setText("(public)");
			mTextOwner->setEnabled(FALSE);
			mBtnProfile->setEnabled(FALSE);
			mTextClaimDate->setText("");
			mTextClaimDate->setEnabled(FALSE);
			mTextGroup->setText("(none)");
			mTextGroup->setEnabled(FALSE);
			mBtnStartAuction->setEnabled(FALSE);
		}
		else
		{
			if(!is_leased && (owner_id == gAgent.getID()))
			{
				mTextSalePending->setText(NEED_TIER_TO_MODIFY_STRING);
				mTextSalePending->setEnabled(TRUE);
			}
			else if(parcel->getAuctionID())
			{
				char auction_str[MAX_STRING];		/*Flawfinder: ignore*/
				snprintf(auction_str, sizeof(auction_str), "Auction ID: %u", parcel->getAuctionID());	/*Flawfinder: ignore*/
				mTextSalePending->setText(auction_str);
				mTextSalePending->setEnabled(TRUE);
			}
			else
			{
				// not the owner, or it is leased
				mTextSalePending->setText("");
				mTextSalePending->setEnabled(FALSE);
			}
			//refreshNames();
			mTextOwner->setEnabled(TRUE);

			// We support both group and personal profiles
			mBtnProfile->setEnabled(TRUE);

			if (parcel->getGroupID().isNull())
			{
				// Not group owned, so "Profile"
				mBtnProfile->setLabelSelected("Profile...");
				mBtnProfile->setLabelUnselected("Profile...");

				mTextGroup->setText("(none)");
				mTextGroup->setEnabled(FALSE);
			}
			else
			{
				// Group owned, so "Info"
				mBtnProfile->setLabelSelected("Info...");
				mBtnProfile->setLabelUnselected("Info...");

				//mTextGroup->setText("HIPPOS!");//parcel->getGroupName());
				mTextGroup->setEnabled(TRUE);
			}

			// Display claim date
			time_t claim_date = parcel->getClaimDate();
			char time_buf[TIME_STR_LENGTH];		/*Flawfinder: ignore*/
			mTextClaimDate->setText(formatted_time(claim_date, time_buf));
			mTextClaimDate->setEnabled(is_leased);

			BOOL enable_auction = (gAgent.getGodLevel() >= GOD_LIAISON)
								  && (owner_id == GOVERNOR_LINDEN_ID)
								  && (parcel->getAuctionID() == 0);
			mBtnStartAuction->setEnabled(enable_auction);
		}

		// Display options
		BOOL can_edit_identity = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_CHANGE_IDENTITY);
		mEditName->setEnabled(can_edit_identity);
		mEditDesc->setEnabled(can_edit_identity);

		BOOL can_edit_agent_only = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_NO_POWERS);
		mBtnSetGroup->setEnabled(can_edit_agent_only && !parcel->getIsGroupOwned());

		const LLUUID& group_id = parcel->getGroupID();

		// Can only allow deeding if you own it and it's got a group.
		BOOL enable_deed = (owner_id == gAgent.getID()
							&& group_id.notNull()
							&& gAgent.isInGroup(group_id));
		// You don't need special powers to allow your object to
		// be deeded to the group.
		mCheckDeedToGroup->setEnabled(enable_deed);
		mCheckDeedToGroup->set( parcel->getAllowDeedToGroup() );
		mCheckContributeWithDeed->setEnabled(enable_deed && parcel->getAllowDeedToGroup());
		mCheckContributeWithDeed->set(parcel->getContributeWithDeed());

		// Actually doing the deeding requires you to have GP_LAND_DEED
		// powers in the group.
		BOOL can_deed = gAgent.hasPowerInGroup(group_id, GP_LAND_DEED);
		mBtnDeedToGroup->setEnabled(   parcel->getAllowDeedToGroup()
									&& group_id.notNull()
									&& can_deed
									&& !parcel->getIsGroupOwned()
									);

		mEditName->setText( parcel->getName() );
		mEditDesc->setText( parcel->getDesc() );

		BOOL for_sale = parcel->getForSale();
				
		mBtnSellLand->setVisible(FALSE);
		mBtnStopSellLand->setVisible(FALSE);
		
		if (for_sale)
		{
			mSaleInfoForSale1->setVisible(TRUE);
			mSaleInfoForSale2->setVisible(TRUE);
			if (parcel->getSellWithObjects())
			{
				mSaleInfoForSaleObjects->setVisible(TRUE);
				mSaleInfoForSaleNoObjects->setVisible(FALSE);
			}
			else
			{
				mSaleInfoForSaleObjects->setVisible(FALSE);
				mSaleInfoForSaleNoObjects->setVisible(TRUE);
			}
			mSaleInfoNotForSale->setVisible(FALSE);
			mSaleInfoForSale1->setTextArg("[PRICE]", llformat("%d", parcel->getSalePrice()));
			if (can_be_sold)
			{
				mBtnStopSellLand->setVisible(TRUE);
			}
		}
		else
		{
			mSaleInfoForSale1->setVisible(FALSE);
			mSaleInfoForSale2->setVisible(FALSE);
			mSaleInfoForSaleObjects->setVisible(FALSE);
			mSaleInfoForSaleNoObjects->setVisible(FALSE);
			mSaleInfoNotForSale->setVisible(TRUE);
			if (can_be_sold)
			{
				mBtnSellLand->setVisible(TRUE);
			}
		}
		
		refreshNames();

		mBtnBuyLand->setEnabled(
			gParcelMgr->canAgentBuyParcel(parcel, false));
		mBtnBuyGroupLand->setEnabled(
			gParcelMgr->canAgentBuyParcel(parcel, true));

		// show pricing information
		char price[64];		/*Flawfinder: ignore*/
		const char* label = NULL;
		S32 area;
		S32 claim_price;
		S32 rent_price;
		F32 dwell;
		gParcelMgr->getDisplayInfo(&area,
								   &claim_price,
								   &rent_price,
								   &for_sale,
								   &dwell);

		// Area
		snprintf(price, sizeof(price), "%d sq. m.", area);		/*Flawfinder: ignore*/
		label = AREA;
		
		mTextPriceLabel->setText(label);
		mTextPrice->setText(price);
		
		snprintf(price, sizeof(price), "%.0f", dwell); 		/*Flawfinder: ignore*/
		mTextDwell->setText(price);

		if(region_owner)
		{
			mBtnReclaimLand->setEnabled(
				!is_public && (parcel->getOwnerID() != gAgent.getID()));
		}
		else
		{
			BOOL is_owner_release = LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_RELEASE);
			BOOL is_manager_release = (gAgent.canManageEstate() && 
									regionp && 
									(parcel->getOwnerID() != regionp->getOwner()));
			BOOL can_release = is_owner_release || is_manager_release;
			mBtnReleaseLand->setEnabled( can_release );
		}

		BOOL use_pass = parcel->getParcelFlag(PF_USE_PASS_LIST) && !gParcelMgr->isCollisionBanned();;
		mBtnBuyPass->setEnabled(use_pass);
	}
}

// public
void LLPanelLandGeneral::refreshNames()
{
	LLParcel *parcel = mParcel->getParcel();
	if (!parcel)
	{
		mTextOwner->setText("");
		return;
	}

	char buffer[MAX_STRING];		/*Flawfinder: ignore*/
	if (parcel->getIsGroupOwned())
	{
		buffer[0] = '\0';
		strcat(buffer, "(Group Owned)");	/*Flawfinder: ignore*/
	}
	else
	{
		// Figure out the owner's name
		char owner_first[MAX_STRING];	/*Flawfinder: ignore*/
		char owner_last[MAX_STRING];	/*Flawfinder: ignore*/
		gCacheName->getName(parcel->getOwnerID(), owner_first, owner_last);
		snprintf(buffer, sizeof(buffer), "%s %s", owner_first, owner_last); 	/*Flawfinder: ignore*/
	}

	if(LLParcel::OS_LEASE_PENDING == parcel->getOwnershipStatus())
	{
		strcat(buffer, " (Sale Pending)");	/*Flawfinder: ignore*/
	}
	mTextOwner->setText(buffer);

	if(!parcel->getGroupID().isNull())
	{
		gCacheName->getGroupName(parcel->getGroupID(), buffer);
	}
	else
	{
		buffer[0] = '\0';
	}
	mTextGroup->setText(buffer);

	const LLUUID& auth_buyer_id = parcel->getAuthorizedBuyerID();
	if(auth_buyer_id.notNull())
	{
		LLString name;
		char firstname[MAX_STRING];		/*Flawfinder: ignore*/
		char lastname[MAX_STRING];		/*Flawfinder: ignore*/
		gCacheName->getName(auth_buyer_id, firstname, lastname);
		name.assign(firstname);
		name.append(" ");
		name.append(lastname);

		mSaleInfoForSale2->setTextArg("[BUYER]", name);
	}
	else if(parcel->getReservedForNewbie())
	{
		mSaleInfoForSale2->setTextArg("[BUYER]", childGetText("new users only"));
	}
	else
	{
		mSaleInfoForSale2->setTextArg("[BUYER]", childGetText("anyone"));
	}
}


// virtual
void LLPanelLandGeneral::draw()
{
	refreshNames();
	LLPanel::draw();
}

// static
void LLPanelLandGeneral::onClickSetGroup(void* userdata)
{
	LLFloaterGroups* fg;
	fg = LLFloaterGroups::show(gAgent.getID(), LLFloaterGroups::CHOOSE_ONE);
	fg->setOkCallback( cbGroupID, userdata );
}

// static
void LLPanelLandGeneral::onClickProfile(void* data)
{
	LLPanelLandGeneral* panelp = (LLPanelLandGeneral*)data;
	LLParcel* parcel = panelp->mParcel->getParcel();
	if (!parcel) return;

	if (parcel->getIsGroupOwned())
	{
		const LLUUID& group_id = parcel->getGroupID();
		LLFloaterGroupInfo::showFromUUID(group_id);
	}
	else
	{
		const LLUUID& avatar_id = parcel->getOwnerID();
		LLFloaterAvatarInfo::showFromObject(avatar_id);
	}
}

// static
void LLPanelLandGeneral::cbGroupID(LLUUID group_id, void* userdata)
{
	LLPanelLandGeneral* self = (LLPanelLandGeneral*)userdata;
	self->setGroup(group_id);
}

// public
void LLPanelLandGeneral::setGroup(const LLUUID& group_id)
{
	LLParcel* parcel = mParcel->getParcel();
	if (!parcel) return;

	// Set parcel properties and send message
	parcel->setGroupID(group_id);
	//parcel->setGroupName(group_name);
	//mTextGroup->setText(group_name);

	// Send update
	gParcelMgr->sendParcelPropertiesUpdate(parcel);

	// Update UI
	refresh();
}

// static
void LLPanelLandGeneral::onClickBuyLand(void* data)
{
	BOOL* for_group = (BOOL*)data;
	gParcelMgr->startBuyLand(*for_group);
}

BOOL LLPanelLandGeneral::enableDeedToGroup(void* data)
{
	LLPanelLandGeneral* panelp = (LLPanelLandGeneral*)data;
	LLParcel* parcel = panelp->mParcel->getParcel();
	return (parcel != NULL) && (parcel->getParcelFlag(PF_ALLOW_DEED_TO_GROUP));
}

// static
void LLPanelLandGeneral::onClickDeed(void*)
{
	//LLParcel* parcel = mParcel->getParcel();
	//if (parcel)
	//{
	gParcelMgr->startDeedLandToGroup();
	//}
}

// static
void LLPanelLandGeneral::onClickRelease(void*)
{
	gParcelMgr->startReleaseLand();
}

// static
void LLPanelLandGeneral::onClickReclaim(void*)
{
	lldebugs << "LLPanelLandGeneral::onClickReclaim()" << llendl;
	gParcelMgr->reclaimParcel();
}

// static
BOOL LLPanelLandGeneral::enableBuyPass(void* data)
{
	LLPanelLandGeneral* panelp = (LLPanelLandGeneral*)data;
	LLParcel* parcel = panelp != NULL ? panelp->mParcel->getParcel() : gParcelMgr->getParcelSelection()->getParcel();
	return (parcel != NULL) && (parcel->getParcelFlag(PF_USE_PASS_LIST) && !gParcelMgr->isCollisionBanned());
}


// static
void LLPanelLandGeneral::onClickBuyPass(void* data)
{
	LLPanelLandGeneral* panelp = (LLPanelLandGeneral*)data;
	LLParcel* parcel = panelp != NULL ? panelp->mParcel->getParcel() : gParcelMgr->getParcelSelection()->getParcel();

	if (!parcel) return;

	S32 pass_price = parcel->getPassPrice();
	const char* parcel_name = parcel->getName();
	F32 pass_hours = parcel->getPassHours();

	char cost[256], time[256];		/*Flawfinder: ignore*/
	snprintf(cost, sizeof(cost), "%d", pass_price);	/*Flawfinder: ignore*/
	snprintf(time, sizeof(time), "%.2f", pass_hours);		/*Flawfinder: ignore*/

	LLStringBase<char>::format_map_t args;
	args["[COST]"] = cost;
	args["[PARCEL_NAME]"] = parcel_name;
	args["[TIME]"] = time;
	
	sBuyPassDialogHandle = gViewerWindow->alertXml("LandBuyPass", args, cbBuyPass)->getHandle();
}

// static
void LLPanelLandGeneral::onClickStartAuction(void* data)
{
	LLPanelLandGeneral* panelp = (LLPanelLandGeneral*)data;
	LLParcel* parcelp = panelp->mParcel->getParcel();
	if(parcelp)
	{
		if(parcelp->getForSale())
		{
			gViewerWindow->alertXml("CannotStartAuctionAlreadForSale");
		}
		else
		{
			LLFloaterAuction::show();
		}
	}
}

// static
void LLPanelLandGeneral::cbBuyPass(S32 option, void* data)
{
	if (0 == option)
	{
		// User clicked OK
		gParcelMgr->buyPass();
	}
}

//static 
BOOL LLPanelLandGeneral::buyPassDialogVisible()
{
	return LLFloater::getFloaterByHandle(sBuyPassDialogHandle) != NULL;
}

// static
void LLPanelLandGeneral::onCommitAny(LLUICtrl *ctrl, void *userdata)
{
	LLPanelLandGeneral *panelp = (LLPanelLandGeneral *)userdata;

	LLParcel* parcel = panelp->mParcel->getParcel();
	if (!parcel)
	{
		return;
	}

	// Extract data from UI
	std::string name = panelp->mEditName->getText();
	std::string desc = panelp->mEditDesc->getText();

	// Valid data from UI

	// Stuff data into selected parcel
	parcel->setName(name.c_str());
	parcel->setDesc(desc.c_str());

	BOOL allow_deed_to_group= panelp->mCheckDeedToGroup->get();
	BOOL contribute_with_deed = panelp->mCheckContributeWithDeed->get();

	parcel->setParcelFlag(PF_ALLOW_DEED_TO_GROUP, allow_deed_to_group);
	parcel->setContributeWithDeed(contribute_with_deed);

	// Send update to server
	gParcelMgr->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	panelp->refresh();
}

// static
void LLPanelLandGeneral::onClickSellLand(void* data)
{
	gParcelMgr->startSellLand();
}

// static
void LLPanelLandGeneral::onClickStopSellLand(void* data)
{
	LLPanelLandGeneral* panelp = (LLPanelLandGeneral*)data;
	LLParcel* parcel = panelp->mParcel->getParcel();

	parcel->setParcelFlag(PF_FOR_SALE, FALSE);
	parcel->setSalePrice(0);
	parcel->setAuthorizedBuyerID(LLUUID::null);

	gParcelMgr->sendParcelPropertiesUpdate(parcel);
}

//---------------------------------------------------------------------------
// LLPanelLandObjects
//---------------------------------------------------------------------------
LLPanelLandObjects::LLPanelLandObjects(LLParcelSelectionHandle& parcel)
:	LLPanel("land_objects_panel"), mParcel(parcel)
{
}



BOOL LLPanelLandObjects::postBuild()
{
	
	mFirstReply = TRUE;
	mParcelObjectBonus = LLUICtrlFactory::getTextBoxByName(this, "Simulator Primitive Bonus Factor: 1.00");
	
	mSWTotalObjectsLabel = LLUICtrlFactory::getTextBoxByName(this, "Simulator primitive usage:");
	mSWTotalObjects = LLUICtrlFactory::getTextBoxByName(this, "0 out of 0 available");
	
	mObjectContributionLabel = LLUICtrlFactory::getTextBoxByName(this, "Primitives parcel supports:");
	mObjectContribution = LLUICtrlFactory::getTextBoxByName(this, "object_contrib_text");

	
	mTotalObjectsLabel = LLUICtrlFactory::getTextBoxByName(this, "Primitives on parcel:");
	mTotalObjects = LLUICtrlFactory::getTextBoxByName(this, "total_objects_text");

	
	mOwnerObjectsLabel = LLUICtrlFactory::getTextBoxByName(this, "Owned by parcel owner:");
	mOwnerObjects = LLUICtrlFactory::getTextBoxByName(this, "owner_objects_text");

	
	mBtnShowOwnerObjects = LLUICtrlFactory::getButtonByName(this, "ShowOwner");
	mBtnShowOwnerObjects->setClickedCallback(onClickShowOwnerObjects, this);

	mBtnReturnOwnerObjects = LLUICtrlFactory::getButtonByName(this, "ReturnOwner...");
	mBtnReturnOwnerObjects->setClickedCallback(onClickReturnOwnerObjects, this);

	
	mGroupObjectsLabel = LLUICtrlFactory::getTextBoxByName(this, "Set to group:");
	mGroupObjects = LLUICtrlFactory::getTextBoxByName(this, "group_objects_text");

	
	mBtnShowGroupObjects = LLUICtrlFactory::getButtonByName(this, "ShowGroup");
	mBtnShowGroupObjects->setClickedCallback(onClickShowGroupObjects, this);

	mBtnReturnGroupObjects = LLUICtrlFactory::getButtonByName(this, "ReturnGroup...");
	mBtnReturnGroupObjects->setClickedCallback(onClickReturnGroupObjects, this);

	
	mOtherObjectsLabel = LLUICtrlFactory::getTextBoxByName(this, "Owned by others:");
	mOtherObjects = LLUICtrlFactory::getTextBoxByName(this, "other_objects_text");
	
	mBtnShowOtherObjects = LLUICtrlFactory::getButtonByName(this, "ShowOther");
	mBtnShowOtherObjects->setClickedCallback(onClickShowOtherObjects, this);

	mBtnReturnOtherObjects = LLUICtrlFactory::getButtonByName(this, "ReturnOther...");
	mBtnReturnOtherObjects->setClickedCallback(onClickReturnOtherObjects, this);

	mSelectedObjectsLabel = LLUICtrlFactory::getTextBoxByName(this, "Selected / sat upon:");
	mSelectedObjects = LLUICtrlFactory::getTextBoxByName(this, "selected_objects_text");
	
	mCleanOtherObjectsLabel = LLUICtrlFactory::getTextBoxByName(this, "Autoreturn other resident's objects (minutes, 0 for off):");

	mCleanOtherObjectsTime = LLUICtrlFactory::getLineEditorByName(this, "clean other time");
	mCleanOtherObjectsTime->setFocusLostCallback(onLostFocus);	
	childSetPrevalidate("clean other time", LLLineEditor::prevalidateNonNegativeS32);
	childSetUserData("clean other time", this);
	
	mOwnerListText = LLUICtrlFactory::getTextBoxByName(this, "Object Owners:");

	
	mBtnRefresh = LLUICtrlFactory::getButtonByName(this, "Refresh List");
	mBtnRefresh->setClickedCallback(onClickRefresh, this);

	
	mBtnReturnOwnerList = LLUICtrlFactory::getButtonByName(this, "Return objects...");
	mBtnReturnOwnerList->setClickedCallback(onClickReturnOwnerList, this);

	LLUUID image_id;

	image_id.set( gViewerArt.getString("icon_avatar_online.tga") );
	mIconAvatarOnline = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);
	
	image_id.set( gViewerArt.getString("icon_avatar_offline.tga") );
	mIconAvatarOffline = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("icon_group.tga") );
	mIconGroup = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	mCurrentSortColumn = 3; // sort by number of objects by default.
	mCurrentSortAscending = FALSE;

	// Column widths for various columns
	const S32 SORTER_WIDTH		= 308;
	const S32 DESC_BTN_WIDTH	= 64;
	const S32 ICON_WIDTH		= 24;
	mColWidth[0] = ICON_WIDTH;	// type icon
	mColWidth[1] = -1;	// hidden type code
	mColWidth[2] = SORTER_WIDTH - mColWidth[0] - DESC_BTN_WIDTH;
	mColWidth[3] = DESC_BTN_WIDTH;			// info
	mColWidth[4] = -1;						// type data 1
	mColWidth[5] = -1;
	mColWidth[6] = -1;	// type data 3
	mColWidth[7] = -1;	// type data 4
	mColWidth[8] = -1;	// type data 5

	// Adjust description for other widths
	S32 sum = 0;
	for (S32 i = 0; i < 8; i++)
	{
		if (mColWidth[i] > 0)
		{
			sum += mColWidth[i];
		}
	}
	mColWidth[8] = mRect.getWidth() - HPAD - sum - HPAD - HPAD;

	mBtnType = LLUICtrlFactory::getButtonByName(this, "Type");
	mBtnType->setClickedCallback(onClickType, this);

	mBtnName = LLUICtrlFactory::getButtonByName(this, "Name");
	mBtnName->setClickedCallback(onClickName, this);

	mBtnDescription = LLUICtrlFactory::getButtonByName(this, "Count");
	mBtnDescription->setClickedCallback(onClickDesc, this);
	
	mOwnerList = LLUICtrlFactory::getNameListByName(this, "owner list");
	childSetCommitCallback("owner list", onCommitList, this);
	mOwnerList->setDoubleClickCallback(onDoubleClickOwner);

	return TRUE;
}




// virtual
LLPanelLandObjects::~LLPanelLandObjects()
{ }

// static
void LLPanelLandObjects::onDoubleClickOwner(void *userdata)
{
	LLPanelLandObjects *self = (LLPanelLandObjects *)userdata;

	LLScrollListItem* item = self->mOwnerList->getFirstSelected();
	if (item)
	{
		LLUUID owner_id = item->getUUID();
		// Look up the selected name, for future dialog box use.
		const LLScrollListCell* cell;
		cell = item->getColumn(1);
		if (!cell)
		{
			return;
		}
		// Is this a group?
		BOOL is_group = cell->getText() == OWNER_GROUP;
		if (is_group)
		{
			LLFloaterGroupInfo::showFromUUID(owner_id);
		}
		else
		{
			LLFloaterAvatarInfo::showFromDirectory(owner_id);
		}
	}
}

// public
void LLPanelLandObjects::refresh()
{
	LLParcel *parcel = mParcel->getParcel();

	mBtnShowOwnerObjects->setEnabled(FALSE);
	mBtnShowGroupObjects->setEnabled(FALSE);
	mBtnShowOtherObjects->setEnabled(FALSE);
	mBtnReturnOwnerObjects->setEnabled(FALSE);
	mBtnReturnGroupObjects->setEnabled(FALSE);
	mBtnReturnOtherObjects->setEnabled(FALSE);
	mCleanOtherObjectsTime->setEnabled(FALSE);
	mBtnRefresh->			setEnabled(FALSE);
	mBtnReturnOwnerList->	setEnabled(FALSE);

	mSelectedOwners.clear();
	mOwnerList->deleteAllItems();
	mOwnerList->setEnabled(FALSE);

	if (!parcel)
	{
		mSWTotalObjects->setText("0 out of 0 available");
		mObjectContribution->setText("0");
		mTotalObjects->setText("0");
		mOwnerObjects->setText("0");
		mGroupObjects->setText("0");
		mOtherObjects->setText("0");
		mSelectedObjects->setText("0");
	}
	else
	{
		char count[MAX_STRING];		/*Flawfinder: ignore*/
		S32 sw_max = 0;
		S32 sw_total = 0;
		S32 max = 0;
		S32 total = 0;
		S32 owned = 0;
		S32 group = 0;
		S32 other = 0;
		S32 selected = 0;
		F32 parcel_object_bonus = 0.f;

		gParcelMgr->getPrimInfo(sw_max, sw_total, 
								  max, total, owned, group, other, selected,
								  parcel_object_bonus, mOtherTime);

		// Can't have more than region max tasks, regardless of parcel
		// object bonus factor.
		LLViewerRegion* region = gParcelMgr->getSelectionRegion();
		if (region)
		{
			S32 max_tasks_per_region = (S32)region->getMaxTasks();
			sw_max = llmin(sw_max, max_tasks_per_region);
			max = llmin(max, max_tasks_per_region);
		}

		if (parcel_object_bonus != 1.0f)
		{
			snprintf(count, sizeof(count), "Region Object Bonus Factor: %.2f", 		/*Flawfinder: ignore*/
					parcel_object_bonus);
			mParcelObjectBonus->setText(count);
		}
		else
		{
			mParcelObjectBonus->setText("");
		}

		if (sw_total > sw_max)
		{
			snprintf(count, sizeof(count), "%d out of %d (%d will be deleted)", 		/*Flawfinder: ignore*/
					sw_total, sw_max, sw_total - sw_max);
		}
		else
		{
			snprintf(count, sizeof(count), "%d out of %d (%d available)",  			/*Flawfinder: ignore*/
					sw_total, sw_max, sw_max - sw_total);
		}
		mSWTotalObjects->setText(count);

		snprintf(count, sizeof(count),  "%d", max);		/*Flawfinder: ignore*/
		mObjectContribution->setText(count);

		snprintf(count, sizeof(count), "%d", total);		/*Flawfinder: ignore*/
		mTotalObjects->setText(count);

		snprintf(count, sizeof(count), "%d", owned);	/*Flawfinder: ignore*/
		mOwnerObjects->setText(count);

		snprintf(count, sizeof(count), "%d", group);		/*Flawfinder: ignore*/
		mGroupObjects->setText(count);

		snprintf(count, sizeof(count), "%d", other);		/*Flawfinder: ignore*/
		mOtherObjects->setText(count);

		snprintf(count, sizeof(count), "%d", selected);	/*Flawfinder: ignore*/
		mSelectedObjects->setText(count);

		snprintf(count, sizeof(count), "%d", mOtherTime);			/*Flawfinder: ignore*/
		mCleanOtherObjectsTime->setText(count);

		BOOL can_return_owned = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_RETURN_GROUP_OWNED);
		BOOL can_return_group_set = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_RETURN_GROUP_SET);
		BOOL can_return_other = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_RETURN_NON_GROUP);

		if (can_return_owned || can_return_group_set || can_return_other)
		{
			if (owned && can_return_owned)
			{
				mBtnShowOwnerObjects->setEnabled(TRUE);
				mBtnReturnOwnerObjects->setEnabled(TRUE);
			}
			if (group && can_return_group_set)
			{
				mBtnShowGroupObjects->setEnabled(TRUE);
				mBtnReturnGroupObjects->setEnabled(TRUE);
			}
			if (other && can_return_other)
			{
				mBtnShowOtherObjects->setEnabled(TRUE);
				mBtnReturnOtherObjects->setEnabled(TRUE);
			}

			mCleanOtherObjectsTime->setEnabled(TRUE);
			mBtnRefresh->setEnabled(TRUE);
		}
	}
}

// virtual
void LLPanelLandObjects::draw()
{
	LLPanel::draw();
}

void send_other_clean_time_message(S32 parcel_local_id, S32 other_clean_time)
{
	LLMessageSystem *msg = gMessageSystem;

	LLViewerRegion* region = gParcelMgr->getSelectionRegion();
	if (!region) return;

	msg->newMessageFast(_PREHASH_ParcelSetOtherCleanTime);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_LocalID, parcel_local_id);
	msg->addS32Fast(_PREHASH_OtherCleanTime, other_clean_time);

	msg->sendReliable(region->getHost());
}

void send_return_objects_message(S32 parcel_local_id, S32 return_type, 
								 uuid_list_t* owner_ids = NULL)
{
	LLMessageSystem *msg = gMessageSystem;

	LLViewerRegion* region = gParcelMgr->getSelectionRegion();
	if (!region) return;

	msg->newMessageFast(_PREHASH_ParcelReturnObjects);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_LocalID, parcel_local_id);
	msg->addU32Fast(_PREHASH_ReturnType, (U32) return_type);

	// Dummy task id, not used
	msg->nextBlock("TaskIDs");
	msg->addUUID("TaskID", LLUUID::null);

	// Throw all return ids into the packet.
	// TODO: Check for too many ids.
	if (owner_ids)
	{
		uuid_list_t::iterator end = owner_ids->end();
		for (uuid_list_t::iterator it = owner_ids->begin();
			 it != end;
			 ++it)
		{
			msg->nextBlockFast(_PREHASH_OwnerIDs);
			msg->addUUIDFast(_PREHASH_OwnerID, (*it));
		}
	}
	else
	{
		msg->nextBlockFast(_PREHASH_OwnerIDs);
		msg->addUUIDFast(_PREHASH_OwnerID, LLUUID::null);
	}

	msg->sendReliable(region->getHost());
}

// static
void LLPanelLandObjects::callbackReturnOwnerObjects(S32 option, void* userdata)
{
	LLPanelLandObjects	*lop = (LLPanelLandObjects *)userdata;
	LLParcel *parcel = lop->mParcel->getParcel();
	if (0 == option)
	{
		if (parcel)
		{
			LLUUID owner_id = parcel->getOwnerID();	
			LLString::format_map_t args;
			if (owner_id == gAgentID)
			{
				LLNotifyBox::showXml("OwnedObjectsReturned");
			}
			else
			{
				char first[DB_FIRST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
				char last[DB_LAST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
				gCacheName->getName(owner_id, first, last);
				args["[FIRST]"] = first;
				args["[LAST]"] = last;
				LLNotifyBox::showXml("OtherObjectsReturned", args);
			}
			send_return_objects_message(parcel->getLocalID(), RT_OWNER);
		}
	}

	gSelectMgr->unhighlightAll();
	gParcelMgr->sendParcelPropertiesUpdate( parcel );
	lop->refresh();
}

// static
void LLPanelLandObjects::callbackReturnGroupObjects(S32 option, void* userdata)
{
	LLPanelLandObjects	*lop = (LLPanelLandObjects *)userdata;
	LLParcel *parcel = lop->mParcel->getParcel();
	if (0 == option)
	{
		if (parcel)
		{
			char group_name[MAX_STRING];		/*Flawfinder: ignore*/
			gCacheName->getGroupName(parcel->getGroupID(), group_name);
			LLString::format_map_t args;
			args["[GROUPNAME]"] = group_name;
			LLNotifyBox::showXml("GroupObjectsReturned", args);
			send_return_objects_message(parcel->getLocalID(), RT_GROUP);
		}
	}
	gSelectMgr->unhighlightAll();
	gParcelMgr->sendParcelPropertiesUpdate( parcel );
	lop->refresh();
}

// static
void LLPanelLandObjects::callbackReturnOtherObjects(S32 option, void* userdata)
{
	LLPanelLandObjects	*lop = (LLPanelLandObjects *)userdata;
	LLParcel *parcel = lop->mParcel->getParcel();
	if (0 == option)
	{
		if (parcel)
		{
			LLNotifyBox::showXml("UnOwnedObjectsReturned");
			send_return_objects_message(parcel->getLocalID(), RT_OTHER);
		}
	}
	gSelectMgr->unhighlightAll();
	gParcelMgr->sendParcelPropertiesUpdate( parcel );
	lop->refresh();
}

// static
void LLPanelLandObjects::callbackReturnOwnerList(S32 option, void* userdata)
{
	LLPanelLandObjects	*self = (LLPanelLandObjects *)userdata;
	LLParcel *parcel = self->mParcel->getParcel();
	if (0 == option)
	{
		if (parcel)
		{
			// Make sure we have something selected.
			uuid_list_t::iterator selected = self->mSelectedOwners.begin();
			if (selected != self->mSelectedOwners.end())
			{
				LLString::format_map_t args;
				if (self->mSelectedIsGroup)
				{
					args["[GROUPNAME]"] = self->mSelectedName;
					LLNotifyBox::showXml("GroupObjectsReturned", args);
				}
				else
				{
					// XUI:translate NAME -> FIRST LAST
					args["[NAME]"] = self->mSelectedName;
					LLNotifyBox::showXml("OtherObjectsReturned2", args);
				}

				send_return_objects_message(parcel->getLocalID(), RT_LIST, &(self->mSelectedOwners));
			}
		}
	}
	gSelectMgr->unhighlightAll();
	gParcelMgr->sendParcelPropertiesUpdate( parcel );
	self->refresh();
}


// static
void LLPanelLandObjects::onClickReturnOwnerList(void* userdata)
{
	LLPanelLandObjects	*self = (LLPanelLandObjects *)userdata;

	S32 sw_max, sw_total;
	S32 max, total;
	S32 owned, group, other, selected;
	F32 parcel_object_bonus;
	S32 other_time;

	gParcelMgr->getPrimInfo(sw_max, sw_total, max, total, owned, group, other, selected, parcel_object_bonus, other_time);

	LLParcel* parcelp = self->mParcel->getParcel();
	if (!parcelp) return;

	// Make sure we have something selected.
	if (self->mSelectedOwners.empty())
	{
		return;
	}
	//uuid_list_t::iterator selected_itr = self->mSelectedOwners.begin();
	//if (selected_itr == self->mSelectedOwners.end()) return;

	send_parcel_select_objects(parcelp->getLocalID(), RT_LIST, &(self->mSelectedOwners));

	LLStringBase<char>::format_map_t args;
	args["[NAME]"] = self->mSelectedName;
	args["[N]"] = llformat("%d",self->mSelectedCount);
	if (self->mSelectedIsGroup)
	{
		gViewerWindow->alertXml("ReturnObjectsDeededToGroup", args, callbackReturnOwnerList, userdata);	
	}
	else 
	{
		gViewerWindow->alertXml("ReturnObjectsOwnedByUser", args, callbackReturnOwnerList, userdata);	
	}
}


// static
void LLPanelLandObjects::onClickRefresh(void* userdata)
{
	LLPanelLandObjects *self = (LLPanelLandObjects*)userdata;

	LLMessageSystem *msg = gMessageSystem;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel) return;

	LLViewerRegion* region = gParcelMgr->getSelectionRegion();
	if (!region) return;

	// ready the list for results
	self->mOwnerList->deleteAllItems();
	self->mOwnerList->addSimpleItem("Searching...");
	self->mOwnerList->setEnabled(FALSE);
	self->mFirstReply = TRUE;

	// send the message
	msg->newMessageFast(_PREHASH_ParcelObjectOwnersRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ParcelData);
	msg->addS32Fast(_PREHASH_LocalID, parcel->getLocalID());

	msg->sendReliable(region->getHost());
}

// static
void LLPanelLandObjects::processParcelObjectOwnersReply(LLMessageSystem *msg, void **)
{
	LLPanelLandObjects* self = LLFloaterLand::getCurrentPanelLandObjects();

	const LLFontGL* FONT = LLFontGL::sSansSerif;

	// Extract all of the owners.
	S32 rows = msg->getNumberOfBlocksFast(_PREHASH_Data);
	//uuid_list_t return_ids;
	LLUUID	owner_id;
	BOOL	is_group_owned;
	S32		object_count;
	BOOL	is_online;
	char object_count_str[MAX_STRING];	/*Flawfinder: ignore*/
	//BOOL b_need_refresh = FALSE;

	// If we were waiting for the first reply, clear the "Searching..." text.
	if (self->mFirstReply)
	{
		self->mOwnerList->deleteAllItems();
		self->mFirstReply = FALSE;
	}

	for(S32 i = 0; i < rows; ++i)
	{
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_OwnerID,		owner_id,		i);
		msg->getBOOLFast(_PREHASH_Data, _PREHASH_IsGroupOwned,	is_group_owned,	i);
		msg->getS32Fast (_PREHASH_Data, _PREHASH_Count,			object_count,	i);
		msg->getBOOLFast(_PREHASH_Data, _PREHASH_OnlineStatus,	is_online,		i);

		if (owner_id.isNull())
		{
			continue;
		}

		LLScrollListItem *row = new LLScrollListItem( TRUE, NULL, owner_id);
		if (is_group_owned)
		{
			row->addColumn(self->mIconGroup, self->mColWidth[0]);
			row->addColumn(OWNER_GROUP, FONT, self->mColWidth[1]);
		}
		else if (is_online)
		{
			row->addColumn(self->mIconAvatarOnline, self->mColWidth[0]);
			row->addColumn(OWNER_ONLINE, FONT, self->mColWidth[1]);
		}
		else  // offline
		{
			row->addColumn(self->mIconAvatarOffline, self->mColWidth[0]);
			row->addColumn(OWNER_OFFLINE, FONT, self->mColWidth[1]);
		}
		// Placeholder for name.
		row->addColumn("", FONT, self->mColWidth[2]);

		snprintf(object_count_str, sizeof(object_count_str), "%d", object_count); 	/*Flawfinder: ignore*/
		row->addColumn(object_count_str, FONT, self->mColWidth[3]);

		if (is_group_owned)
		{
			self->mOwnerList->addGroupNameItem(row, ADD_BOTTOM);
		}
		else
		{
			self->mOwnerList->addNameItem(row, ADD_BOTTOM);
		}

		lldebugs << "object owner " << owner_id << " (" << (is_group_owned ? "group" : "agent")
				<< ") owns " << object_count << " objects." << llendl;
	}
	self->mOwnerList->sortByColumn(self->mCurrentSortColumn, self->mCurrentSortAscending);

	// check for no results
	if (0 == self->mOwnerList->getItemCount())
	{
		self->mOwnerList->addSimpleItem("None found.");
	}
	else
	{
		self->mOwnerList->setEnabled(TRUE);
	}
}

void LLPanelLandObjects::sortBtnCore(S32 column)
{
	if (column == (S32)mCurrentSortColumn)  // is this already our sorted column?
	{
		mCurrentSortAscending = !mCurrentSortAscending;
	}
	else  // default to ascending first time a column is clicked
	{
		mCurrentSortColumn = column;
		mCurrentSortAscending = TRUE;
	}

	mOwnerList->sortByColumn(column, mCurrentSortAscending);
}

// static
void LLPanelLandObjects::onCommitList(LLUICtrl* ctrl, void* data)
{
	LLPanelLandObjects* self = (LLPanelLandObjects*)data;

	if (FALSE == self->mOwnerList->getCanSelect())
	{
		return;
	}
	LLScrollListItem *item = self->mOwnerList->getFirstSelected();
	if (item)
	{
		// Look up the selected name, for future dialog box use.
		const LLScrollListCell* cell;
		cell = item->getColumn(1);
		if (!cell)
		{
			return;
		}
		// Is this a group?
		self->mSelectedIsGroup = cell->getText() == OWNER_GROUP;
		cell = item->getColumn(2);
		self->mSelectedName = cell->getText();
		cell = item->getColumn(3);
		self->mSelectedCount = atoi(cell->getText().c_str());

		// Set the selection, and enable the return button.
		self->mSelectedOwners.clear();
		self->mSelectedOwners.insert(item->getUUID());
		self->mBtnReturnOwnerList->setEnabled(TRUE);

		// Highlight this user's objects
		clickShowCore(self, RT_LIST, &(self->mSelectedOwners));
	}
}

// static
void LLPanelLandObjects::onClickType(void* userdata)
{
	// Sort on hidden type column
	LLPanelLandObjects* self = (LLPanelLandObjects*)userdata;
	self->sortBtnCore(1);
}

// static
void LLPanelLandObjects::onClickDesc(void* userdata)
{
	LLPanelLandObjects* self = (LLPanelLandObjects*)userdata;
	self->sortBtnCore(3);
}

// static
void LLPanelLandObjects::onClickName(void* userdata)
{
	LLPanelLandObjects* self = (LLPanelLandObjects*)userdata;
	self->sortBtnCore(2);
}

// static
void LLPanelLandObjects::clickShowCore(LLPanelLandObjects* self, S32 return_type, uuid_list_t* list)
{
	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel) return;

	send_parcel_select_objects(parcel->getLocalID(), return_type, list);
}

// static
void LLPanelLandObjects::onClickShowOwnerObjects(void* userdata)
{
	clickShowCore((LLPanelLandObjects*)userdata, RT_OWNER);
}

// static
void LLPanelLandObjects::onClickShowGroupObjects(void* userdata)
{
	clickShowCore((LLPanelLandObjects*)userdata, (RT_GROUP));
}

// static
void LLPanelLandObjects::onClickShowOtherObjects(void* userdata)
{
	clickShowCore((LLPanelLandObjects*)userdata, RT_OTHER);
}

// static
void LLPanelLandObjects::onClickReturnOwnerObjects(void* userdata)
{
	S32 sw_max=0, sw_total=0;
	S32 max=0, total=0;
	S32 owned=0, group=0, other=0, selected=0;
	F32 parcel_object_bonus=0;
	S32 other_time=0;

	gParcelMgr->getPrimInfo(sw_max, sw_total, max, total, owned, group, other, selected, parcel_object_bonus, other_time);

	LLPanelLandObjects* panelp = (LLPanelLandObjects*)userdata;
	LLParcel* parcel = panelp->mParcel->getParcel();
	if (!parcel) return;

	send_parcel_select_objects(parcel->getLocalID(), RT_OWNER);

	LLUUID owner_id = parcel->getOwnerID();
	
	LLStringBase<char>::format_map_t args;
	args["[N]"] = llformat("%d",owned);

	if (owner_id == gAgent.getID())
	{
		gViewerWindow->alertXml("ReturnObjectsOwnedBySelf", args, callbackReturnOwnerObjects, userdata);
	}
	else
	{
		char first[DB_FIRST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
		char last[DB_LAST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
		gCacheName->getName(owner_id, first, last);
		std::string name = first;
		name += " ";
		name += last;
		args["[NAME]"] = name;
		gViewerWindow->alertXml("ReturnObjectsOwnedByUser", args, callbackReturnOwnerObjects, userdata);
	}
}

// static
void LLPanelLandObjects::onClickReturnGroupObjects(void* userdata)
{
	S32 sw_max=0, sw_total=0;
	S32 max=0, total=0;
	S32 owned=0, group=0, other=0, selected=0;
	F32 parcel_object_bonus=0;
	S32 other_time=0;
	
	gParcelMgr->getPrimInfo(sw_max, sw_total, max, total, owned, group, other, selected, parcel_object_bonus, other_time);

	LLPanelLandObjects* panelp = (LLPanelLandObjects*)userdata;
	LLParcel* parcel = panelp->mParcel->getParcel();
	if (!parcel) return;

	send_parcel_select_objects(parcel->getLocalID(), RT_GROUP);

	char group_name[MAX_STRING];	/*Flawfinder: ignore*/
	gCacheName->getGroupName(parcel->getGroupID(), group_name);
	
	LLStringBase<char>::format_map_t args;
	args["[NAME]"] = group_name;
	args["[N]"] = llformat("%d",group);

	// create and show confirmation textbox
	gViewerWindow->alertXml("ReturnObjectsDeededToGroup", args, callbackReturnGroupObjects, userdata);
}

// static
void LLPanelLandObjects::onClickReturnOtherObjects(void* userdata)
{
	S32 sw_max=0, sw_total=0;
	S32 max=0, total=0;
	S32 owned=0, group=0, other=0, selected=0;
	F32 parcel_object_bonus=0;
	S32 other_time=0;

	gParcelMgr->getPrimInfo(sw_max, sw_total, max, total, owned, group, other, selected, parcel_object_bonus, other_time);

	LLPanelLandObjects* panelp = (LLPanelLandObjects*)userdata;
	LLParcel* parcel = panelp->mParcel->getParcel();
	if (!parcel) return;

	send_parcel_select_objects(parcel->getLocalID(), RT_OTHER);

	LLStringBase<char>::format_map_t args;
	args["[N]"] = llformat("%d", other);
	
	if (parcel->getIsGroupOwned())
	{
		char group_name[MAX_STRING];	/*Flawfinder: ignore*/
		gCacheName->getGroupName(parcel->getGroupID(), group_name);
		args["[NAME]"] = group_name;

		gViewerWindow->alertXml("ReturnObjectsNotOwnedByGroup", args, callbackReturnOtherObjects, userdata);
	}
	else
	{
		LLUUID owner_id = parcel->getOwnerID();

		if (owner_id == gAgent.getID())
		{
			gViewerWindow->alertXml("ReturnObjectsNotOwnedBySelf", args, callbackReturnOtherObjects, userdata);
		}
		else
		{
			char first[DB_FIRST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
			char last[DB_LAST_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
			gCacheName->getName(owner_id, first, last);
			std::string name;
			name += first;
			name += " ";
			name += last;
			args["[NAME]"] = name;

			gViewerWindow->alertXml("ReturnObjectsNotOwnedByUser", args, callbackReturnOtherObjects, userdata);
		}
	}
}

// static
void LLPanelLandObjects::onLostFocus(LLLineEditor *caller, void* user_data)
{
	LLPanelLandObjects	*lop = (LLPanelLandObjects *)user_data;
	LLParcel* parcel = lop->mParcel->getParcel();
	if (parcel)
	{
		lop->mOtherTime = atoi(lop->mCleanOtherObjectsTime->getText().c_str());

		parcel->setCleanOtherTime(lop->mOtherTime);
		send_other_clean_time_message(parcel->getLocalID(), lop->mOtherTime);
	}
}


//---------------------------------------------------------------------------
// LLPanelLandOptions
//---------------------------------------------------------------------------

LLPanelLandOptions::LLPanelLandOptions(LLParcelSelectionHandle& parcel)
:	LLPanel("land_options_panel"),
	mCheckEditObjects(NULL),
	mCheckEditGroupObjects(NULL),
	mCheckAllObjectEntry(NULL),
	mCheckGroupObjectEntry(NULL),
	mCheckEditLand(NULL),
	mCheckSafe(NULL),
	mCheckFly(NULL),
	mCheckGroupScripts(NULL),
	mCheckOtherScripts(NULL),
	mCheckLandmark(NULL),
	mCheckShowDirectory(NULL),
	mCategoryCombo(NULL),
	mLandingTypeCombo(NULL),
	mSnapshotCtrl(NULL),
	mLocationText(NULL),
	mSetBtn(NULL),
	mClearBtn(NULL),
	mAllowPublishCtrl(NULL),
	mMatureCtrl(NULL),
	mPushRestrictionCtrl(NULL),
	mPublishHelpButton(NULL),
	mParcel(parcel)
{

}


BOOL LLPanelLandOptions::postBuild()
{

	
	mCheckEditObjects = LLUICtrlFactory::getCheckBoxByName(this, "edit objects check");
	childSetCommitCallback("edit objects check", onCommitAny, this);
	
	mCheckEditGroupObjects = LLUICtrlFactory::getCheckBoxByName(this, "edit group objects check");
	childSetCommitCallback("edit group objects check", onCommitAny, this);

	mCheckAllObjectEntry = LLUICtrlFactory::getCheckBoxByName(this, "all object entry check");
	childSetCommitCallback("all object entry check", onCommitAny, this);

	mCheckGroupObjectEntry = LLUICtrlFactory::getCheckBoxByName(this, "group object entry check");
	childSetCommitCallback("group object entry check", onCommitAny, this);
	
	mCheckEditLand = LLUICtrlFactory::getCheckBoxByName(this, "edit land check");
	childSetCommitCallback("edit land check", onCommitAny, this);

	
	mCheckLandmark = LLUICtrlFactory::getCheckBoxByName(this, "check landmark");
	childSetCommitCallback("check landmark", onCommitAny, this);

	
	mCheckGroupScripts = LLUICtrlFactory::getCheckBoxByName(this, "check group scripts");
	childSetCommitCallback("check group scripts", onCommitAny, this);

	
	mCheckFly = LLUICtrlFactory::getCheckBoxByName(this, "check fly");
	childSetCommitCallback("check fly", onCommitAny, this);

	
	mCheckOtherScripts = LLUICtrlFactory::getCheckBoxByName(this, "check other scripts");
	childSetCommitCallback("check other scripts", onCommitAny, this);

	
	mCheckSafe = LLUICtrlFactory::getCheckBoxByName(this, "check safe");
	childSetCommitCallback("check safe", onCommitAny, this);

	
	mPushRestrictionCtrl = LLUICtrlFactory::getCheckBoxByName(this, "PushRestrictCheck");
	childSetCommitCallback("PushRestrictCheck", onCommitAny, this);

	mCheckShowDirectory = LLUICtrlFactory::getCheckBoxByName(this, "ShowDirectoryCheck");
	childSetCommitCallback("ShowDirectoryCheck", onCommitAny, this);

	
	mCategoryCombo = LLUICtrlFactory::getComboBoxByName(this, "land category");
	childSetCommitCallback("land category", onCommitAny, this);

	mAllowPublishCtrl = LLUICtrlFactory::getCheckBoxByName(this, "PublishCheck");
	childSetCommitCallback("PublishCheck", onCommitAny, this);

	
	mMatureCtrl = LLUICtrlFactory::getCheckBoxByName(this, "MatureCheck");
	childSetCommitCallback("MatureCheck", onCommitAny, this);
	
	mPublishHelpButton = LLUICtrlFactory::getButtonByName(this, "?");
	mPublishHelpButton->setClickedCallback(onClickPublishHelp, this);


	if (gAgent.mAccess < SIM_ACCESS_MATURE)
	{
		// Disable these buttons if they are PG (Teen) users
		mAllowPublishCtrl->setVisible(FALSE);
		mAllowPublishCtrl->setEnabled(FALSE);
		mPublishHelpButton->setVisible(FALSE);
		mPublishHelpButton->setEnabled(FALSE);
		mMatureCtrl->setVisible(FALSE);
		mMatureCtrl->setEnabled(FALSE);
	}

		// Load up the category list
	//now in xml file
	/*
	S32 i;
	for (i = 0; i < LLParcel::C_COUNT; i++)
	{
		LLParcel::ECategory cat = (LLParcel::ECategory)i;

		// Selecting Linden Location when you're not a god
		// is also blocked on the server.
		BOOL enabled = TRUE;
		if (!gAgent.isGodlike()
			&& i == LLParcel::C_LINDEN) 
		{
			enabled = FALSE;
		}

		mCategoryCombo->add( LLParcel::getCategoryUIString(cat), ADD_BOTTOM, enabled );
	}*/

	
	mSnapshotCtrl = LLUICtrlFactory::getTexturePickerByName(this, "snapshot_ctrl");
	mSnapshotCtrl->setCommitCallback( onCommitAny );
	mSnapshotCtrl->setCallbackUserData( this );
	mSnapshotCtrl->setAllowNoTexture ( TRUE );
	mSnapshotCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
	mSnapshotCtrl->setNonImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);



	mLocationText = LLUICtrlFactory::getTextBoxByName(this, "Landing Point: (none)");

	mSetBtn = LLUICtrlFactory::getButtonByName(this, "Set");
	mSetBtn->setClickedCallback(onClickSet, this);

	
	mClearBtn = LLUICtrlFactory::getButtonByName(this, "Clear");
	mClearBtn->setClickedCallback(onClickClear, this);


	mLandingTypeCombo = LLUICtrlFactory::getComboBoxByName(this, "landing type");
	childSetCommitCallback("landing type", onCommitAny, this);

	return TRUE;
}


// virtual
LLPanelLandOptions::~LLPanelLandOptions()
{ }


// public
void LLPanelLandOptions::refresh()
{
	LLParcel *parcel = mParcel->getParcel();
	
	if (!parcel)
	{
		mCheckEditObjects	->set(FALSE);
		mCheckEditObjects	->setEnabled(FALSE);

		mCheckEditGroupObjects	->set(FALSE);
		mCheckEditGroupObjects	->setEnabled(FALSE);

		mCheckAllObjectEntry	->set(FALSE);
		mCheckAllObjectEntry	->setEnabled(FALSE);

		mCheckGroupObjectEntry	->set(FALSE);
		mCheckGroupObjectEntry	->setEnabled(FALSE);

		mCheckEditLand		->set(FALSE);
		mCheckEditLand		->setEnabled(FALSE);

		mCheckSafe			->set(FALSE);
		mCheckSafe			->setEnabled(FALSE);

		mCheckFly			->set(FALSE);
		mCheckFly			->setEnabled(FALSE);

		mCheckLandmark		->set(FALSE);
		mCheckLandmark		->setEnabled(FALSE);

		mCheckGroupScripts	->set(FALSE);
		mCheckGroupScripts	->setEnabled(FALSE);

		mCheckOtherScripts	->set(FALSE);
		mCheckOtherScripts	->setEnabled(FALSE);

		mCheckShowDirectory	->set(FALSE);
		mCheckShowDirectory	->setEnabled(FALSE);

		mPushRestrictionCtrl->set(FALSE);
		mPushRestrictionCtrl->setEnabled(FALSE);

		const char* none_string = LLParcel::getCategoryUIString(LLParcel::C_NONE);
		mCategoryCombo->setSimple(none_string);
		mCategoryCombo->setEnabled(FALSE);

		mLandingTypeCombo->setCurrentByIndex(0);
		mLandingTypeCombo->setEnabled(FALSE);

		mSnapshotCtrl->setImageAssetID(LLUUID::null);
		mSnapshotCtrl->setEnabled(FALSE);

		mLocationText->setText("Landing Point: (none)");
		mSetBtn->setEnabled(FALSE);
		mClearBtn->setEnabled(FALSE);

		mAllowPublishCtrl->setEnabled(FALSE);
		mMatureCtrl->setEnabled(FALSE);
		mPublishHelpButton->setEnabled(FALSE);
	}
	else
	{
		// something selected, hooray!

		// Display options
		BOOL can_change_options = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_OPTIONS);
		mCheckEditObjects	->set( parcel->getAllowModify() );
		mCheckEditObjects	->setEnabled( can_change_options );
		
		mCheckEditGroupObjects	->set( parcel->getAllowGroupModify() ||  parcel->getAllowModify());
		mCheckEditGroupObjects	->setEnabled( can_change_options && !parcel->getAllowModify() );		// If others edit is enabled, then this is explicitly enabled.

		mCheckAllObjectEntry	->set( parcel->getAllowAllObjectEntry() );
		mCheckAllObjectEntry	->setEnabled( can_change_options );

		mCheckGroupObjectEntry	->set( parcel->getAllowGroupObjectEntry() ||  parcel->getAllowAllObjectEntry());
		mCheckGroupObjectEntry	->setEnabled( can_change_options && !parcel->getAllowAllObjectEntry() );

		BOOL can_change_terraform = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_EDIT);
		mCheckEditLand		->set( parcel->getAllowTerraform() );
		mCheckEditLand		->setEnabled( can_change_terraform );

		mCheckSafe			->set( !parcel->getAllowDamage() );
		mCheckSafe			->setEnabled( can_change_options );

		mCheckFly			->set( parcel->getAllowFly() );
		mCheckFly			->setEnabled( can_change_options );

		mCheckLandmark		->set( parcel->getAllowLandmark() );
		mCheckLandmark		->setEnabled( can_change_options );

		mCheckGroupScripts	->set( parcel->getAllowGroupScripts() || parcel->getAllowOtherScripts());
		mCheckGroupScripts	->setEnabled( can_change_options && !parcel->getAllowOtherScripts());

		mCheckOtherScripts	->set( parcel->getAllowOtherScripts() );
		mCheckOtherScripts	->setEnabled( can_change_options );

		BOOL can_change_identity = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, 
														GP_LAND_CHANGE_IDENTITY);
		mCheckShowDirectory	->set( parcel->getParcelFlag(PF_SHOW_DIRECTORY));
		mCheckShowDirectory	->setEnabled( can_change_identity );

		mPushRestrictionCtrl->set( parcel->getRestrictPushObject() );
		if(parcel->getRegionPushOverride())
		{
			mPushRestrictionCtrl->setLabel("Restrict Pushing (Region Override)");
			mPushRestrictionCtrl->setEnabled(false);
			mPushRestrictionCtrl->set(TRUE);
		}
		else
		{
			mPushRestrictionCtrl->setLabel("Restrict Pushing");
			mPushRestrictionCtrl->setEnabled(can_change_options);
		}

		// Set by string in case the order in UI doesn't match the order
		// by index.
		LLParcel::ECategory cat = parcel->getCategory();
		const char* category_string = LLParcel::getCategoryUIString(cat);
		mCategoryCombo->setSimple(category_string);
		mCategoryCombo->setEnabled( can_change_identity );

		BOOL can_change_landing_point = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, 
														GP_LAND_SET_LANDING_POINT);
		mLandingTypeCombo->setCurrentByIndex((S32)parcel->getLandingType());
		mLandingTypeCombo->setEnabled( can_change_landing_point );

		mSnapshotCtrl->setImageAssetID(parcel->getSnapshotID());
		mSnapshotCtrl->setEnabled( can_change_identity );

		LLVector3 pos = parcel->getUserLocation();
		if (pos.isExactlyZero())
		{
			mLocationText->setText("Landing Point: (none)");
		}
		else
		{
			char buffer[256];	/*Flawfinder: ignore*/
			snprintf(buffer, sizeof(buffer), "Landing Point: %d, %d, %d",	/*Flawfinder: ignore*/
					llround(pos.mV[VX]),
					llround(pos.mV[VY]),
					llround(pos.mV[VZ]));
			mLocationText->setText(buffer);
		}

		mSetBtn->setEnabled( can_change_landing_point );
		mClearBtn->setEnabled( can_change_landing_point );

		mAllowPublishCtrl->set(parcel->getAllowPublish());
		mAllowPublishCtrl->setEnabled( can_change_identity );
		mMatureCtrl->set(parcel->getMaturePublish());
		mMatureCtrl->setEnabled( can_change_identity );
		mPublishHelpButton->setEnabled( can_change_identity );

		if (gAgent.mAccess < SIM_ACCESS_MATURE)
		{
			// Disable these buttons if they are PG (Teen) users
			mAllowPublishCtrl->setVisible(FALSE);
			mAllowPublishCtrl->setEnabled(FALSE);
			mPublishHelpButton->setVisible(FALSE);
			mPublishHelpButton->setEnabled(FALSE);
			mMatureCtrl->setVisible(FALSE);
			mMatureCtrl->setEnabled(FALSE);
		}
	}
}


// static
void LLPanelLandOptions::onCommitAny(LLUICtrl *ctrl, void *userdata)
{
	LLPanelLandOptions *self = (LLPanelLandOptions *)userdata;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel)
	{
		return;
	}

	// Extract data from UI
	BOOL create_objects		= self->mCheckEditObjects->get();
	BOOL create_group_objects	= self->mCheckEditGroupObjects->get() || self->mCheckEditObjects->get();
	BOOL all_object_entry		= self->mCheckAllObjectEntry->get();
	BOOL group_object_entry	= self->mCheckGroupObjectEntry->get() || self->mCheckAllObjectEntry->get();
	BOOL allow_terraform	= self->mCheckEditLand->get();
	BOOL allow_damage		= !self->mCheckSafe->get();
	BOOL allow_fly			= self->mCheckFly->get();
	BOOL allow_landmark		= self->mCheckLandmark->get();
	BOOL allow_group_scripts	= self->mCheckGroupScripts->get() || self->mCheckOtherScripts->get();
	BOOL allow_other_scripts	= self->mCheckOtherScripts->get();
	BOOL allow_publish		= self->mAllowPublishCtrl->get();
	BOOL mature_publish		= self->mMatureCtrl->get();
	BOOL push_restriction	= self->mPushRestrictionCtrl->get();
	BOOL show_directory		= self->mCheckShowDirectory->get();
	S32  category_index		= self->mCategoryCombo->getCurrentIndex();
	S32  landing_type_index	= self->mLandingTypeCombo->getCurrentIndex();
	LLUUID snapshot_id		= self->mSnapshotCtrl->getImageAssetID();
	LLViewerRegion* region;
	region = gParcelMgr->getSelectionRegion();

	if (!allow_other_scripts && region && region->getAllowDamage())
	{

		gViewerWindow->alertXml("UnableToDisableOutsideScripts");
		return;
	}

	// Push data into current parcel
	parcel->setParcelFlag(PF_CREATE_OBJECTS, create_objects);
	parcel->setParcelFlag(PF_CREATE_GROUP_OBJECTS, create_group_objects);
	parcel->setParcelFlag(PF_ALLOW_ALL_OBJECT_ENTRY, all_object_entry);
	parcel->setParcelFlag(PF_ALLOW_GROUP_OBJECT_ENTRY, group_object_entry);
	parcel->setParcelFlag(PF_ALLOW_TERRAFORM, allow_terraform);
	parcel->setParcelFlag(PF_ALLOW_DAMAGE, allow_damage);
	parcel->setParcelFlag(PF_ALLOW_FLY, allow_fly);
	parcel->setParcelFlag(PF_ALLOW_LANDMARK, allow_landmark);
	parcel->setParcelFlag(PF_ALLOW_GROUP_SCRIPTS, allow_group_scripts);
	parcel->setParcelFlag(PF_ALLOW_OTHER_SCRIPTS, allow_other_scripts);
	parcel->setParcelFlag(PF_SHOW_DIRECTORY, show_directory);
	parcel->setParcelFlag(PF_ALLOW_PUBLISH, allow_publish);
	parcel->setParcelFlag(PF_MATURE_PUBLISH, mature_publish);
	parcel->setParcelFlag(PF_RESTRICT_PUSHOBJECT, push_restriction);
	parcel->setCategory((LLParcel::ECategory)category_index);
	parcel->setLandingType((LLParcel::ELandingType)landing_type_index);
	parcel->setSnapshotID(snapshot_id);

	// Send current parcel data upstream to server
	gParcelMgr->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	self->refresh();
}


// static
void LLPanelLandOptions::onClickSet(void* userdata)
{
	LLPanelLandOptions* self = (LLPanelLandOptions*)userdata;

	LLParcel* selected_parcel = self->mParcel->getParcel();
	if (!selected_parcel) return;

	LLParcel* agent_parcel = gParcelMgr->getAgentParcel();
	if (!agent_parcel) return;

	if (agent_parcel->getLocalID() != selected_parcel->getLocalID())
	{
		gViewerWindow->alertXml("MustBeInParcel");
		return;
	}

	LLVector3 pos_region = gAgent.getPositionAgent();
	selected_parcel->setUserLocation(pos_region);
	selected_parcel->setUserLookAt(gAgent.getFrameAgent().getAtAxis());

	gParcelMgr->sendParcelPropertiesUpdate(selected_parcel);

	self->refresh();
}

void LLPanelLandOptions::onClickClear(void* userdata)
{
	LLPanelLandOptions* self = (LLPanelLandOptions*)userdata;

	LLParcel* selected_parcel = self->mParcel->getParcel();
	if (!selected_parcel) return;

	// yes, this magic number of 0,0,0 means that it is clear
	LLVector3 zero_vec(0.f, 0.f, 0.f);
	selected_parcel->setUserLocation(zero_vec);
	selected_parcel->setUserLookAt(zero_vec);

	gParcelMgr->sendParcelPropertiesUpdate(selected_parcel);

	self->refresh();
}

// static
void LLPanelLandOptions::onClickPublishHelp(void*)
{
	gViewerWindow->alertXml("ClickPublishHelpLand");
}

//---------------------------------------------------------------------------
// LLPanelLandMedia
//---------------------------------------------------------------------------

LLPanelLandMedia::LLPanelLandMedia(LLParcelSelectionHandle& parcel)
:	LLPanel("land_media_panel"), mParcel(parcel)
{
}




BOOL LLPanelLandMedia::postBuild()
{
		
	mCheckSoundLocal = LLUICtrlFactory::getCheckBoxByName(this, "check sound local");
	childSetCommitCallback("check sound local", onCommitAny, this);

	mMusicURLEdit = LLUICtrlFactory::getLineEditorByName(this, "music_url");
	childSetCommitCallback("music_url", onCommitAny, this);


	mMediaTextureCtrl = LLUICtrlFactory::getTexturePickerByName(this, "media texture");
	mMediaTextureCtrl->setCommitCallback( onCommitAny );
	mMediaTextureCtrl->setCallbackUserData( this );
	mMediaTextureCtrl->setAllowNoTexture ( TRUE );
	mMediaTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
	mMediaTextureCtrl->setNonImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);

	mMediaAutoScaleCheck = LLUICtrlFactory::getCheckBoxByName(this, "media_auto_scale");
	childSetCommitCallback("media_auto_scale", onCommitAny, this);

	mMediaURLEdit = LLUICtrlFactory::getLineEditorByName(this, "media_url");
	childSetCommitCallback("media_url", onCommitAny, this);
	
	return TRUE;
}


// virtual
LLPanelLandMedia::~LLPanelLandMedia()
{ }


// public
void LLPanelLandMedia::refresh()
{
	LLParcel *parcel = mParcel->getParcel();

	if (!parcel)
	{
		mCheckSoundLocal->set(FALSE);
		mCheckSoundLocal->setEnabled(FALSE);

		mMusicURLEdit->setText("");
		mMusicURLEdit->setEnabled(FALSE);

		mMediaURLEdit->setText("");
		mMediaURLEdit->setEnabled(FALSE);

		mMediaAutoScaleCheck->set ( FALSE );
		mMediaAutoScaleCheck->setEnabled(FALSE);

		mMediaTextureCtrl->clear();
		mMediaTextureCtrl->setEnabled(FALSE);

		#if 0
		mMediaStopButton->setEnabled ( FALSE );
		mMediaStartButton->setEnabled ( FALSE );
		#endif
	}
	else
	{
		// something selected, hooray!

		// Display options
		BOOL can_change_media = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_CHANGE_MEDIA);

		mCheckSoundLocal->set( parcel->getSoundLocal() );
		mCheckSoundLocal->setEnabled( can_change_media );

		// don't display urls if you're not able to change it
		// much requested change in forums so people can't 'steal' urls
		// NOTE: bug#2009 means this is still vunerable - however, bug 
		// should be closed since this bug opens up major security issues elsewhere.
		if ( can_change_media )
		{
			mMusicURLEdit->setDrawAsterixes ( FALSE );
			mMediaURLEdit->setDrawAsterixes ( FALSE );
		}
		else
		{
				mMusicURLEdit->setDrawAsterixes ( TRUE );
				mMediaURLEdit->setDrawAsterixes ( TRUE );
			}

		mMusicURLEdit->setText(parcel->getMusicURL());
		mMusicURLEdit->setEnabled( can_change_media );

		mMediaURLEdit->setText(parcel->getMediaURL());
		mMediaURLEdit->setEnabled( can_change_media );

		mMediaAutoScaleCheck->set ( parcel->getMediaAutoScale () );
		mMediaAutoScaleCheck->setEnabled ( can_change_media );

		LLUUID tmp = parcel->getMediaID();
		mMediaTextureCtrl->setImageAssetID ( parcel->getMediaID() );
		mMediaTextureCtrl->setEnabled( can_change_media );

		#if 0
		// there is a media url and a media texture selected
		if ( ( ! ( std::string ( parcel->getMediaURL() ).empty () ) ) && ( ! ( parcel->getMediaID ().isNull () ) ) )
		{
			// turn on transport controls if allowed for this parcel
			mMediaStopButton->setEnabled ( editable );
			mMediaStartButton->setEnabled ( editable );
		}
		else
		{
			// no media url or no media texture
			mMediaStopButton->setEnabled ( FALSE );
			mMediaStartButton->setEnabled ( FALSE );
		};
		#endif
	}
}

// static
void LLPanelLandMedia::onCommitAny(LLUICtrl *ctrl, void *userdata)
{
	LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel)
	{
		return;
	}

	// Extract data from UI
	BOOL sound_local		= self->mCheckSoundLocal->get();
	std::string music_url	= self->mMusicURLEdit->getText();
	std::string media_url	= self->mMediaURLEdit->getText();
	U8 media_auto_scale		= self->mMediaAutoScaleCheck->get();
	LLUUID media_id			= self->mMediaTextureCtrl->getImageAssetID();

	// Push data into current parcel
	parcel->setParcelFlag(PF_SOUND_LOCAL, sound_local);
	parcel->setMusicURL(music_url.c_str());
	parcel->setMediaURL(media_url.c_str());
	parcel->setMediaID(media_id);
	parcel->setMediaAutoScale ( media_auto_scale );

	// Send current parcel data upstream to server
	gParcelMgr->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	self->refresh();
}

void LLPanelLandMedia::onClickStopMedia ( void* data )
{
	LLMediaEngine::getInstance ()->stop ();
}

void LLPanelLandMedia::onClickStartMedia ( void* data )
{
	// force a commit
	gFocusMgr.setKeyboardFocus ( NULL, NULL );

	// force a reload
	LLMediaEngine::getInstance ()->convertImageAndLoadUrl ( true, false, std::string());
}

//---------------------------------------------------------------------------
// LLPanelLandAccess
//---------------------------------------------------------------------------

LLPanelLandAccess::LLPanelLandAccess(LLParcelSelectionHandle& parcel)
:	LLPanel("land_access_panel"), mParcel(parcel)
{
}



BOOL LLPanelLandAccess::postBuild()
{

	
	mCheckGroup = LLUICtrlFactory::getCheckBoxByName(this, "GroupCheck");
	childSetCommitCallback("GroupCheck", onCommitAny, this);
	
	mCheckAccess = LLUICtrlFactory::getCheckBoxByName(this, "AccessCheck");
	childSetCommitCallback("AccessCheck", onCommitAny, this);

	mListAccess = LLUICtrlFactory::getNameListByName(this, "AccessList");

	mBtnAddAccess = LLUICtrlFactory::getButtonByName(this, "Add...");

	mBtnAddAccess->setClickedCallback(onClickAdd, this);

	mBtnRemoveAccess = LLUICtrlFactory::getButtonByName(this, "Remove");

	mBtnRemoveAccess->setClickedCallback(onClickRemove, this);
	
	mCheckPass = LLUICtrlFactory::getCheckBoxByName(this, "PassCheck");
	childSetCommitCallback("PassCheck", onCommitAny, this);

	
	mSpinPrice = LLUICtrlFactory::getSpinnerByName(this, "PriceSpin");
	childSetCommitCallback("PriceSpin", onCommitAny, this);
	
	mSpinHours = LLUICtrlFactory::getSpinnerByName(this, "HoursSpin");
	childSetCommitCallback("HoursSpin", onCommitAny, this);


	return TRUE;
}


LLPanelLandAccess::~LLPanelLandAccess()
{ 
}

void LLPanelLandAccess::refresh()
{
	mListAccess->deleteAllItems();

	LLParcel *parcel = mParcel->getParcel();

	if (parcel)
	{
		char label[256];	/*Flawfinder: ignore*/

		// Display options
		BOOL use_group = parcel->getParcelFlag(PF_USE_ACCESS_GROUP);
		mCheckGroup->set( use_group );

		char group_name[MAX_STRING];	/*Flawfinder: ignore*/
		gCacheName->getGroupName(parcel->getGroupID(), group_name);
		snprintf(label, sizeof(label), "Group: %s", group_name);	/*Flawfinder: ignore*/
		mCheckGroup->setLabel( label );

		S32 count = parcel->mAccessList.size();

		BOOL use_list = parcel->getParcelFlag(PF_USE_ACCESS_LIST);
		mCheckAccess->set( use_list );
		snprintf(label, sizeof(label), "Avatars: (%d listed, %d max)",	/*Flawfinder: ignore*/
				count, PARCEL_MAX_ACCESS_LIST);
		mCheckAccess->setLabel( label );

		access_map_const_iterator cit = parcel->mAccessList.begin();
		access_map_const_iterator end = parcel->mAccessList.end();

		for (; cit != end; ++cit)
		{
			const LLAccessEntry& entry = (*cit).second;
			LLString suffix;
			if (entry.mTime != 0)
			{
				S32 now = time(NULL);
				S32 seconds = entry.mTime - now;
				if (seconds < 0) seconds = 0;
				suffix.assign(" (");
				if (seconds >= 120)
				{
					char buf[30];	/*Flawfinder: ignore*/
					snprintf(buf, sizeof(buf), "%d minutes", (seconds/60));	/*Flawfinder: ignore*/
					suffix.append(buf);
				}
				else if (seconds >= 60)
				{
					suffix.append("1 minute");
				}
				else
				{
					char buf[30];		/*Flawfinder: ignore*/
					snprintf(buf, sizeof(buf), "%d seconds", seconds);	/*Flawfinder: ignore*/
					suffix.append(buf);
				}
				suffix.append(" remaining)");
			}
			mListAccess->addNameItem(entry.mID, ADD_BOTTOM, TRUE, suffix);
		}

		BOOL can_manage_allowed = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_MANAGE_ALLOWED);

		BOOL enable_add = can_manage_allowed && (count < PARCEL_MAX_ACCESS_LIST);
		mBtnAddAccess->setEnabled(enable_add);

		BOOL enable_remove = can_manage_allowed && (count > 0);
		mBtnRemoveAccess->setEnabled(enable_remove);

		// Can only sell passes when limiting the access.
		BOOL can_manage_passes = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_MANAGE_PASSES);
		mCheckPass->setEnabled( (use_group || use_list) && can_manage_passes );

		BOOL use_pass = parcel->getParcelFlag(PF_USE_PASS_LIST);
		mCheckPass->set( use_pass );

		BOOL enable_pass = can_manage_passes && use_pass;
		mSpinPrice->setEnabled( enable_pass );
		mSpinHours->setEnabled( enable_pass );

		S32 pass_price = parcel->getPassPrice();
		mSpinPrice->set( F32(pass_price) );

		F32 pass_hours = parcel->getPassHours();
		mSpinHours->set( pass_hours );

		mCheckGroup->setEnabled( can_manage_allowed );
		mCheckAccess->setEnabled( can_manage_allowed );

	}
	else
	{
		mCheckGroup->set(FALSE);
		mCheckGroup->setLabel("Group:");
		mCheckAccess->set(FALSE);
		mCheckAccess->setLabel("Avatars:");
		mBtnAddAccess->setEnabled(FALSE);
		mBtnRemoveAccess->setEnabled(FALSE);
		mSpinPrice->set((F32)PARCEL_PASS_PRICE_DEFAULT);
		mSpinPrice->setEnabled(FALSE);
		mSpinHours->set( PARCEL_PASS_HOURS_DEFAULT );
		mSpinHours->setEnabled(FALSE);
		mCheckGroup->setEnabled(FALSE);
		mCheckAccess->setEnabled(FALSE);
	}
}

// public
void LLPanelLandAccess::refreshNames()
{
	LLParcel* parcel = mParcel->getParcel();
	char group_name[DB_GROUP_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
	group_name[0] = '\0';
	if(parcel)
	{
		gCacheName->getGroupName(parcel->getGroupID(), group_name);
	}
	char label[MAX_STRING];		/*Flawfinder: ignore*/
	snprintf(label, sizeof(label), "Group: %s", group_name);	/*Flawfinder: ignore*/
	mCheckGroup->setLabel(label);
}


// virtual
void LLPanelLandAccess::draw()
{
	refreshNames();
	LLPanel::draw();
}


void LLPanelLandAccess::onAccessLevelChange(LLUICtrl*, void *userdata)
{
	LLPanelLandAccess::onCommitAny(NULL, userdata);
}

// static
void LLPanelLandAccess::onCommitAny(LLUICtrl *ctrl, void *userdata)
{
	LLPanelLandAccess *self = (LLPanelLandAccess *)userdata;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel)
	{
		return;
	}

	// Extract data from UI
	BOOL use_access_group	= self->mCheckGroup->get();
	BOOL use_access_list	= self->mCheckAccess->get();
	BOOL use_pass_list		= self->mCheckPass->get();
	


	// Must be limiting access to sell passes
	if (!use_access_group && !use_access_list)
	{
		use_pass_list = FALSE;
	}

	S32 pass_price = llfloor(self->mSpinPrice->get());
	F32 pass_hours = self->mSpinHours->get();

	// Validate extracted data

	// Push data into current parcel
	parcel->setParcelFlag(PF_USE_ACCESS_GROUP,	use_access_group);
	parcel->setParcelFlag(PF_USE_ACCESS_LIST,	use_access_list);
	parcel->setParcelFlag(PF_USE_PASS_LIST,		use_pass_list);

	parcel->setPassPrice( pass_price );
	parcel->setPassHours( pass_hours );

	// Send current parcel data upstream to server
	gParcelMgr->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	self->refresh();
}

// static
void LLPanelLandAccess::onClickAdd(void* data)
{
	LLPanelLandAccess* panelp = (LLPanelLandAccess*)data;
	gFloaterView->getParentFloater(panelp)->addDependentFloater(LLFloaterAvatarPicker::show(callbackAvatarID, data) );
}

// static
void LLPanelLandAccess::callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata)
{
	LLPanelLandAccess* self = (LLPanelLandAccess*)userdata;
	if (names.empty() || ids.empty()) return;
	self->addAvatar(ids[0]);
}


void LLPanelLandAccess::addAvatar(LLUUID id)
{
	LLParcel* parcel = mParcel->getParcel();
	if (!parcel) return;

	parcel->addToAccessList(id, 0);

	gParcelMgr->sendParcelAccessListUpdate(AL_ACCESS);

	refresh();
}


// static
void LLPanelLandAccess::onClickRemove(void* data)
{
	LLPanelLandAccess* self = (LLPanelLandAccess*)data;
	if (!self) return;

	LLScrollListItem* item = self->mListAccess->getFirstSelected();
	if (!item) return;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel) return;

	const LLUUID& agent_id = item->getUUID();

	parcel->removeFromAccessList(agent_id);

	gParcelMgr->sendParcelAccessListUpdate(AL_ACCESS);

	self->refresh();
}



//---------------------------------------------------------------------------
// LLPanelLandBan
//---------------------------------------------------------------------------
LLPanelLandBan::LLPanelLandBan(LLParcelSelectionHandle& parcel)
:	LLPanel("land_ban_panel"), mParcel(parcel)
{

}



BOOL LLPanelLandBan::postBuild()
{

	mCheck = LLUICtrlFactory::getCheckBoxByName(this, "LandBanCheck");
	childSetCommitCallback("LandBanCheck", onCommitAny, this);
	
	mList = LLUICtrlFactory::getNameListByName(this, "LandBanList");

	mBtnAdd = LLUICtrlFactory::getButtonByName(this, "Add...");
	
	mBtnAdd->setClickedCallback(onClickAdd, this);

	mBtnRemove = LLUICtrlFactory::getButtonByName(this, "Remove");
	
	mBtnRemove->setClickedCallback(onClickRemove, this);

	mCheckDenyAnonymous = LLUICtrlFactory::getCheckBoxByName(this, "DenyAnonymousCheck");
	childSetCommitCallback("DenyAnonymousCheck", onCommitAny, this);

	mCheckDenyIdentified = LLUICtrlFactory::getCheckBoxByName(this, "DenyIdentifiedCheck");
	childSetCommitCallback("DenyIdentifiedCheck", onCommitAny, this);

	mCheckDenyTransacted = LLUICtrlFactory::getCheckBoxByName(this, "DenyTransactedCheck");
	childSetCommitCallback("DenyTransactedCheck", onCommitAny, this);

	return TRUE;

}


LLPanelLandBan::~LLPanelLandBan()
{ }

void LLPanelLandBan::refresh()
{
	mList->deleteAllItems();

	LLParcel *parcel = mParcel->getParcel();

	if (parcel)
	{
		char label[256];	/*Flawfinder: ignore*/

		// Display options

		S32 count = parcel->mBanList.size();

		BOOL use_ban = parcel->getParcelFlag(PF_USE_BAN_LIST);
		mCheck->set( use_ban );

		snprintf(label, sizeof(label), "Ban these avatars: (%d listed, %d max)",	/*Flawfinder: ignore*/
				count, PARCEL_MAX_ACCESS_LIST);
		mCheck->setLabel( label );

		access_map_const_iterator cit = parcel->mBanList.begin();
		access_map_const_iterator end = parcel->mBanList.end();
		for ( ; cit != end; ++cit)
		{
			const LLAccessEntry& entry = (*cit).second;
			LLString suffix;
			if (entry.mTime != 0)
			{
				S32 now = time(NULL);
				S32 seconds = entry.mTime - now;
				if (seconds < 0) seconds = 0;
				suffix.assign(" (");
				if (seconds >= 120)
				{
					char buf[30];		/*Flawfinder: ignore*/
					snprintf(buf, sizeof(buf), "%d minutes", (seconds/60));	/*Flawfinder: ignore*/
					suffix.append(buf);
				}
				else if (seconds >= 60)
				{
					suffix.append("1 minute");
				}
				else
				{
					char buf[30];	/*Flawfinder: ignore*/
					snprintf(buf, sizeof(buf), "%d seconds", seconds);	/*Flawfinder: ignore*/
					suffix.append(buf);
				}
				suffix.append(" remaining)");
			}
			mList->addNameItem(entry.mID, ADD_BOTTOM, TRUE, suffix);
		}

		BOOL can_manage_banned = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_MANAGE_BANNED);
		mCheck->setEnabled( can_manage_banned );
		mCheckDenyAnonymous->setEnabled( FALSE );
		mCheckDenyIdentified->setEnabled( FALSE );
		mCheckDenyTransacted->setEnabled( FALSE );

		if(parcel->getRegionDenyAnonymousOverride())
		{
			mCheckDenyAnonymous->set(TRUE);
		}
		else if(can_manage_banned)
		{
			mCheckDenyAnonymous->setEnabled(TRUE);
			mCheckDenyAnonymous->set(parcel->getParcelFlag(PF_DENY_ANONYMOUS));
		}
		if(parcel->getRegionDenyIdentifiedOverride())
		{
			mCheckDenyIdentified->set(TRUE);
		}
		else if(can_manage_banned)
		{
			mCheckDenyIdentified->setEnabled(TRUE);
			mCheckDenyIdentified->set(parcel->getParcelFlag(PF_DENY_IDENTIFIED));
		}
		if(parcel->getRegionDenyTransactedOverride())
		{
			mCheckDenyTransacted->set(TRUE);
		}
		else if(can_manage_banned)
		{
			mCheckDenyTransacted->setEnabled(TRUE);
			mCheckDenyTransacted->set(parcel->getParcelFlag(PF_DENY_TRANSACTED));
		}


		BOOL enable_add = can_manage_banned && (count < PARCEL_MAX_ACCESS_LIST);
		mBtnAdd->setEnabled(enable_add);

		BOOL enable_remove = can_manage_banned && (count > 0);
		mBtnRemove->setEnabled(enable_remove);
	}
	else
	{
		mCheck->set(FALSE);
		mCheck->setLabel("Ban these avatars:");
		mCheck->setEnabled(FALSE);
		mBtnAdd->setEnabled(FALSE);
		mBtnRemove->setEnabled(FALSE);
		mCheckDenyAnonymous->set(FALSE);
		mCheckDenyAnonymous->setEnabled(FALSE);
		mCheckDenyIdentified->set(FALSE);
		mCheckDenyIdentified->setEnabled(FALSE);
		mCheckDenyTransacted->set(FALSE);
		mCheckDenyTransacted->setEnabled(FALSE);
	}
}

// static
void LLPanelLandBan::onCommitAny(LLUICtrl *ctrl, void *userdata)
{
	LLPanelLandBan *self = (LLPanelLandBan*)userdata;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel)
	{
		return;
	}

	// Extract data from UI
	BOOL use_ban_list = self->mCheck->get();
	BOOL deny_access_anonymous = self->mCheckDenyAnonymous->get();
	BOOL deny_access_identified = self->mCheckDenyIdentified->get();
	BOOL deny_access_transacted = self->mCheckDenyTransacted->get();	

	// Push data into current parcel
	parcel->setParcelFlag(PF_USE_BAN_LIST,		use_ban_list);
	parcel->setParcelFlag(PF_DENY_ANONYMOUS, deny_access_anonymous);
	parcel->setParcelFlag(PF_DENY_IDENTIFIED, deny_access_identified);
	parcel->setParcelFlag(PF_DENY_TRANSACTED, deny_access_transacted);

	// Send current parcel data upstream to server
	gParcelMgr->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	self->refresh();
}

// static
void LLPanelLandBan::onClickAdd(void* data)
{
	LLPanelLandBan* panelp = (LLPanelLandBan*)data;
	gFloaterView->getParentFloater(panelp)->addDependentFloater(LLFloaterAvatarPicker::show(callbackAvatarID, data) );
}

// static
void LLPanelLandBan::callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata)
{
	LLPanelLandBan* self = (LLPanelLandBan*)userdata;
	if (names.empty() || ids.empty()) return;
	self->addAvatar(ids[0]);
}


void LLPanelLandBan::addAvatar(LLUUID id)
{
	LLParcel* parcel = mParcel->getParcel();
	if (!parcel) return;

	parcel->addToBanList(id, 0);

	gParcelMgr->sendParcelAccessListUpdate(AL_BAN);

	refresh();
}


// static
void LLPanelLandBan::onClickRemove(void* data)
{
	LLPanelLandBan* self = (LLPanelLandBan*)data;
	if (!self) return;

	LLScrollListItem* item = self->mList->getFirstSelected();
	if (!item) return;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel) return;

	const LLUUID& agent_id = item->getUUID();

	parcel->removeFromBanList(agent_id);

	gParcelMgr->sendParcelAccessListUpdate(AL_BAN);

	self->refresh();
}

//---------------------------------------------------------------------------
// LLPanelLandRenters
//---------------------------------------------------------------------------
LLPanelLandRenters::LLPanelLandRenters(LLParcelSelectionHandle& parcel)
:	LLPanel("landrenters", LLRect(0,500,500,0)), mParcel(parcel)
{
	const S32 BTN_WIDTH = 64;

	S32 x = LEFT;
	S32 y = mRect.getHeight() - VPAD;

	LLCheckBoxCtrl* check = NULL;
	LLButton* btn = NULL;
	LLNameListCtrl* list = NULL;

	check = new LLCheckBoxCtrl("RentersCheck",
							LLRect(RULER0, y, RIGHT, y-LINE),
							"Rent space to these avatars:");
	addChild(check);
	mCheckRenters = check;

	y -= VPAD + LINE;

	const S32 ITEMS = 5;
	const BOOL NO_MULTIPLE_SELECT = FALSE;

	list = new LLNameListCtrl("RentersList",
							LLRect(RULER05, y, RIGHT, y-LINE*ITEMS),
							NULL, NULL,
							NO_MULTIPLE_SELECT);
	list->addSimpleItem("foo tester");
	list->addSimpleItem("bar tester");
	list->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
	addChild(list);
	mListRenters = list;

	y -= VPAD + LINE*ITEMS;

	x = RULER05;
	btn = new LLButton("Add...",
						LLRect(x, y, x+BTN_WIDTH, y-LINE),
						"",
						onClickAdd, this);
	btn->setFont( LLFontGL::sSansSerifSmall );
	btn->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
	addChild(btn);
	mBtnAddRenter = btn;

	x += HPAD + BTN_WIDTH;

	btn = new LLButton("Remove",
						LLRect(x, y, x+BTN_WIDTH, y-LINE),
						"",
						onClickRemove, this);
	btn->setFont( LLFontGL::sSansSerifSmall );
	btn->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
	addChild(btn);
	mBtnRemoveRenter = btn;
}

LLPanelLandRenters::~LLPanelLandRenters()
{ }

void LLPanelLandRenters::refresh()
{ }

// static
void LLPanelLandRenters::onClickAdd(void*)
{
}

// static
void LLPanelLandRenters::onClickRemove(void*)
{
}

//---------------------------------------------------------------------------
// LLPanelLandCovenant
//---------------------------------------------------------------------------
LLPanelLandCovenant::LLPanelLandCovenant(LLParcelSelectionHandle& parcel)
:	LLPanel("land_covenant_panel"), mParcel(parcel)
{	
}

LLPanelLandCovenant::~LLPanelLandCovenant()
{
}

BOOL LLPanelLandCovenant::postBuild()
{
	refresh();
	return TRUE;
}

// virtual
void LLPanelLandCovenant::refresh()
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
	
	// send EstateCovenantInfo message
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessage("EstateCovenantRequest");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->sendReliable(region->getHost());
}

// static
void LLPanelLandCovenant::updateCovenantText(const std::string &string)
{
	LLPanelLandCovenant* self = LLFloaterLand::getCurrentPanelLandCovenant();
	if (self)
	{
		LLViewerTextEditor* editor = (LLViewerTextEditor*)self->getChildByName("covenant_editor");
		if (editor)
		{
			editor->setHandleEditKeysDirectly(TRUE);
			editor->setText(string);
		}
	}
}

// static
void LLPanelLandCovenant::updateEstateName(const std::string& name)
{
	LLPanelLandCovenant* self = LLFloaterLand::getCurrentPanelLandCovenant();
	if (self)
	{
		LLTextBox* editor = (LLTextBox*)self->getChildByName("estate_name_text");
		if (editor) editor->setText(name);
	}
}

// static
void LLPanelLandCovenant::updateLastModified(const std::string& text)
{
	LLPanelLandCovenant* self = LLFloaterLand::getCurrentPanelLandCovenant();
	if (self)
	{
		LLTextBox* editor = (LLTextBox*)self->getChildByName("covenant_timestamp_text");
		if (editor) editor->setText(text);
	}
}

// static
void LLPanelLandCovenant::updateEstateOwnerName(const std::string& name)
{
	LLPanelLandCovenant* self = LLFloaterLand::getCurrentPanelLandCovenant();
	if (self)
	{
		LLTextBox* editor = (LLTextBox*)self->getChildByName("estate_owner_text");
		if (editor) editor->setText(name);
	}
}
