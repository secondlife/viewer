/**
 * @file llpanelplaceprofile.cpp
 * @brief Displays place profile in Side Tray.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llpanelplaceprofile.h"

#include "llavatarnamecache.h"
#include "llparcel.h"
#include "message.h"

#include "llexpandabletextbox.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "lltexteditor.h"

#include "lltrans.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llagent.h"
#include "llagentui.h"
#include "llappviewer.h"
#include "llcallbacklist.h"
#include "llbuycurrencyhtml.h"
#include "llslurl.h"
#include "llstatusbar.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

static LLRegisterPanelClassWrapper<LLPanelPlaceProfile> t_place_profile("panel_place_profile");

// Statics for textures filenames
static std::string icon_pg;
static std::string icon_m;
static std::string icon_r;
static std::string icon_voice;
static std::string icon_voice_no;
static std::string icon_fly;
static std::string icon_fly_no;
static std::string icon_push;
static std::string icon_push_no;
static std::string icon_build;
static std::string icon_build_no;
static std::string icon_scripts;
static std::string icon_scripts_no;
static std::string icon_damage;
static std::string icon_damage_no;
static std::string icon_see_avs_on;
static std::string icon_see_avs_off;

LLPanelPlaceProfile::LLPanelPlaceProfile()
:	LLPanelPlaceInfo(),
	mForSalePanel(NULL),
	mYouAreHerePanel(NULL),
	mSelectedParcelID(-1),
	mAccordionCtrl(NULL)
{}

// virtual
LLPanelPlaceProfile::~LLPanelPlaceProfile()
{
	gIdleCallbacks.deleteFunction(&LLPanelPlaceProfile::updateYouAreHereBanner, this);
}

// virtual
BOOL LLPanelPlaceProfile::postBuild()
{
	LLPanelPlaceInfo::postBuild();

	mForSalePanel = getChild<LLPanel>("for_sale_panel");
	mYouAreHerePanel = getChild<LLPanel>("here_panel");
	gIdleCallbacks.addFunction(&LLPanelPlaceProfile::updateYouAreHereBanner, this);

	//Icon value should contain sale price of last selected parcel.
	mForSalePanel->getChild<LLIconCtrl>("icon_for_sale")->
				setMouseDownCallback(boost::bind(&LLPanelPlaceProfile::onForSaleBannerClick, this));

	mParcelOwner = getChild<LLTextBox>("owner_value");

	mParcelRatingIcon = getChild<LLIconCtrl>("rating_icon");
	mParcelRatingText = getChild<LLTextBox>("rating_value");
	mVoiceIcon = getChild<LLIconCtrl>("voice_icon");
	mVoiceText = getChild<LLTextBox>("voice_value");
	mFlyIcon = getChild<LLIconCtrl>("fly_icon");
	mFlyText = getChild<LLTextBox>("fly_value");
	mPushIcon = getChild<LLIconCtrl>("push_icon");
	mPushText = getChild<LLTextBox>("push_value");
	mBuildIcon = getChild<LLIconCtrl>("build_icon");
	mBuildText = getChild<LLTextBox>("build_value");
	mScriptsIcon = getChild<LLIconCtrl>("scripts_icon");
	mScriptsText = getChild<LLTextBox>("scripts_value");
	mDamageIcon = getChild<LLIconCtrl>("damage_icon");
	mDamageText = getChild<LLTextBox>("damage_value");
	mSeeAVsIcon = getChild<LLIconCtrl>("see_avatars_icon");
	mSeeAVsText = getChild<LLTextBox>("see_avatars_value");

	mRegionNameText = getChild<LLTextBox>("region_name");
	mRegionTypeText = getChild<LLTextBox>("region_type");
	mRegionRatingIcon = getChild<LLIconCtrl>("region_rating_icon");
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
	mAccordionCtrl = getChild<LLAccordionCtrl>("advanced_info_accordion");

	icon_pg = getString("icon_PG");
	icon_m = getString("icon_M");
	icon_r = getString("icon_R");
	icon_voice = getString("icon_Voice");
	icon_voice_no = getString("icon_VoiceNo");
	icon_fly = getString("icon_Fly");
	icon_fly_no = getString("icon_FlyNo");
	icon_push = getString("icon_Push");
	icon_push_no = getString("icon_PushNo");
	icon_build = getString("icon_Build");
	icon_build_no = getString("icon_BuildNo");
	icon_scripts = getString("icon_Scripts");
	icon_scripts_no = getString("icon_ScriptsNo");
	icon_damage = getString("icon_Damage");
	icon_damage_no = getString("icon_DamageNo");
	icon_see_avs_on = getString("icon_SeeAVs_On");
	icon_see_avs_off = getString("icon_SeeAVs_Off");

	return TRUE;
}

// virtual
void LLPanelPlaceProfile::resetLocation()
{
	LLPanelPlaceInfo::resetLocation();

	mForSalePanel->setVisible(FALSE);
	mYouAreHerePanel->setVisible(FALSE);

	std::string loading = LLTrans::getString("LoadingData");
	mParcelOwner->setValue(loading);

	mParcelRatingIcon->setValue(loading);
	mParcelRatingText->setText(loading);
	mVoiceIcon->setValue(loading);
	mVoiceText->setText(loading);
	mFlyIcon->setValue(loading);
	mFlyText->setText(loading);
	mPushIcon->setValue(loading);
	mPushText->setText(loading);
	mBuildIcon->setValue(loading);
	mBuildText->setText(loading);
	mScriptsIcon->setValue(loading);
	mScriptsText->setText(loading);
	mDamageIcon->setValue(loading);
	mDamageText->setText(loading);
	mSeeAVsIcon->setValue(loading);
	mSeeAVsText->setText(loading);

	mRegionNameText->setValue(loading);
	mRegionTypeText->setValue(loading);
	mRegionRatingIcon->setValue(loading);
	mRegionRatingText->setValue(loading);
	mRegionOwnerText->setValue(loading);
	mRegionGroupText->setValue(loading);

	mEstateNameText->setValue(loading);
	mEstateRatingText->setValue(loading);
	mEstateOwnerText->setValue(loading);
	mCovenantText->setValue(loading);

	mSalesPriceText->setValue(loading);
	mAreaText->setValue(loading);
	mTrafficText->setValue(loading);
	mPrimitivesText->setValue(loading);
	mParcelScriptsText->setValue(loading);
	mTerraformLimitsText->setValue(loading);
	mSubdivideText->setValue(loading);
	mResaleText->setValue(loading);
	mSaleToText->setValue(loading);
}

// virtual
void LLPanelPlaceProfile::setInfoType(EInfoType type)
{
	bool is_info_type_agent = type == AGENT;

	mMaturityRatingIcon->setVisible(!is_info_type_agent);
	mMaturityRatingText->setVisible(!is_info_type_agent);

	getChild<LLTextBox>("owner_label")->setVisible(is_info_type_agent);
	mParcelOwner->setVisible(is_info_type_agent);

	getChild<LLAccordionCtrl>("advanced_info_accordion")->setVisible(is_info_type_agent);

	// If we came from search we want larger description area, approx. 10 lines (see STORM-1311).
	// Don't use the maximum available space because that leads to nasty artifacts
	// in text editor and expandable text box.
	{
		const S32 SEARCH_DESC_HEIGHT = 150;

		// Remember original geometry (once).
		static const S32 sOrigDescVPad = getChildView("parcel_title")->getRect().mBottom - mDescEditor->getRect().mTop;
		static const S32 sOrigDescHeight = mDescEditor->getRect().getHeight();
		static const S32 sOrigMRIconVPad = mDescEditor->getRect().mBottom - mMaturityRatingIcon->getRect().mTop;
		static const S32 sOrigMRTextVPad = mDescEditor->getRect().mBottom - mMaturityRatingText->getRect().mTop;

		// Resize the description.
		const S32 desc_height = is_info_type_agent ? sOrigDescHeight : SEARCH_DESC_HEIGHT;
		const S32 desc_top = getChildView("parcel_title")->getRect().mBottom - sOrigDescVPad;
		LLRect desc_rect = mDescEditor->getRect();
		desc_rect.setOriginAndSize(desc_rect.mLeft, desc_top - desc_height, desc_rect.getWidth(), desc_height);
		mDescEditor->reshape(desc_rect.getWidth(), desc_rect.getHeight());
		mDescEditor->setRect(desc_rect);
		mDescEditor->updateTextShape();

		// Move the maturity rating icon/text accordingly.
		const S32 mr_icon_bottom = mDescEditor->getRect().mBottom - sOrigMRIconVPad - mMaturityRatingIcon->getRect().getHeight();
		const S32 mr_text_bottom = mDescEditor->getRect().mBottom - sOrigMRTextVPad - mMaturityRatingText->getRect().getHeight();
		mMaturityRatingIcon->setOrigin(mMaturityRatingIcon->getRect().mLeft, mr_icon_bottom);
		mMaturityRatingText->setOrigin(mMaturityRatingText->getRect().mLeft, mr_text_bottom);
	}

	switch(type)
	{
		case AGENT:
		case PLACE:
		default:
			mCurrentTitle = getString("title_place");
		break;

		case TELEPORT_HISTORY:
			mCurrentTitle = getString("title_teleport_history");
		break;
	}

	if (mAccordionCtrl != NULL)
	{
		mAccordionCtrl->expandDefaultTab();
	}

	LLPanelPlaceInfo::setInfoType(type);
}

// virtual
void LLPanelPlaceProfile::processParcelInfo(const LLParcelData& parcel_data)
{
	LLPanelPlaceInfo::processParcelInfo(parcel_data);

	// HACK: Flag 0x2 == adult region,
	// Flag 0x1 == mature region, otherwise assume PG
	if (parcel_data.flags & 0x2)
	{
		mMaturityRatingIcon->setValue(icon_r);
		mMaturityRatingText->setText(LLViewerRegion::accessToString(SIM_ACCESS_ADULT));
	}
	else if (parcel_data.flags & 0x1)
	{
		mMaturityRatingIcon->setValue(icon_m);
		mMaturityRatingText->setText(LLViewerRegion::accessToString(SIM_ACCESS_MATURE));
	}
	else
	{
		mMaturityRatingIcon->setValue(icon_pg);
		mMaturityRatingText->setText(LLViewerRegion::accessToString(SIM_ACCESS_PG));
	}
}

// virtual
void LLPanelPlaceProfile::handleVisibilityChange(BOOL new_visibility)
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
			parcel_mgr->deselectUnused();
		}
	}
}

void LLPanelPlaceProfile::displaySelectedParcelInfo(LLParcel* parcel,
													LLViewerRegion* region,
													const LLVector3d& pos_global,
													bool is_current_parcel)
{
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
	U8 sim_access = region->getSimAccess();
	switch(sim_access)
	{
	case SIM_ACCESS_MATURE:
		parcel_data.flags = 0x1;

		mParcelRatingIcon->setValue(icon_m);
		mRegionRatingIcon->setValue(icon_m);
		break;

	case SIM_ACCESS_ADULT:
		parcel_data.flags = 0x2;

		mParcelRatingIcon->setValue(icon_r);
		mRegionRatingIcon->setValue(icon_r);
		break;

	default:
		parcel_data.flags = 0;

		mParcelRatingIcon->setValue(icon_pg);
		mRegionRatingIcon->setValue(icon_pg);
	}

	std::string rating = LLViewerRegion::accessToString(sim_access);
	mParcelRatingText->setText(rating);
	mRegionRatingText->setText(rating);

	parcel_data.desc = parcel->getDesc();
	parcel_data.name = parcel->getName();
	parcel_data.sim_name = region->getName();
	parcel_data.snapshot_id = parcel->getSnapshotID();
	mPosRegion.setVec((F32)fmod(pos_global.mdV[VX], (F64)REGION_WIDTH_METERS),
					  (F32)fmod(pos_global.mdV[VY], (F64)REGION_WIDTH_METERS),
					  (F32)pos_global.mdV[VZ]);
	parcel_data.global_x = pos_global.mdV[VX];
	parcel_data.global_y = pos_global.mdV[VY];
	parcel_data.global_z = pos_global.mdV[VZ];

	std::string on = getString("on");
	std::string off = getString("off");

	LLViewerParcelMgr* vpm = LLViewerParcelMgr::getInstance();

	// Processing parcel characteristics
	if (vpm->allowAgentVoice(region, parcel))
	{
		mVoiceIcon->setValue(icon_voice);
		mVoiceText->setText(on);
	}
	else
	{
		mVoiceIcon->setValue(icon_voice_no);
		mVoiceText->setText(off);
	}

	if (vpm->allowAgentFly(region, parcel))
	{
		mFlyIcon->setValue(icon_fly);
		mFlyText->setText(on);
	}
	else
	{
		mFlyIcon->setValue(icon_fly_no);
		mFlyText->setText(off);
	}

	if (vpm->allowAgentPush(region, parcel))
	{
		mPushIcon->setValue(icon_push);
		mPushText->setText(on);
	}
	else
	{
		mPushIcon->setValue(icon_push_no);
		mPushText->setText(off);
	}

	if (vpm->allowAgentBuild(parcel))
	{
		mBuildIcon->setValue(icon_build);
		mBuildText->setText(on);
	}
	else
	{
		mBuildIcon->setValue(icon_build_no);
		mBuildText->setText(off);
	}

	if (vpm->allowAgentScripts(region, parcel))
	{
		mScriptsIcon->setValue(icon_scripts);
		mScriptsText->setText(on);
	}
	else
	{
		mScriptsIcon->setValue(icon_scripts_no);
		mScriptsText->setText(off);
	}

	if (vpm->allowAgentDamage(region, parcel))
	{
		mDamageIcon->setValue(icon_damage);
		mDamageText->setText(on);
	}
	else
	{
		mDamageIcon->setValue(icon_damage_no);
		mDamageText->setText(off);
	}

	if (parcel->getSeeAVs())
	{
		mSeeAVsIcon->setValue(icon_see_avs_on);
		mSeeAVsText->setText(on);
	}
	else
	{
		mSeeAVsIcon->setValue(icon_see_avs_off);
		mSeeAVsText->setText(off);
	}

	mRegionNameText->setText(region->getName());
	mRegionTypeText->setText(region->getLocalizedSimProductName());

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
				gCacheName->getGroup(parcel->getGroupID(),
								boost::bind(&LLPanelPlaceInfo::onNameCache, mRegionGroupText, _2));

				gCacheName->getGroup(parcel->getGroupID(),
								boost::bind(&LLPanelPlaceInfo::onNameCache, mParcelOwner, _2));
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
			std::string parcel_owner =
				LLSLURL("agent", parcel->getOwnerID(), "inspect").getSLURLString();
			mParcelOwner->setText(parcel_owner);
			LLAvatarNameCache::get(region->getOwner(),
								   boost::bind(&LLPanelPlaceInfo::onAvatarNameCache,
											   _1, _2, mRegionOwnerText));
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
	BOOL for_sale;
	vpm->getDisplayInfo(&area, &claim_price, &rent_price, &for_sale, &dwell);
	if (for_sale)
	{
		const LLUUID& auth_buyer_id = parcel->getAuthorizedBuyerID();
		if(auth_buyer_id.notNull())
		{
			LLAvatarNameCache::get(auth_buyer_id,
								   boost::bind(&LLPanelPlaceInfo::onAvatarNameCache,
											   _1, _2, mSaleToText));
			
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

		mForSalePanel->setVisible(for_sale);

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

		if (region->getRegionFlag(REGION_FLAGS_ALLOW_PARCEL_CHANGES))
		{
			mSubdivideText->setText(getString("can_change"));
		}
		else
		{
			mSubdivideText->setText(getString("can_not_change"));
		}
		if (region->getRegionFlag(REGION_FLAGS_BLOCK_LAND_RESELL))
		{
			mResaleText->setText(getString("can_not_resell"));
		}
		else
		{
			mResaleText->setText(getString("can_resell"));
		}
	}

	mSelectedParcelID = parcel->getLocalID();
	mLastSelectedRegionID = region->getRegionID();
	LLPanelPlaceInfo::processParcelInfo(parcel_data);

	mYouAreHerePanel->setVisible(is_current_parcel);
	getChild<LLAccordionCtrlTab>("sales_tab")->setVisible(for_sale);
}

void LLPanelPlaceProfile::updateEstateName(const std::string& name)
{
	mEstateNameText->setText(name);
}

void LLPanelPlaceProfile::updateEstateOwnerName(const std::string& name)
{
	mEstateOwnerText->setText(name);
}

void LLPanelPlaceProfile::updateCovenantText(const std::string &text)
{
	mCovenantText->setText(text);
}

void LLPanelPlaceProfile::onForSaleBannerClick()
{
	LLViewerParcelMgr* mgr = LLViewerParcelMgr::getInstance();
	LLParcel* parcel = mgr->getFloatingParcelSelection()->getParcel();
	LLViewerRegion* selected_region =  mgr->getSelectionRegion();
	if(parcel && selected_region)
	{
		if(parcel->getLocalID() == mSelectedParcelID &&
				mLastSelectedRegionID ==selected_region->getRegionID())
		{
			S32 price = parcel->getSalePrice();

			if(price - gStatusBar->getBalance() > 0)
			{
				LLStringUtil::format_map_t args;
				args["AMOUNT"] = llformat("%d", price);
				LLBuyCurrencyHTML::openCurrencyFloater( LLTrans::getString("buying_selected_land", args), price );
			}
			else
			{
				LLViewerParcelMgr::getInstance()->startBuyLand();
			}
		}
		else
		{
			LL_WARNS("Places") << "User  is trying  to buy remote parcel.Operation is not supported"<< LL_ENDL;
		}

	}
}

// static
void LLPanelPlaceProfile::updateYouAreHereBanner(void* userdata)
{
	//YouAreHere Banner should be displayed only for selected places,
	// If you want to display it for landmark or teleport history item, you should check by mParcelId

	LLPanelPlaceProfile* self = static_cast<LLPanelPlaceProfile*>(userdata);
	if(!self->getVisible())
		return;

	if(!gDisconnected && gAgent.getRegion())
	{
		static F32 radius = gSavedSettings.getF32("YouAreHereDistance");

		BOOL display_banner = gAgent.getRegion()->getRegionID() == self->mLastSelectedRegionID &&
										LLAgentUI::checkAgentDistance(self->mPosRegion, radius);

		self->mYouAreHerePanel->setVisible(display_banner);
	}
}
