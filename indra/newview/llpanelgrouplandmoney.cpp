/** 
 * @file llpanelgrouplandmoney.cpp
 * @brief Panel for group land and L$.
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

#include "llpanelgrouplandmoney.h"

#include "lluiconstants.h"
#include "roles_constants.h"

#include "llparcel.h"
#include "llqueryflags.h"

#include "llagent.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lltabcontainer.h"
#include "lltransactiontypes.h"
#include "llvieweruictrlfactory.h"

#include "llstatusbar.h"
#include "llfloaterworldmap.h"
#include "llviewermessage.h"

////////////////////////////////////////////////////////////////////////////

class LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyTabEventHandler(LLButton* earlier_button,
								LLButton* later_button,
								LLTextEditor* text_editor,
								LLTabContainerCommon* tab_containerp,
								LLPanel* panelp,
								const LLString& loading_text,
								const LLUUID& group_id,
								S32 interval_length_days,
								S32 max_interval_days);
	virtual ~LLGroupMoneyTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);

	virtual void onClickEarlier();
	virtual void onClickLater();
	virtual void onClickTab();

	static void clickEarlierCallback(void* data);
	static void clickLaterCallback(void* data);
	static void clickTabCallback(void* user_data, bool from_click);

	static LLMap<LLUUID, LLGroupMoneyTabEventHandler*> sInstanceIDs;
	static std::map<LLPanel*, LLGroupMoneyTabEventHandler*> sTabsToHandlers;
protected:
	class impl;
	impl* mImplementationp;
};

class LLGroupMoneyDetailsTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyDetailsTabEventHandler(LLButton* earlier_buttonp,
									   LLButton* later_buttonp,
									   LLTextEditor* text_editorp,
									   LLTabContainerCommon* tab_containerp,
									   LLPanel* panelp,
									   const LLString& loading_text,
									   const LLUUID& group_id);
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
									 LLTabContainerCommon* tab_containerp,
									 LLPanel* panelp,
									 const LLString& loading_text,
									 const LLUUID& group_id);
	virtual ~LLGroupMoneySalesTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);
};

class LLGroupMoneyPlanningTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyPlanningTabEventHandler(LLTextEditor* text_editor,
										LLTabContainerCommon* tab_containerp,
										LLPanel* panelp,
										const LLString& loading_text,
										const LLUUID& group_id);
	virtual ~LLGroupMoneyPlanningTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);
};

////////////////////////////////////////////////////////////////////////////

class LLPanelGroupLandMoney::impl
{
public:
	impl(LLPanelGroupLandMoney& panel, const LLUUID& group_id); //constructor
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

	LLUUID mGroupID;
	LLUUID mTransID;

	bool mBeenActivated;
	bool mNeedsSendGroupLandRequest;
	bool mNeedsApply;

	std::string mCantViewParcelsText;
	std::string mCantViewAccountsText;
};

//*******************************************
//** LLPanelGroupLandMoney::impl Functions **
//*******************************************
LLPanelGroupLandMoney::impl::impl(LLPanelGroupLandMoney& panel, const LLUUID& group_id)
	: mPanel(panel),
	  mGroupID(group_id)
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

	send_places_query(mGroupID, mTransID, "", query_flags, LLParcel::C_ANY, "");
}

void LLPanelGroupLandMoney::impl::onMapButton()
{
	LLScrollListItem* itemp;
	itemp = mGroupParcelsp->getFirstSelected();
	if (!itemp) return;

	const LLScrollListCell* cellp;
	// name
	// location
	// area
	cellp = itemp->getColumn(3);	// hidden

	F32 global_x = 0.f;
	F32 global_y = 0.f;
	sscanf(cellp->getValue().asString().c_str(), "%f %f", &global_x, &global_y);

	// Hack: Use the agent's z-height
	F64 global_z = gAgent.getPositionGlobal().mdV[VZ];

	LLVector3d pos_global(global_x, global_y, global_z);
	gFloaterWorldMap->trackLocation(pos_global);

	LLFloaterWorldMap::show(NULL, TRUE);
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
		if(!gAgent.setGroupContribution(mGroupID, new_contribution))
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
	gAgent.getGroupData(mGroupID, group_data);

	return group_data.mContribution;
}

// Fills in the text field with the contribution, contrib
void LLPanelGroupLandMoney::impl::setYourContributionTextField(int contrib)
{
	LLString buffer = llformat("%d", contrib);

	if ( mYourContributionEditorp )
	{
		mYourContributionEditorp->setText(buffer);
		mYourContributionEditorp->draw();
	}
}

void LLPanelGroupLandMoney::impl::setYourMaxContributionTextBox(int max)
{
	mPanel.childSetTextArg("your_contribution_max_value", "[AMOUNT]", llformat("%d", max));
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
			mPanel.childSetTextArg("total_contributed_land_value", "[AREA]", llformat("%d", total_contribution));

			S32 committed;
			msg->getS32("QueryData", "BillableArea", committed, 0);
			mPanel.childSetTextArg("total_land_in_use_value", "[AREA]", llformat("%d", committed));
			
			S32 available = total_contribution - committed;
			mPanel.childSetTextArg("land_available_value", "[AREA]", llformat("%d", available));

			if ( mGroupOverLimitTextp && mGroupOverLimitIconp )
			{
				mGroupOverLimitIconp->setVisible(available < 0);
				mGroupOverLimitTextp->setVisible(available < 0);
			}
		}

		if ( trans_id != mTransID ) return;
		// This power was removed to make group roles simpler
		//if ( !gAgent.hasPowerInGroup(mGroupID, GP_LAND_VIEW_OWNED) ) return;
		if (!gAgent.isInGroup(mGroupID)) return;

		//we updated more than just the available area special block
		if ( count > 1)
		{
			mMapButtonp->setEnabled(TRUE);
		}

		char name[MAX_STRING];		/*Flawfinder: ignore*/
		char desc[MAX_STRING];		/*Flawfinder: ignore*/
		S32 actual_area;
		S32 billable_area;
		U8 flags;
		F32 global_x;
		F32 global_y;
		char sim_name[MAX_STRING];		/*Flawfinder: ignore*/
		for(S32 i = first_block; i < count; ++i)
		{
			msg->getUUID("QueryData", "OwnerID", owner_id, i);
			msg->getString("QueryData", "Name", MAX_STRING, name, i);
			msg->getString("QueryData", "Desc", MAX_STRING, desc, i);
			msg->getS32("QueryData", "ActualArea", actual_area, i);
			msg->getS32("QueryData", "BillableArea", billable_area, i);
			msg->getU8("QueryData", "Flags", flags, i);
			msg->getF32("QueryData", "GlobalX", global_x, i);
			msg->getF32("QueryData", "GlobalY", global_y, i);
			msg->getString("QueryData", "SimName", MAX_STRING, sim_name, i);

			S32 region_x = llround(global_x) % REGION_WIDTH_UNITS;
			S32 region_y = llround(global_y) % REGION_WIDTH_UNITS;
			char location[MAX_STRING];		/*Flawfinder: ignore*/
			snprintf(location, MAX_STRING, "%s (%d, %d)", sim_name, region_x, region_y);			/* Flawfinder: ignore */
			char area[MAX_STRING];		/*Flawfinder: ignore*/
			if(billable_area == actual_area)
			{
				snprintf(area, MAX_STRING, "%d", billable_area);			/* Flawfinder: ignore */
			}
			else
			{
				snprintf(area, MAX_STRING, "%d / %d", billable_area, actual_area);			/* Flawfinder: ignore */
			}
			char hidden[MAX_STRING];		/*Flawfinder: ignore*/
			snprintf(hidden, MAX_STRING, "%f %f", global_x, global_y);			/* Flawfinder: ignore */

			LLSD row;

			row["columns"][0]["column"] = "name";
			row["columns"][0]["value"] = name;
			row["columns"][0]["font"] = "SANSSERIFSMALL";

			row["columns"][1]["column"] = "location";
			row["columns"][1]["value"] = location;
			row["columns"][1]["font"] = "SANSSERIFSMALL";
			
			row["columns"][2]["column"] = "area";
			row["columns"][2]["value"] = area;
			row["columns"][2]["font"] = "SANSSERIFSMALL";
			
			row["columns"][3]["column"] = "hidden";
			row["columns"][3]["value"] = hidden;
			
			mGroupParcelsp->addElement(row, ADD_SORTED);
		}
	}
}

//*************************************
//** LLPanelGroupLandMoney Functions **
//*************************************

//static 
void* LLPanelGroupLandMoney::createTab(void* data)
{
	LLUUID* group_id = static_cast<LLUUID*>(data);
	return new LLPanelGroupLandMoney("panel group land money", *group_id);
}

//static
LLMap<LLUUID, LLPanelGroupLandMoney*> LLPanelGroupLandMoney::sGroupIDs;

LLPanelGroupLandMoney::LLPanelGroupLandMoney(const std::string& name, 
											 const LLUUID& group_id) :
	LLPanelGroupTab(name, group_id) 
{
	mImplementationp = new impl(*this, group_id);

	//problem what if someone has both the group floater open and the finder
	//open to the same group?  Some maps that map group ids to panels
	//will then only be working for the last panel for a given group id :(
	LLPanelGroupLandMoney::sGroupIDs.addData(group_id, this);
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
		LLTabContainerCommon* tabp = (LLTabContainerCommon*) getChildByName("group_money_tab_container");

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

	update(GC_ALL);
}

void LLPanelGroupLandMoney::update(LLGroupChange gc)
{
	if (gc != GC_ALL) return;  //Don't update if it's the wrong panel!

	LLTabContainerCommon* tabp = (LLTabContainerCommon*) getChildByName("group_money_tab_container");

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

bool LLPanelGroupLandMoney::needsApply(LLString& mesg)
{
	return mImplementationp->mNeedsApply;
}

bool LLPanelGroupLandMoney::apply(LLString& mesg)
{
	if (!mImplementationp->applyContribution() )
	{
		mesg.assign(getUIString("land_contrib_error")); 
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
		(LLIconCtrl*) getChildByName("group_over_limit_icon");
	mImplementationp->mGroupOverLimitTextp = 
		(LLTextBox*) getChildByName("group_over_limit_text");

	mImplementationp->mYourContributionEditorp 
		= (LLLineEditor*) getChildByName("your_contribution_line_editor");
	if ( mImplementationp->mYourContributionEditorp )
	{
		LLLineEditor* editor = mImplementationp->mYourContributionEditorp;

	    editor->setCommitCallback(mImplementationp->contributionCommitCallback);
		editor->setKeystrokeCallback(mImplementationp->contributionKeystrokeCallback);
		editor->setCallbackUserData(this);
	}

	mImplementationp->mMapButtonp = (LLButton*) getChildByName("map_button");

	mImplementationp->mGroupParcelsp = 
		(LLScrollListCtrl*) getChildByName("group_parcel_list");

	LLTextBox *no_permsp = 
		(LLTextBox*) getChildByName("cant_view_group_land_text");
	if ( no_permsp )
	{
		mImplementationp->mCantViewParcelsText = no_permsp->getText();
		removeChild(no_permsp, TRUE);
	}

	no_permsp = (LLTextBox*) getChildByName("cant_view_group_accounting_text");
	if ( no_permsp )
	{
		mImplementationp->mCantViewAccountsText = no_permsp->getText();
		removeChild(no_permsp, TRUE);
	}

	
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

	if ( mImplementationp->mMapButtonp )
	{
		mImplementationp->mMapButtonp->setEnabled(FALSE);
	}

	if ( !can_view )
	{
		if ( mImplementationp->mGroupParcelsp )
		{
			mImplementationp->mGroupParcelsp->addSimpleItem(
							mImplementationp->mCantViewParcelsText);
			mImplementationp->mGroupParcelsp->setEnabled(FALSE);
		}
	}



	LLButton* earlierp, *laterp;
	LLTextEditor* textp;
	LLPanel* panelp;

	LLTabContainerCommon* tabcp = (LLTabContainerCommon*) 
		getChildByName("group_money_tab_container", true);

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

	LLString loading_text = childGetText("loading_txt");
	
	//pull out the widgets for the L$ details tab
	earlierp = (LLButton*) getChildByName("earlier_details_button", true);
	laterp = (LLButton*) getChildByName("later_details_button", true);
	textp = (LLTextEditor*) getChildByName("group_money_details_text", true);
	panelp = (LLPanel*) getChildByName("group_money_details_tab", true);

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
												   loading_text,
												   mGroupID);
	}

	textp = (LLTextEditor*) getChildByName("group_money_planning_text", true);
	panelp = (LLPanel*) getChildByName("group_money_planning_tab", true);

	if ( !can_view )
	{
		textp->setText(mImplementationp->mCantViewAccountsText);
	}
	else
	{
		mImplementationp->mMoneyPlanningTabEHp = 
			new LLGroupMoneyPlanningTabEventHandler(textp,
													tabcp,
													panelp,
													loading_text,
													mGroupID);
	}

	//pull out the widgets for the L$ sales tab
	earlierp = (LLButton*) getChildByName("earlier_sales_button", true);
	laterp = (LLButton*) getChildByName("later_sales_button", true);
	textp = (LLTextEditor*) getChildByName("group_money_sales_text", true);
	panelp = (LLPanel*) getChildByName("group_money_sales_tab", true);

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
												 loading_text,
												 mGroupID);
	}

	return LLPanelGroupTab::postBuild();
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

//*************************************************
//** LLGroupMoneyTabEventHandler::impl Functions **
//*************************************************

class LLGroupMoneyTabEventHandler::impl
{
public:
	impl(LLButton* earlier_buttonp,
		 LLButton* later_buttonp,
		 LLTextEditor* text_editorp,
		 LLPanel* tabpanelp,
		 const LLString& loading_text,
		 const LLUUID& group_id,
		 S32 interval_length_days,
		 S32 max_interval_days);
	~impl();

	bool getCanClickLater();
	bool getCanClickEarlier();

	void updateButtons();

//member variables
public:
	LLUUID mGroupID;
	LLUUID mPanelID;

	LLPanel* mTabPanelp;

	int mIntervalLength;
	int mMaxInterval;
	int mCurrentInterval;

	LLTextEditor* mTextEditorp;
	LLButton*     mEarlierButtonp;
	LLButton*     mLaterButtonp;

	LLString mLoadingText;
};

LLGroupMoneyTabEventHandler::impl::impl(LLButton* earlier_buttonp,
										LLButton* later_buttonp,
										LLTextEditor* text_editorp,
										LLPanel* tabpanelp,
										const LLString& loading_text,
										const LLUUID& group_id,
										S32 interval_length_days,
										S32 max_interval_days)
{
	mGroupID = group_id;
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

LLGroupMoneyTabEventHandler::impl::~impl()
{
}

bool LLGroupMoneyTabEventHandler::impl::getCanClickEarlier()
{
	return (mCurrentInterval < mMaxInterval);
}

bool LLGroupMoneyTabEventHandler::impl::getCanClickLater()
{
	return ( mCurrentInterval > 0 );
}

void LLGroupMoneyTabEventHandler::impl::updateButtons()
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
														 LLTabContainerCommon* tab_containerp,
														 LLPanel* panelp,
														 const LLString& loading_text,
														 const LLUUID& group_id,
														 S32 interval_length_days,
														 S32 max_interval_days)
{
	mImplementationp = new impl(earlier_buttonp,
								later_buttonp,
								text_editorp,
								panelp,
								loading_text,
								group_id,
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
		tab_containerp->setTabChangeCallback(panelp, clickTabCallback);
		tab_containerp->setTabUserData(panelp, this);
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

//static
void LLGroupMoneyTabEventHandler::clickTabCallback(void* data, bool from_click)
{
	LLGroupMoneyTabEventHandler* selfp = (LLGroupMoneyTabEventHandler*) data;
	if ( selfp ) selfp->onClickTab();
}

//**************************************************
//** LLGroupMoneyDetailsTabEventHandler Functions **
//**************************************************

LLGroupMoneyDetailsTabEventHandler::LLGroupMoneyDetailsTabEventHandler(LLButton* earlier_buttonp,
																	   LLButton* later_buttonp,
																	   LLTextEditor* text_editorp,
																	   LLTabContainerCommon* tab_containerp,
																	   LLPanel* panelp,
																	   const LLString& loading_text,
																	   const LLUUID& group_id)
	: LLGroupMoneyTabEventHandler(earlier_buttonp,
								  later_buttonp,
								  text_editorp,
								  tab_containerp,
								  panelp,
								  loading_text,
								  group_id,
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
	msg->addUUIDFast(_PREHASH_GroupID,  mImplementationp->mGroupID );
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
	if (mImplementationp->mGroupID != group_id) 
	{
		llwarns << "Group Account details not for this group!" << llendl;
		return;
	}

	char line[MAX_STRING];		/*Flawfinder: ignore*/
	LLString text;

	char start_date[MAX_STRING];		/*Flawfinder: ignore*/
	S32 interval_days;
	S32 current_interval;

	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_IntervalDays, interval_days );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_CurrentInterval, current_interval );
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_StartDate, MAX_STRING, start_date);

	if ( interval_days != mImplementationp->mIntervalLength || 
		 current_interval != mImplementationp->mCurrentInterval )
	{
		llinfos << "Out of date details packet " << interval_days << " " 
			<< current_interval << llendl;
		return;
	}

	snprintf(line, MAX_STRING,  "%s\n\n", start_date);			/* Flawfinder: ignore */
	text.append(line);

	S32 total_amount = 0;
	S32 transactions = msg->getNumberOfBlocksFast(_PREHASH_HistoryData);
	for(S32 i = 0; i < transactions; i++)
	{
		S32			amount = 0;
		char		desc[MAX_STRING];		/*Flawfinder: ignore*/

		msg->getStringFast(_PREHASH_HistoryData, _PREHASH_Description,	MAX_STRING, desc, i );
		msg->getS32Fast(_PREHASH_HistoryData, _PREHASH_Amount,		amount, i);

		if (amount != 0)
		{
			snprintf(line, MAX_STRING, "%-24s %6d\n", desc, amount );			/* Flawfinder: ignore */
			text.append(line);
		}
		else
		{
			// skip it
		}

		total_amount += amount;
	}

	text.append(1, '\n');

	snprintf(line, MAX_STRING, "%-24s %6d\n", "Total", total_amount );			/* Flawfinder: ignore */
	text.append(line);

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
																   LLTabContainerCommon* tab_containerp,
																   LLPanel* panelp,
																   const LLString& loading_text,
																   const LLUUID& group_id)
	: LLGroupMoneyTabEventHandler(earlier_buttonp,
								  later_buttonp,
								  text_editorp,
								  tab_containerp,
								  panelp,
								  loading_text,
								  group_id,
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
	msg->addUUIDFast(_PREHASH_GroupID, mImplementationp->mGroupID );
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
	if (mImplementationp->mGroupID != group_id) 
	{
		llwarns << "Group Account Transactions not for this group!" << llendl;
		return;
	}

	char line[MAX_STRING];		/*Flawfinder: ignore*/
	std::string text = mImplementationp->mTextEditorp->getText();

	char start_date[MAX_STRING];		/*Flawfinder: ignore*/
	S32 interval_days;
	S32 current_interval;

	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_IntervalDays, interval_days );
	msg->getS32Fast(_PREHASH_MoneyData, _PREHASH_CurrentInterval, current_interval );
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_StartDate, MAX_STRING, start_date);

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
		text.clear();

		snprintf(line, MAX_STRING, "%s\n\n", start_date); 			/* Flawfinder: ignore */
		text.append(line);
	}

	S32 transactions = msg->getNumberOfBlocksFast(_PREHASH_HistoryData);
	if (transactions == 0)
	{
		text.append("(none)");
	}
	else
	{
		for(S32 i = 0; i < transactions; i++)
		{
			const S32 SHORT_STRING = 64;
			char		time[SHORT_STRING];		/*Flawfinder: ignore*/
			S32			type = 0;
			S32			amount = 0;
			char		user[SHORT_STRING];		/*Flawfinder: ignore*/
			char		item[SHORT_STRING];		/*Flawfinder: ignore*/

			msg->getStringFast(_PREHASH_HistoryData, _PREHASH_Time,		SHORT_STRING, time, i);
			msg->getStringFast(_PREHASH_HistoryData, _PREHASH_User,		SHORT_STRING, user, i );
			msg->getS32Fast(_PREHASH_HistoryData, _PREHASH_Type,		type, i);
			msg->getStringFast(_PREHASH_HistoryData, _PREHASH_Item,		SHORT_STRING, item, i );
			msg->getS32Fast(_PREHASH_HistoryData, _PREHASH_Amount,	amount, i);

			if (amount != 0)
			{
				char* verb;

				switch(type)
				{
				case TRANS_OBJECT_SALE:
					verb = "bought";
					break;
				case TRANS_GIFT:
					verb = "paid you";
					break;
				case TRANS_PAY_OBJECT:
					verb = "paid into";
					break;
				case TRANS_LAND_PASS_SALE:
					verb = "bought pass to";
					break;
				case TRANS_EVENT_FEE:
					verb = "paid fee for event";
					break;
				case TRANS_EVENT_PRIZE:
					verb = "paid prize for event";
					break;
				default:
					verb = "";
					break;
				}

				snprintf(line, sizeof(line), "%s %6d - %s %s %s\n", time, amount, user, verb, item);			/* Flawfinder: ignore */
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
																		 LLTabContainerCommon* tab_containerp,
																		 LLPanel* panelp,
																		 const LLString& loading_text,
																		 const LLUUID& group_id)
	: LLGroupMoneyTabEventHandler(NULL,
								  NULL,
								  text_editorp,
								  tab_containerp,
								  panelp,
								  loading_text,
								  group_id,
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
	msg->addUUIDFast(_PREHASH_GroupID, mImplementationp->mGroupID );
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
	if (mImplementationp->mGroupID != group_id) 
	{
		llwarns << "Group Account Summary received not for this group!" << llendl;
		return;
	}

	char line[MAX_STRING];		/*Flawfinder: ignore*/
	LLString text;

	char start_date[MAX_STRING];		/*Flawfinder: ignore*/
	char last_stipend_date[MAX_STRING];		/*Flawfinder: ignore*/
	char next_stipend_date[MAX_STRING];		/*Flawfinder: ignore*/
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
	S32 cur_total_tax;
	S32 proj_object_tax;
	S32 proj_light_tax;
	S32 proj_land_tax;
	S32 proj_group_tax;
	S32 proj_parcel_dir_fee;
	S32 proj_total_tax;
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

	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_StartDate, MAX_STRING, start_date);
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_LastTaxDate, MAX_STRING, last_stipend_date);
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_TaxDate, MAX_STRING, next_stipend_date);

	cur_total_tax = cur_object_tax + cur_light_tax + cur_land_tax + cur_group_tax +  cur_parcel_dir_fee;
	proj_total_tax = proj_object_tax + proj_light_tax + proj_land_tax + proj_group_tax + proj_parcel_dir_fee;

	if (interval_days != mImplementationp->mIntervalLength || 
		current_interval != mImplementationp->mCurrentInterval)
	{
		llinfos << "Out of date summary packet " << interval_days << " " 
			<< current_interval << llendl;
		return;
	}

	snprintf(line, MAX_STRING, "Summary for this week, beginning on %s\n", start_date);			/* Flawfinder: ignore */
	text.append(line);

	if (current_interval == 0)
	{
		snprintf(line, MAX_STRING, "The next stipend day is %s\n\n", next_stipend_date);			/* Flawfinder: ignore */
		text.append(line);
		snprintf(line, MAX_STRING, "%-24sL$%6d\n", "Balance", balance );			/* Flawfinder: ignore */
		text.append(line);

		text.append(1, '\n');
	}

	snprintf(line, MAX_STRING,  "                      Group       Individual Share\n");			/* Flawfinder: ignore */
	text.append(line);
	snprintf(line, MAX_STRING,     "%-24s %6d      %6d \n", "Credits", total_credits, (S32)floor((F32)total_credits/(F32)non_exempt_members));			/* Flawfinder: ignore */
	text.append(line);
	snprintf(line, MAX_STRING,     "%-24s %6d      %6d \n", "Debits", total_debits,  (S32)floor((F32)total_debits/(F32)non_exempt_members));			/* Flawfinder: ignore */
	text.append(line);
	snprintf(line, MAX_STRING,     "%-24s %6d      %6d \n", "Total", total_credits + total_debits,  (S32)floor((F32)(total_credits + total_debits)/(F32)non_exempt_members));			/* Flawfinder: ignore */
	text.append(line);

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
