/** 
 * @file llpanelclassified.cpp
 * @brief LLPanelClassified class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

// Display of a classified used both for the global view in the
// Find directory, and also for each individual user's classified in their
// profile.

#include "llviewerprecompiledheaders.h"

#include "llpanelclassified.h"

#include "lldispatcher.h"
#include "llfloaterreg.h"
#include "llhttpclient.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llparcel.h"

#include "llagent.h"
#include "llclassifiedflags.h"
#include "llclassifiedstatsresponder.h"
#include "llcommandhandler.h" // for classified HTML detail page click tracking
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llcombobox.h"
#include "lltexturectrl.h"
#include "lltexteditor.h"
#include "llviewerparcelmgr.h"
#include "llfloaterworldmap.h"
#include "llviewergenericmessage.h"	// send_generic_message
#include "llviewerregion.h"
#include "lltrans.h"
#include "llscrollcontainer.h"
#include "llstatusbar.h"
#include "llviewertexture.h"

const S32 MINIMUM_PRICE_FOR_LISTING = 50;	// L$

//static
LLPanelClassifiedInfo::panel_list_t LLPanelClassifiedInfo::sAllPanels;

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

// Just to debug errors. Can be thrown away later.
class LLClassifiedClickMessageResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLClassifiedClickMessageResponder);

public:
	// If we get back an error (not found, etc...), handle it here
	virtual void errorWithContent(
		U32 status,
		const std::string& reason,
		const LLSD& content)
	{
		llwarns << "Sending click message failed (" << status << "): [" << reason << "]" << llendl;
		llwarns << "Content: [" << content << "]" << llendl;
	}
};

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
 , mSnapshotCtrl(NULL)
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
	panel->buildFromFile("panel_classified_info.xml");
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

	mSnapshotCtrl = getChild<LLTextureCtrl>("classified_snapshot");
	mSnapshotRect = getDefaultSnapshotRect();

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

	mSnapshotRect = getDefaultSnapshotRect();
	stretchSnapshot();
}

void LLPanelClassifiedInfo::onOpen(const LLSD& key)
{
	LLUUID avatar_id = key["classified_creator_id"];
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
	scrollToTop();

	setClassifiedId(key["classified_id"]);
	setClassifiedName(key["classified_name"]);
	setDescription(key["classified_desc"]);
	setSnapshotId(key["classified_snapshot_id"]);
	setFromSearch(key["from_search"]);

	llinfos << "Opening classified [" << getClassifiedName() << "] (" << getClassifiedId() << ")" << llendl;

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

	// Update classified click stats.
	// *TODO: Should we do this when opening not from search?
	sendClickMessage("profile");

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
			setSimName(c_info->sim_name);

			setClassifiedLocation(createLocationText(c_info->parcel_name, c_info->sim_name, c_info->pos_global));
			getChild<LLUICtrl>("category")->setValue(LLClassifiedInfo::sCategories[c_info->category]);

			static std::string mature_str = getString("type_mature");
			static std::string pg_str = getString("type_pg");
			static LLUIString  price_str = getString("l$_price");
			static std::string date_fmt = getString("date_fmt");

			bool mature = is_cf_mature(c_info->flags);
			getChild<LLUICtrl>("content_type")->setValue(mature ? mature_str : pg_str);
			getChild<LLIconCtrl>("content_type_moderate")->setVisible(mature);
			getChild<LLIconCtrl>("content_type_general")->setVisible(!mature);

			std::string auto_renew_str = is_cf_auto_renew(c_info->flags) ? 
				getString("auto_renew_on") : getString("auto_renew_off");
			getChild<LLUICtrl>("auto_renew")->setValue(auto_renew_str);

			price_str.setArg("[PRICE]", llformat("%d", c_info->price_for_listing));
			getChild<LLUICtrl>("price_for_listing")->setValue(LLSD(price_str));

			std::string date_str = date_fmt;
			LLStringUtil::format(date_str, LLSD().with("datetime", (S32) c_info->creation_date));
			getChild<LLUICtrl>("creation_date")->setValue(date_str);

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
	setPosGlobal(LLVector3d::zero);
	setParcelId(LLUUID::null);
	setSimName(LLStringUtil::null);
	setFromSearch(false);

	// reset click stats
	mTeleportClicksOld	= 0;
	mMapClicksOld		= 0;
	mProfileClicksOld	= 0;
	mTeleportClicksNew	= 0;
	mMapClicksNew		= 0;
	mProfileClicksNew	= 0;

	getChild<LLUICtrl>("category")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("content_type")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("click_through_text")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("price_for_listing")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("auto_renew")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("creation_date")->setValue(LLStringUtil::null);
	getChild<LLUICtrl>("click_through_text")->setValue(LLStringUtil::null);
	getChild<LLIconCtrl>("content_type_moderate")->setVisible(FALSE);
	getChild<LLIconCtrl>("content_type_general")->setVisible(FALSE);
}

void LLPanelClassifiedInfo::resetControls()
{
	bool is_self = getAvatarId() == gAgent.getID();

	getChildView("edit_btn")->setEnabled(is_self);
	getChildView("edit_btn")->setVisible( is_self);
	getChildView("price_layout_panel")->setVisible( is_self);
	getChildView("clickthrough_layout_panel")->setVisible( is_self);
}

void LLPanelClassifiedInfo::setClassifiedName(const std::string& name)
{
	getChild<LLUICtrl>("classified_name")->setValue(name);
}

std::string LLPanelClassifiedInfo::getClassifiedName()
{
	return getChild<LLUICtrl>("classified_name")->getValue().asString();
}

void LLPanelClassifiedInfo::setDescription(const std::string& desc)
{
	getChild<LLUICtrl>("classified_desc")->setValue(desc);
}

std::string LLPanelClassifiedInfo::getDescription()
{
	return getChild<LLUICtrl>("classified_desc")->getValue().asString();
}

void LLPanelClassifiedInfo::setClassifiedLocation(const std::string& location)
{
	getChild<LLUICtrl>("classified_location")->setValue(location);
}

std::string LLPanelClassifiedInfo::getClassifiedLocation()
{
	return getChild<LLUICtrl>("classified_location")->getValue().asString();
}

void LLPanelClassifiedInfo::setSnapshotId(const LLUUID& id)
{
	mSnapshotCtrl->setValue(id);
	mSnapshotStreched = false;
}

void LLPanelClassifiedInfo::draw()
{
	LLPanel::draw();

	// Stretch in draw because it takes some time to load a texture,
	// going to try to stretch snapshot until texture is loaded
	if(!mSnapshotStreched)
	{
		stretchSnapshot();
	}
}

LLUUID LLPanelClassifiedInfo::getSnapshotId()
{
	return getChild<LLUICtrl>("classified_snapshot")->getValue().asUUID();
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

		self->getChild<LLUICtrl>("click_through_text")->setValue(ct_str.getString());
		// *HACK: remove this when there is enough room for click stats in the info panel
		self->getChildView("click_through_text")->setToolTip(ct_str.getString());  

		llinfos << "teleport: " << llformat("%d", self->mTeleportClicksNew + self->mTeleportClicksOld)
				<< ", map: "    << llformat("%d", self->mMapClicksNew + self->mMapClicksOld)
				<< ", profile: " << llformat("%d", self->mProfileClicksNew + self->mProfileClicksOld)
				<< llendl;
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

	LLViewerFetchedTexture* texture = mSnapshotCtrl->getTexture();

	if(!texture)
	{
		return;
	}

	if(0 == texture->getOriginalWidth() || 0 == texture->getOriginalHeight())
	{
		// looks like texture is not loaded yet
		return;
	}

	LLRect rc = mSnapshotRect;
	// *HACK dzaporozhan
	// LLTextureCtrl uses BTN_HEIGHT_SMALL as bottom for texture which causes
	// drawn texture to be smaller than expected. (see LLTextureCtrl::draw())
	// Lets increase texture height to force texture look as expected.
	rc.mBottom -= BTN_HEIGHT_SMALL;

	F32 t_width = texture->getFullWidth();
	F32 t_height = texture->getFullHeight();

	F32 ratio = llmin<F32>( (rc.getWidth() / t_width), (rc.getHeight() / t_height) );

	t_width *= ratio;
	t_height *= ratio;

	rc.setCenterAndSize(rc.getCenterX(), rc.getCenterY(), llfloor(t_width), llfloor(t_height));
	mSnapshotCtrl->setShape(rc);

	mSnapshotStreched = true;
}

LLRect LLPanelClassifiedInfo::getDefaultSnapshotRect()
{
	// Using scroll container makes getting default rect a hard task
	// because rect in postBuild() and in first reshape() is not the same.
	// Using snapshot_panel makes it easier to reshape snapshot.
	return getChild<LLUICtrl>("snapshot_panel")->getLocalRect();
}

void LLPanelClassifiedInfo::scrollToTop()
{
	LLScrollContainer* scrollContainer = findChild<LLScrollContainer>("profile_scroll");
	if (scrollContainer)
		scrollContainer->goToTop();
}

// static
// *TODO: move out of the panel
void LLPanelClassifiedInfo::sendClickMessage(
		const std::string& type,
		bool from_search,
		const LLUUID& classified_id,
		const LLUUID& parcel_id,
		const LLVector3d& global_pos,
		const std::string& sim_name)
{
	// You're allowed to click on your own ads to reassure yourself
	// that the system is working.
	LLSD body;
	body["type"]			= type;
	body["from_search"]		= from_search;
	body["classified_id"]	= classified_id;
	body["parcel_id"]		= parcel_id;
	body["dest_pos_global"]	= global_pos.getValue();
	body["region_name"]		= sim_name;

	std::string url = gAgent.getRegion()->getCapability("SearchStatTracking");
	llinfos << "Sending click msg via capability (url=" << url << ")" << llendl;
	llinfos << "body: [" << body << "]" << llendl;
	LLHTTPClient::post(url, body, new LLClassifiedClickMessageResponder());
}

void LLPanelClassifiedInfo::sendClickMessage(const std::string& type)
{
	sendClickMessage(
		type,
		fromSearch(),
		getClassifiedId(),
		getParcelId(),
		getPosGlobal(),
		getSimName());
}

void LLPanelClassifiedInfo::onMapClick()
{
	sendClickMessage("map");
	LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
	LLFloaterReg::showInstance("world_map", "center");
}

void LLPanelClassifiedInfo::onTeleportClick()
{
	if (!getPosGlobal().isExactlyZero())
	{
		sendClickMessage("teleport");
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
 , mIsNewWithErrors(false)
 , mCanClose(false)
 , mPublishFloater(NULL)
{
}

LLPanelClassifiedEdit::~LLPanelClassifiedEdit()
{
}

//static
LLPanelClassifiedEdit* LLPanelClassifiedEdit::create()
{
	LLPanelClassifiedEdit* panel = new LLPanelClassifiedEdit();
	panel->buildFromFile("panel_edit_classified.xml");
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

	mSnapshotCtrl->setOnSelectCallback(boost::bind(&LLPanelClassifiedEdit::onTextureSelected, this));

	return TRUE;
}

void LLPanelClassifiedEdit::fillIn(const LLSD& key)
{
	setAvatarId(gAgent.getID());

	if(key.isUndefined())
	{
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

		getChild<LLUICtrl>("classified_name")->setValue(makeClassifiedName());
		getChild<LLUICtrl>("classified_desc")->setValue(desc);
		setSnapshotId(snapshot_id);
		setClassifiedLocation(createLocationText(getLocationNotice(), region_name, getPosGlobal()));
		// server will set valid parcel id
		setParcelId(LLUUID::null);
	}
	else
	{
		setClassifiedId(key["classified_id"]);
		setClassifiedName(key["name"]);
		setDescription(key["desc"]);
		setSnapshotId(key["snapshot_id"]);
		setCategory((U32)key["category"].asInteger());
		setContentType((U32)key["content_type"].asInteger());
		setClassifiedLocation(key["location_text"]);
		getChild<LLUICtrl>("auto_renew")->setValue(key["auto_renew"]);
		getChild<LLUICtrl>("price_for_listing")->setValue(key["price_for_listing"].asInteger());
	}
}

void LLPanelClassifiedEdit::onOpen(const LLSD& key)
{
	mIsNew = key.isUndefined();
	
	scrollToTop();

	// classified is not created yet
	bool is_new = isNew() || isNewWithErrors();

	if(is_new)
	{
		resetData();
		resetControls();

		fillIn(key);

		if(isNew())
		{
			LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(), this);
		}
	}
	else
	{
		LLPanelClassifiedInfo::onOpen(key);
	}

	std::string save_btn_label = is_new ? getString("publish_label") : getString("save_label");
	getChild<LLUICtrl>("save_changes_btn")->setLabelArg("[LABEL]", save_btn_label);

	enableVerbs(is_new);
	enableEditing(is_new);
	showEditing(!is_new);
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
			// see LLPanelClassifiedEdit::sendUpdate() for notes
			mIsNewWithErrors = false;
			// for just created classified - panel will probably be closed when we get here.
			if(!getVisible())
			{
				return;
			}

			enableEditing(true);

			setClassifiedName(c_info->name);
			setDescription(c_info->description);
			setSnapshotId(c_info->snapshot_id);
			setPosGlobal(c_info->pos_global);

			setClassifiedLocation(createLocationText(c_info->parcel_name, c_info->sim_name, c_info->pos_global));
			// *HACK see LLPanelClassifiedEdit::sendUpdate()
			setCategory(c_info->category - 1);

			bool mature = is_cf_mature(c_info->flags);
			bool auto_renew = is_cf_auto_renew(c_info->flags);

			setContentType(mature ? CB_ITEM_MATURE : CB_ITEM_PG);
			getChild<LLUICtrl>("auto_renew")->setValue(auto_renew);
			getChild<LLUICtrl>("price_for_listing")->setValue(c_info->price_for_listing);
			getChildView("price_for_listing")->setEnabled(isNew());

			resetDirty();
			setInfoLoaded(true);
			enableVerbs(false);

			// for just created classified - in case user opened edit panel before processProperties() callback 
			getChild<LLUICtrl>("save_changes_btn")->setLabelArg("[LABEL]", getString("save_label"));
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

	LLTextEditor* desc = getChild<LLTextEditor>("classified_desc");
	// call blockUndo() to really reset dirty(and make isDirty work as intended)
	desc->blockUndo();
	desc->resetDirty();

	getChild<LLUICtrl>("category")->resetDirty();
	getChild<LLUICtrl>("content_type")->resetDirty();
	getChild<LLUICtrl>("auto_renew")->resetDirty();
	getChild<LLUICtrl>("price_for_listing")->resetDirty();
}

void LLPanelClassifiedEdit::setSaveCallback(const commit_signal_t::slot_type& cb)
{
	mSaveButtonClickedSignal.connect(cb);
}

void LLPanelClassifiedEdit::setCancelCallback(const commit_signal_t::slot_type& cb)
{
	getChild<LLButton>("cancel_btn")->setClickedCallback(cb);
}

void LLPanelClassifiedEdit::resetControls()
{
	LLPanelClassifiedInfo::resetControls();

	getChild<LLComboBox>("category")->setCurrentByIndex(0);
	getChild<LLComboBox>("content_type")->setCurrentByIndex(0);
	getChild<LLUICtrl>("auto_renew")->setValue(false);
	getChild<LLUICtrl>("price_for_listing")->setValue(MINIMUM_PRICE_FOR_LISTING);
	getChildView("price_for_listing")->setEnabled(TRUE);
}

bool LLPanelClassifiedEdit::canClose()
{
	return mCanClose;
}

void LLPanelClassifiedEdit::draw()
{
	LLPanel::draw();

	// Need to re-stretch on every draw because LLTextureCtrl::onSelectCallback
	// does not trigger callbacks when user navigates through images.
	stretchSnapshot();
}

void LLPanelClassifiedEdit::stretchSnapshot()
{
	LLPanelClassifiedInfo::stretchSnapshot();

	getChild<LLUICtrl>("edit_icon")->setShape(mSnapshotCtrl->getRect());
}

U32 LLPanelClassifiedEdit::getContentType()
{
	LLComboBox* ct_cb = getChild<LLComboBox>("content_type");
	return ct_cb->getCurrentIndex();
}

void LLPanelClassifiedEdit::setContentType(U32 content_type)
{
	LLComboBox* ct_cb = getChild<LLComboBox>("content_type");
	ct_cb->setCurrentByIndex(content_type);
	ct_cb->resetDirty();
}

bool LLPanelClassifiedEdit::getAutoRenew()
{
	return getChild<LLUICtrl>("auto_renew")->getValue().asBoolean();
}

void LLPanelClassifiedEdit::sendUpdate()
{
	LLAvatarClassifiedInfo c_data;

	if(getClassifiedId().isNull())
	{
		setClassifiedId(LLUUID::generateNewID());
	}

	c_data.agent_id = gAgent.getID();
	c_data.classified_id = getClassifiedId();
	// *HACK 
	// Categories on server start with 1 while combo-box index starts with 0
	c_data.category = getCategory() + 1;
	c_data.name = getClassifiedName();
	c_data.description = getDescription();
	c_data.parcel_id = getParcelId();
	c_data.snapshot_id = getSnapshotId();
	c_data.pos_global = getPosGlobal();
	c_data.flags = getFlags();
	c_data.price_for_listing = getPriceForListing();

	LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoUpdate(&c_data);
	
	if(isNew())
	{
		// Lets assume there will be some error.
		// Successful sendClassifiedInfoUpdate will trigger processProperties and
		// let us know there was no error.
		mIsNewWithErrors = true;
	}
}

U32 LLPanelClassifiedEdit::getCategory()
{
	LLComboBox* cat_cb = getChild<LLComboBox>("category");
	return cat_cb->getCurrentIndex();
}

void LLPanelClassifiedEdit::setCategory(U32 category)
{
	LLComboBox* cat_cb = getChild<LLComboBox>("category");
	cat_cb->setCurrentByIndex(category);
	cat_cb->resetDirty();
}

U8 LLPanelClassifiedEdit::getFlags()
{
	bool auto_renew = getChild<LLUICtrl>("auto_renew")->getValue().asBoolean();

	LLComboBox* content_cb = getChild<LLComboBox>("content_type");
	bool mature = content_cb->getCurrentIndex() == CB_ITEM_MATURE;
	
	return pack_classified_flags_request(auto_renew, false, mature, false);
}

void LLPanelClassifiedEdit::enableVerbs(bool enable)
{
	getChildView("save_changes_btn")->setEnabled(enable);
}

void LLPanelClassifiedEdit::enableEditing(bool enable)
{
	getChildView("classified_snapshot")->setEnabled(enable);
	getChildView("classified_name")->setEnabled(enable);
	getChildView("classified_desc")->setEnabled(enable);
	getChildView("set_to_curr_location_btn")->setEnabled(enable);
	getChildView("category")->setEnabled(enable);
	getChildView("content_type")->setEnabled(enable);
	getChildView("price_for_listing")->setEnabled(enable);
	getChildView("auto_renew")->setEnabled(enable);
}

void LLPanelClassifiedEdit::showEditing(bool show)
{
	getChildView("price_for_listing_label")->setVisible( show);
	getChildView("price_for_listing")->setVisible( show);
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
	return getChild<LLUICtrl>("price_for_listing")->getValue().asInteger();
}

void LLPanelClassifiedEdit::setPriceForListing(S32 price)
{
	getChild<LLUICtrl>("price_for_listing")->setValue(price);
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
	if(isNew() || isNewWithErrors())
	{
		if(gStatusBar->getBalance() < getPriceForListing())
		{
			LLNotificationsUtil::add("ClassifiedInsufficientFunds");
			return;
		}

		mPublishFloater = LLFloaterReg::findTypedInstance<LLPublishClassifiedFloater>(
			"publish_classified", LLSD());

		if(!mPublishFloater)
		{
			mPublishFloater = LLFloaterReg::getTypedInstance<LLPublishClassifiedFloater>(
				"publish_classified", LLSD());

			mPublishFloater->setPublishClickedCallback(boost::bind
				(&LLPanelClassifiedEdit::onPublishFloaterPublishClicked, this));
		}

		// set spinner value before it has focus or value wont be set
		mPublishFloater->setPrice(getPriceForListing());
		mPublishFloater->openFloater(mPublishFloater->getKey());
		mPublishFloater->center();
	}
	else
	{
		doSave();
	}
}

void LLPanelClassifiedEdit::doSave()
{
	mCanClose = true;
	sendUpdate();
	resetDirty();

	mSaveButtonClickedSignal(this, LLSD());
}

void LLPanelClassifiedEdit::onPublishFloaterPublishClicked()
{
	setPriceForListing(mPublishFloater->getPrice());

	doSave();
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

void LLPanelClassifiedEdit::onTextureSelected()
{
	setSnapshotId(mSnapshotCtrl->getValue().asUUID());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPublishClassifiedFloater::LLPublishClassifiedFloater(const LLSD& key)
 : LLFloater(key)
{
}

LLPublishClassifiedFloater::~LLPublishClassifiedFloater()
{
}

BOOL LLPublishClassifiedFloater::postBuild()
{
	LLFloater::postBuild();

	childSetAction("publish_btn", boost::bind(&LLFloater::closeFloater, this, false));
	childSetAction("cancel_btn", boost::bind(&LLFloater::closeFloater, this, false));

	return TRUE;
}

void LLPublishClassifiedFloater::setPrice(S32 price)
{
	getChild<LLUICtrl>("price_for_listing")->setValue(price);
}

S32 LLPublishClassifiedFloater::getPrice()
{
	return getChild<LLUICtrl>("price_for_listing")->getValue().asInteger();
}

void LLPublishClassifiedFloater::setPublishClickedCallback(const commit_signal_t::slot_type& cb)
{
	getChild<LLButton>("publish_btn")->setClickedCallback(cb);
}

void LLPublishClassifiedFloater::setCancelClickedCallback(const commit_signal_t::slot_type& cb)
{
	getChild<LLButton>("cancel_btn")->setClickedCallback(cb);
}

//EOF
