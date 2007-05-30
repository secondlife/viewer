/** 
 * @file llpanelclassified.cpp
 * @brief LLPanelClassified class implementation
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Display of a classified used both for the global view in the
// Find directory, and also for each individual user's classified in their
// profile.

#include "llviewerprecompiledheaders.h"

#include "llpanelclassified.h"

#include "lldir.h"
#include "lldispatcher.h"
#include "llparcel.h"
#include "message.h"

#include "llagent.h"
#include "llalertdialog.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llclassifiedflags.h"
#include "llviewercontrol.h"
#include "lllineeditor.h"
#include "llfloateravatarinfo.h"
#include "lltabcontainervertical.h"
#include "lltextbox.h"
#include "llcombobox.h"
#include "llviewertexteditor.h"
#include "lltexturectrl.h"
#include "lluiconstants.h"
#include "llvieweruictrlfactory.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llworldmap.h"
#include "llfloaterworldmap.h"
#include "llviewergenericmessage.h"	// send_generic_message
#include "llviewerwindow.h"	// for window width, height
#include "viewer.h"	// app_abort_quit()

const S32 MINIMUM_PRICE_FOR_LISTING = 50;	// L$

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
		LLPanelClassified::setClickThrough(classified_id, teleport_clicks,
										   map_clicks,
										   profile_clicks);
		return true;
	}
};

static LLDispatchClassifiedClickThrough sClassifiedClickThrough;

//static
std::list<LLPanelClassified*> LLPanelClassified::sAllPanels;

LLPanelClassified::LLPanelClassified(BOOL in_finder)
:	LLPanel("Classified Panel"),
	mInFinder(in_finder),
	mDirty(false),
	mForceClose(false),
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
	mUpdateBtn(NULL),
    mTeleportBtn(NULL),
    mMapBtn(NULL),
	mProfileBtn(NULL),
	mInfoText(NULL),
	mMatureCheck(NULL),
	mAutoRenewCheck(NULL),
    mSetBtn(NULL),
	mClickThroughText(NULL)
{
    sAllPanels.push_back(this);

	std::string classified_def_file;
	if (mInFinder)
	{
		gUICtrlFactory->buildPanel(this, "panel_classified.xml");
	}
	else
	{
		gUICtrlFactory->buildPanel(this, "panel_avatar_classified.xml");
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
}


BOOL LLPanelClassified::postBuild()
{
    mSnapshotCtrl = LLViewerUICtrlFactory::getTexturePickerByName(this, "snapshot_ctrl");
	mSnapshotCtrl->setCommitCallback(onCommitAny);
	mSnapshotCtrl->setCallbackUserData(this);
	mSnapshotSize = mSnapshotCtrl->getRect();

    mNameEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "given_name_editor");
	mNameEditor->setMaxTextLength(DB_PARCEL_NAME_LEN);
	mNameEditor->setCommitOnFocusLost(TRUE);
	mNameEditor->setFocusReceivedCallback(onFocusReceived);
	mNameEditor->setCommitCallback(onCommitAny);
	mNameEditor->setCallbackUserData(this);
	mNameEditor->setPrevalidate( LLLineEditor::prevalidateASCII );

    mDescEditor = LLUICtrlFactory::getTextEditorByName(this, "desc_editor");
	mDescEditor->setCommitOnFocusLost(TRUE);
	mDescEditor->setFocusReceivedCallback(onFocusReceived);
	mDescEditor->setCommitCallback(onCommitAny);
	mDescEditor->setCallbackUserData(this);
	mDescEditor->setTabToNextField(TRUE);

    mLocationEditor = LLViewerUICtrlFactory::getLineEditorByName(this, "location_editor");

    mSetBtn = LLViewerUICtrlFactory::getButtonByName(this, "set_location_btn");
    mSetBtn->setClickedCallback(onClickSet);
    mSetBtn->setCallbackUserData(this);

    mTeleportBtn = LLViewerUICtrlFactory::getButtonByName(this, "classified_teleport_btn");
    mTeleportBtn->setClickedCallback(onClickTeleport);
    mTeleportBtn->setCallbackUserData(this);

    mMapBtn = LLViewerUICtrlFactory::getButtonByName(this, "classified_map_btn");
    mMapBtn->setClickedCallback(onClickMap);
    mMapBtn->setCallbackUserData(this);

	if(mInFinder)
	{
		mProfileBtn  = LLViewerUICtrlFactory::getButtonByName(this, "classified_profile_btn");
		mProfileBtn->setClickedCallback(onClickProfile);
		mProfileBtn->setCallbackUserData(this);
	}

	mCategoryCombo = LLViewerUICtrlFactory::getComboBoxByName(this, "classified_category_combo");
	LLClassifiedInfo::cat_map::iterator iter;
	for (iter = LLClassifiedInfo::sCategories.begin();
		iter != LLClassifiedInfo::sCategories.end();
		iter++)
	{
		mCategoryCombo->add(iter->second, (void *)((intptr_t)iter->first), ADD_BOTTOM);
	}
	mCategoryCombo->setCurrentByIndex(0);
	mCategoryCombo->setCommitCallback(onCommitAny);
	mCategoryCombo->setCallbackUserData(this);

	mMatureCheck = LLViewerUICtrlFactory::getCheckBoxByName(this, "classified_mature_check");
	mMatureCheck->setCommitCallback(onCommitAny);
	mMatureCheck->setCallbackUserData(this);
	if (gAgent.mAccess < SIM_ACCESS_MATURE)
	{
		// Teens don't get to set mature flag. JC
		mMatureCheck->setVisible(FALSE);
	}

	if (!mInFinder)
	{
		mAutoRenewCheck = LLUICtrlFactory::getCheckBoxByName(this, "auto_renew_check");
		mAutoRenewCheck->setCommitCallback(onCommitAny);
		mAutoRenewCheck->setCallbackUserData(this);
	}

	mUpdateBtn = LLUICtrlFactory::getButtonByName(this, "classified_update_btn");
    mUpdateBtn->setClickedCallback(onClickUpdate);
    mUpdateBtn->setCallbackUserData(this);

	if (!mInFinder)
	{
		mClickThroughText = LLUICtrlFactory::getTextBoxByName(this, "click_through_text");
	}
	
    return TRUE;
}

BOOL LLPanelClassified::titleIsValid()
{
	// Disallow leading spaces, punctuation, etc. that screw up
	// sort order.
	const LLString& name = mNameEditor->getText();
	if (name.empty())
	{
		gViewerWindow->alertXml("BlankClassifiedName");
		return FALSE;
	}
	if (!isalnum(name[0]))
	{
		gViewerWindow->alertXml("ClassifiedMustBeAlphanumeric");
		return FALSE;
	}

	return TRUE;
}

void LLPanelClassified::apply()
{
	// Apply is used for automatically saving results, so only
	// do that if there is a difference, and this is a save not create.
	if (mDirty && mPaidFor)
	{
		sendClassifiedInfoUpdate();
	}
}


// static
void LLPanelClassified::saveCallback(S32 option, void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;
	switch(option)
	{
		case 0: // Save
			self->sendClassifiedInfoUpdate();
			// fall through to close

		case 1: // Don't Save
			{
				self->mForceClose = true;
				// Close containing floater
				LLView* view = self;
				while (view)
				{
					if (view->getWidgetType() == WIDGET_TYPE_FLOATER)
					{
						LLFloater* f = (LLFloater*)view;
						f->close();
						break;
					}
					view = view->getParent();
				}
			}
			break;

		case 2: // Cancel
		default:
			app_abort_quit();
			break;
	}
}

BOOL LLPanelClassified::canClose()
{
	if (mForceClose || !mDirty) return TRUE;

	LLString::format_map_t args;
	args["[NAME]"] = mNameEditor->getText();
	LLAlertDialog::showXml("ClassifiedSave", args, saveCallback, this);
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
	LLParcel* parcel = gParcelMgr->getAgentParcel();
	if (parcel)
	{
		mNameEditor->setText(parcel->getName());
		//mDescEditor->setText(parcel->getDesc());
		mSnapshotCtrl->setImageAssetID(parcel->getSnapshotID());
		//mPriceEditor->setText("0");
		mCategoryCombo->setCurrentByIndex(0);
	}

	// default new classifieds to publish
	//mEnabledCheck->set(TRUE);

	// delay commit until user hits save
	// sendClassifiedInfoUpdate();

	mUpdateBtn->setLabelSelected("Publish...");
	mUpdateBtn->setLabelUnselected("Publish...");
}


void LLPanelClassified::setClassifiedID(const LLUUID& id)
{
	mClassifiedID = id;
}


//static
void LLPanelClassified::setClickThrough(const LLUUID& classified_id,
										S32 teleport,
										S32 map,
										S32 profile)
{
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelClassified* self = *iter;
		// For top picks, must match pick id
		if (self->mClassifiedID != classified_id)
		{
			continue;
		}

		if (self->mClickThroughText)
		{
			std::string msg = llformat("Clicks: %d teleport, %d map, %d profile",
									teleport,
									map,
									profile);
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
	BOOL mature = mMatureCheck->get();
	// Classifieds are always enabled/published 11/2005 JC
	//BOOL enabled = mEnabledCheck->get();
	BOOL auto_renew = FALSE;
	if (mAutoRenewCheck) 
	{
		auto_renew = mAutoRenewCheck->get();
	}
	U8 flags = pack_classified_flags(mature, auto_renew);
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

	char name[DB_PARCEL_NAME_SIZE];		/*Flawfinder: ignore*/
	msg->getStringFast(_PREHASH_Data, _PREHASH_Name, DB_PARCEL_NAME_SIZE, name);

	char desc[DB_PICK_DESC_SIZE];		/*Flawfinder: ignore*/
	msg->getStringFast(_PREHASH_Data, _PREHASH_Desc, DB_PICK_DESC_SIZE, desc);

	LLUUID snapshot_id;
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_SnapshotID, snapshot_id);

    // "Location text" is actually the original
    // name that owner gave the parcel, and the location.
	char buffer[256];		/*Flawfinder: ignore*/
    LLString location_text;

    msg->getStringFast(_PREHASH_Data, _PREHASH_ParcelName, 256, buffer);
	if (buffer[0] != '\0')
	{
		location_text.assign(buffer);
		location_text.append(", ");
	}
	else
	{
		location_text.assign("");
	}

	char sim_name[256];		/*Flawfinder: ignore*/
	msg->getStringFast(_PREHASH_Data, _PREHASH_SimName, 256, sim_name);

	LLVector3d pos_global;
	msg->getVector3dFast(_PREHASH_Data, _PREHASH_PosGlobal, pos_global);

    S32 region_x = llround((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)pos_global.mdV[VZ]);
   
    snprintf(buffer, sizeof(buffer), "%s (%d, %d, %d)", sim_name, region_x, region_y, region_z);			/* Flawfinder: ignore */
    location_text.append(buffer);

	U8 flags;
	msg->getU8Fast(_PREHASH_Data, _PREHASH_ClassifiedFlags, flags);
	//BOOL enabled = is_cf_enabled(flags);
	bool mature = is_cf_mature(flags);
	bool auto_renew = is_cf_auto_renew(flags);

	U32 date = 0;
	msg->getU32Fast(_PREHASH_Data, _PREHASH_CreationDate, date);
	time_t tim = date;
	tm *now=localtime(&tim);

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

		self->mCategoryCombo->setCurrentByIndex(category - 1);
		self->mMatureCheck->set(mature);
		if (self->mAutoRenewCheck)
		{
			self->mAutoRenewCheck->set(auto_renew);
		}

		LLString datestr = llformat("%02d/%02d/%d", now->tm_mon+1, now->tm_mday, now->tm_year+1900);
		self->childSetTextArg("ad_placed_paid", "[DATE]", datestr);
		self->childSetTextArg("ad_placed_paid", "[AMT]", llformat("%d", price_for_listing));		
		self->childSetText("classified_info_text", self->childGetValue("ad_placed_paid").asString());

		// If we got data from the database, we know the listing is paid for.
		self->mPaidFor = TRUE;

		self->mUpdateBtn->setLabelSelected("Update");
		self->mUpdateBtn->setLabelUnselected("Update");
    }
}

void LLPanelClassified::draw()
{
	if (getVisible())
	{
		refresh();

		LLPanel::draw();
	}
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
			mSnapshotCtrl->setRect(LLRect(20, 375, 320, 175));
			
		}
		else
		{
			mSnapshotCtrl->setRect(mSnapshotSize);
			//normal
		}
		mNameEditor->setEnabled(godlike);
		mDescEditor->setEnabled(godlike);
		mCategoryCombo->setEnabled(godlike);
		mCategoryCombo->setVisible(godlike);

		//mEnabledCheck->setVisible(godlike);
		//mEnabledCheck->setEnabled(godlike);

		mMatureCheck->setEnabled(godlike);
		mMatureCheck->setVisible(godlike);

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

		//mEnabledCheck->setVisible(FALSE);
		//mEnabledCheck->setEnabled(is_self);
		mMatureCheck->setEnabled(is_self);

		mAutoRenewCheck->setEnabled(is_self);
		mAutoRenewCheck->setVisible(is_self);

		mClickThroughText->setEnabled(is_self);
		mClickThroughText->setVisible(is_self);

		mSetBtn->setVisible(is_self);
		mSetBtn->setEnabled(is_self);

		mUpdateBtn->setEnabled(is_self && mDirty);
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

	// if already paid for, just do the update
	if (self->mPaidFor)
	{
		callbackConfirmPublish(0, self);
	}
	else
	{
		// Ask the user how much they want to pay
		LLFloaterPriceForListing::show( callbackGotPriceForListing, self );
	}
}

// static
void LLPanelClassified::callbackGotPriceForListing(S32 option, LLString text, void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;

	// Only do something if user hits publish
	if (option != 0) return;

	S32 price_for_listing = strtol(text.c_str(), NULL, 10);
	if (price_for_listing < MINIMUM_PRICE_FOR_LISTING)
	{
		LLString::format_map_t args;
		LLString price_text = llformat("%d", MINIMUM_PRICE_FOR_LISTING);
		args["[MIN_PRICE]"] = price_text;
			
		gViewerWindow->alertXml("MinClassifiedPrice", args);
		return;
	}

	// price is acceptable, put it in the dialog for later read by 
	// update send
	self->mPriceForListing = price_for_listing;

	LLString::format_map_t args;
	args["[AMOUNT]"] = llformat("%d", price_for_listing);
	gViewerWindow->alertXml("PublishClassified", args, &callbackConfirmPublish, self);

}

// static
void LLPanelClassified::callbackConfirmPublish(S32 option, void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;

	// Option 0 = publish
	if (option != 0) return;

	self->sendClassifiedInfoUpdate();

	// Big hack - assume that top picks are always in a browser,
	// and non-finder-classifieds are always in a tab container.
	if (self->mInFinder)
	{
		// TODO: enable this
		//LLPanelDirClassifieds* panel = (LLPanelDirClassifieds*)self->getParent();
		//panel->renameClassified(self->mClassifiedID, self->mNameEditor->getText().c_str());
	}
	else
	{
		LLTabContainerVertical* tab = (LLTabContainerVertical*)self->getParent();
		tab->setCurrentTabName(self->mNameEditor->getText());
	}
}

// static
void LLPanelClassified::onClickTeleport(void* data)
{
    LLPanelClassified* self = (LLPanelClassified*)data;

    if (!self->mPosGlobal.isExactlyZero())
    {
        gAgent.teleportViaLocation(self->mPosGlobal);
        gFloaterWorldMap->trackLocation(self->mPosGlobal);

		self->sendClassifiedClickMessage("teleport");
    }
}


// static
void LLPanelClassified::onClickMap(void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;
	gFloaterWorldMap->trackLocation(self->mPosGlobal);
	LLFloaterWorldMap::show(NULL, TRUE);

	self->sendClassifiedClickMessage("map");
}

// static
void LLPanelClassified::onClickProfile(void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;
	LLFloaterAvatarInfo::showFromDirectory(self->mCreatorID);
	self->sendClassifiedClickMessage("profile");
}

// static
/*
void LLPanelClassified::onClickLandmark(void* data)
{
    LLPanelClassified* self = (LLPanelClassified*)data;
	create_landmark(self->mNameEditor->getText().c_str(), "", self->mPosGlobal);
}
*/

// static
void LLPanelClassified::onClickSet(void* data)
{
    LLPanelClassified* self = (LLPanelClassified*)data;

	// Save location for later.
	self->mPosGlobal = gAgent.getPositionGlobal();

	LLString location_text;
	location_text.assign("(will update after publish)");
	location_text.append(", ");

    S32 region_x = llround((F32)self->mPosGlobal.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)self->mPosGlobal.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)self->mPosGlobal.mdV[VZ]);
   
	location_text.append(self->mSimName);
    location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));

	self->mLocationEditor->setText(location_text);

	// Set this to null so it updates on the next save.
	self->mParcelID.setNull();

	onCommitAny(NULL, data);
}


// static
void LLPanelClassified::onCommitAny(LLUICtrl* ctrl, void* data)
{
	LLPanelClassified* self = (LLPanelClassified*)data;
	if (self)
	{
		self->mDirty = true;
	}
}

// static
void LLPanelClassified::onFocusReceived(LLUICtrl* ctrl, void* data)
{
	// allow the data to be saved
	onCommitAny(ctrl, data);
}


void LLPanelClassified::sendClassifiedClickMessage(const char* type)
{
	// You're allowed to click on your own ads to reassure yourself
	// that the system is working.
	std::vector<std::string> strings;
	strings.push_back(mClassifiedID.asString());
	strings.push_back(type);
	LLUUID no_invoice;
	send_generic_message("classifiedclick", strings, no_invoice);
}

////////////////////////////////////////////////////////////////////////////////////////////

LLFloaterPriceForListing::LLFloaterPriceForListing()
:	LLFloater("PriceForListing"),
	mCallback(NULL),
	mUserData(NULL)
{ }

//virtual
LLFloaterPriceForListing::~LLFloaterPriceForListing()
{ }

//virtual
BOOL LLFloaterPriceForListing::postBuild()
{
	LLLineEditor* edit = LLUICtrlFactory::getLineEditorByName(this, "price_edit");
	if (edit)
	{
		edit->setPrevalidate(LLLineEditor::prevalidateNonNegativeS32);
		LLString min_price = llformat("%d", MINIMUM_PRICE_FOR_LISTING);
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
void LLFloaterPriceForListing::show( void (*callback)(S32, LLString, void*), void* userdata)
{
	LLFloaterPriceForListing *self = new LLFloaterPriceForListing();

	// Builds and adds to gFloaterView
	gUICtrlFactory->buildFloater(self, "floater_price_for_listing.xml");
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
		LLString text = self->childGetText("price_edit");
		self->mCallback(button, text, self->mUserData);
		self->close();
	}
}
