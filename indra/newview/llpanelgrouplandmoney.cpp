/** 
 * @file llpanelgrouplandmoney.cpp
 * @brief Panel for group land and L$.
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

#include "llpanelgrouplandmoney.h"

#include "lluiconstants.h"
#include "roles_constants.h"

#include "llparcel.h"
#include "llqueryflags.h"

#include "llagent.h"
#include "lldateutil.h"
#include "lliconctrl.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llproductinforequest.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "lltextbox.h"
#include "lltabcontainer.h"
#include "lltexteditor.h"
#include "lltrans.h"
#include "lltransactiontypes.h"
#include "lltrans.h"
#include "lluictrlfactory.h"

#include "llstatusbar.h"
#include "llfloaterworldmap.h"
#include "llviewermessage.h"

static LLRegisterPanelClassWrapper<LLPanelGroupLandMoney> t_panel_group_money("panel_group_land_money");



////////////////////////////////////////////////////////////////////////////
//*************************************************
//** LLGroupMoneyTabEventHandler::impl Functions **
//*************************************************

class LLGroupMoneyTabEventHandlerImpl
{
public:
	LLGroupMoneyTabEventHandlerImpl(LLButton* earlier_buttonp,
		 LLButton* later_buttonp,
		 LLTextEditor* text_editorp,
		 LLPanel* tabpanelp,
		 const std::string& loading_text,
		 S32 interval_length_days,
		 S32 max_interval_days);
	~LLGroupMoneyTabEventHandlerImpl();

	bool getCanClickLater();
	bool getCanClickEarlier();

	void updateButtons();

	void setGroupID(const LLUUID&	group_id) { mGroupID = group_id; } ;
	const LLUUID&	getGroupID() const { return mGroupID;} 


//member variables
public:
	LLUUID mPanelID;
	LLUUID mGroupID;

	LLPanel* mTabPanelp;

	int mIntervalLength;
	int mMaxInterval;
	int mCurrentInterval;

	LLTextEditor* mTextEditorp;
	LLButton*     mEarlierButtonp;
	LLButton*     mLaterButtonp;

	std::string mLoadingText;
};


class LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyTabEventHandler(LLButton* earlier_button,
								LLButton* later_button,
								LLTextEditor* text_editor,
								LLTabContainer* tab_containerp,
								LLPanel* panelp,
								const std::string& loading_text,
								S32 interval_length_days,
								S32 max_interval_days);
	virtual ~LLGroupMoneyTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);

	virtual void onClickEarlier();
	virtual void onClickLater();
	virtual void onClickTab();

	void setGroupID(const LLUUID&	group_id) { if(mImplementationp) mImplementationp->setGroupID(group_id); } ;

	static void clickEarlierCallback(void* data);
	static void clickLaterCallback(void* data);



	static LLMap<LLUUID, LLGroupMoneyTabEventHandler*> sInstanceIDs;
	static std::map<LLPanel*, LLGroupMoneyTabEventHandler*> sTabsToHandlers;
protected:
	LLGroupMoneyTabEventHandlerImpl* mImplementationp;
};

class LLGroupMoneyDetailsTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyDetailsTabEventHandler(LLButton* earlier_buttonp,
									   LLButton* later_buttonp,
									   LLTextEditor* text_editorp,
									   LLTabContainer* tab_containerp,
									   LLPanel* panelp,
									   const std::string& loading_text
									   );
	virtual ~LLGroupMoneyDetailsTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);
};


class LLGroupMoneySalesTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneySalesTabEventHandler(LLButton* earlier_buttonp,
									 LLButton* later_buttonp,
									 LLTextEditor* text_editorp,
									 LLTabContainer* tab_containerp,
									 LLPanel* panelp,
									 const std::string& loading_text
									 );
	virtual ~LLGroupMoneySalesTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);
};

class LLGroupMoneyPlanningTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyPlanningTabEventHandler(LLTextEditor* text_editor,
										LLTabContainer* tab_containerp,
										LLPanel* panelp,
										const std::string& loading_text
										);
	virtual ~LLGroupMoneyPlanningTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);
};

////////////////////////////////////////////////////////////////////////////

class LLPanelGroupLandMoney::impl
{
public:
	impl(LLPanelGroupLandMoney& panel); //constructor
	virtual ~impl();

	void requestGroupLandInfo();

	int getStoredContribution();
	void setYourContributionTextField(int contrib);
	void setYourMaxContributionTextBox(int max);

	virtual void onMapButton();
	virtual bool applyContribution();
	virtual void processGroupLand(LLMessageSystem* msg);

	static void mapCallback(void* data);
	static void contributionCommitCallback(LLUICtrl* ctrl, void* userdata);
	static void contributionKeystrokeCallback(LLLineEditor* caller, void* userdata);

//member variables
public:
	LLPanelGroupLandMoney& mPanel;
	
	LLTextBox* mGroupOverLimitTextp;
	LLIconCtrl* mGroupOverLimitIconp;

	LLLineEditor* mYourContributionEditorp;

	LLButton* mMapButtonp;

	LLGroupMoneyTabEventHandler* mMoneyDetailsTabEHp;
	LLGroupMoneyTabEventHandler* mMoneyPlanningTabEHp;
	LLGroupMoneyTabEventHandler* mMoneySalesTabEHp;

	LLScrollListCtrl* mGroupParcelsp;

	LLUUID mTransID;

	bool mBeenActivated;
	bool mNeedsSendGroupLandRequest;
	bool mNeedsApply;

	std::string mCantViewParcelsText;
	std::string mCantViewAccountsText;
	std::string mEmptyParcelsText;
};

//*******************************************
//** LLPanelGroupLandMoney::impl Functions **
//*******************************************
LLPanelGroupLandMoney::impl::impl(LLPanelGroupLandMoney& panel)
	: mPanel(panel)
{
	mTransID = LLUUID::null;

	mBeenActivated = false;
	mNeedsSendGroupLandRequest = true;
	mNeedsApply = false;

	mYourContributionEditorp = NULL;
	mMapButtonp = NULL;
	mGroupParcelsp = NULL;
	mGroupOverLimitTextp = NULL;
	mGroupOverLimitIconp = NULL;

	mMoneySalesTabEHp    = NULL;
	mMoneyPlanningTabEHp = NULL;
	mMoneyDetailsTabEHp  = NULL;
}

LLPanelGroupLandMoney::impl::~impl()
{
	if ( mMoneySalesTabEHp ) delete mMoneySalesTabEHp;
	if ( mMoneyDetailsTabEHp ) delete mMoneyDetailsTabEHp;
	if ( mMoneyPlanningTabEHp ) delete mMoneyPlanningTabEHp;
}

void LLPanelGroupLandMoney::impl::requestGroupLandInfo()
{
	U32 query_flags = DFQ_GROUP_OWNED;

	mTransID.generate();
	mGroupParcelsp->deleteAllItems();

	send_places_query(mPanel.mGroupID, mTransID, "", query_flags, LLParcel::C_ANY, "");
}

void LLPanelGroupLandMoney::impl::onMapButton()
{
	LLScrollListItem* itemp;
	itemp = mGroupParcelsp->getFirstSelected();
	if (!itemp) return;

	const LLScrollListCell* cellp;
	cellp = itemp->getColumn(itemp->getNumColumns() - 1);	// hidden column is last

	F32 global_x = 0.f;
	F32 global_y = 0.f;
	sscanf(cellp->getValue().asString().c_str(), "%f %f", &global_x, &global_y);

	// Hack: Use the agent's z-height
	F64 global_z = gAgent.getPositionGlobal().mdV[VZ];

	LLVector3d pos_global(global_x, global_y, global_z);
	LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
    if(worldmap_instance)
	{
		worldmap_instance->trackLocation(pos_global);
		LLFloaterReg::showInstance("world_map", "center");
	}
}

bool LLPanelGroupLandMoney::impl::applyContribution()
{
	// calculate max donation, which is sum of available and current.
	S32 your_contribution = 0;
	S32 sqm_avail;

	your_contribution = getStoredContribution();
	sqm_avail = your_contribution;
	
	if(gStatusBar)
	{
		sqm_avail += gStatusBar->getSquareMetersLeft();
	}

	// get new contribution and compare to available
	S32 new_contribution = atoi(mYourContributionEditorp->getText().c_str());

	if( new_contribution != your_contribution &&
		new_contribution >= 0 && 
	    new_contribution <= sqm_avail )
	{
		// update group info and server
		if(!gAgent.setGroupContribution(mPanel.mGroupID, new_contribution))
		{
			// should never happen...
			llwarns << "Unable to set contribution." << llendl;
			return false;
		}
	}
	else
	{
		//TODO: throw up some error message here and return?  right now we just
		//fail silently and force the previous value  -jwolk
		new_contribution =  your_contribution;
	}

	//set your contribution
	setYourContributionTextField(new_contribution);

	return true;
}

// Retrieves the land contribution for this agent that is currently
// stored in the database, NOT what is currently entered in the text field
int LLPanelGroupLandMoney::impl::getStoredContribution()
{
	LLGroupData group_data;

	group_data.mContribution = 0;
	gAgent.getGroupData(mPanel.mGroupID, group_data);

	return group_data.mContribution;
}

// Fills in the text field with the contribution, contrib
void LLPanelGroupLandMoney::impl::setYourContributionTextField(int contrib)
{
	std::string buffer = llformat("%d", contrib);

	if ( mYourContributionEditorp )
	{
		mYourContributionEditorp->setText(buffer);
	}
}

void LLPanelGroupLandMoney::impl::setYourMaxContributionTextBox(int max)
{
	mPanel.getChild<LLUICtrl>("your_contribution_max_value")->setTextArg("[AMOUNT]", llformat("%d", max));
}

//static
void LLPanelGroupLandMoney::impl::mapCallback(void* data)
{
	LLPanelGroupLandMoney::impl* selfp = (LLPanelGroupLandMoney::impl*) data;

	if ( selfp ) selfp->onMapButton();
}

void LLPanelGroupLandMoney::impl::contributionCommitCallback(LLUICtrl* ctrl, 
															 void* userdata)
{
	LLPanelGroupLandMoney* tabp    = (LLPanelGroupLandMoney*) userdata;
	LLLineEditor*          editorp = (LLLineEditor*) ctrl;

	if ( tabp && editorp )
	{
		impl* self = tabp->mImplementationp;
		int your_contribution = 0;
		int new_contribution = 0;

		new_contribution= atoi(editorp->getText().c_str());
		your_contribution = self->getStoredContribution();

		//reset their junk data to be "good" data to us
		self->setYourContributionTextField(new_contribution);

		//check to see if they're contribution text has changed
		self->mNeedsApply = new_contribution != your_contribution;
		tabp->notifyObservers();
	}
}

void LLPanelGroupLandMoney::impl::contributionKeystrokeCallback(LLLineEditor* caller,
																void* userdata)
{
	impl::contributionCommitCallback(caller, userdata);
}

//static
void LLPanelGroupLandMoney::impl::processGroupLand(LLMessageSystem* msg)
{
	S32 count = msg->getNumberOfBlocks("QueryData");
	if(count > 0)
	{
		S32 first_block = 0;

		LLUUID owner_id;
		LLUUID trans_id;

		msg->getUUID("QueryData", "OwnerID", owner_id, 0);
		msg->getUUID("TransactionData", "TransactionID", trans_id);

		if(owner_id.isNull())
		{
			// special block which has total contribution
			++first_block;
			
			S32 total_contribution;
			msg->getS32("QueryData", "ActualArea", total_contribution, 0);
			mPanel.getChild<LLUICtrl>("total_contributed_land_value")->setTextArg("[AREA]", llformat("%d", total_contribution));

			S32 committed;
			msg->getS32("QueryData", "BillableArea", committed, 0);
			mPanel.getChild<LLUICtrl>("total_land_in_use_value")->setTextArg("[AREA]", llformat("%d", committed));
			
			S32 available = total_contribution - committed;
			mPanel.getChild<LLUICtrl>("land_available_value")->setTextArg("[AREA]", llformat("%d", available));

			if ( mGroupOverLimitTextp && mGroupOverLimitIconp )
			{
				mGroupOverLimitIconp->setVisible(available < 0);
				mGroupOverLimitTextp->setVisible(available < 0);
			}
		}

		if ( trans_id != mTransID ) return;
		// This power was removed to make group roles simpler
		//if ( !gAgent.hasPowerInGroup(mGroupID, GP_LAND_VIEW_OWNED) ) return;
		if (!gAgent.isInGroup(mPanel.mGroupID)) return;
		mGroupParcelsp->setCommentText(mEmptyParcelsText);

		std::string name;
		std::string desc;
		S32 actual_area;
		S32 billable_area;
		U8 flags;
		F32 global_x;
		F32 global_y;
		std::string sim_name;
		std::string land_sku;
		std::string land_type;
		
		for(S32 i = first_block; i < count; ++i)
		{
			msg->getUUID("QueryData", "OwnerID", owner_id, i);
			msg->getString("QueryData", "Name", name, i);
			msg->getString("QueryData", "Desc", desc, i);
			msg->getS32("QueryData", "ActualArea", actual_area, i);
			msg->getS32("QueryData", "BillableArea", billable_area, i);
			msg->getU8("QueryData", "Flags", flags, i);
			msg->getF32("QueryData", "GlobalX", global_x, i);
			msg->getF32("QueryData", "GlobalY", global_y, i);
			msg->getString("QueryData", "SimName", sim_name, i);

			if ( msg->getSizeFast(_PREHASH_QueryData, i, _PREHASH_ProductSKU) > 0 )
			{
				msg->getStringFast(	_PREHASH_QueryData, _PREHASH_ProductSKU, land_sku, i);
				llinfos << "Land sku: " << land_sku << llendl;
				land_type = LLProductInfoRequestManager::instance().getDescriptionForSku(land_sku);
			}
			else
			{
				land_sku.clear();
				land_type = LLTrans::getString("land_type_unknown");
			}

			S32 region_x = llround(global_x) % REGION_WIDTH_UNITS;
			S32 region_y = llround(global_y) % REGION_WIDTH_UNITS;
			std::string location = sim_name + llformat(" (%d, %d)", region_x, region_y);
			std::string area;
			if(billable_area == actual_area)
			{
				area = llformat("%d", billable_area);
			}
			else
			{
				area = llformat("%d / %d", billable_area, actual_area);	
			}
			
			std::string hidden;
			hidden = llformat("%f %f", global_x, global_y);

			LLSD row;

			row["columns"][0]["column"] = "name";
			row["columns"][0]["value"] = name;
			row["columns"][0]["font"] = "SANSSERIF_SMALL";

			row["columns"][1]["column"] = "location";
			row["columns"][1]["value"] = location;
			row["columns"][1]["font"] = "SANSSERIF_SMALL";
			
			row["columns"][2]["column"] = "area";
			row["columns"][2]["value"] = area;
			row["columns"][2]["font"] = "SANSSERIF_SMALL";
			
			row["columns"][3]["column"] = "type";
			row["columns"][3]["value"] = land_type;
			row["columns"][3]["font"] = "SANSSERIF_SMALL";
			
			// hidden is always last column
			row["columns"][4]["column"] = "hidden";
			row["columns"][4]["value"] = hidden;
			
			mGroupParcelsp->addElement(row);
		}
	}
}

//*************************************
//** LLPanelGroupLandMoney Functions **
//*************************************


//static
LLMap<LLUUID, LLPanelGroupLandMoney*> LLPanelGroupLandMoney::sGroupIDs;

LLPanelGroupLandMoney::LLPanelGroupLandMoney() :
	LLPanelGroupTab() 
{
	//FIXME - add setGroupID();
	mImplementationp = new impl(*this);

	//problem what if someone has both the group floater open and the finder
	//open to the same group?  Some maps that map group ids to panels
	//will then only be working for the last panel for a given group id :(

	//FIXME - add to setGroupID()
	//LLPanelGroupLandMoney::sGroupIDs.addData(group_id, this);
}

LLPanelGroupLandMoney::~LLPanelGroupLandMoney()
{
	delete mImplementationp;
	LLPanelGroupLandMoney::sGroupIDs.removeData(mGroupID);
}

void LLPanelGroupLandMoney::activate()
{
	if ( !mImplementationp->mBeenActivated )
	{
		//select the first tab
		LLTabContainer* tabp = getChild<LLTabContainer>("group_money_tab_container");

		if ( tabp )
		{
			tabp->selectFirstTab();
			mImplementationp->mBeenActivated = true;
		}

		//fill in the max contribution

		//This calculation is unfortunately based on
		//the status bar's concept of how much land the user has
		//which can change dynamically if the user buys new land, gives
		//more land to a group, etc.
		//A race condition can occur if we want to update the UI's
		//concept of the user's max contribution before the status
		//bar has been updated from a change in the user's group contribution.

		//Since the max contribution should not change solely on changing
		//a user's group contribution, (it would only change through
		//purchasing of new land) this code is placed here
		//and only updated once to prevent the race condition
		//at the price of having stale data.
		//We need to have the status bar have observers
		//or find better way of distributing up to date land data. - jwolk
		S32 max_avail = mImplementationp->getStoredContribution();
		if(gStatusBar)
		{
			max_avail += gStatusBar->getSquareMetersLeft();
		}
		mImplementationp->setYourMaxContributionTextBox(max_avail);
	}

	mImplementationp->mMapButtonp->setEnabled(false);
	update(GC_ALL);
}

void LLPanelGroupLandMoney::update(LLGroupChange gc)
{
	if (gc != GC_ALL) return;  //Don't update if it's the wrong panel!

	LLTabContainer* tabp = getChild<LLTabContainer>("group_money_tab_container");

	if ( tabp )
	{
		LLPanel* panelp;
		LLGroupMoneyTabEventHandler* eh;

		panelp = tabp->getCurrentPanel();

		//now pull the event handler associated with that L$ tab
		if ( panelp )
		{
			eh = get_if_there(LLGroupMoneyTabEventHandler::sTabsToHandlers,
							  panelp,
							  (LLGroupMoneyTabEventHandler*)NULL);
			if ( eh ) eh->onClickTab();
		}
	}

	mImplementationp->requestGroupLandInfo();
	mImplementationp->setYourContributionTextField(mImplementationp->getStoredContribution());
}

bool LLPanelGroupLandMoney::needsApply(std::string& mesg)
{
	return mImplementationp->mNeedsApply;
}

bool LLPanelGroupLandMoney::apply(std::string& mesg)
{
	if (!mImplementationp->applyContribution() )
	{
		mesg = getString("land_contrib_error"); 
		return false;
	}

	mImplementationp->mNeedsApply = false;
	notifyObservers();

	return true;
}

void LLPanelGroupLandMoney::cancel()
{
	//set the contribution back to the "stored value"
	mImplementationp->setYourContributionTextField(mImplementationp->getStoredContribution());

	mImplementationp->mNeedsApply = false;
	notifyObservers();
}


BOOL LLPanelGroupLandMoney::postBuild()
{
	/* This power was removed to make group roles simpler
	bool has_parcel_view     = gAgent.hasPowerInGroup(mGroupID,
													  GP_LAND_VIEW_OWNED);
	bool has_accounting_view = gAgent.hasPowerInGroup(mGroupID,
													  GP_ACCOUNTING_VIEW);
	*/
	
	bool can_view = gAgent.isInGroup(mGroupID);

	mImplementationp->mGroupOverLimitIconp = 
		getChild<LLIconCtrl>("group_over_limit_icon");
	mImplementationp->mGroupOverLimitTextp = 
		getChild<LLTextBox>("group_over_limit_text");

	mImplementationp->mYourContributionEditorp 
		= getChild<LLLineEditor>("your_contribution_line_editor");
	if ( mImplementationp->mYourContributionEditorp )
	{
		LLLineEditor* editor = mImplementationp->mYourContributionEditorp;

	    editor->setCommitCallback(mImplementationp->contributionCommitCallback, this);
		editor->setKeystrokeCallback(mImplementationp->contributionKeystrokeCallback, this);
	}

	mImplementationp->mMapButtonp = getChild<LLButton>("map_button");

	mImplementationp->mGroupParcelsp = 
		getChild<LLScrollListCtrl>("group_parcel_list");

	if ( mImplementationp->mGroupParcelsp )
	{
		mImplementationp->mGroupParcelsp->setCommitCallback(boost::bind(&LLPanelGroupLandMoney::onLandSelectionChanged, this));
		mImplementationp->mGroupParcelsp->setCommitOnSelectionChange(true);
	}

	mImplementationp->mCantViewParcelsText = getString("cant_view_group_land_text");
	mImplementationp->mCantViewAccountsText = getString("cant_view_group_accounting_text");
	mImplementationp->mEmptyParcelsText = getString("epmty_view_group_land_text");
	
	if ( mImplementationp->mMapButtonp )
	{
		mImplementationp->mMapButtonp->setClickedCallback(LLPanelGroupLandMoney::impl::mapCallback, mImplementationp);
	}

	if ( mImplementationp->mGroupOverLimitTextp )
	{
		mImplementationp->mGroupOverLimitTextp->setVisible(FALSE);
	}

	if ( mImplementationp->mGroupOverLimitIconp )
	{
		mImplementationp->mGroupOverLimitIconp->setVisible(FALSE);
	}

	if ( !can_view )
	{
		if ( mImplementationp->mGroupParcelsp )
		{
			mImplementationp->mGroupParcelsp->setCommentText(
							mImplementationp->mCantViewParcelsText);
			mImplementationp->mGroupParcelsp->setEnabled(FALSE);
		}
	}



	LLButton* earlierp, *laterp;
	LLTextEditor* textp;
	LLPanel* panelp;

	LLTabContainer* tabcp = getChild<LLTabContainer>("group_money_tab_container");

	if ( !can_view )
	{
		if ( tabcp )
		{
			S32 i;
			S32 tab_count = tabcp->getTabCount();

			for (i = tab_count - 1; i >=0; --i)
			{
				tabcp->enableTabButton(i, false);
			}
		}
	}

	std::string loading_text = getString("loading_txt");
	
	//pull out the widgets for the L$ details tab
	earlierp = getChild<LLButton>("earlier_details_button", true);
	laterp = getChild<LLButton>("later_details_button", true);
	textp = getChild<LLTextEditor>("group_money_details_text", true);
	panelp = getChild<LLPanel>("group_money_details_tab", true);

	if ( !can_view )
	{
		textp->setText(mImplementationp->mCantViewAccountsText);
	}
	else
	{
		mImplementationp->mMoneyDetailsTabEHp = 
			new LLGroupMoneyDetailsTabEventHandler(earlierp,
												   laterp,
												   textp,
												   tabcp,
												   panelp,
												   loading_text);
	}

	textp = getChild<LLTextEditor>("group_money_planning_text", true);
	panelp = getChild<LLPanel>("group_money_planning_tab", true);

	if ( !can_view )
	{
		textp->setText(mImplementationp->mCantViewAccountsText);
	}
	else
	{
		//Temporally disabled for DEV-11287.
		mImplementationp->mMoneyPlanningTabEHp = 
			new LLGroupMoneyPlanningTabEventHandler(textp,
													tabcp,
													panelp,
													loading_text);
	}

	//pull out the widgets for the L$ sales tab
	earlierp = getChild<LLButton>("earlier_sales_button", true);
	laterp = getChild<LLButton>("later_sales_button", true);
	textp = getChild<LLTextEditor>("group_money_sales_text", true);
	panelp = getChild<LLPanel>("group_money_sales_tab", true);

	if ( !can_view )
	{
		textp->setText(mImplementationp->mCantViewAccountsText);
	}
	else
	{
		mImplementationp->mMoneySalesTabEHp = 
			new LLGroupMoneySalesTabEventHandler(earlierp,
												 laterp,
												 textp,
												 tabcp,
												 panelp,
												 loading_text);
	}

	return LLPanelGroupTab::postBuild();
}

void LLPanelGroupLandMoney::onLandSelectionChanged()
{
	mImplementationp->mMapButtonp->setEnabled( mImplementationp->mGroupParcelsp->getItemCount() > 0 );
}

BOOL LLPanelGroupLandMoney::isVisibleByAgent(LLAgent* agentp)
{
	return mAllowEdit && agentp->isInGroup(mGroupID);
}

void LLPanelGroupLandMoney::processPlacesReply(LLMessageSystem* msg, void**)
{
	LLUUID group_id;
	msg->getUUID("AgentData", "QueryID", group_id);

	LLPanelGroupLandMoney* selfp = sGroupIDs.getIfThere(group_id);
	if(!selfp)
	{
		llinfos << "Group Panel Land L$ " << group_id << " no longer in existence."
				<< llendl;
		return;
	}

	selfp->mImplementationp->processGroupLand(msg);
}


LLGroupMoneyTabEventHandlerImpl::LLGroupMoneyTabEventHandlerImpl(LLButton* earlier_buttonp,
										LLButton* later_buttonp,
										LLTextEditor* text_editorp,
										LLPanel* tabpanelp,
										const std::string& loading_text,
										S32 interval_length_days,
										S32 max_interval_days)
{
	mPanelID.generate();

	mIntervalLength = interval_length_days;
	mMaxInterval = max_interval_days;
	mCurrentInterval = 0;

	mTextEditorp = text_editorp;
	mEarlierButtonp = earlier_buttonp;
	mLaterButtonp = later_buttonp;
	mTabPanelp = tabpanelp;

	mLoadingText = loading_text;
}

LLGroupMoneyTabEventHandlerImpl::~LLGroupMoneyTabEventHandlerImpl()
{
}

bool LLGroupMoneyTabEventHandlerImpl::getCanClickEarlier()
{
	return (mCurrentInterval < mMaxInterval);
}

bool LLGroupMoneyTabEventHandlerImpl::getCanClickLater()
{
	return ( mCurrentInterval > 0 );
}

void LLGroupMoneyTabEventHandlerImpl::updateButtons()
{
	if ( mEarlierButtonp )
	{
		mEarlierButtonp->setEnabled(getCanClickEarlier());
	}
	if ( mLaterButtonp )
	{
		mLaterButtonp->setEnabled(getCanClickLater());
	}
}

//*******************************************
//** LLGroupMoneyTabEventHandler Functions **
//*******************************************

LLMap<LLUUID, LLGroupMoneyTabEventHandler*> LLGroupMoneyTabEventHandler::sInstanceIDs;
std::map<LLPanel*, LLGroupMoneyTabEventHandler*> LLGroupMoneyTabEventHandler::sTabsToHandlers;

LLGroupMoneyTabEventHandler::LLGroupMoneyTabEventHandler(LLButton* earlier_buttonp,
														 LLButton* later_buttonp,
														 LLTextEditor* text_editorp,
														 LLTabContainer* tab_containerp,
														 LLPanel* panelp,
														 const std::string& loading_text,
														 S32 interval_length_days,
														 S32 max_interval_days)
{
	mImplementationp = new LLGroupMoneyTabEventHandlerImpl(earlier_buttonp,
								later_buttonp,
								text_editorp,
								panelp,
								loading_text,
								interval_length_days,
								max_interval_days);

	if ( earlier_buttonp )
	{
		earlier_buttonp->setClickedCallback(clickEarlierCallback, this);
	}

	if ( later_buttonp )
	{
		later_buttonp->setClickedCallback(clickLaterCallback, this);
	}

	mImplementationp->updateButtons();

	if ( tab_containerp && panelp )
	{
		tab_containerp->setCommitCallback(boost::bind(&LLGroupMoneyTabEventHandler::onClickTab, this));
	}

	sInstanceIDs.addData(mImplementationp->mPanelID, this);
	sTabsToHandlers[panelp] = this;
}

LLGroupMoneyTabEventHandler::~LLGroupMoneyTabEventHandler()
{
	sInstanceIDs.removeData(mImplementationp->mPanelID);
	sTabsToHandlers.erase(mImplementationp->mTabPanelp);

	delete mImplementationp;
}


void LLGroupMoneyTabEventHandler::onClickTab()
{
	requestData(gMessageSystem);
}

void LLGroupMoneyTabEventHandler::requestData(LLMessageSystem* msg)
{
	//do nothing
}

void LLGroupMoneyTabEventHandler::processReply(LLMessageSystem* msg, void** data)
{
	//do nothing
}

void LLGroupMoneyTabEventHandler::onClickEarlier()
{
	if ( mImplementationp->mTextEditorp) 
	{
		mImplementationp->mTextEditorp->setText(mImplementationp->mLoadingText);
	}
	mImplementationp->mCurrentInterval++;

	mImplementationp->updateButtons();

	requestData(gMessageSystem);
}

void LLGroupMoneyTabEventHandler::onClickLater()
{
	if ( mImplementationp->mTextEditorp )
	{
		mImplementationp->mTextEditorp->setText(mImplementationp->mLoadingText);
	}
	mImplementationp->mCurrentInterval--;

	mImplementationp->updateButtons();

	requestData(gMessageSystem);
}

//static
void LLGroupMoneyTabEventHandler::clickEarlierCallback(void* data)
{
	LLGroupMoneyTabEventHandler* selfp = (LLGroupMoneyTabEventHandler*) data;

	if ( selfp ) selfp->onClickEarlier();
}

//static
void LLGroupMoneyTabEventHandler::clickLaterCallback(void* data)
{
	LLGroupMoneyTabEventHandler* selfp = (LLGroupMoneyTabEventHandler*) data;
	if ( selfp ) selfp->onClickLater();
}

//**************************************************
//** LLGroupMoneyDetailsTabEventHandler Functions **
//**************************************************

LLGroupMoneyDetailsTabEventHandler::LLGroupMoneyDetailsTabEventHandler(LLButton* earlier_buttonp,
																	   LLButton* later_buttonp,
																	   LLTextEditor* text_editorp,
																	   LLTabContainer* tab_containerp,
																	   LLPanel* panelp,
																	   const std::string& loading_text)
	: LLGroupMoneyTabEventHandler(earlier_buttonp,
								  later_buttonp,
								  text_editorp,
								  tab_containerp,
								  panelp,
								  loading_text,
								  SUMMARY_INTERVAL,
								  SUMMARY_MAX)
{
}

LLGroupMoneyDetailsTabEventHandler::~LLGroupMoneyDetailsTabEventHandler()
{
}

void LLGroupMoneyDetailsTabEventHandler::requestData(LLMessageSystem* msg)
{
	msg->newMessageFast(_PREHASH_GroupAccountDetailsRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->addUUIDFast(_PREHASH_GroupID,  mImplementationp->getGroupID() );
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_RequestID, mImplementationp->mPanelID );
	msg->addS32Fast(_PREHASH_IntervalDays, mImplementationp->mIntervalLength );
	msg->addS32Fast(_PREHASH_CurrentInterval, mImplementationp->mCurrentInterval);

	gAgent.sendReliableMessage();

	if ( mImplementationp->mTextEditorp )
	{
		mImplementationp->mTextEditorp->setText(mImplementationp->mLoadingText);
	}

	LLGroupMoneyTabEventHandler::requestData(msg);
}

void LLGroupMoneyDetailsTabEventHandler::processReply(LLMessageSystem* msg, 
													  void** data)
{
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );
	if (mImplementationp->getGroupID() != group_id) 
	{
		llwarns << "Group Account details not for this group!" << llendl;
		return;
	}

	std::string start_date;
	S32 interval_days;
	S32 current_interval;

	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_IntervalDays, interval_days );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_CurrentInterval, current_interval );
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_StartDate, start_date);

	std::string time_str = LLTrans::getString("GroupMoneyDate");
	LLSD substitution;

	// We don't do time zone corrections of the calculated number of seconds
	// because we don't have a full time stamp, only a date.
	substitution["datetime"] = LLDateUtil::secondsSinceEpochFromString("%Y-%m-%d", start_date);
	LLStringUtil::format (time_str, substitution);

	if ( interval_days != mImplementationp->mIntervalLength || 
		 current_interval != mImplementationp->mCurrentInterval )
	{
		llinfos << "Out of date details packet " << interval_days << " " 
			<< current_interval << llendl;
		return;
	}

	std::string text = time_str;
	text.append("\n\n");

	S32 total_amount = 0;
	S32 transactions = msg->getNumberOfBlocksFast(_PREHASH_HistoryData);
	for(S32 i = 0; i < transactions; i++)
	{
		S32 amount = 0;
		std::string desc;

		msg->getStringFast(_PREHASH_HistoryData, _PREHASH_Description, desc, i );
		msg->getS32Fast(_PREHASH_HistoryData, _PREHASH_Amount, amount, i);

		if (amount != 0)
		{
			text.append(llformat("%-24s %6d\n", desc.c_str(), amount));
		}
		else
		{
			// skip it
		}

		total_amount += amount;
	}

	text.append(1, '\n');

	text.append(llformat("%-24s %6d\n", LLTrans::getString("GroupMoneyTotal").c_str(), total_amount));

	if ( mImplementationp->mTextEditorp )
	{
		mImplementationp->mTextEditorp->setText(text);
	}
}

//static
void LLPanelGroupLandMoney::processGroupAccountDetailsReply(LLMessageSystem* msg, 
															void** data)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		llwarns << "Got group L$ history reply for another agent!" << llendl;
		return;
	}

	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_MoneyData, _PREHASH_RequestID, request_id );
	LLGroupMoneyTabEventHandler* selfp = LLGroupMoneyTabEventHandler::sInstanceIDs.getIfThere(request_id);
	if (!selfp)
	{
		llwarns << "GroupAccountDetails recieved for non-existent group panel." << llendl;
		return;
	}

	selfp->processReply(msg, data);
}

//************************************************
//** LLGroupMoneySalesTabEventHandler Functions **
//************************************************

LLGroupMoneySalesTabEventHandler::LLGroupMoneySalesTabEventHandler(LLButton* earlier_buttonp,
																   LLButton* later_buttonp,
																   LLTextEditor* text_editorp,
																   LLTabContainer* tab_containerp,
																   LLPanel* panelp,
																   const std::string& loading_text)
	: LLGroupMoneyTabEventHandler(earlier_buttonp,
								  later_buttonp,
								  text_editorp,
								  tab_containerp,
								  panelp,
								  loading_text,
								  SUMMARY_INTERVAL,
								  SUMMARY_MAX)
{
}

LLGroupMoneySalesTabEventHandler::~LLGroupMoneySalesTabEventHandler()
{
}

void LLGroupMoneySalesTabEventHandler::requestData(LLMessageSystem* msg)
{
	msg->newMessageFast(_PREHASH_GroupAccountTransactionsRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->addUUIDFast(_PREHASH_GroupID, mImplementationp->getGroupID() );
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_RequestID, mImplementationp->mPanelID );
	msg->addS32Fast(_PREHASH_IntervalDays, mImplementationp->mIntervalLength );
	msg->addS32Fast(_PREHASH_CurrentInterval, mImplementationp->mCurrentInterval);

	gAgent.sendReliableMessage();

	if ( mImplementationp->mTextEditorp )
	{
		mImplementationp->mTextEditorp->setText(mImplementationp->mLoadingText);
	}

	LLGroupMoneyTabEventHandler::requestData(msg);
}

void LLGroupMoneySalesTabEventHandler::processReply(LLMessageSystem* msg, 
													void** data)
{
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );
	if (mImplementationp->getGroupID() != group_id) 
	{
		llwarns << "Group Account Transactions not for this group!" << llendl;
		return;
	}

	std::string text = mImplementationp->mTextEditorp->getText();

	std::string start_date;
	S32 interval_days;
	S32 current_interval;

	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_IntervalDays, interval_days );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_CurrentInterval, current_interval );
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_StartDate, start_date);

	if (interval_days != mImplementationp->mIntervalLength ||
	    current_interval != mImplementationp->mCurrentInterval)
	{
		llinfos << "Out of date details packet " << interval_days << " " 
			<< current_interval << llendl;
		return;
	}

	// If this is the first packet, clear the text, don't append.
	// Start with the date.
	if (text == mImplementationp->mLoadingText)
	{
		std::string time_str = LLTrans::getString("GroupMoneyDate");
		LLSD substitution;

		// We don't do time zone corrections of the calculated number of seconds
		// because we don't have a full time stamp, only a date.
		substitution["datetime"] = LLDateUtil::secondsSinceEpochFromString("%Y-%m-%d", start_date);
		LLStringUtil::format (time_str, substitution);

		text = time_str + "\n\n";
	}

	S32 transactions = msg->getNumberOfBlocksFast(_PREHASH_HistoryData);
	if (transactions == 0)
	{
		text.append(LLTrans::getString("none_text")); 
	}
	else
	{
		for(S32 i = 0; i < transactions; i++)
		{
			std::string	time;
			S32			type = 0;
			S32			amount = 0;
			std::string	user;
			std::string	item;

			msg->getStringFast(_PREHASH_HistoryData, _PREHASH_Time,		time, i);
			msg->getStringFast(_PREHASH_HistoryData, _PREHASH_User,		user, i );
			msg->getS32Fast(_PREHASH_HistoryData, _PREHASH_Type,		type, i);
			msg->getStringFast(_PREHASH_HistoryData, _PREHASH_Item,		item, i );
			msg->getS32Fast(_PREHASH_HistoryData, _PREHASH_Amount,		amount, i);

			if (amount != 0)
			{
				std::string verb;

				switch(type)
				{
				case TRANS_OBJECT_SALE:
					verb = LLTrans::getString("GroupMoneyBought").c_str();
					break;
				case TRANS_GIFT:
					verb = LLTrans::getString("GroupMoneyPaidYou").c_str();
					break;
				case TRANS_PAY_OBJECT:
					verb = LLTrans::getString("GroupMoneyPaidInto").c_str();
					break;
				case TRANS_LAND_PASS_SALE:
					verb = LLTrans::getString("GroupMoneyBoughtPassTo").c_str();
					break;
				case TRANS_EVENT_FEE:
					verb = LLTrans::getString("GroupMoneyPaidFeeForEvent").c_str();
					break;
				case TRANS_EVENT_PRIZE:
					verb = LLTrans::getString("GroupMoneyPaidPrizeForEvent").c_str();
					break;
				default:
					verb = "";
					break;
				}

				std::string line = llformat("%s %6d - %s %s %s\n", time.c_str(), amount, user.c_str(), verb.c_str(), item.c_str());
				text.append(line);
			}
		}
	}

	if ( mImplementationp->mTextEditorp)
	{
		mImplementationp->mTextEditorp->setText(text);
	}
}

//static
void LLPanelGroupLandMoney::processGroupAccountTransactionsReply(LLMessageSystem* msg, 
																 void** data)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		llwarns << "Got group L$ history reply for another agent!" << llendl;
		return;
	}

	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_MoneyData, _PREHASH_RequestID, request_id );

	LLGroupMoneyTabEventHandler* self;

	self = LLGroupMoneyTabEventHandler::sInstanceIDs.getIfThere(request_id);
	if (!self)
	{
		llwarns << "GroupAccountTransactions recieved for non-existent group panel." << llendl;
		return;
	}

	self->processReply(msg, data);
}

//***************************************************
//** LLGroupMoneyPlanningTabEventHandler Functions **
//***************************************************

LLGroupMoneyPlanningTabEventHandler::LLGroupMoneyPlanningTabEventHandler(LLTextEditor* text_editorp,
																		 LLTabContainer* tab_containerp,
																		 LLPanel* panelp,
																		 const std::string& loading_text)
	: LLGroupMoneyTabEventHandler(NULL,
								  NULL,
								  text_editorp,
								  tab_containerp,
								  panelp,
								  loading_text,
								  SUMMARY_INTERVAL,
								  SUMMARY_MAX)
{
}

LLGroupMoneyPlanningTabEventHandler::~LLGroupMoneyPlanningTabEventHandler()
{
}

void LLGroupMoneyPlanningTabEventHandler::requestData(LLMessageSystem* msg)
{
	msg->newMessageFast(_PREHASH_GroupAccountSummaryRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->addUUIDFast(_PREHASH_GroupID, mImplementationp->getGroupID() );
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_RequestID, mImplementationp->mPanelID );
	msg->addS32Fast(_PREHASH_IntervalDays, mImplementationp->mIntervalLength);
	msg->addS32Fast(_PREHASH_CurrentInterval, 0); //planning has 0 interval

	gAgent.sendReliableMessage();

	if ( mImplementationp->mTextEditorp )
	{
		mImplementationp->mTextEditorp->setText(mImplementationp->mLoadingText);
	}

	LLGroupMoneyTabEventHandler::requestData(msg);
}

void LLGroupMoneyPlanningTabEventHandler::processReply(LLMessageSystem* msg, 
															void** data)
{
	LLUUID group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );
	if (mImplementationp->getGroupID() != group_id) 
	{
		llwarns << "Group Account Summary received not for this group!" << llendl;
		return;
	}

	std::string text;

	std::string start_date;
	std::string last_stipend_date;
	std::string next_stipend_date;
	S32 interval_days;
	S32 current_interval;
	S32 balance;
	S32 total_credits;
	S32 total_debits;
	S32 cur_object_tax;
	S32 cur_light_tax;
	S32 cur_land_tax;
	S32 cur_group_tax;
	S32 cur_parcel_dir_fee;
	S32 proj_object_tax;
	S32 proj_light_tax;
	S32 proj_land_tax;
	S32 proj_group_tax;
	S32 proj_parcel_dir_fee;
	S32	non_exempt_members;

	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_IntervalDays, interval_days );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_CurrentInterval, current_interval );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_Balance, balance );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_TotalCredits, total_credits );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_TotalDebits, total_debits );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_ObjectTaxCurrent, cur_object_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_LightTaxCurrent, cur_light_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_LandTaxCurrent, cur_land_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_GroupTaxCurrent, cur_group_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_ParcelDirFeeCurrent, cur_parcel_dir_fee );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_ObjectTaxEstimate, proj_object_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_LightTaxEstimate, proj_light_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_LandTaxEstimate, proj_land_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_GroupTaxEstimate, proj_group_tax );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_ParcelDirFeeEstimate, proj_parcel_dir_fee );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_NonExemptMembers, non_exempt_members );

	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_StartDate, start_date);
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_LastTaxDate, last_stipend_date);
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_TaxDate, next_stipend_date);


	if (interval_days != mImplementationp->mIntervalLength || 
		current_interval != mImplementationp->mCurrentInterval)
	{
		llinfos << "Out of date summary packet " << interval_days << " " 
			<< current_interval << llendl;
		return;
	}

	text.append(LLTrans::getString("SummaryForTheWeek"));

	std::string date_format_str = LLTrans::getString("GroupPlanningDate");
	std::string time_str = date_format_str;
	LLSD substitution;
	// We don't do time zone corrections of the calculated number of seconds
	// because we don't have a full time stamp, only a date.
	substitution["datetime"] = LLDateUtil::secondsSinceEpochFromString("%Y-%m-%d", start_date);
	LLStringUtil::format (time_str, substitution);

	text.append(time_str);
	text.append(".  ");

	if (current_interval == 0)
	{
		text.append(LLTrans::getString("NextStipendDay"));

		time_str = date_format_str;
		substitution["datetime"] = LLDateUtil::secondsSinceEpochFromString("%Y-%m-%d", next_stipend_date);
		LLStringUtil::format (time_str, substitution);

		text.append(time_str);
		text.append(".\n\n");
		text.append(llformat("%-23sL$%6d\n", LLTrans::getString("GroupMoneyBalance").c_str(), balance ));
		text.append(1, '\n');
	}

	// [DEV-29503] Hide the individual info since
	// non_exempt_member here is a wrong choice to calculate individual shares.
// 	text.append( LLTrans::getString("GroupIndividualShare"));
// 	text.append(llformat( "%-24s %6d      %6d \n", LLTrans::getString("GroupMoneyCredits").c_str(), total_credits, (S32)floor((F32)total_credits/(F32)non_exempt_members)));
// 	text.append(llformat( "%-24s %6d      %6d \n", LLTrans::getString("GroupMoneyDebits").c_str(), total_debits, (S32)floor((F32)total_debits/(F32)non_exempt_members)));
// 	text.append(llformat( "%-24s %6d      %6d \n", LLTrans::getString("GroupMoneyTotal").c_str(), total_credits + total_debits,  (S32)floor((F32)(total_credits + total_debits)/(F32)non_exempt_members)));

	text.append(llformat( "%s\n", LLTrans::getString("GroupColumn").c_str()));
	text.append(llformat( "%-24s %6d\n", LLTrans::getString("GroupMoneyCredits").c_str(), total_credits));
	text.append(llformat( "%-24s %6d\n", LLTrans::getString("GroupMoneyDebits").c_str(), total_debits));
	text.append(llformat( "%-24s %6d\n", LLTrans::getString("GroupMoneyTotal").c_str(), total_credits + total_debits));
	
	if ( mImplementationp->mTextEditorp )
	{
		mImplementationp->mTextEditorp->setText(text);
	}
}

//static
void LLPanelGroupLandMoney::processGroupAccountSummaryReply(LLMessageSystem* msg, 
															void** data)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
	if (gAgent.getID() != agent_id)
	{
		llwarns << "Got group L$ history reply for another agent!" << llendl;
		return;
	}

	LLUUID request_id;
	msg->getUUIDFast(_PREHASH_MoneyData, _PREHASH_RequestID, request_id );

	LLGroupMoneyTabEventHandler* self;

	self = LLGroupMoneyTabEventHandler::sInstanceIDs.getIfThere(request_id);
	if (!self)
	{
		llwarns << "GroupAccountSummary recieved for non-existent group L$ planning tab." << llendl;
		return;
	}

	self->processReply(msg, data);
}

void LLPanelGroupLandMoney::setGroupID(const LLUUID& id)
{
	LLPanelGroupLandMoney::sGroupIDs.removeData(mGroupID);
	LLPanelGroupTab::setGroupID(id);
	LLPanelGroupLandMoney::sGroupIDs.addData(mGroupID, this);


	bool can_view = gAgent.isInGroup(mGroupID);

	mImplementationp->mGroupOverLimitIconp = 
		getChild<LLIconCtrl>("group_over_limit_icon");
	mImplementationp->mGroupOverLimitTextp = 
		getChild<LLTextBox>("group_over_limit_text");

	mImplementationp->mYourContributionEditorp 
		= getChild<LLLineEditor>("your_contribution_line_editor");
	if ( mImplementationp->mYourContributionEditorp )
	{
		LLLineEditor* editor = mImplementationp->mYourContributionEditorp;

	    editor->setCommitCallback(mImplementationp->contributionCommitCallback, this);
		editor->setKeystrokeCallback(mImplementationp->contributionKeystrokeCallback, this);
	}

	mImplementationp->mMapButtonp = getChild<LLButton>("map_button");

	mImplementationp->mGroupParcelsp = 
		getChild<LLScrollListCtrl>("group_parcel_list");

	if ( mImplementationp->mGroupParcelsp )
	{
		mImplementationp->mGroupParcelsp->setCommitCallback(boost::bind(&LLPanelGroupLandMoney::onLandSelectionChanged, this));
		mImplementationp->mGroupParcelsp->setCommitOnSelectionChange(true);
	}

	mImplementationp->mCantViewParcelsText = getString("cant_view_group_land_text");
	mImplementationp->mCantViewAccountsText = getString("cant_view_group_accounting_text");
	
	if ( mImplementationp->mMapButtonp )
	{
		mImplementationp->mMapButtonp->setClickedCallback(LLPanelGroupLandMoney::impl::mapCallback, mImplementationp);
	}

	if ( mImplementationp->mGroupOverLimitTextp )
	{
		mImplementationp->mGroupOverLimitTextp->setVisible(FALSE);
	}

	if ( mImplementationp->mGroupOverLimitIconp )
	{
		mImplementationp->mGroupOverLimitIconp->setVisible(FALSE);
	}

	if ( mImplementationp->mGroupParcelsp )
	{
		mImplementationp->mGroupParcelsp->setEnabled(can_view);
	}

	if ( !can_view && mImplementationp->mGroupParcelsp )
	{
		mImplementationp->mGroupParcelsp->setEnabled(FALSE);
	}


	LLButton* earlierp, *laterp;
	LLTextEditor* textp;
	LLPanel* panelp;

	LLTabContainer* tabcp = getChild<LLTabContainer>("group_money_tab_container");

	if ( tabcp )
	{
		S32 i;
		S32 tab_count = tabcp->getTabCount();

		for (i = tab_count - 1; i >=0; --i)
		{
			tabcp->enableTabButton(i, can_view );
		}
	}

	std::string loading_text = getString("loading_txt");
	
	//pull out the widgets for the L$ details tab
	earlierp = getChild<LLButton>("earlier_details_button", true);
	laterp = getChild<LLButton>("later_details_button", true);
	textp = getChild<LLTextEditor>("group_money_details_text", true);
	panelp = getChild<LLPanel>("group_money_details_tab", true);

	if ( !can_view )
	{
		textp->setText(mImplementationp->mCantViewAccountsText);
	}
	else
	{
		if(mImplementationp->mMoneyDetailsTabEHp == 0)
			mImplementationp->mMoneyDetailsTabEHp = new LLGroupMoneyDetailsTabEventHandler(earlierp,laterp,textp,tabcp,panelp,loading_text);
		mImplementationp->mMoneyDetailsTabEHp->setGroupID(mGroupID);
	}

	textp = getChild<LLTextEditor>("group_money_planning_text", true);
	

	if ( !can_view )
	{
		textp->setText(mImplementationp->mCantViewAccountsText);
	}
	else
	{
		panelp = getChild<LLPanel>("group_money_planning_tab", true);
		if(mImplementationp->mMoneyPlanningTabEHp == 0)
			mImplementationp->mMoneyPlanningTabEHp = new LLGroupMoneyPlanningTabEventHandler(textp,tabcp,panelp,loading_text);
		mImplementationp->mMoneyPlanningTabEHp->setGroupID(mGroupID);
	}

	//pull out the widgets for the L$ sales tab
	textp = getChild<LLTextEditor>("group_money_sales_text", true);


	if ( !can_view )
	{
		textp->setText(mImplementationp->mCantViewAccountsText);
	}
	else
	{
		earlierp = getChild<LLButton>("earlier_sales_button", true);
		laterp = getChild<LLButton>("later_sales_button", true);
		panelp = getChild<LLPanel>("group_money_sales_tab", true);
		if(mImplementationp->mMoneySalesTabEHp == NULL) 
			mImplementationp->mMoneySalesTabEHp = new LLGroupMoneySalesTabEventHandler(earlierp,laterp,textp,tabcp,panelp,loading_text);
		mImplementationp->mMoneySalesTabEHp->setGroupID(mGroupID);
	}

	mImplementationp->mBeenActivated = false;

	activate();
}

