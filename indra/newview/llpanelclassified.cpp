/** 
 * @file llpanelclassified.cpp
 * @brief LLPanelClassified class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

// Display of a classified used both for the global view in the
// Find directory, and also for each individual user's classified in their
// profile.

#include "llviewerprecompiledheaders.h"

#include "llpanelclassified.h"

#include "lldir.h"
#include "lldispatcher.h"
#include "llfloaterreg.h"
#include "llhttpclient.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "lltabcontainer.h"
#include "message.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llclassifiedflags.h"
#include "llclassifiedstatsresponder.h"
#include "llcommandhandler.h" // for classified HTML detail page click tracking
#include "llviewercontrol.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "llcombobox.h"
#include "lltexturectrl.h"
#include "lltexteditor.h"
#include "lluiconstants.h"
#include "llurldispatcher.h"	// for classified HTML detail click teleports
#include "lluictrlfactory.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llworldmap.h"
#include "llfloaterworldmap.h"
#include "llviewergenericmessage.h"	// send_generic_message
#include "llviewerregion.h"
#include "llviewerwindow.h"	// for window width, height
#include "llappviewer.h"	// abortQuit()
#include "lltrans.h"
#include "llscrollcontainer.h"
#include "llstatusbar.h"

const S32 MINIMUM_PRICE_FOR_LISTING = 50;	// L$
const S32 MATURE_UNDEFINED = -1;
const S32 MATURE_CONTENT = 1;
const S32 PG_CONTENT = 2;
const S32 DECLINE_TO_STATE = 0;

//static
std::list<LLPanelClassified*> LLPanelClassified::sAllPanels;
std::list<LLPanelClassifiedInfo*> LLPanelClassifiedInfo::sAllPanels;

// "classifiedclickthrough"
// strings[0] = classified_id
// strings[1] = teleport_clicks
// strings[2] = map_clicks
// strings[3] = profile_clicks
class LLDispatchClassifiedClickThrough : public LLDispatchHandler
{
public:
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings)
	{
		if (strings.size() != 4) return false;
		LLUUID classified_id(strings[0]);
		S32 teleport_clicks = atoi(strings[1].c_str());
		S32 map_clicks = atoi(strings[2].c_str());
		S32 profile_clicks = atoi(strings[3].c_str());

		LLPanelClassifiedInfo::setClickThrough(
			classified_id, teleport_clicks, map_clicks, profile_clicks, false);

		return true;
	}
};
static LLDispatchClassifiedClickThrough sClassifiedClickThrough;


/* Re-expose this if we need to have classified ad HTML detail
   pages.  JC

// We need to count classified teleport clicks from the search HTML detail pages,
// so we need have a teleport that also sends a click count message.
class LLClassifiedTeleportHandler : public LLCommandHandler
{
public:
	// don't allow from external browsers because it moves you immediately
	LLClassifiedTeleportHandler() : LLCommandHandler("classifiedteleport", UNTRUSTED_BLOCK) { }

	bool handle(const LLSD& tokens, const LLSD& queryMap)
	{
		// Need at least classified id and region name, so 2 params
		if (tokens.size() < 2) return false;
		LLUUID classified_id = tokens[0].asUUID();
		if (classified_id.isNull()) return false;
		// *HACK: construct a SLURL to do the teleport
		std::string url("secondlife:///app/teleport/");
		// skip the uuid we took off above, rebuild URL
		// separated by slashes.
		for (S32 i = 1; i < tokens.size(); ++i)
		{
			url += tokens[i].asString();
			url += "/";
		}
		llinfos << "classified teleport to " << url << llendl;
		// *TODO: separately track old search, sidebar, and new search
		// Right now detail HTML pages count as new search.
		const bool from_search = true;
		LLPanelClassified::sendClassifiedClickMessage(classified_id, "teleport", from_search);
		// Invoke teleport
		LLMediaCtrl* web = NULL;
		const bool trusted_browser = true;
		return LLURLDispatcher::dispatch(url, web, trusted_browser);
	}
};
// Creating the object registers with the dispatcher.
LLClassifiedTeleportHandler gClassifiedTeleportHandler;
*/

LLPanelClassified::LLPanelClassified(bool in_finder, bool from_search)
:	LLPanel(),
	mInFinder(in_finder),
	mFromSearch(from_search),
	mDirty(false),
	mForceClose(false),
	mLocationChanged(false),
	mClassifiedID(),
	mCreatorID(),
	mPriceForListing(0),
	mDataRequested(FALSE),
	mPaidFor(FALSE),
	mPosGlobal(),
	mSnapshotCtrl(NULL),
	mNameEditor(NULL),
	mDescEditor(NULL),
	mLocationEditor(NULL),
	mCategoryCombo(NULL),
	mMatureCombo(NULL),
	mAutoRenewCheck(NULL),
	mUpdateBtn(NULL),
	mTeleportBtn(NULL),
	mMapBtn(NULL),
	mProfileBtn(NULL),
	mInfoText(NULL),
	mSetBtn(NULL),
	mClickThroughText(NULL),
	mTeleportClicksOld(0),
	mMapClicksOld(0),
	mProfileClicksOld(0),
	mTeleportClicksNew(0),
	mMapClicksNew(0),
	mProfileClicksNew(0)

{
    sAllPanels.push_back(this);

	std::string classified_def_file;
	if (mInFinder)
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_classified.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_classified.xml");
	}

	// Register dispatcher
	gGenericDispatcher.addHandler("classifiedclickthrough", 
								  &sClassifiedClickThrough);
}


LLPanelClassified::~LLPanelClassified()
{
    sAllPanels.remove(this);
}


void LLPanelClassified::reset()
{
	mClassifiedID.setNull();
	mCreatorID.setNull();
	mParcelID.setNull();

	// Don't request data, this isn't valid
	mDataRequested = TRUE;

	mDirty = false;
	mPaidFor = FALSE;

	mPosGlobal.clearVec();

	clearCtrls();
	resetDirty();
}


BOOL LLPanelClassified::postBuild()
{
    mSnapshotCtrl = getChild<LLTextureCtrl>("snapshot_ctrl");
	mSnapshotCtrl->setCommitCallback(onCommitAny, this);
	mSnapshotSize = mSnapshotCtrl->getRect();

    mNameEditor = getChild<LLLineEditor>("given_name_editor");
	mNameEditor->setMaxTextLength(DB_PARCEL_NAME_LEN);
	mNameEditor->setCommitOnFocusLost(TRUE);
	mNameEditor->setFocusReceivedCallback(boost::bind(focusReceived, _1, this));
	mNameEditor->setCommitCallback(onCommitAny, this);
	mNameEditor->setPrevalidate( LLTextValidate::validateASCII );

    mDescEditor = getChild<LLTextEditor>("desc_editor");
	mDescEditor->setCommitOnFocusLost(TRUE);
	mDescEditor->setFocusReceivedCallback(boost::bind(focusReceived, _1, this));
	mDescEditor->setCommitCallback(onCommitAny, this);
	
    mLocationEditor = getChild<LLLineEditor>("location_editor");

    mSetBtn = getChild<LLButton>( "set_location_btn");
    mSetBtn->setClickedCallback(onClickSet, this);

    mTeleportBtn = getChild<LLButton>( "classified_teleport_btn");
    mTeleportBtn->setClickedCallback(onClickTeleport, this);

    mMapBtn = getChild<LLButton>( "classified_map_btn");
    mMapBtn->setClickedCallback(onClickMap, this);

	if(mInFinder)
	{
		mProfileBtn  = getChild<LLButton>( "classified_profile_btn");
		mProfileBtn->setClickedCallback(onClickProfile, this);
	}

	mCategoryCombo = getChild<LLComboBox>( "classified_category_combo");
	LLClassifiedInfo::cat_map::iterator iter;
	for (iter = LLClassifiedInfo::sCategories.begin();
		iter != LLClassifiedInfo::sCategories.end();
		iter++)
	{
		mCategoryCombo->add(LLTrans::getString(iter->second), (void *)((intptr_t)iter->first), ADD_BOTTOM);
	}
	mCategoryCombo->setCurrentByIndex(0);
	mCategoryCombo->setCommitCallback(onCommitAny, this);

	mMatureCombo = getChild<LLComboBox>( "classified_mature_check");
	mMatureCombo->setCurrentByIndex(0);
	mMatureCombo->setCommitCallback(onCommitAny, this);
	if (gAgent.wantsPGOnly())
	{
		// Teens don't get to set mature flag. JC
		mMatureCombo->setVisible(FALSE);
		mMatureCombo->setCurrentByIndex(PG_CONTENT);
	}

	if (!mInFinder)
	{
		mAutoRenewCheck = getChild<LLCheckBoxCtrl>( "auto_renew_check");
		mAutoRenewCheck->setCommitCallback(onCommitAny, this);
	}

	mUpdateBtn = getChild<LLButton>("classified_update_btn");
    mUpdateBtn->setClickedCallback(onClickUpdate, this);

	if (!mInFinder)
	{
		mClickThroughText = getChild<LLTextBox>("click_through_text");
	}
	
	resetDirty();
    return TRUE;
}

BOOL LLPanelClassified::titleIsValid()
{
	// Disallow leading spaces, punctuation, etc. that screw up
	// sort order.
	const std::string& name = mNameEditor->getText();
	if (name.empty())
	{
		LLNotificationsUtil::add("BlankClassifiedName");
		return FALSE;
	}
	if (!isalnum(name[0]))
	{
		LLNotificationsUtil::add("ClassifiedMustBeAlphanumeric");
		return FALSE;
	}

	return TRUE;
}

void LLPanelClassified::apply()
{
	// Apply is used for automatically saving results, so only
	// do that if there is a difference, and this is a save not create.
	if (checkDirty() && mPaidFor)
	{
		sendClassifiedInfoUpdate();
	}
}

bool LLPanelClassified::saveCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	switch(option)
	{
		case 0: // Save
			sendClassifiedInfoUpdate();
			// fall through to close

		case 1: // Don't Save
			{
				mForceClose = true;
				// Close containing floater
				LLFloater* parent_floater = gFloaterView->getParentFloater(this);
				if (parent_floater)
				{
					parent_floater->closeFloater();
				}
			}
			break;

		case 2: // Cancel
		default:
            LLAppViewer::instance()->abortQuit();
			break;
	}
	return false;
}


BOOL LLPanelClassified::canClose()
{
	if (mForceClose || !checkDirty()) 
		return TRUE;

	LLSD args;
	args["NAME"] = mNameEditor->getText();
	LLNotificationsUtil::add("ClassifiedSave", args, LLSD(), boost::bind(&LLPanelClassified::saveCallback, this, _1, _2));
	return FALSE;
}

// Fill in some reasonable defaults for a new classified.
void LLPanelClassified::initNewClassified()
{
	// TODO:  Don't generate this on the client.
	mClassifiedID.generate();

	mCreatorID = gAgent.getID();

	mPosGlobal = gAgent.getPositionGlobal();

	mPaidFor = FALSE;

	// Try to fill in the current parcel
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		mNameEditor->setText(parcel->getName());
		//mDescEditor->setText(parcel->getDesc());
		mSnapshotCtrl->setImageAssetID(parcel->getSnapshotID());
		//mPriceEditor->setText("0");
		mCategoryCombo->setCurrentByIndex(0);
	}

	mUpdateBtn->setLabel(getString("publish_txt"));
	
	// simulate clicking the "location" button
	LLPanelClassified::onClickSet(this);
}


void LLPanelClassified::setClassifiedID(const LLUUID& id)
{
	mClassifiedID = id;
}

//static
void LLPanelClassified::setClickThrough(const LLUUID& classified_id,
										S32 teleport,
										S32 map,
										S32 profile,
										bool from_new_table)
{
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelClassified* self = *iter;
		// For top picks, must match pick id
		if (self->mClassifiedID != classified_id)
		{
			continue;
		}

		// We need to check to see if the data came from the new stat_table 
		// or the old classified table. We also need to cache the data from 
		// the two separate sources so as to display the aggregate totals.

		if (from_new_table)
		{
			self->mTeleportClicksNew = teleport;
			self->mMapClicksNew = map;
			self->mProfileClicksNew = profile;
		}
		else
		{
			self->mTeleportClicksOld = teleport;
			self->mMapClicksOld = map;
			self->mProfileClicksOld = profile;
		}

		if (self->mClickThroughText)
		{
			LLStringUtil::format_map_t args;
			args["[TELEPORT]"] = llformat ("%d", self->mTeleportClicksNew + self->mTeleportClicksOld);
			args["[MAP]"] = llformat ("%d", self->mMapClicksNew + self->mMapClicksOld);
			args["[PROFILE]"] = llformat ("%d", self->mProfileClicksNew + self->mProfileClicksOld);
			std::string msg = LLTrans::getString ("ClassifiedClicksTxt", args);
			self->mClickThroughText->setText(msg);
		}
	}
}

// Schedules the panel to request data
// from the server next time it is drawn.
void LLPanelClassified::markForServerRequest()
{
	mDataRequested = FALSE;
}


std::string LLPanelClassified::getClassifiedName()
{
	return mNameEditor->getText();
}


void LLPanelClassified::sendClassifiedInfoRequest()
{
    LLMessageSystem *msg = gMessageSystem;

	if (mClassifiedID != mRequestedID)
	{
		msg->newMessageFast(_PREHASH_ClassifiedInfoRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->nextBlockFast(_PREHASH_Data);
		msg->addUUIDFast(_PREHASH_ClassifiedID, mClassifiedID);
		gAgent.sendReliableMessage();

		mDataRequested = TRUE;
		mRequestedID = mClassifiedID;

		// While we're at it let's get the stats from the new table if that
		// capability exists.
		std::string url = gAgent.getRegion()->getCapability("SearchStatRequest");
		LLSD body;
		body["classified_id"] = mClassifiedID;

		if (!url.empty())
		{
			llinfos << "Classified stat request via capability" << llendl;
			LLHTTPClient::post(url, body, new LLClassifiedStatsResponder(mClassifiedID));
		}
	}
}


void LLPanelClassified::sendClassifiedInfoUpdate()
{
	// If we don't have a classified id yet, we'll need to generate one,
	// otherwise we'll keep overwriting classified_id 00000 in the database.
	if (mClassifiedID.isNull())
	{
		// TODO:  Don't do this on the client.
		mClassifiedID.generate();
	}

	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ClassifiedInfoUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addUUIDFast(_PREHASH_ClassifiedID, mClassifiedID);
	// TODO: fix this
	U32 category = mCategoryCombo->getCurrentIndex() + 1;
	msg->addU32Fast(_PREHASH_Category, category);
	msg->addStringFast(_PREHASH_Name, mNameEditor->getText());
	msg->addStringFast(_PREHASH_Desc, mDescEditor->getText());

	// fills in on simulator if null
	msg->addUUIDFast(_PREHASH_ParcelID, mParcelID);
	// fills in on simulator if null
	msg->addU32Fast(_PREHASH_ParentEstate, 0);
	msg->addUUIDFast(_PREHASH_SnapshotID, mSnapshotCtrl->getImageAssetID());
	msg->addVector3dFast(_PREHASH_PosGlobal, mPosGlobal);
	BOOL mature = mMatureCombo->getCurrentIndex() == MATURE_CONTENT;
	BOOL auto_renew = FALSE;
	if (mAutoRenewCheck) 
	{
		auto_renew = mAutoRenewCheck->get();
	}
    // These flags doesn't matter here.
    const bool adult_enabled = false;
	const bool is_pg = false;
	U8 flags = pack_classified_flags_request(auto_renew, is_pg, mature, adult_enabled);
	msg->addU8Fast(_PREHASH_ClassifiedFlags, flags);
	msg->addS32("PriceForListing", mPriceForListing);
	gAgent.sendReliableMessage();

	mDirty = false;
}


//static
void LLPanelClassified::processClassifiedInfoReply(LLMessageSystem *msg, void **)
{
	lldebugs << "processClassifiedInfoReply()" << llendl;
    // Extract the agent id and verify the message is for this
    // client.
    LLUUID agent_id;
    msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );
    if (agent_id != gAgent.getID())
    {
        llwarns << "Agent ID mismatch in processClassifiedInfoReply"
            << llendl;
		return;
    }

    LLUUID classified_id;
    msg->getUUIDFast(_PREHASH_Data, _PREHASH_ClassifiedID, classified_id);

    LLUUID creator_id;
    msg->getUUIDFast(_PREHASH_Data, _PREHASH_CreatorID, creator_id);

    LLUUID parcel_id;
    msg->getUUIDFast(_PREHASH_Data, _PREHASH_ParcelID, parcel_id);

	std::string name;
	msg->getStringFast(_PREHASH_Data, _PREHASH_Name, name);

	std::string desc;
	msg->getStringFast(_PREHASH_Data, _PREHASH_Desc, desc);

	LLUUID snapshot_id;
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_SnapshotID, snapshot_id);

    // "Location text" is actually the original
    // name that owner gave the parcel, and the location.
	std::string location_text;

    msg->getStringFast(_PREHASH_Data, _PREHASH_ParcelName, location_text);
	if (!location_text.empty())
	{
		location_text.append(", ");
	}

	std::string sim_name;
	msg->getStringFast(_PREHASH_Data, _PREHASH_SimName, sim_name);

	LLVector3d pos_global;
	msg->getVector3dFast(_PREHASH_Data, _PREHASH_PosGlobal, pos_global);

    S32 region_x = llround((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)pos_global.mdV[VZ]);

	std::string buffer = llformat("%s (%d, %d, %d)", sim_name.c_str(), region_x, region_y, region_z);
    location_text.append(buffer);

	U8 flags;
	msg->getU8Fast(_PREHASH_Data, _PREHASH_ClassifiedFlags, flags);
	//BOOL enabled = is_cf_enabled(flags);
	bool mature = is_cf_mature(flags);
	bool auto_renew = is_cf_auto_renew(flags);

	U32 date = 0;
	msg->getU32Fast(_PREHASH_Data, _PREHASH_CreationDate, date);
	time_t tim = date;

	// future use
	U32 expiration_date = 0;
	msg->getU32("Data", "ExpirationDate", expiration_date);

	U32 category = 0;
	msg->getU32Fast(_PREHASH_Data, _PREHASH_Category, category);

	S32 price_for_listing = 0;
	msg->getS32("Data", "PriceForListing", price_for_listing);

    // Look up the panel to fill in
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelClassified* self = *iter;
		// For top picks, must match pick id
		if (self->mClassifiedID != classified_id)
		{
			continue;
		}

        // Found the panel, now fill in the information
		self->mClassifiedID = classified_id;
		self->mCreatorID = creator_id;
		self->mParcelID = parcel_id;
		self->mPriceForListing = price_for_listing;
		self->mSimName.assign(sim_name);
		self->mPosGlobal = pos_global;

		// Update UI controls
        self->mNameEditor->setText(name);
        self->mDescEditor->setText(desc);
        self->mSnapshotCtrl->setImageAssetID(snapshot_id);
        self->mLocationEditor->setText(location_text);
		self->mLocationChanged = false;

		self->mCategoryCombo->setCurrentByIndex(category - 1);
		if(mature)
		{
			self->mMatureCombo->setCurrentByIndex(MATURE_CONTENT);
		}
		else
		{
			self->mMatureCombo->setCurrentByIndex(PG_CONTENT);
		}
		if (self->mAutoRenewCheck)
		{
			self->mAutoRenewCheck->set(auto_renew);
		}

		std::string dateStr = self->getString("dateStr");
		LLSD substitution;
		substitution["datetime"] = (S32) tim;
		LLStringUtil::format (dateStr, substitution);

		LLStringUtil::format_map_t string_args;
		string_args["[DATE]"] = dateStr;
		string_args["[AMT]"] = llformat("%d", price_for_listing);
		self->childSetText("classified_info_text", self->getString("ad_placed_paid", string_args));

		// If we got data from the database, we know the listing is paid for.
		self->mPaidFor = TRUE;

		self->mUpdateBtn->setLabel(self->getString("update_txt"));

		self->resetDirty();

		// I don't know if a second call is deliberate or a bad merge, so I'm leaving it here. 
		self->resetDirty();
    }
}

void LLPanelClassified::draw()
{
	refresh();

	LLPanel::draw();
}


void LLPanelClassified::refresh()
{
	if (!mDataRequested)
	{
		sendClassifiedInfoRequest();
	}

    // Check for god mode
    BOOL godlike = gAgent.isGodlike();
	BOOL is_self = (gAgent.getID() == mCreatorID);

    // Set button visibility/enablement appropriately
	if (mInFinder)
	{

		// End user doesn't ned to see price twice, or date posted.

		mSnapshotCtrl->setEnabled(godlike);
		if(godlike)
		{
			//make it smaller, so text is more legible
			mSnapshotCtrl->setOrigin(20, 175);
			mSnapshotCtrl->reshape(300, 200);
		}
		else
		{
			mSnapshotCtrl->setOrigin(mSnapshotSize.mLeft, mSnapshotSize.mBottom);
			mSnapshotCtrl->reshape(mSnapshotSize.getWidth(), mSnapshotSize.getHeight());
			//normal
		}
		mNameEditor->setEnabled(godlike);
		mDescEditor->setEnabled(godlike);
		mCategoryCombo->setEnabled(godlike);
		mCategoryCombo->setVisible(godlike);

		mMatureCombo->setEnabled(godlike);
		mMatureCombo->setVisible(godlike);

		// Jesse (who is the only one who uses this, as far as we can tell
		// Says that he does not want a set location button - he has used it
		// accidently in the past.
		mSetBtn->setVisible(FALSE);
		mSetBtn->setEnabled(FALSE);

		mUpdateBtn->setEnabled(godlike);
		mUpdateBtn->setVisible(godlike);
	}
	else
	{
		mSnapshotCtrl->setEnabled(is_self);
		mNameEditor->setEnabled(is_self);
		mDescEditor->setEnabled(is_self);
		//mPriceEditor->setEnabled(is_self);
		mCategoryCombo->setEnabled(is_self);
		mMatureCombo->setEnabled(is_self);

		if( is_self )
		{							
			if( mMatureCombo->getCurrentIndex() == 0 )
			{
				// It's a new panel.
				// PG regions should have PG classifieds. AO should have mature.
								
				setDefaultAccessCombo();
			}
		}
		
		if (mAutoRenewCheck)
		{
			mAutoRenewCheck->setEnabled(is_self);
			mAutoRenewCheck->setVisible(is_self);
		}
		
		mClickThroughText->setEnabled(is_self);
		mClickThroughText->setVisible(is_self);

		mSetBtn->setVisible(is_self);
		mSetBtn->setEnabled(is_self);

		mUpdateBtn->setEnabled(is_self && checkDirty());
		mUpdateBtn->setVisible(is_self);
	}
}

// static
void LLPanelClassified::onClickUpdate(void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;

	if(self == NULL) return;

	// Disallow leading spaces, punctuation, etc. that screw up
	// sort order.
	if ( ! self->titleIsValid() )
	{
		return;
	};

	// If user has not set mature, do not allow publish
	if(self->mMatureCombo->getCurrentIndex() == DECLINE_TO_STATE)
	{
		// Tell user about it
		LLNotificationsUtil::add("SetClassifiedMature", 
				LLSD(), 
				LLSD(), 
				boost::bind(&LLPanelClassified::confirmMature, self, _1, _2));
		return;
	}

	// Mature content flag is set, proceed
	self->gotMature();
}

// Callback from a dialog indicating response to mature notification
bool LLPanelClassified::confirmMature(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	
	// 0 == Yes
	// 1 == No
	// 2 == Cancel
	switch(option)
	{
	case 0:
		mMatureCombo->setCurrentByIndex(MATURE_CONTENT);
		break;
	case 1:
		mMatureCombo->setCurrentByIndex(PG_CONTENT);
		break;
	default:
		return false;
	}
	
	// If we got here it means they set a valid value
	gotMature();
	return false;
}

// Called after we have determined whether this classified has
// mature content or not.
void LLPanelClassified::gotMature()
{
	// if already paid for, just do the update
	if (mPaidFor)
	{
		LLNotification::Params params("PublishClassified");
		params.functor.function(boost::bind(&LLPanelClassified::confirmPublish, this, _1, _2));
		LLNotifications::instance().forceResponse(params, 0);
	}
	else
	{
		// Ask the user how much they want to pay
		LLFloaterPriceForListing::show( callbackGotPriceForListing, this );
	}
}

// static
void LLPanelClassified::callbackGotPriceForListing(S32 option, std::string text, void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;

	// Only do something if user hits publish
	if (option != 0) return;

	S32 price_for_listing = strtol(text.c_str(), NULL, 10);
	if (price_for_listing < MINIMUM_PRICE_FOR_LISTING)
	{
		LLSD args;
		std::string price_text = llformat("%d", MINIMUM_PRICE_FOR_LISTING);
		args["MIN_PRICE"] = price_text;
			
		LLNotificationsUtil::add("MinClassifiedPrice", args);
		return;
	}

	// price is acceptable, put it in the dialog for later read by 
	// update send
	self->mPriceForListing = price_for_listing;

	LLSD args;
	args["AMOUNT"] = llformat("%d", price_for_listing);
	LLNotificationsUtil::add("PublishClassified", args, LLSD(), 
									boost::bind(&LLPanelClassified::confirmPublish, self, _1, _2));
}

void LLPanelClassified::resetDirty()
{
	// Tell all the widgets to reset their dirty state since the ad was just saved
	if (mSnapshotCtrl)
		mSnapshotCtrl->resetDirty();
	if (mNameEditor)
		mNameEditor->resetDirty();
	if (mDescEditor)
		mDescEditor->resetDirty();
	if (mLocationEditor)
		mLocationEditor->resetDirty();
	mLocationChanged = false;
	if (mCategoryCombo)
		mCategoryCombo->resetDirty();
	if (mMatureCombo)
		mMatureCombo->resetDirty();
	if (mAutoRenewCheck)
		mAutoRenewCheck->resetDirty();
}

// invoked from callbackConfirmPublish
bool LLPanelClassified::confirmPublish(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	// Option 0 = publish
	if (option != 0) return false;

	sendClassifiedInfoUpdate();

	// Big hack - assume that top picks are always in a browser,
	// and non-finder-classifieds are always in a tab container.
	if (! mInFinder)
	{
		LLTabContainer* tab = (LLTabContainer*)getParent();
		tab->setCurrentTabName(mNameEditor->getText());
	}

	resetDirty();
	return false;
}


// static
void LLPanelClassified::onClickTeleport(void* data)
{
    LLPanelClassified* self = (LLPanelClassified*)data;
	LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
	
    if (!self->mPosGlobal.isExactlyZero()&&worldmap_instance)
    {
        gAgent.teleportViaLocation(self->mPosGlobal);		
        worldmap_instance->trackLocation(self->mPosGlobal);
		self->sendClassifiedClickMessage("teleport");
    }
}


// static
void LLPanelClassified::onClickMap(void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;
	LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
	if(worldmap_instance)
	{
		worldmap_instance->trackLocation(self->mPosGlobal);
		LLFloaterReg::showInstance("world_map", "center");
	}
	self->sendClassifiedClickMessage("map");
}

// static
void LLPanelClassified::onClickProfile(void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;
	LLAvatarActions::showProfile(self->mCreatorID);
	self->sendClassifiedClickMessage("profile");
}

// static
/*
void LLPanelClassified::onClickLandmark(void* data)
{
    LLPanelClassified* self = (LLPanelClassified*)data;
	create_landmark(self->mNameEditor->getText(), "", self->mPosGlobal);
}
*/

// static
void LLPanelClassified::onClickSet(void* data)
{
    LLPanelClassified* self = (LLPanelClassified*)data;

	// Save location for later.
	self->mPosGlobal = gAgent.getPositionGlobal();

	std::string location_text;
	std::string regionName = LLTrans::getString("ClassifiedUpdateAfterPublish");
	LLViewerRegion* pRegion = gAgent.getRegion();
	if (pRegion)
	{
		regionName = pRegion->getName();
	}
	location_text.assign(regionName);
	location_text.append(", ");

    S32 region_x = llround((F32)self->mPosGlobal.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)self->mPosGlobal.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)self->mPosGlobal.mdV[VZ]);
   
	location_text.append(self->mSimName);
    location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));

	self->mLocationEditor->setText(location_text);
	self->mLocationChanged = true;
	
	self->setDefaultAccessCombo();	

	// Set this to null so it updates on the next save.
	self->mParcelID.setNull();

	onCommitAny(NULL, data);
}


BOOL LLPanelClassified::checkDirty()
{
	mDirty = FALSE;
	if	( mSnapshotCtrl )			mDirty |= mSnapshotCtrl->isDirty();
	if	( mNameEditor )				mDirty |= mNameEditor->isDirty();
	if	( mDescEditor )				mDirty |= mDescEditor->isDirty();
	if	( mLocationEditor )			mDirty |= mLocationEditor->isDirty();
	if  ( mLocationChanged )		mDirty |= TRUE;
	if	( mCategoryCombo )			mDirty |= mCategoryCombo->isDirty();
	if	( mMatureCombo )			mDirty |= mMatureCombo->isDirty();
	if	( mAutoRenewCheck )			mDirty |= mAutoRenewCheck->isDirty();

	return mDirty;
}

// static
void LLPanelClassified::onCommitAny(LLUICtrl* ctrl, void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;
	if (self)
	{
		self->checkDirty();
	}
}

// static
void LLPanelClassified::focusReceived(LLFocusableElement* ctrl, void* data)
{
	// allow the data to be saved
	onCommitAny((LLUICtrl*)ctrl, data);
}


void LLPanelClassified::sendClassifiedClickMessage(const std::string& type)
{
	// You're allowed to click on your own ads to reassure yourself
	// that the system is working.
	LLSD body;
	body["type"] = type;
	body["from_search"] = mFromSearch;
	body["classified_id"] = mClassifiedID;
	body["parcel_id"] = mParcelID;
	body["dest_pos_global"] = mPosGlobal.getValue();
	body["region_name"] = mSimName;

	std::string url = gAgent.getRegion()->getCapability("SearchStatTracking");
	llinfos << "LLPanelClassified::sendClassifiedClickMessage via capability" << llendl;
	LLHTTPClient::post(url, body, new LLHTTPClient::Responder());
}

////////////////////////////////////////////////////////////////////////////////////////////

LLFloaterPriceForListing::LLFloaterPriceForListing()
:	LLFloater(LLSD()),
	mCallback(NULL),
	mUserData(NULL)
{ }

//virtual
LLFloaterPriceForListing::~LLFloaterPriceForListing()
{ }

//virtual
BOOL LLFloaterPriceForListing::postBuild()
{
	LLLineEditor* edit = getChild<LLLineEditor>("price_edit");
	if (edit)
	{
		edit->setPrevalidate(LLTextValidate::validateNonNegativeS32);
		std::string min_price = llformat("%d", MINIMUM_PRICE_FOR_LISTING);
		edit->setText(min_price);
		edit->selectAll();
		edit->setFocus(TRUE);
	}

	childSetAction("set_price_btn", onClickSetPrice, this);

	childSetAction("cancel_btn", onClickCancel, this);

	setDefaultBtn("set_price_btn");
	return TRUE;
}

//static
void LLFloaterPriceForListing::show( void (*callback)(S32, std::string, void*), void* userdata)
{
	LLFloaterPriceForListing *self = new LLFloaterPriceForListing();

	// Builds and adds to gFloaterView
	LLUICtrlFactory::getInstance()->buildFloater(self, "floater_price_for_listing.xml", NULL);
	self->center();

	self->mCallback = callback;
	self->mUserData = userdata;
}

//static
void LLFloaterPriceForListing::onClickSetPrice(void* data)
{
	buttonCore(0, data);
}

//static
void LLFloaterPriceForListing::onClickCancel(void* data)
{
	buttonCore(1, data);
}

//static
void LLFloaterPriceForListing::buttonCore(S32 button, void* data)
{
	LLFloaterPriceForListing* self = (LLFloaterPriceForListing*)data;

	if (self->mCallback)
	{
		std::string text = self->childGetText("price_edit");
		self->mCallback(button, text, self->mUserData);
		self->closeFloater();
	}
}

void LLPanelClassified::setDefaultAccessCombo()
{
	// PG regions should have PG classifieds. AO should have mature.

	LLViewerRegion *regionp = gAgent.getRegion();

	switch( regionp->getSimAccess() )
	{
		case SIM_ACCESS_PG:	
			mMatureCombo->setCurrentByIndex(PG_CONTENT);
			break;
		case SIM_ACCESS_ADULT:
			mMatureCombo->setCurrentByIndex(MATURE_CONTENT);
			break;
		default:
			// You are free to move about the cabin.
			break;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelClassifiedInfo::LLPanelClassifiedInfo()
 : LLPanel()
 , mInfoLoaded(false)
 , mScrollingPanel(NULL)
 , mScrollContainer(NULL)
 , mScrollingPanelMinHeight(0)
 , mScrollingPanelWidth(0)
 , mSnapshotStreched(false)
 , mTeleportClicksOld(0)
 , mMapClicksOld(0)
 , mProfileClicksOld(0)
 , mTeleportClicksNew(0)
 , mMapClicksNew(0)
 , mProfileClicksNew(0)
{
	sAllPanels.push_back(this);
}

LLPanelClassifiedInfo::~LLPanelClassifiedInfo()
{
	sAllPanels.remove(this);
}

// static
LLPanelClassifiedInfo* LLPanelClassifiedInfo::create()
{
	LLPanelClassifiedInfo* panel = new LLPanelClassifiedInfo();
	LLUICtrlFactory::getInstance()->buildPanel(panel, "panel_classified_info.xml");
	return panel;
}

BOOL LLPanelClassifiedInfo::postBuild()
{
	childSetAction("back_btn", boost::bind(&LLPanelClassifiedInfo::onExit, this));
	childSetAction("show_on_map_btn", boost::bind(&LLPanelClassifiedInfo::onMapClick, this));
	childSetAction("teleport_btn", boost::bind(&LLPanelClassifiedInfo::onTeleportClick, this));

	mScrollingPanel = getChild<LLPanel>("scroll_content_panel");
	mScrollContainer = getChild<LLScrollContainer>("profile_scroll");

	mScrollingPanelMinHeight = mScrollContainer->getScrolledViewRect().getHeight();
	mScrollingPanelWidth = mScrollingPanel->getRect().getWidth();

	mSnapshotRect = getChild<LLUICtrl>("classified_snapshot")->getRect();

	return TRUE;
}

void LLPanelClassifiedInfo::setExitCallback(const commit_callback_t& cb)
{
	getChild<LLButton>("back_btn")->setClickedCallback(cb);
}

void LLPanelClassifiedInfo::setEditClassifiedCallback(const commit_callback_t& cb)
{
	getChild<LLButton>("edit_btn")->setClickedCallback(cb);
}

void LLPanelClassifiedInfo::reshape(S32 width, S32 height, BOOL called_from_parent /* = TRUE */)
{
	LLPanel::reshape(width, height, called_from_parent);

	if (!mScrollContainer || !mScrollingPanel)
		return;

	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	S32 scroll_height = mScrollContainer->getRect().getHeight();
	if (mScrollingPanelMinHeight >= scroll_height)
	{
		mScrollingPanel->reshape(mScrollingPanelWidth, mScrollingPanelMinHeight);
	}
	else
	{
		mScrollingPanel->reshape(mScrollingPanelWidth + scrollbar_size, scroll_height);
	}

	mSnapshotRect = getChild<LLUICtrl>("classified_snapshot")->getRect();
}

void LLPanelClassifiedInfo::onOpen(const LLSD& key)
{
	LLUUID avatar_id = key["avatar_id"];
	if(avatar_id.isNull())
	{
		return;
	}

	if(getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
	}

	setAvatarId(avatar_id);

	resetData();
	resetControls();

	setClassifiedId(key["classified_id"]);
	setClassifiedName(key["name"]);
	setDescription(key["desc"]);
	setSnapshotId(key["snapshot_id"]);

	LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(), this);
	LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoRequest(getClassifiedId());
	gGenericDispatcher.addHandler("classifiedclickthrough", &sClassifiedClickThrough);

	// While we're at it let's get the stats from the new table if that
	// capability exists.
	std::string url = gAgent.getRegion()->getCapability("SearchStatRequest");
	if (!url.empty())
	{
		llinfos << "Classified stat request via capability" << llendl;
		LLSD body;
		body["classified_id"] = getClassifiedId();
		LLHTTPClient::post(url, body, new LLClassifiedStatsResponder(getClassifiedId()));
	}

	setInfoLoaded(false);
}

void LLPanelClassifiedInfo::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_CLASSIFIED_INFO == type)
	{
		LLAvatarClassifiedInfo* c_info = static_cast<LLAvatarClassifiedInfo*>(data);
		if(c_info && getClassifiedId() == c_info->classified_id)
		{
			setClassifiedName(c_info->name);
			setDescription(c_info->description);
			setSnapshotId(c_info->snapshot_id);
			setParcelId(c_info->parcel_id);
			setPosGlobal(c_info->pos_global);
			setClassifiedLocation(createLocationText(c_info->parcel_name, c_info->sim_name, c_info->pos_global));
			childSetValue("category", LLClassifiedInfo::sCategories[c_info->category]);

			static std::string mature_str = getString("type_mature");
			static std::string pg_str = getString("type_pg");
			static LLUIString  price_str = getString("l$_price");
			static std::string date_fmt = getString("date_fmt");

			bool mature = is_cf_mature(c_info->flags);
			childSetValue("content_type", mature ? mature_str : pg_str);
			childSetValue("auto_renew", is_cf_auto_renew(c_info->flags));

			price_str.setArg("[PRICE]", llformat("%d", c_info->price_for_listing));
			childSetValue("price_for_listing", LLSD(price_str));

			std::string date_str = date_fmt;
			LLStringUtil::format(date_str, LLSD().with("datetime", (S32) c_info->creation_date));
			childSetText("creation_date", date_str);

			setInfoLoaded(true);
		}
	}
}

void LLPanelClassifiedInfo::resetData()
{
	setClassifiedName(LLStringUtil::null);
	setDescription(LLStringUtil::null);
	setClassifiedLocation(LLStringUtil::null);
	setClassifiedId(LLUUID::null);
	setSnapshotId(LLUUID::null);
	mPosGlobal.clearVec();
	childSetValue("category", LLStringUtil::null);
	childSetValue("content_type", LLStringUtil::null);
	childSetText("click_through_text", LLStringUtil::null);
}

void LLPanelClassifiedInfo::resetControls()
{
	bool is_self = getAvatarId() == gAgent.getID();

	childSetEnabled("edit_btn", is_self);
	childSetVisible("edit_btn", is_self);
	childSetVisible("price_layout_panel", is_self);
	childSetVisible("clickthrough_layout_panel", is_self);
}

void LLPanelClassifiedInfo::setClassifiedName(const std::string& name)
{
	childSetValue("classified_name", name);
}

std::string LLPanelClassifiedInfo::getClassifiedName()
{
	return childGetValue("classified_name").asString();
}

void LLPanelClassifiedInfo::setDescription(const std::string& desc)
{
	childSetValue("classified_desc", desc);
}

std::string LLPanelClassifiedInfo::getDescription()
{
	return childGetValue("classified_desc").asString();
}

void LLPanelClassifiedInfo::setClassifiedLocation(const std::string& location)
{
	childSetValue("classified_location", location);
}

void LLPanelClassifiedInfo::setSnapshotId(const LLUUID& id)
{
	childSetValue("classified_snapshot", id);
	if(!mSnapshotStreched)
	{
		LLUICtrl* snapshot = getChild<LLUICtrl>("classified_snapshot");
		snapshot->setRect(mSnapshotRect);
	}
	mSnapshotStreched = false;
}

void LLPanelClassifiedInfo::draw()
{
	LLPanel::draw();

	// Stretch in draw because it takes some time to load a texture,
	// going to try to stretch snapshot until texture is loaded
	stretchSnapshot();
}

LLUUID LLPanelClassifiedInfo::getSnapshotId()
{
	return childGetValue("classified_snapshot").asUUID();
}

// static
void LLPanelClassifiedInfo::setClickThrough(
	const LLUUID& classified_id,
	S32 teleport,
	S32 map,
	S32 profile,
	bool from_new_table)
{
	llinfos << "Click-through data for classified " << classified_id << " arrived: ["
			<< teleport << ", " << map << ", " << profile << "] ("
			<< (from_new_table ? "new" : "old") << ")" << llendl;

	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelClassifiedInfo* self = *iter;
		if (self->getClassifiedId() != classified_id)
		{
			continue;
		}

		// *HACK: Skip LLPanelClassifiedEdit instances: they don't display clicks data.
		// Those instances should not be in the list at all.
		if (typeid(*self) != typeid(LLPanelClassifiedInfo))
		{
			continue;
		}

		llinfos << "Updating classified info panel" << llendl;

		// We need to check to see if the data came from the new stat_table 
		// or the old classified table. We also need to cache the data from 
		// the two separate sources so as to display the aggregate totals.

		if (from_new_table)
		{
			self->mTeleportClicksNew = teleport;
			self->mMapClicksNew = map;
			self->mProfileClicksNew = profile;
		}
		else
		{
			self->mTeleportClicksOld = teleport;
			self->mMapClicksOld = map;
			self->mProfileClicksOld = profile;
		}

		static LLUIString ct_str = self->getString("click_through_text_fmt");

		ct_str.setArg("[TELEPORT]",	llformat("%d", self->mTeleportClicksNew + self->mTeleportClicksOld));
		ct_str.setArg("[MAP]",		llformat("%d", self->mMapClicksNew + self->mMapClicksOld));
		ct_str.setArg("[PROFILE]",	llformat("%d", self->mProfileClicksNew + self->mProfileClicksOld));

		self->childSetText("click_through_text", ct_str.getString());
	}
}

// static
std::string LLPanelClassifiedInfo::createLocationText(
	const std::string& original_name, 
	const std::string& sim_name, 
	const LLVector3d& pos_global)
{
	std::string location_text;
	
	location_text.append(original_name);

	if (!sim_name.empty())
	{
		if (!location_text.empty()) 
			location_text.append(", ");
		location_text.append(sim_name);
	}

	if (!location_text.empty()) 
		location_text.append(" ");

	if (!pos_global.isNull())
	{
		S32 region_x = llround((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
		S32 region_y = llround((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
		S32 region_z = llround((F32)pos_global.mdV[VZ]);
		location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));
	}

	return location_text;
}

void LLPanelClassifiedInfo::stretchSnapshot()
{
	// *NOTE dzaporozhan
	// Could be moved to LLTextureCtrl

	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("classified_snapshot");
	LLViewerFetchedTexture* texture = texture_ctrl->getTexture();

	if(!texture || mSnapshotStreched)
	{
		return;
	}

	if(0 == texture->getOriginalWidth() || 0 == texture->getOriginalHeight())
	{
		// looks like texture is not loaded yet
		llinfos << "Missing image size" << llendl;
		return;
	}

	LLRect rc = mSnapshotRect;
	F32 t_width = texture->getFullWidth();
	F32 t_height = texture->getFullHeight();

	F32 ratio = llmin<F32>( (rc.getWidth() / t_width), (rc.getHeight() / t_height) );

	t_width *= ratio;
	t_height *= ratio;

	rc.setCenterAndSize(rc.getCenterX(), rc.getCenterY(), llfloor(t_width), llfloor(t_height));
	texture_ctrl->setRect(rc);

	mSnapshotStreched = true;
}

void LLPanelClassifiedInfo::onMapClick()
{
	LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
	LLFloaterReg::showInstance("world_map", "center");
}

void LLPanelClassifiedInfo::onTeleportClick()
{
	if (!getPosGlobal().isExactlyZero())
	{
		gAgent.teleportViaLocation(getPosGlobal());
		LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
	}
}

void LLPanelClassifiedInfo::onExit()
{
	LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
	gGenericDispatcher.addHandler("classifiedclickthrough", NULL); // deregister our handler
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static const S32 CB_ITEM_MATURE = 0;
static const S32 CB_ITEM_PG	   = 1;

LLPanelClassifiedEdit::LLPanelClassifiedEdit()
 : LLPanelClassifiedInfo()
 , mIsNew(false)
 , mCanClose(false)
{
}

LLPanelClassifiedEdit::~LLPanelClassifiedEdit()
{
}

//static
LLPanelClassifiedEdit* LLPanelClassifiedEdit::create()
{
	LLPanelClassifiedEdit* panel = new LLPanelClassifiedEdit();
	LLUICtrlFactory::getInstance()->buildPanel(panel, "panel_edit_classified.xml");
	return panel;
}

BOOL LLPanelClassifiedEdit::postBuild()
{
	LLPanelClassifiedInfo::postBuild();

	LLTextureCtrl* snapshot = getChild<LLTextureCtrl>("classified_snapshot");
	snapshot->setOnSelectCallback(boost::bind(&LLPanelClassifiedEdit::onChange, this));

	LLUICtrl* edit_icon = getChild<LLUICtrl>("edit_icon");
	snapshot->setMouseEnterCallback(boost::bind(&LLPanelClassifiedEdit::onTexturePickerMouseEnter, this, edit_icon));
	snapshot->setMouseLeaveCallback(boost::bind(&LLPanelClassifiedEdit::onTexturePickerMouseLeave, this, edit_icon));
	edit_icon->setVisible(false);

	LLLineEditor* line_edit = getChild<LLLineEditor>("classified_name");
	line_edit->setKeystrokeCallback(boost::bind(&LLPanelClassifiedEdit::onChange, this), NULL);

	LLTextEditor* text_edit = getChild<LLTextEditor>("classified_desc");
	text_edit->setKeystrokeCallback(boost::bind(&LLPanelClassifiedEdit::onChange, this));

	LLComboBox* combobox = getChild<LLComboBox>( "category");
	LLClassifiedInfo::cat_map::iterator iter;
	for (iter = LLClassifiedInfo::sCategories.begin();
		iter != LLClassifiedInfo::sCategories.end();
		iter++)
	{
		combobox->add(LLTrans::getString(iter->second));
	}

	combobox->setCommitCallback(boost::bind(&LLPanelClassifiedEdit::onChange, this));

	childSetCommitCallback("content_type", boost::bind(&LLPanelClassifiedEdit::onChange, this), NULL);
	childSetCommitCallback("price_for_listing", boost::bind(&LLPanelClassifiedEdit::onChange, this), NULL);
	childSetCommitCallback("auto_renew", boost::bind(&LLPanelClassifiedEdit::onChange, this), NULL);

	childSetAction("save_changes_btn", boost::bind(&LLPanelClassifiedEdit::onSaveClick, this));
	childSetAction("set_to_curr_location_btn", boost::bind(&LLPanelClassifiedEdit::onSetLocationClick, this));

	return TRUE;
}

void LLPanelClassifiedEdit::onOpen(const LLSD& key)
{
	LLUUID classified_id = key["classified_id"];

	mIsNew = classified_id.isNull();

	if(mIsNew)
	{
		setAvatarId(gAgent.getID());

		resetData();
		resetControls();

		setPosGlobal(gAgent.getPositionGlobal());

		LLUUID snapshot_id = LLUUID::null;
		std::string desc;
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

		if(parcel)
		{
			desc = parcel->getDesc();
			snapshot_id = parcel->getSnapshotID();
		}

		std::string region_name = LLTrans::getString("ClassifiedUpdateAfterPublish");
		LLViewerRegion* region = gAgent.getRegion();
		if (region)
		{
			region_name = region->getName();
		}

		childSetValue("classified_name", makeClassifiedName());
		childSetValue("classified_desc", desc);
		setSnapshotId(snapshot_id);
		
		setClassifiedLocation(createLocationText(getLocationNotice(), region_name, getPosGlobal()));
		
		// server will set valid parcel id
		setParcelId(LLUUID::null);

		enableVerbs(true);
		enableEditing(true);
	}
	else
	{
		LLPanelClassifiedInfo::onOpen(key);
		enableVerbs(false);
		enableEditing(false);
	}

	resetDirty();
	setInfoLoaded(false);
}

void LLPanelClassifiedEdit::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_CLASSIFIED_INFO == type)
	{
		LLAvatarClassifiedInfo* c_info = static_cast<LLAvatarClassifiedInfo*>(data);
		if(c_info && getClassifiedId() == c_info->classified_id)
		{
			enableEditing(true);

			setClassifiedName(c_info->name);
			setDescription(c_info->description);
			setSnapshotId(c_info->snapshot_id);
			setPosGlobal(c_info->pos_global);

			setClassifiedLocation(createLocationText(c_info->parcel_name, c_info->sim_name, c_info->pos_global));
			getChild<LLComboBox>("category")->setCurrentByIndex(c_info->category + 1);
			getChild<LLComboBox>("category")->resetDirty();

			bool mature = is_cf_mature(c_info->flags);
			bool auto_renew = is_cf_auto_renew(c_info->flags);

			getChild<LLComboBox>("content_type")->setCurrentByIndex(mature ? CB_ITEM_MATURE : CB_ITEM_PG);
			childSetValue("auto_renew", auto_renew);
			childSetValue("price_for_listing", c_info->price_for_listing);

			resetDirty();
			setInfoLoaded(true);
		}
	}
}

BOOL LLPanelClassifiedEdit::isDirty() const
{
	if(mIsNew) 
	{
		return TRUE;
	}

	BOOL dirty = false;

	dirty |= LLPanelClassifiedInfo::isDirty();
	dirty |= getChild<LLUICtrl>("classified_snapshot")->isDirty();
	dirty |= getChild<LLUICtrl>("classified_name")->isDirty();
	dirty |= getChild<LLUICtrl>("classified_desc")->isDirty();
	dirty |= getChild<LLUICtrl>("category")->isDirty();
	dirty |= getChild<LLUICtrl>("content_type")->isDirty();
	dirty |= getChild<LLUICtrl>("auto_renew")->isDirty();
	dirty |= getChild<LLUICtrl>("price_for_listing")->isDirty();

	return dirty;
}

void LLPanelClassifiedEdit::resetDirty()
{
	LLPanelClassifiedInfo::resetDirty();
	getChild<LLUICtrl>("classified_snapshot")->resetDirty();
	getChild<LLUICtrl>("classified_name")->resetDirty();
	getChild<LLUICtrl>("classified_desc")->resetDirty();
	getChild<LLUICtrl>("category")->resetDirty();
	getChild<LLUICtrl>("content_type")->resetDirty();
	getChild<LLUICtrl>("auto_renew")->resetDirty();
	getChild<LLUICtrl>("price_for_listing")->resetDirty();
}

void LLPanelClassifiedEdit::setSaveCallback(const commit_callback_t& cb)
{
	getChild<LLButton>("save_changes_btn")->setClickedCallback(cb);
}

void LLPanelClassifiedEdit::setCancelCallback(const commit_callback_t& cb)
{
	getChild<LLButton>("cancel_btn")->setClickedCallback(cb);
}

void LLPanelClassifiedEdit::resetControls()
{
	LLPanelClassifiedInfo::resetControls();

	getChild<LLComboBox>("category")->setCurrentByIndex(0);
	getChild<LLComboBox>("content_type")->setCurrentByIndex(0);
	childSetValue("auto_renew", false);
	childSetValue("price_for_listing", MINIMUM_PRICE_FOR_LISTING);
}

bool LLPanelClassifiedEdit::canClose()
{
	return mCanClose;
}

void LLPanelClassifiedEdit::sendUpdate()
{
	LLAvatarClassifiedInfo c_data;

	if(getClassifiedId().isNull())
	{
		LLUUID id;
		id.generate();
		setClassifiedId(id);
	}

	c_data.agent_id = gAgent.getID();
	c_data.classified_id = getClassifiedId();
	c_data.category = getCategory();
	c_data.name = getClassifiedName();
	c_data.description = getDescription();
	c_data.parcel_id = getParcelId();
	c_data.snapshot_id = getSnapshotId();
	c_data.pos_global = getPosGlobal();
	c_data.flags = getFlags();
	c_data.price_for_listing = getPriceForListing();

	LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoUpdate(&c_data);
}

U32 LLPanelClassifiedEdit::getCategory()
{
	LLComboBox* cat_cb = getChild<LLComboBox>("category");
	return cat_cb->getCurrentIndex() + 1;
}

U8 LLPanelClassifiedEdit::getFlags()
{
	bool auto_renew = childGetValue("auto_renew").asBoolean();

	LLComboBox* content_cb = getChild<LLComboBox>("content_type");
	bool mature = content_cb->getCurrentIndex() == CB_ITEM_MATURE;
	
	return pack_classified_flags_request(auto_renew, false, mature, false);
}

void LLPanelClassifiedEdit::enableVerbs(bool enable)
{
	childSetEnabled("save_changes_btn", enable);
}

void LLPanelClassifiedEdit::enableEditing(bool enable)
{
	childSetEnabled("classified_snapshot", enable);
	childSetEnabled("classified_name", enable);
	childSetEnabled("classified_desc", enable);
	childSetEnabled("set_to_curr_location_btn", enable);
	childSetEnabled("category", enable);
	childSetEnabled("content_type", enable);
	childSetEnabled("price_for_listing", enable);
	childSetEnabled("auto_renew", enable);
}

std::string LLPanelClassifiedEdit::makeClassifiedName()
{
	std::string name;

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if(parcel)
	{
		name = parcel->getName();
	}

	if(!name.empty())
	{
		return name;
	}

	LLViewerRegion* region = gAgent.getRegion();
	if(region)
	{
		name = region->getName();
	}

	return name;
}

S32 LLPanelClassifiedEdit::getPriceForListing()
{
	return childGetValue("price_for_listing").asInteger();
}

void LLPanelClassifiedEdit::onSetLocationClick()
{
	setPosGlobal(gAgent.getPositionGlobal());
	setParcelId(LLUUID::null);

	std::string region_name = LLTrans::getString("ClassifiedUpdateAfterPublish");
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		region_name = region->getName();
	}

	setClassifiedLocation(createLocationText(getLocationNotice(), region_name, getPosGlobal()));

	// mark classified as dirty
	setValue(LLSD());

	onChange();
}

void LLPanelClassifiedEdit::onChange()
{
	enableVerbs(isDirty());
}

void LLPanelClassifiedEdit::onSaveClick()
{
	mCanClose = false;

	if(!isValidName())
	{
		notifyInvalidName();
		return;
	}
	if(isNew())
	{
		if(gStatusBar->getBalance() < getPriceForListing())
		{
			LLNotificationsUtil::add("ClassifiedInsufficientFunds");
			return;
		}
	}

	mCanClose = true;
	sendUpdate();
	resetDirty();
}

std::string LLPanelClassifiedEdit::getLocationNotice()
{
	static std::string location_notice = getString("location_notice");
	return location_notice;
}

bool LLPanelClassifiedEdit::isValidName()
{
	std::string name = getClassifiedName();
	if (name.empty())
	{
		return false;
	}
	if (!isalnum(name[0]))
	{
		return false;
	}

	return true;
}

void LLPanelClassifiedEdit::notifyInvalidName()
{
	std::string name = getClassifiedName();
	if (name.empty())
	{
		LLNotificationsUtil::add("BlankClassifiedName");
	}
	else if (!isalnum(name[0]))
	{
		LLNotificationsUtil::add("ClassifiedMustBeAlphanumeric");
	}
}

void LLPanelClassifiedEdit::onTexturePickerMouseEnter(LLUICtrl* ctrl)
{
	ctrl->setVisible(TRUE);
}

void LLPanelClassifiedEdit::onTexturePickerMouseLeave(LLUICtrl* ctrl)
{
	ctrl->setVisible(FALSE);
}

//EOF
