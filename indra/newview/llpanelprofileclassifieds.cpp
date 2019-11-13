/**
 * @file llpanelprofileclassifieds.cpp
 * @brief LLPanelProfileClassifieds and related class implementations
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

#include "llpanelprofileclassifieds.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llclassifiedflags.h"
#include "llcombobox.h"
#include "llcommandhandler.h" // for classified HTML detail page click tracking
#include "llcorehttputil.h"
#include "lldispatcher.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanelavatar.h"
#include "llparcel.h"
#include "llregistry.h"
#include "llscrollcontainer.h"
#include "llstatusbar.h"
#include "lltabcontainer.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewergenericmessage.h" // send_generic_message
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewertexture.h"
#include "llviewertexture.h"


//*TODO: verify this limit
const S32 MAX_AVATAR_CLASSIFIEDS = 100;

const S32 MINIMUM_PRICE_FOR_LISTING = 50; // L$
const S32 DEFAULT_EDIT_CLASSIFIED_SCROLL_HEIGHT = 530;

//static
LLPanelProfileClassified::panel_list_t LLPanelProfileClassified::sAllPanels;

static LLPanelInjector<LLPanelProfileClassifieds> t_panel_profile_classifieds("panel_profile_classifieds");
static LLPanelInjector<LLPanelProfileClassified> t_panel_profile_classified("panel_profile_classified");

class LLClassifiedHandler : public LLCommandHandler
{
public:
    // throttle calls from untrusted browsers
    LLClassifiedHandler() : LLCommandHandler("classified", UNTRUSTED_THROTTLE) {}

    bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
    {
        if (!LLUI::getInstance()->mSettingGroups["config"]->getBOOL("EnableClassifieds"))
        {
            LLNotificationsUtil::add("NoClassifieds", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
            return true;
        }

        // handle app/classified/create urls first
        if (params.size() == 1 && params[0].asString() == "create")
        {
            LLAvatarActions::showClassifieds(gAgent.getID());
            return true;
        }

        // then handle the general app/classified/{UUID}/{CMD} urls
        if (params.size() < 2)
        {
            return false;
        }

        // get the ID for the classified
        LLUUID classified_id;
        if (!classified_id.set(params[0], FALSE))
        {
            return false;
        }

        // show the classified in the side tray.
        // need to ask the server for more info first though...
        const std::string verb = params[1].asString();
        if (verb == "about")
        {
            LLAvatarActions::showClassified(gAgent.getID(), classified_id, false);
            return true;
        }
        else if (verb == "edit")
        {
            LLAvatarActions::showClassified(gAgent.getID(), classified_id, true);
            return true;
        }

        return false;
    }
};
LLClassifiedHandler gClassifiedHandler;

//////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// LLPanelProfileClassifieds
//-----------------------------------------------------------------------------

LLPanelProfileClassifieds::LLPanelProfileClassifieds()
 : LLPanelProfileTab()
 , mClassifiedToSelectOnLoad(LLUUID::null)
 , mClassifiedEditOnLoad(false)
{
}

LLPanelProfileClassifieds::~LLPanelProfileClassifieds()
{
}

void LLPanelProfileClassifieds::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    resetData();

    if (getSelfProfile() && !getEmbedded())
    {
        mNewButton->setVisible(TRUE);
        mNewButton->setEnabled(FALSE);

        mDeleteButton->setVisible(TRUE);
        mDeleteButton->setEnabled(FALSE);
    }
}

void LLPanelProfileClassifieds::selectClassified(const LLUUID& classified_id, bool edit)
{
    if (getIsLoaded())
    {
        for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
        {
            LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mTabContainer->getPanelByIndex(tab_idx));
            if (classified_panel)
            {
                if (classified_panel->getClassifiedId() == classified_id)
                {
                    mTabContainer->selectTabPanel(classified_panel);
                    if (edit)
                    {
                        classified_panel->setEditMode(TRUE);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        mClassifiedToSelectOnLoad = classified_id;
        mClassifiedEditOnLoad = edit;
    }
}

BOOL LLPanelProfileClassifieds::postBuild()
{
    mTabContainer = getChild<LLTabContainer>("tab_classifieds");
    mNoItemsLabel = getChild<LLUICtrl>("classifieds_panel_text");
    mNewButton = getChild<LLButton>("new_btn");
    mDeleteButton = getChild<LLButton>("delete_btn");

    mNewButton->setCommitCallback(boost::bind(&LLPanelProfileClassifieds::onClickNewBtn, this));
    mDeleteButton->setCommitCallback(boost::bind(&LLPanelProfileClassifieds::onClickDelete, this));

    return TRUE;
}

void LLPanelProfileClassifieds::onClickNewBtn()
{
    mNoItemsLabel->setVisible(FALSE);
    LLPanelProfileClassified* classified_panel = LLPanelProfileClassified::create();
    classified_panel->onOpen(LLSD());
    mTabContainer->addTabPanel(
        LLTabContainer::TabPanelParams().
        panel(classified_panel).
        select_tab(true).
        label(classified_panel->getClassifiedName()));
    updateButtons();
}

void LLPanelProfileClassifieds::onClickDelete()
{
    LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mTabContainer->getCurrentPanel());
    if (classified_panel)
    {
        LLUUID classified_id = classified_panel->getClassifiedId();
        LLSD args;
        args["PICK"] = classified_panel->getClassifiedName();
        LLSD payload;
        payload["classified_id"] = classified_id;
        payload["tab_idx"] = mTabContainer->getCurrentPanelIndex();
        LLNotificationsUtil::add("DeleteAvatarPick", args, payload,
            boost::bind(&LLPanelProfileClassifieds::callbackDeleteClassified, this, _1, _2));
    }
}

void LLPanelProfileClassifieds::callbackDeleteClassified(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

    if (0 == option)
    {
        LLUUID classified_id = notification["payload"]["classified_id"].asUUID();
        S32 tab_idx = notification["payload"]["tab_idx"].asInteger();

        LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mTabContainer->getPanelByIndex(tab_idx));
        if (classified_panel && classified_panel->getClassifiedId() == classified_id)
        {
            mTabContainer->removeTabPanel(classified_panel);
        }

        if (classified_id.notNull())
        {
            LLAvatarPropertiesProcessor::getInstance()->sendClassifiedDelete(classified_id);
        }

        updateButtons();
    }
}

void LLPanelProfileClassifieds::processProperties(void* data, EAvatarProcessorType type)
{
    if ((APT_CLASSIFIEDS == type) || (APT_CLASSIFIED_INFO == type))
    {
        LLUUID avatar_id = getAvatarId();

        LLAvatarClassifieds* c_info = static_cast<LLAvatarClassifieds*>(data);
        if (c_info && getAvatarId() == c_info->target_id)
        {
            // do not clear classified list in case we will receive two or more data packets.
            // list has been cleared in updateData(). (fix for EXT-6436)
            LLUUID selected_id = mClassifiedToSelectOnLoad;

            LLAvatarClassifieds::classifieds_list_t::const_iterator it = c_info->classifieds_list.begin();
            for (; c_info->classifieds_list.end() != it; ++it)
            {
                LLAvatarClassifieds::classified_data c_data = *it;

                LLPanelProfileClassified* classified_panel = LLPanelProfileClassified::create();

                LLSD params;
                params["classified_creator_id"] = avatar_id;
                params["classified_id"] = c_data.classified_id;
                params["classified_name"] = c_data.name;
                params["from_search"] = (selected_id == c_data.classified_id); //SLURL handling and stats tracking
                params["edit"] = (selected_id == c_data.classified_id) && mClassifiedEditOnLoad;
                classified_panel->onOpen(params);

                mTabContainer->addTabPanel(
                    LLTabContainer::TabPanelParams().
                    panel(classified_panel).
                    select_tab(selected_id == c_data.classified_id).
                    label(c_data.name));

                if (selected_id == c_data.classified_id)
                {
                    mClassifiedToSelectOnLoad = LLUUID::null;
                    mClassifiedEditOnLoad = false;
                }
            }

            BOOL no_data = !mTabContainer->getTabCount();
            mNoItemsLabel->setVisible(no_data);
            if (no_data)
            {
                if(getSelfProfile())
                {
                    mNoItemsLabel->setValue(LLTrans::getString("NoClassifiedsText"));
                }
                else
                {
                    mNoItemsLabel->setValue(LLTrans::getString("NoAvatarClassifiedsText"));
                }
            }
            else if (selected_id.isNull())
            {
                mTabContainer->selectFirstTab();
            }

            updateButtons();
        }
    }
}

void LLPanelProfileClassifieds::resetData()
{
    resetLoading();
    mTabContainer->deleteAllTabs();
}

void LLPanelProfileClassifieds::updateButtons()
{
    LLPanelProfileTab::updateButtons();

    if (getSelfProfile() && !getEmbedded())
    {
        mNewButton->setEnabled(canAddNewClassified());
        mDeleteButton->setEnabled(canDeleteClassified());
    }
}

void LLPanelProfileClassifieds::updateData()
{
    // Send picks request only once
    LLUUID avatar_id = getAvatarId();
    if (!getIsLoading() && avatar_id.notNull())
    {
        setIsLoading();
        mNoItemsLabel->setValue(LLTrans::getString("PicksClassifiedsLoadingText"));
        mNoItemsLabel->setVisible(TRUE);

        LLAvatarPropertiesProcessor::getInstance()->sendAvatarClassifiedsRequest(avatar_id);
    }
}

bool LLPanelProfileClassifieds::canAddNewClassified()
{
    return (mTabContainer->getTabCount() < MAX_AVATAR_CLASSIFIEDS);
}

bool LLPanelProfileClassifieds::canDeleteClassified()
{
    return (mTabContainer->getTabCount() > 0);
}

void LLPanelProfileClassifieds::apply()
{
    if (getIsLoaded())
    {
        for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
        {
            LLPanelProfileClassified* classified_panel = dynamic_cast<LLPanelProfileClassified*>(mTabContainer->getPanelByIndex(tab_idx));
            if (classified_panel && classified_panel->isDirty() && !classified_panel->isNew())
            {
                classified_panel->doSave();
            }
        }
    }
}
//-----------------------------------------------------------------------------
// LLDispatchClassifiedClickThrough
//-----------------------------------------------------------------------------

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

        LLPanelProfileClassified::setClickThrough(
            classified_id, teleport_clicks, map_clicks, profile_clicks, false);

        return true;
    }
};
static LLDispatchClassifiedClickThrough sClassifiedClickThrough;


//-----------------------------------------------------------------------------
// LLPanelProfileClassified
//-----------------------------------------------------------------------------

static const S32 CB_ITEM_MATURE = 0;
static const S32 CB_ITEM_PG    = 1;

LLPanelProfileClassified::LLPanelProfileClassified()
 : LLPanelProfileTab()
 , mInfoLoaded(false)
 , mTeleportClicksOld(0)
 , mMapClicksOld(0)
 , mProfileClicksOld(0)
 , mTeleportClicksNew(0)
 , mMapClicksNew(0)
 , mProfileClicksNew(0)
 , mPriceForListing(0)
 , mSnapshotCtrl(NULL)
 , mPublishFloater(NULL)
 , mIsNew(false)
 , mIsNewWithErrors(false)
 , mCanClose(false)
 , mEditMode(false)
 , mEditOnLoad(false)
{
    sAllPanels.push_back(this);
}

LLPanelProfileClassified::~LLPanelProfileClassified()
{
    sAllPanels.remove(this);
    gGenericDispatcher.addHandler("classifiedclickthrough", NULL); // deregister our handler
}

//static
LLPanelProfileClassified* LLPanelProfileClassified::create()
{
    LLPanelProfileClassified* panel = new LLPanelProfileClassified();
    panel->buildFromFile("panel_profile_classified.xml");
    return panel;
}

BOOL LLPanelProfileClassified::postBuild()
{
    mScrollContainer    = getChild<LLScrollContainer>("profile_scroll");
    mInfoPanel          = getChild<LLView>("info_panel");
    mInfoScroll         = getChild<LLPanel>("info_scroll_content_panel");
    mEditPanel          = getChild<LLPanel>("edit_panel");

    mSnapshotCtrl       = getChild<LLTextureCtrl>("classified_snapshot");
    mEditIcon           = getChild<LLUICtrl>("edit_icon");

    //info
    mClassifiedNameText = getChild<LLUICtrl>("classified_name");
    mClassifiedDescText = getChild<LLTextEditor>("classified_desc");
    mLocationText       = getChild<LLUICtrl>("classified_location");
    mCategoryText       = getChild<LLUICtrl>("category");
    mContentTypeText    = getChild<LLUICtrl>("content_type");
    mContentTypeM       = getChild<LLIconCtrl>("content_type_moderate");
    mContentTypeG       = getChild<LLIconCtrl>("content_type_general");
    mPriceText          = getChild<LLUICtrl>("price_for_listing");
    mAutoRenewText      = getChild<LLUICtrl>("auto_renew");

    mMapButton          = getChild<LLButton>("show_on_map_btn");
    mTeleportButton     = getChild<LLButton>("teleport_btn");
    mEditButton         = getChild<LLButton>("edit_btn");

    //edit
    mClassifiedNameEdit = getChild<LLLineEditor>("classified_name_edit");
    mClassifiedDescEdit = getChild<LLTextEditor>("classified_desc_edit");
    mLocationEdit       = getChild<LLUICtrl>("classified_location_edit");
    mCategoryCombo      = getChild<LLComboBox>("category_edit");
    mContentTypeCombo   = getChild<LLComboBox>("content_type_edit");
    mAutoRenewEdit      = getChild<LLUICtrl>("auto_renew_edit");

    mSaveButton         = getChild<LLButton>("save_changes_btn");
    mSetLocationButton  = getChild<LLButton>("set_to_curr_location_btn");
    mCancelButton       = getChild<LLButton>("cancel_btn");

    mTeleportBtnCnt = getChild<LLPanel>("teleport_btn_lp");
    mMapBtnCnt = getChild<LLPanel>("map_btn_lp");
    mEditBtnCnt = getChild<LLPanel>("edit_btn_lp");
    mCancelBtnCnt = getChild<LLPanel>("cancel_btn_lp");
    mSaveBtnCnt = getChild<LLPanel>("save_btn_lp");

    mSnapshotCtrl->setOnSelectCallback(boost::bind(&LLPanelProfileClassified::onTextureSelected, this));
    mSnapshotCtrl->setMouseEnterCallback(boost::bind(&LLPanelProfileClassified::onTexturePickerMouseEnter, this));
    mSnapshotCtrl->setMouseLeaveCallback(boost::bind(&LLPanelProfileClassified::onTexturePickerMouseLeave, this));
    mEditIcon->setVisible(false);

    mMapButton->setCommitCallback(boost::bind(&LLPanelProfileClassified::onMapClick, this));
    mTeleportButton->setCommitCallback(boost::bind(&LLPanelProfileClassified::onTeleportClick, this));
    mEditButton->setCommitCallback(boost::bind(&LLPanelProfileClassified::onEditClick, this));
    mSaveButton->setCommitCallback(boost::bind(&LLPanelProfileClassified::onSaveClick, this));
    mSetLocationButton->setCommitCallback(boost::bind(&LLPanelProfileClassified::onSetLocationClick, this));
    mCancelButton->setCommitCallback(boost::bind(&LLPanelProfileClassified::onCancelClick, this));

    LLClassifiedInfo::cat_map::iterator iter;
    for (iter = LLClassifiedInfo::sCategories.begin();
        iter != LLClassifiedInfo::sCategories.end();
        iter++)
    {
        mCategoryCombo->add(LLTrans::getString(iter->second));
    }

    mClassifiedNameEdit->setKeystrokeCallback(boost::bind(&LLPanelProfileClassified::onChange, this), NULL);
    mClassifiedDescEdit->setKeystrokeCallback(boost::bind(&LLPanelProfileClassified::onChange, this));
    mCategoryCombo->setCommitCallback(boost::bind(&LLPanelProfileClassified::onChange, this));
    mContentTypeCombo->setCommitCallback(boost::bind(&LLPanelProfileClassified::onChange, this));
    mAutoRenewEdit->setCommitCallback(boost::bind(&LLPanelProfileClassified::onChange, this));

    return TRUE;
}

void LLPanelProfileClassified::onOpen(const LLSD& key)
{
    mIsNew = key.isUndefined();

    resetData();
    resetControls();
    scrollToTop();

    // classified is not created yet
    bool is_new = isNew() || isNewWithErrors();

    if(is_new)
    {
        LLPanelProfileTab::setAvatarId(gAgent.getID());

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

        setClassifiedName(makeClassifiedName());
        setDescription(desc);
        setSnapshotId(snapshot_id);
        setClassifiedLocation(createLocationText(getLocationNotice(), region_name, getPosGlobal()));
        // server will set valid parcel id
        setParcelId(LLUUID::null);

        mSaveButton->setLabelArg("[LABEL]", getString("publish_label"));

        setEditMode(TRUE);
        enableSave(true);
        enableEditing(true);
        resetDirty();
        setInfoLoaded(false);
    }
    else
    {
        LLUUID avatar_id = key["classified_creator_id"];
        if(avatar_id.isNull())
        {
            return;
        }
        LLPanelProfileTab::setAvatarId(avatar_id);

        setClassifiedId(key["classified_id"]);
        setClassifiedName(key["classified_name"]);
        setFromSearch(key["from_search"]);
        mEditOnLoad = key["edit"];

        LL_INFOS() << "Opening classified [" << getClassifiedName() << "] (" << getClassifiedId() << ")" << LL_ENDL;

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
                    boost::bind(&LLPanelProfileClassified::handleSearchStatResponse, classifiedId, _1));
            }
        }
        // Update classified click stats.
        // *TODO: Should we do this when opening not from search?
        if (!fromSearch() )
        {
            sendClickMessage("profile");
        }

        setInfoLoaded(false);
    }


    bool is_self = getSelfProfile();
    getChildView("auto_renew_layout_panel")->setVisible(is_self);
    getChildView("clickthrough_layout_panel")->setVisible(is_self);

    updateButtons();
}

void LLPanelProfileClassified::processProperties(void* data, EAvatarProcessorType type)
{
    if (APT_CLASSIFIED_INFO != type)
    {
        return;
    }

    LLAvatarClassifiedInfo* c_info = static_cast<LLAvatarClassifiedInfo*>(data);
    if(c_info && getClassifiedId() == c_info->classified_id)
    {
        // see LLPanelProfileClassified::sendUpdate() for notes
        mIsNewWithErrors = false;

        setClassifiedName(c_info->name);
        setDescription(c_info->description);
        setSnapshotId(c_info->snapshot_id);
        setParcelId(c_info->parcel_id);
        setPosGlobal(c_info->pos_global);
        setSimName(c_info->sim_name);

        setClassifiedLocation(createLocationText(c_info->parcel_name, c_info->sim_name, c_info->pos_global));

        mCategoryText->setValue(LLClassifiedInfo::sCategories[c_info->category]);
        // *HACK see LLPanelProfileClassified::sendUpdate()
        setCategory(c_info->category - 1);

        bool mature = is_cf_mature(c_info->flags);
        setContentType(mature);

        bool auto_renew = is_cf_auto_renew(c_info->flags);
        std::string auto_renew_str = auto_renew ? getString("auto_renew_on") : getString("auto_renew_off");
        mAutoRenewText->setValue(auto_renew_str);
        mAutoRenewEdit->setValue(auto_renew);

        static LLUIString  price_str = getString("l$_price");
        price_str.setArg("[PRICE]", llformat("%d", c_info->price_for_listing));
        mPriceText->setValue(LLSD(price_str));

        static std::string date_fmt = getString("date_fmt");
        std::string date_str = date_fmt;
        LLStringUtil::format(date_str, LLSD().with("datetime", (S32) c_info->creation_date));
        getChild<LLUICtrl>("creation_date")->setValue(date_str);

        resetDirty();
        setInfoLoaded(true);
        enableSave(false);
        enableEditing(true);

        // for just created classified - in case user opened edit panel before processProperties() callback
        mSaveButton->setLabelArg("[LABEL]", getString("save_label"));

        updateButtons();

        if (mEditOnLoad)
        {
            setEditMode(TRUE);
        }
    }

}

void LLPanelProfileClassified::setEditMode(BOOL edit_mode)
{
    mEditMode = edit_mode;

    mInfoPanel->setVisible(!edit_mode);
    mEditPanel->setVisible(edit_mode);

    // snapshot control is common between info and edit,
    // enable it only when in edit mode
    mSnapshotCtrl->setEnabled(edit_mode);

    scrollToTop();
    updateButtons();
    updateInfoRect();
}

void LLPanelProfileClassified::updateButtons()
{
    bool edit_mode = getEditMode();
    mTeleportBtnCnt->setVisible(!edit_mode);
    mMapBtnCnt->setVisible(!edit_mode);
    mEditBtnCnt->setVisible(!edit_mode);
    mCancelBtnCnt->setVisible(edit_mode);
    mSaveBtnCnt->setVisible(edit_mode);
    mEditButton->setVisible(!edit_mode && getSelfProfile());
}

void LLPanelProfileClassified::updateInfoRect()
{
    if (getEditMode())
    {
        // info_scroll_content_panel contains both info and edit panel
        // info panel can be very large and scroll bar will carry over.
        // Resize info panel to prevent scroll carry over when in edit mode.
        mInfoScroll->reshape(mInfoScroll->getRect().getWidth(), DEFAULT_EDIT_CLASSIFIED_SCROLL_HEIGHT, FALSE);
    }
    else
    {
        // Adjust text height to make description scrollable.
        S32 new_height = mClassifiedDescText->getTextBoundingRect().getHeight();
        LLRect visible_rect = mClassifiedDescText->getVisibleDocumentRect();
        S32 delta_height = new_height - visible_rect.getHeight() + 5;

        LLRect rect = mInfoScroll->getRect();
        mInfoScroll->reshape(rect.getWidth(), rect.getHeight() + delta_height, FALSE);
    }
}

void LLPanelProfileClassified::enableEditing(bool enable)
{
    mEditButton->setEnabled(enable);
    mClassifiedNameEdit->setEnabled(enable);
    mClassifiedDescEdit->setEnabled(enable);
    mSetLocationButton->setEnabled(enable);
    mCategoryCombo->setEnabled(enable);
    mContentTypeCombo->setEnabled(enable);
    mAutoRenewEdit->setEnabled(enable);
}

void LLPanelProfileClassified::resetControls()
{
    updateButtons();

    mCategoryCombo->setCurrentByIndex(0);
    mContentTypeCombo->setCurrentByIndex(0);
    mAutoRenewEdit->setValue(false);
    mPriceForListing = MINIMUM_PRICE_FOR_LISTING;
}

void LLPanelProfileClassified::onEditClick()
{
    setEditMode(TRUE);
}

void LLPanelProfileClassified::onCancelClick()
{
    if (isNew())
    {
        mClassifiedNameEdit->setValue(mClassifiedNameText->getValue());
        mClassifiedDescEdit->setValue(mClassifiedDescText->getValue());
        mLocationEdit->setValue(mLocationText->getValue());
        mCategoryCombo->setCurrentByIndex(0);
        mContentTypeCombo->setCurrentByIndex(0);
        mAutoRenewEdit->setValue(false);
        mPriceForListing = MINIMUM_PRICE_FOR_LISTING;
    }
    else
    {
        // Reload data to undo changes to forms
        LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoRequest(getClassifiedId());
    }

    setInfoLoaded(false);

    setEditMode(FALSE);
}

void LLPanelProfileClassified::onSaveClick()
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
                (&LLPanelProfileClassified::onPublishFloaterPublishClicked, this));
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

/*static*/
void LLPanelProfileClassified::handleSearchStatResponse(LLUUID classifiedId, LLSD result)
{
    S32 teleport = result["teleport_clicks"].asInteger();
    S32 map = result["map_clicks"].asInteger();
    S32 profile = result["profile_clicks"].asInteger();
    S32 search_teleport = result["search_teleport_clicks"].asInteger();
    S32 search_map = result["search_map_clicks"].asInteger();
    S32 search_profile = result["search_profile_clicks"].asInteger();

    LLPanelProfileClassified::setClickThrough(classifiedId,
        teleport + search_teleport,
        map + search_map,
        profile + search_profile,
        true);
}

void LLPanelProfileClassified::resetData()
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

    mPriceForListing = MINIMUM_PRICE_FOR_LISTING;

    mCategoryText->setValue(LLStringUtil::null);
    mContentTypeText->setValue(LLStringUtil::null);
    getChild<LLUICtrl>("click_through_text")->setValue(LLStringUtil::null);
    mEditButton->setValue(LLStringUtil::null);
    getChild<LLUICtrl>("creation_date")->setValue(LLStringUtil::null);
    mContentTypeM->setVisible(FALSE);
    mContentTypeG->setVisible(FALSE);
}

void LLPanelProfileClassified::setClassifiedName(const std::string& name)
{
    mClassifiedNameText->setValue(name);
    mClassifiedNameEdit->setValue(name);
}

std::string LLPanelProfileClassified::getClassifiedName()
{
    return mClassifiedNameEdit->getValue().asString();
}

void LLPanelProfileClassified::setDescription(const std::string& desc)
{
    mClassifiedDescText->setValue(desc);
    mClassifiedDescEdit->setValue(desc);

    updateInfoRect();
}

std::string LLPanelProfileClassified::getDescription()
{
    return mClassifiedDescEdit->getValue().asString();
}

void LLPanelProfileClassified::setClassifiedLocation(const std::string& location)
{
    mLocationText->setValue(location);
    mLocationEdit->setValue(location);
}

std::string LLPanelProfileClassified::getClassifiedLocation()
{
    return mLocationText->getValue().asString();
}

void LLPanelProfileClassified::setSnapshotId(const LLUUID& id)
{
    mSnapshotCtrl->setValue(id);
}

LLUUID LLPanelProfileClassified::getSnapshotId()
{
    return mSnapshotCtrl->getValue().asUUID();
}

// static
void LLPanelProfileClassified::setClickThrough(
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
        LLPanelProfileClassified* self = *iter;
        if (self->getClassifiedId() != classified_id)
        {
            continue;
        }

        // *HACK: Skip LLPanelProfileClassified instances: they don't display clicks data.
        // Those instances should not be in the list at all.
        if (typeid(*self) != typeid(LLPanelProfileClassified))
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
std::string LLPanelProfileClassified::createLocationText(
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

void LLPanelProfileClassified::scrollToTop()
{
    if (mScrollContainer)
    {
        mScrollContainer->goToTop();
    }
}

//info
// static
// *TODO: move out of the panel
void LLPanelProfileClassified::sendClickMessage(
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

void LLPanelProfileClassified::sendClickMessage(const std::string& type)
{
    sendClickMessage(
        type,
        fromSearch(),
        getClassifiedId(),
        getParcelId(),
        getPosGlobal(),
        getSimName());
}

void LLPanelProfileClassified::onMapClick()
{
    sendClickMessage("map");
    LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
    LLFloaterReg::showInstance("world_map", "center");
}

void LLPanelProfileClassified::onTeleportClick()
{
    if (!getPosGlobal().isExactlyZero())
    {
        sendClickMessage("teleport");
        gAgent.teleportViaLocation(getPosGlobal());
        LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
    }
}

BOOL LLPanelProfileClassified::isDirty() const
{
    if(mIsNew)
    {
        return TRUE;
    }

    BOOL dirty = false;
    dirty |= mSnapshotCtrl->isDirty();
    dirty |= mClassifiedNameEdit->isDirty();
    dirty |= mClassifiedDescEdit->isDirty();
    dirty |= mCategoryCombo->isDirty();
    dirty |= mContentTypeCombo->isDirty();
    dirty |= mAutoRenewEdit->isDirty();

    return dirty;
}

void LLPanelProfileClassified::resetDirty()
{
    mSnapshotCtrl->resetDirty();
    mClassifiedNameEdit->resetDirty();

    // call blockUndo() to really reset dirty(and make isDirty work as intended)
    mClassifiedDescEdit->blockUndo();
    mClassifiedDescEdit->resetDirty();

    mCategoryCombo->resetDirty();
    mContentTypeCombo->resetDirty();
    mAutoRenewEdit->resetDirty();
}

bool LLPanelProfileClassified::canClose()
{
    return mCanClose;
}

U32 LLPanelProfileClassified::getContentType()
{
    return mContentTypeCombo->getCurrentIndex();
}

void LLPanelProfileClassified::setContentType(bool mature)
{
    static std::string mature_str = getString("type_mature");
    static std::string pg_str = getString("type_pg");
    mContentTypeText->setValue(mature ? mature_str : pg_str);
    mContentTypeM->setVisible(mature);
    mContentTypeG->setVisible(!mature);
    mContentTypeCombo->setCurrentByIndex(mature ? CB_ITEM_MATURE : CB_ITEM_PG);
    mContentTypeCombo->resetDirty();
}

bool LLPanelProfileClassified::getAutoRenew()
{
    return mAutoRenewEdit->getValue().asBoolean();
}

void LLPanelProfileClassified::sendUpdate()
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

U32 LLPanelProfileClassified::getCategory()
{
    return mCategoryCombo->getCurrentIndex();
}

void LLPanelProfileClassified::setCategory(U32 category)
{
    mCategoryCombo->setCurrentByIndex(category);
    mCategoryCombo->resetDirty();
}

U8 LLPanelProfileClassified::getFlags()
{
    bool auto_renew = mAutoRenewEdit->getValue().asBoolean();

    bool mature = mContentTypeCombo->getCurrentIndex() == CB_ITEM_MATURE;

    return pack_classified_flags_request(auto_renew, false, mature, false);
}

void LLPanelProfileClassified::enableSave(bool enable)
{
    mSaveButton->setEnabled(enable);
}

std::string LLPanelProfileClassified::makeClassifiedName()
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

void LLPanelProfileClassified::onSetLocationClick()
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

void LLPanelProfileClassified::onChange()
{
    enableSave(isDirty());
}

void LLPanelProfileClassified::doSave()
{
    //*TODO: Fix all of this

    mCanClose = true;
    sendUpdate();
    updateTabLabel(getClassifiedName());
    resetDirty();

    if (!canClose())
    {
        return;
    }

    if (!isNew() && !isNewWithErrors())
    {
        setEditMode(FALSE);
        return;
    }

    updateButtons();
}

void LLPanelProfileClassified::onPublishFloaterPublishClicked()
{
    setPriceForListing(mPublishFloater->getPrice());

    doSave();
}

std::string LLPanelProfileClassified::getLocationNotice()
{
    static std::string location_notice = getString("location_notice");
    return location_notice;
}

bool LLPanelProfileClassified::isValidName()
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

void LLPanelProfileClassified::notifyInvalidName()
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

void LLPanelProfileClassified::onTexturePickerMouseEnter()
{
    mEditIcon->setVisible(TRUE);
}

void LLPanelProfileClassified::onTexturePickerMouseLeave()
{
    mEditIcon->setVisible(FALSE);
}

void LLPanelProfileClassified::onTextureSelected()
{
    setSnapshotId(mSnapshotCtrl->getValue().asUUID());
    onChange();
}

void LLPanelProfileClassified::updateTabLabel(const std::string& title)
{
    setLabel(title);
    LLTabContainer* parent = dynamic_cast<LLTabContainer*>(getParent());
    if (parent)
    {
        parent->setCurrentTabName(title);
    }
}


//-----------------------------------------------------------------------------
// LLPublishClassifiedFloater
//-----------------------------------------------------------------------------

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
