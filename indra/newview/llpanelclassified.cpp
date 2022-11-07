/** 
 * @file llpanelclassified.cpp
 * @brief LLPanelClassifiedInfo class implementation
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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
#include "llparcel.h"

#include "llagent.h"
#include "llclassifiedflags.h"
#include "lliconctrl.h"
#include "lltexturectrl.h"
#include "llfloaterworldmap.h"
#include "llviewergenericmessage.h" // send_generic_message
#include "llviewerregion.h"
#include "llscrollcontainer.h"
#include "llcorehttputil.h"

//static
LLPanelClassifiedInfo::panel_list_t LLPanelClassifiedInfo::sAllPanels;
static LLPanelInjector<LLPanelClassifiedInfo> t_panel_panel_classified_info("panel_classified_info");

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

BOOL LLPanelClassifiedInfo::postBuild()
{
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

    LL_INFOS() << "Opening classified [" << getClassifiedName() << "] (" << getClassifiedId() << ")" << LL_ENDL;

    LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(), this);
    LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoRequest(getClassifiedId());
    gGenericDispatcher.addHandler("classifiedclickthrough", &sClassifiedClickThrough);

    if (gAgent.getRegion())
    {
        // While we're at it let's get the stats from the new table if that
        // capability exists.
        std::string url = gAgent.getRegion()->getCapability("SearchStatRequest");
        if (!url.empty())
        {
            LL_INFOS() << "Classified stat request via capability" << LL_ENDL;
            LLSD body;
            LLUUID classifiedId = getClassifiedId();
            body["classified_id"] = classifiedId;
            LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(url, body,
                boost::bind(&LLPanelClassifiedInfo::handleSearchStatResponse, classifiedId, _1));
        }
    }
    // Update classified click stats.
    // *TODO: Should we do this when opening not from search?
    sendClickMessage("profile");

    setInfoLoaded(false);
}

/*static*/
void LLPanelClassifiedInfo::handleSearchStatResponse(LLUUID classifiedId, LLSD result)
{
    S32 teleport = result["teleport_clicks"].asInteger();
    S32 map = result["map_clicks"].asInteger();
    S32 profile = result["profile_clicks"].asInteger();
    S32 search_teleport = result["search_teleport_clicks"].asInteger();
    S32 search_map = result["search_map_clicks"].asInteger();
    S32 search_profile = result["search_profile_clicks"].asInteger();

    LLPanelClassifiedInfo::setClickThrough(classifiedId,
        teleport + search_teleport,
        map + search_map,
        profile + search_profile,
        true);
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

            LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
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
    mTeleportClicksOld  = 0;
    mMapClicksOld       = 0;
    mProfileClicksOld   = 0;
    mTeleportClicksNew  = 0;
    mMapClicksNew       = 0;
    mProfileClicksNew   = 0;

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
    LL_INFOS() << "Click-through data for classified " << classified_id << " arrived: ["
            << teleport << ", " << map << ", " << profile << "] ("
            << (from_new_table ? "new" : "old") << ")" << LL_ENDL;

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

        LL_INFOS() << "Updating classified info panel" << LL_ENDL;

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

        ct_str.setArg("[TELEPORT]", llformat("%d", self->mTeleportClicksNew + self->mTeleportClicksOld));
        ct_str.setArg("[MAP]",      llformat("%d", self->mMapClicksNew + self->mMapClicksOld));
        ct_str.setArg("[PROFILE]",  llformat("%d", self->mProfileClicksNew + self->mProfileClicksOld));

        self->getChild<LLUICtrl>("click_through_text")->setValue(ct_str.getString());
        // *HACK: remove this when there is enough room for click stats in the info panel
        self->getChildView("click_through_text")->setToolTip(ct_str.getString());  

        LL_INFOS() << "teleport: " << llformat("%d", self->mTeleportClicksNew + self->mTeleportClicksOld)
                << ", map: "    << llformat("%d", self->mMapClicksNew + self->mMapClicksOld)
                << ", profile: " << llformat("%d", self->mProfileClicksNew + self->mProfileClicksOld)
                << LL_ENDL;
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
        S32 region_x = ll_round((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
        S32 region_y = ll_round((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
        S32 region_z = ll_round((F32)pos_global.mdV[VZ]);
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
    if (gAgent.getRegion())
    {
        // You're allowed to click on your own ads to reassure yourself
        // that the system is working.
        LLSD body;
        body["type"]            = type;
        body["from_search"]     = from_search;
        body["classified_id"]   = classified_id;
        body["parcel_id"]       = parcel_id;
        body["dest_pos_global"] = global_pos.getValue();
        body["region_name"]     = sim_name;

        std::string url = gAgent.getRegion()->getCapability("SearchStatTracking");
        LL_INFOS() << "Sending click msg via capability (url=" << url << ")" << LL_ENDL;
        LL_INFOS() << "body: [" << body << "]" << LL_ENDL;
        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, body,
            "SearchStatTracking Click report sent.", "SearchStatTracking Click report NOT sent.");
    }
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

//EOF
