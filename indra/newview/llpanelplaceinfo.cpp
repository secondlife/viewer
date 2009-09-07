/**
 * @file llpanelplaceinfo.cpp
 * @brief Displays place information in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llpanelplaceinfo.h"

#include "roles_constants.h"
#include "llsdutil.h"
#include "llsecondlifeurls.h"

#include "llinventory.h"
#include "llparcel.h"

#include "llqueryflags.h"

#include "llbutton.h"
#include "lllineeditor.h"
#include "llscrollcontainer.h"
#include "lltextbox.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llagent.h"
#include "llavatarpropertiesprocessor.h"
#include "llfloaterworldmap.h"
#include "llinventorymodel.h"
#include "lllandmarkactions.h"
#include "lltexturectrl.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewertexteditor.h"
#include "llworldmap.h"

static LLRegisterPanelClassWrapper<LLPanelPlaceInfo> t_place_info("panel_place_info");

LLPanelPlaceInfo::LLPanelPlaceInfo()
:	LLPanel(),
	mParcelID(),
	mRequestedID(),
	mPosRegion(),
	mLandmarkID(),
	mMinHeight(0),
	mScrollingPanel(NULL),
	mInfoPanel(NULL),
	mMediaPanel(NULL)
{}

LLPanelPlaceInfo::~LLPanelPlaceInfo()
{
	if (mParcelID.notNull())
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelID, this);
	}

	LLViewerParcelMgr::getInstance()->removeObserver(this);
}

BOOL LLPanelPlaceInfo::postBuild()
{
	mTitle = getChild<LLTextBox>("panel_title");
	mCurrentTitle = mTitle->getText();

	// Since this is only used in the directory browser, always
	// disable the snapshot control. Otherwise clicking on it will
	// open a texture picker.
	mSnapshotCtrl = getChild<LLTextureCtrl>("logo");
	mSnapshotCtrl->setEnabled(FALSE);

	mRegionName = getChild<LLTextBox>("region_title");
	mParcelName = getChild<LLTextBox>("parcel_title");
	mDescEditor = getChild<LLTextEditor>("description");

	mMaturityRatingText = getChild<LLTextBox>("maturity_value");
	mParcelOwner = getChild<LLTextBox>("owner_value");
	mLastVisited = getChild<LLTextBox>("last_visited_value");

	mRatingText = getChild<LLTextBox>("rating_value");
	mVoiceText = getChild<LLTextBox>("voice_value");
	mFlyText = getChild<LLTextBox>("fly_value");
	mPushText = getChild<LLTextBox>("push_value");
	mBuildText = getChild<LLTextBox>("build_value");
	mScriptsText = getChild<LLTextBox>("scripts_value");
	mDamageText = getChild<LLTextBox>("damage_value");

	mRegionNameText = getChild<LLTextBox>("region_name");
	mRegionTypeText = getChild<LLTextBox>("region_type");
	mRegionRatingText = getChild<LLTextBox>("region_rating");
	mRegionOwnerText = getChild<LLTextBox>("region_owner");
	mRegionGroupText = getChild<LLTextBox>("region_group");

	mEstateNameText = getChild<LLTextBox>("estate_name");
	mEstateRatingText = getChild<LLTextBox>("estate_rating");
	mEstateOwnerText = getChild<LLTextBox>("estate_owner");
	mCovenantText = getChild<LLTextEditor>("covenant");

	mSalesPriceText = getChild<LLTextBox>("sales_price");
	mAreaText = getChild<LLTextBox>("area");
	mTrafficText = getChild<LLTextBox>("traffic");
	mPrimitivesText = getChild<LLTextBox>("primitives");
	mParcelScriptsText = getChild<LLTextBox>("parcel_scripts");
	mTerraformLimitsText = getChild<LLTextBox>("terraform_limits");
	mSubdivideText = getChild<LLTextEditor>("subdivide");
	mResaleText = getChild<LLTextEditor>("resale");
	mSaleToText = getChild<LLTextBox>("sale_to");

	mOwner = getChild<LLTextBox>("owner");
	mCreator = getChild<LLTextBox>("creator");
	mCreated = getChild<LLTextBox>("created");

	mTitleEditor = getChild<LLLineEditor>("title_editor");
	mTitleEditor->setCommitCallback(boost::bind(&LLPanelPlaceInfo::onCommitTitleOrNote, this, TITLE));

	mNotesEditor = getChild<LLTextEditor>("notes_editor");
	mNotesEditor->setCommitCallback(boost::bind(&LLPanelPlaceInfo::onCommitTitleOrNote, this, NOTE));
	mNotesEditor->setCommitOnFocusLost(true);

	LLScrollContainer* scroll_container = getChild<LLScrollContainer>("scroll_container");
	scroll_container->setBorderVisible(FALSE);
	mMinHeight = scroll_container->getScrolledViewRect().getHeight();

	mScrollingPanel = getChild<LLPanel>("scrolling_panel");
	mInfoPanel = getChild<LLPanel>("info_panel");
	mMediaPanel = getChild<LLMediaPanel>("media_panel");
	if (!mMediaPanel)
		return FALSE;

	return TRUE;
}

void LLPanelPlaceInfo::displayItemInfo(const LLInventoryItem* pItem)
{
	if (!pItem)
		return;

	mLandmarkID = pItem->getUUID();

	if(!gCacheName)
		return;

	const LLPermissions& perm = pItem->getPermissions();

	//////////////////
	// CREATOR NAME //
	//////////////////
	if (pItem->getCreatorUUID().notNull())
	{
		std::string name;
		LLUUID creator_id = pItem->getCreatorUUID();
		if (!gCacheName->getFullName(creator_id, name))
		{
			gCacheName->get(creator_id, FALSE,
							boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mCreator, _2, _3));
		}
		mCreator->setText(name);
	}
	else
	{
		mCreator->setText(getString("unknown"));
	}

	////////////////
	// OWNER NAME //
	////////////////
	if(perm.isOwned())
	{
		std::string name;
		if (perm.isGroupOwned())
		{
			LLUUID group_id = perm.getGroup();
			if (!gCacheName->getGroupName(group_id, name))
			{
				gCacheName->get(group_id, TRUE,
								boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mOwner, _2, _3));
			}
		}
		else
		{
			LLUUID owner_id = perm.getOwner();
			if (!gCacheName->getFullName(owner_id, name))
			{
				gCacheName->get(owner_id, FALSE,
								boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mOwner, _2, _3));
			}
		}
		mOwner->setText(name);
	}
	else
	{
		mOwner->setText(getString("public"));
	}
	
	//////////////////
	// ACQUIRE DATE //
	//////////////////
	time_t time_utc = pItem->getCreationDate();
	if (0 == time_utc)
	{
		mCreated->setText(getString("unknown"));
	}
	else
	{
		std::string timeStr = getString("acquired_date");
		LLSD substitution;
		substitution["datetime"] = (S32) time_utc;
		LLStringUtil::format (timeStr, substitution);
		mCreated->setText(timeStr);
	}

	mTitleEditor->setText(pItem->getName());
	mNotesEditor->setText(pItem->getDescription());
}

void LLPanelPlaceInfo::nameUpdatedCallback(
	LLTextBox* text,
	const std::string& first,
	const std::string& last)
{
	text->setText(first + " " + last);
}

void LLPanelPlaceInfo::resetLocation()
{
	mParcelID.setNull();
	mRequestedID.setNull();
	mLandmarkID.setNull();
	mPosRegion.clearVec();
	std::string not_available = getString("not_available");
	mMaturityRatingText->setValue(not_available);
	mParcelOwner->setValue(not_available);
	mLastVisited->setValue(not_available);
	mRegionName->setText(not_available);
	mParcelName->setText(not_available);
	mDescEditor->setText(not_available);
	mCreator->setText(not_available);
	mOwner->setText(not_available);
	mCreated->setText(not_available);
	mTitleEditor->setText(LLStringUtil::null);
	mNotesEditor->setText(LLStringUtil::null);
	mSnapshotCtrl->setImageAssetID(LLUUID::null);
	mSnapshotCtrl->setFallbackImageName("default_land_picture.j2c");

	mRatingText->setText(not_available);
	mVoiceText->setText(not_available);
	mFlyText->setText(not_available);
	mPushText->setText(not_available);
	mBuildText->setText(not_available);
	mParcelScriptsText->setText(not_available);
	mDamageText->setText(not_available);

	mRegionNameText->setValue(not_available);
	mRegionTypeText->setValue(not_available);
	mRegionRatingText->setValue(not_available);
	mRegionOwnerText->setValue(not_available);
	mRegionGroupText->setValue(not_available);

	mEstateNameText->setValue(not_available);
	mEstateRatingText->setValue(not_available);
	mEstateOwnerText->setValue(not_available);
	mCovenantText->setValue(not_available);

	mSalesPriceText->setValue(not_available);
	mAreaText->setValue(not_available);
	mTrafficText->setValue(not_available);
	mPrimitivesText->setValue(not_available);
	mParcelScriptsText->setValue(not_available);
	mTerraformLimitsText->setValue(not_available);
	mSubdivideText->setValue(not_available);
	mResaleText->setValue(not_available);
	mSaleToText->setValue(not_available);
}

//virtual
void LLPanelPlaceInfo::setParcelID(const LLUUID& parcel_id)
{
	mParcelID = parcel_id;
	sendParcelInfoRequest();
}

void LLPanelPlaceInfo::setInfoType(INFO_TYPE type)
{
	LLPanel* landmark_info_panel = getChild<LLPanel>("landmark_info_panel");
	LLPanel* landmark_edit_panel = getChild<LLPanel>("landmark_edit_panel");

	bool is_info_type_agent = type == AGENT;
	bool is_info_type_landmark = type == LANDMARK;
	bool is_info_type_teleport_history = type == TELEPORT_HISTORY;

	getChild<LLTextBox>("maturity_label")->setVisible(!is_info_type_agent);
	mMaturityRatingText->setVisible(!is_info_type_agent);

	getChild<LLTextBox>("owner_label")->setVisible(is_info_type_agent);
	mParcelOwner->setVisible(is_info_type_agent);

	getChild<LLTextBox>("last_visited_label")->setVisible(is_info_type_teleport_history);
	mLastVisited->setVisible(is_info_type_teleport_history);

	landmark_info_panel->setVisible(is_info_type_landmark);
	landmark_edit_panel->setVisible(is_info_type_landmark || type == CREATE_LANDMARK);

	getChild<LLAccordionCtrl>("advanced_info_accordion")->setVisible(is_info_type_agent);

	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();

	switch(type)
	{
		case CREATE_LANDMARK:
			mCurrentTitle = getString("title_create_landmark");
		break;

		case AGENT:
			if (parcel_mgr)
			{
				// If information is requested for current agent location
				// start using LLViewerParcelMgr for land selection.
				parcel_mgr->addObserver(this);
				parcel_mgr->selectParcelAt(gAgent.getPositionGlobal());
			}

		// Fall through to PLACE case
		case PLACE:
			mCurrentTitle = getString("title_place");

			if (!isMediaPanelVisible())
			{
				mTitle->setText(mCurrentTitle);
			}
		break;

		case LANDMARK:
			mCurrentTitle = getString("title_landmark");
		break;

		case TELEPORT_HISTORY:
			mCurrentTitle = getString("title_teleport_history");
		break;
	}

	if (type != AGENT && parcel_mgr != NULL)
	{
		if (!parcel_mgr->selectionEmpty())
		{
			parcel_mgr->deselectUnused();
		}
		parcel_mgr->removeObserver(this);
	}

	if (type != PLACE)
		toggleMediaPanel(FALSE);

	mInfoType = type;
}

BOOL LLPanelPlaceInfo::isMediaPanelVisible()
{
	if (!mMediaPanel)
		return FALSE;

	return mMediaPanel->getVisible();
}

void LLPanelPlaceInfo::toggleMediaPanel(BOOL visible)
{
    if (!mMediaPanel)
        return;

    if (visible)
	{
		mTitle->setText(getString("title_media"));
	}
	else
	{
		mTitle->setText(mCurrentTitle);
	}

    mInfoPanel->setVisible(!visible);
    mMediaPanel->setVisible(visible);
}

void LLPanelPlaceInfo::sendParcelInfoRequest()
{
	if (mParcelID != mRequestedID)
	{
		LLRemoteParcelInfoProcessor::getInstance()->addObserver(mParcelID, this);
		LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(mParcelID);

		mRequestedID = mParcelID;
	}
}

// virtual
void LLPanelPlaceInfo::setErrorStatus(U32 status, const std::string& reason)
{
	// We only really handle 404 and 499 errors
	std::string error_text;
	if(status == 404)
	{
		error_text = getString("server_error_text");
	}
	else if(status == 499)
	{
		error_text = getString("server_forbidden_text");
	}
	mDescEditor->setText(error_text);
}

// virtual
void LLPanelPlaceInfo::processParcelInfo(const LLParcelData& parcel_data)
{
	if(parcel_data.snapshot_id.notNull())
	{
		mSnapshotCtrl->setImageAssetID(parcel_data.snapshot_id);
	}

	if( !parcel_data.name.empty())
	{
		mParcelName->setText(parcel_data.name);
	}

	if( !parcel_data.desc.empty())
	{
		mDescEditor->setText(parcel_data.desc);
	}

	// HACK: Flag 0x2 == adult region,
	// Flag 0x1 == mature region, otherwise assume PG
	std::string rating = LLViewerRegion::accessToString(SIM_ACCESS_PG);
	if (parcel_data.flags & 0x2)
	{
		rating = LLViewerRegion::accessToString(SIM_ACCESS_ADULT);
	}
	else if (parcel_data.flags & 0x1)
	{
		rating = LLViewerRegion::accessToString(SIM_ACCESS_MATURE);
	}

	mMaturityRatingText->setValue(rating);
	mRatingText->setValue(rating);

	//update for_sale banner, here we should use DFQ_FOR_SALE instead of PF_FOR_SALE
	//because we deal with remote parcel response format
	bool isForSale = (parcel_data.flags & DFQ_FOR_SALE)? TRUE : FALSE;
	getChild<LLIconCtrl>("icon_for_sale")->setVisible(isForSale);
	
	// Just use given region position for display
	S32 region_x = llround(mPosRegion.mV[0]);
	S32 region_y = llround(mPosRegion.mV[1]);
	S32 region_z = llround(mPosRegion.mV[2]);

	// If the region position is zero, grab position from the global
	if(mPosRegion.isExactlyZero())
	{
		region_x = llround(parcel_data.global_x) % REGION_WIDTH_UNITS;
		region_y = llround(parcel_data.global_y) % REGION_WIDTH_UNITS;
		region_z = llround(parcel_data.global_z);
	}

	std::string name;
	if (!parcel_data.sim_name.empty())
	{
		name = llformat("%s (%d, %d, %d)",
						parcel_data.sim_name.c_str(), region_x, region_y, region_z);
		mRegionName->setText(name);
	}
	
	if (mInfoType == CREATE_LANDMARK)
	{
		mTitleEditor->setText(parcel_data.name);
		mNotesEditor->setText(LLStringUtil::null);
	}
}

void LLPanelPlaceInfo::displayParcelInfo(const LLVector3& pos_region,
									 const LLUUID& region_id,
									 const LLVector3d& pos_global)
{
	mPosRegion = pos_region;

	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
		return;

	LLSD body;
	std::string url = region->getCapability("RemoteParcelRequest");
	if (!url.empty())
	{
		body["location"] = ll_sd_from_vector3(pos_region);
		if (!region_id.isNull())
		{
			body["region_id"] = region_id;
		}
		if (!pos_global.isExactlyZero())
		{
			U64 region_handle = to_region_handle(pos_global);
			body["region_handle"] = ll_sd_from_U64(region_handle);
		}
		LLHTTPClient::post(url, body, new LLRemoteParcelRequestResponder(getObserverHandle()));
	}
	else
	{
		mDescEditor->setText(getString("server_update_text"));
	}
}

void LLPanelPlaceInfo::displayAgentParcelInfo()
{
	mParcel = LLViewerParcelMgr::getInstance()->getFloatingParcelSelection();

	LLParcel* parcel = mParcel->getParcel();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	if (!region || !parcel)
		return;

	// send EstateCovenantInfo message
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessage("EstateCovenantRequest");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->sendReliable(region->getHost());

	LLParcelData parcel_data;

	// HACK: Converting sim access flags to the format
	// returned by remote parcel response.
	switch(region->getSimAccess())
	{
	case SIM_ACCESS_MATURE:
		parcel_data.flags = 0x1;

	case SIM_ACCESS_ADULT:
		parcel_data.flags = 0x2;

	default:
		parcel_data.flags = 0;
	}
	parcel_data.desc = parcel->getDesc();
	parcel_data.name = parcel->getName();
	parcel_data.sim_name = gAgent.getRegion()->getName();
	parcel_data.snapshot_id = parcel->getSnapshotID();
	LLVector3d global_pos = gAgent.getPositionGlobal();
	parcel_data.global_x = global_pos.mdV[0];
	parcel_data.global_y = global_pos.mdV[1];
	parcel_data.global_z = global_pos.mdV[2];

	mPosRegion = gAgent.getPositionAgent();

	processParcelInfo(parcel_data);

	std::string on = getString("on");
	std::string off = getString("off");

	// Processing parcel characteristics
	if (parcel->getParcelFlagAllowVoice())
	{
		mVoiceText->setText(on);
	}
	else
	{
		mVoiceText->setText(off);
	}
	
	if (!region->getBlockFly() && parcel->getAllowFly())
	{
		mFlyText->setText(on);
	}
	else
	{
		mFlyText->setText(off);
	}

	if (region->getRestrictPushObject() || parcel->getRestrictPushObject())
	{
		mPushText->setText(off);
	}
	else
	{
		mPushText->setText(on);
	}

	if (parcel->getAllowModify())
	{
		mBuildText->setText(on);
	}
	else
	{
		mBuildText->setText(off);
	}

	if((region->getRegionFlags() & REGION_FLAGS_SKIP_SCRIPTS) ||
	   (region->getRegionFlags() & REGION_FLAGS_ESTATE_SKIP_SCRIPTS) ||
	   !parcel->getAllowOtherScripts())
	{
		mScriptsText->setText(off);
	}
	else
	{
		mScriptsText->setText(on);
	}

	if (region->getAllowDamage() || parcel->getAllowDamage())
	{
		mDamageText->setText(on);
	}
	else
	{
		mDamageText->setText(off);
	}

	mRegionNameText->setText(region->getName());
	mRegionTypeText->setText(region->getSimProductName());
	mRegionRatingText->setText(region->getSimAccessString());

	// Determine parcel owner
	if (parcel->isPublic())
	{
		mParcelOwner->setText(getString("public"));
		mRegionOwnerText->setText(getString("public"));
	}
	else
	{
		if (parcel->getIsGroupOwned())
		{
			mRegionOwnerText->setText(getString("group_owned_text"));

			if(!parcel->getGroupID().isNull())
			{
				// FIXME: Using parcel group as region group.
				gCacheName->get(parcel->getGroupID(), TRUE,
								boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mRegionGroupText, _2, _3));

				gCacheName->get(parcel->getGroupID(), TRUE,
								boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mParcelOwner, _2, _3));
			}
			else
			{
				std::string owner = getString("none_text");
				mRegionGroupText->setText(owner);
				mParcelOwner->setText(owner);
			}
		}
		else
		{
			// Figure out the owner's name
			gCacheName->get(parcel->getOwnerID(), FALSE,
							boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mParcelOwner, _2, _3));
			gCacheName->get(region->getOwner(), FALSE,
							boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mRegionOwnerText, _2, _3));
		}

		if(LLParcel::OS_LEASE_PENDING == parcel->getOwnershipStatus())
		{
			mRegionOwnerText->setText(mRegionOwnerText->getText() + getString("sale_pending_text"));
		}
	}

	mEstateRatingText->setText(region->getSimAccessString());

	S32 area;
	S32 claim_price;
	S32 rent_price;
	F32 dwell;
	BOOL for_sale = parcel->getForSale();
	LLViewerParcelMgr::getInstance()->getDisplayInfo(&area,
													 &claim_price,
													 &rent_price,
													 &for_sale,
													 &dwell);

	if (for_sale)
	{
		// Adding "For Sale" flag in remote parcel response format.
		parcel_data.flags |= DFQ_FOR_SALE;

		const LLUUID& auth_buyer_id = parcel->getAuthorizedBuyerID();
		if(auth_buyer_id.notNull())
		{
			gCacheName->get(auth_buyer_id, TRUE,
							boost::bind(&LLPanelPlaceInfo::nameUpdatedCallback, this, mSaleToText, _2, _3));

			// Show sales info to a specific person or a group he belongs to.
			if (auth_buyer_id != gAgent.getID() && !gAgent.isInGroup(auth_buyer_id))
			{
				for_sale = FALSE;
			}
		}
		else
		{
			mSaleToText->setText(getString("anyone"));
		}

		const U8* sign = (U8*)getString("price_text").c_str();
		const U8* sqm = (U8*)getString("area_text").c_str();

		mSalesPriceText->setText(llformat("%s%d ", sign, parcel->getSalePrice()));
		mAreaText->setText(llformat("%d %s", area, sqm));
		mTrafficText->setText(llformat("%.0f", dwell));

		// Can't have more than region max tasks, regardless of parcel
		// object bonus factor.
		S32 primitives = llmin(llround(parcel->getMaxPrimCapacity() * parcel->getParcelPrimBonus()),
							   (S32)region->getMaxTasks());

		const U8* available = (U8*)getString("available").c_str();
		const U8* allocated = (U8*)getString("allocated").c_str();

		mPrimitivesText->setText(llformat("%d %s, %d %s", primitives, available, parcel->getPrimCount(), allocated));

		if (parcel->getAllowOtherScripts())
		{
			mParcelScriptsText->setText(getString("all_residents_text"));
		}
		else if (parcel->getAllowGroupScripts())
		{
			mParcelScriptsText->setText(getString("group_text"));
		}
		else
		{
			mParcelScriptsText->setText(off);
		}

		mTerraformLimitsText->setText(parcel->getAllowTerraform() ? on : off);

		if (region->getRegionFlags() & REGION_FLAGS_ALLOW_PARCEL_CHANGES)
		{
			mSubdivideText->setText(getString("can_change"));
		}
		else
		{
			mSubdivideText->setText(getString("can_not_change"));
		}
		if (region->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
		{
			mResaleText->setText(getString("can_not_resell"));
		}
		else
		{
			mResaleText->setText(getString("can_resell"));
		}
	}

	getChild<LLAccordionCtrlTab>("sales_tab")->setVisible(for_sale);
}

// virtual
void LLPanelPlaceInfo::changed()
{
	resetLocation();
	displayAgentParcelInfo();
}

void LLPanelPlaceInfo::updateEstateName(const std::string& name)
{
	mEstateNameText->setText(name);
}

void LLPanelPlaceInfo::updateEstateOwnerName(const std::string& name)
{
	mEstateOwnerText->setText(name);
}

void LLPanelPlaceInfo::updateCovenantText(const std::string &text)
{
	mCovenantText->setText(text);
}

void LLPanelPlaceInfo::updateLastVisitedText(const LLDate &date)
{
	if (date.isNull())
	{
		mLastVisited->setText(getString("unknown"));
	}
	else
	{
		std::string timeStr = getString("acquired_date");
		LLSD substitution;
		substitution["datetime"] = (S32) date.secondsSinceEpoch();
		LLStringUtil::format (timeStr, substitution);
		mLastVisited->setText(timeStr);
	}
}

void LLPanelPlaceInfo::onCommitTitleOrNote(LANDMARK_INFO_TYPE type)
{
	LLInventoryItem* item = gInventory.getItem(mLandmarkID);
	if (!item)
		return;

	std::string current_value;
	std::string item_value;
	if (type == TITLE)
	{
		if (mTitleEditor)
		{
			current_value = mTitleEditor->getText();
			item_value = item->getName();
		}
	}
	else
	{
		if (mNotesEditor)
		{
			current_value = mNotesEditor->getText();
			item_value = item->getDescription();
		}
	}

	if (item_value != current_value &&
	    gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);

		if (type == TITLE)
		{
			new_item->rename(current_value);
		}
		else
		{
			new_item->setDescription(current_value);
		}

		new_item->updateServer(FALSE);
		gInventory.updateItem(new_item);
		gInventory.notifyObservers();
	}
}

void LLPanelPlaceInfo::createLandmark(const LLUUID& folder_id)
{
	std::string name = mTitleEditor->getText();
	std::string desc = mNotesEditor->getText();

	LLStringUtil::trim(name);
	LLStringUtil::trim(desc);

	// If typed name is empty use the parcel name instead.
	if (name.empty())
	{
		name = mParcelName->getText();
	}

	LLStringUtil::replaceChar(desc, '\n', ' ');
	// If no folder chosen use the "Landmarks" folder.
	LLLandmarkActions::createLandmarkHere(name, desc, 
		folder_id.notNull() ? folder_id : gInventory.findCategoryUUIDForType(LLAssetType::AT_LANDMARK));
}

void LLPanelPlaceInfo::createPick(const LLVector3d& global_pos)
{
	LLPickData pick_data;

	pick_data.agent_id = gAgent.getID();
	pick_data.session_id = gAgent.getSessionID();
	pick_data.pick_id = LLUUID::generateNewID();
	pick_data.creator_id = gAgentID;

	//legacy var  need to be deleted
	pick_data.top_pick = FALSE;
	pick_data.parcel_id = mParcelID;
	pick_data.name = mParcelName->getText();
	pick_data.desc = mDescEditor->getText();
	pick_data.snapshot_id = mSnapshotCtrl->getImageAssetID();
	pick_data.pos_global = global_pos;
	pick_data.sort_order = 0;
	pick_data.enabled = TRUE;

	LLAvatarPropertiesProcessor::instance().sendDataUpdate(&pick_data, APT_PICK_INFO);
}

// virtual
void LLPanelPlaceInfo::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (mMinHeight > 0 && mScrollingPanel != NULL)
	{
		mScrollingPanel->reshape(mScrollingPanel->getRect().getWidth(), mMinHeight);
	}

	LLView::reshape(width, height, called_from_parent);
}

// virtual
void LLPanelPlaceInfo::handleVisibilityChange (BOOL new_visibility)
{
	LLPanel::handleVisibilityChange(new_visibility);

	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
	if (!parcel_mgr)
		return;

	// Remove land selection when panel hides.
	if (!new_visibility)
	{
		if (!parcel_mgr->selectionEmpty())
		{
			parcel_mgr->deselectLand();
		}
	}
}
