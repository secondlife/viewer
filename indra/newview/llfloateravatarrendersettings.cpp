/**
 * @file llfloateravatarrendersettings.cpp
 * @brief Shows the list of avatars with non-default rendering settings
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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

#include "llfloateravatarrendersettings.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloateravatarpicker.h"
#include "llfiltereditor.h"
#include "llfloaterreg.h"
#include "llnamelistctrl.h"
#include "llnotificationsutil.h"
#include "llmenugl.h"
#include "lltrans.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"

class LLSettingsContextMenu : public LLListContextMenu

{
public:
    LLSettingsContextMenu(LLFloaterAvatarRenderSettings* floater_settings)
        :   mFloaterSettings(floater_settings)
    {}
protected:
    LLContextMenu* createMenu()
    {
        LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
        LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
        registrar.add("Settings.SetRendering", { boost::bind(&LLFloaterAvatarRenderSettings::onCustomAction, mFloaterSettings, _2, mUUIDs.front()) });
        enable_registrar.add("Settings.IsSelected", boost::bind(&LLFloaterAvatarRenderSettings::isActionChecked, mFloaterSettings, _2, mUUIDs.front()));
        LLContextMenu* menu = createFromFile("menu_avatar_rendering_settings.xml");

        return menu;
    }

    LLFloaterAvatarRenderSettings* mFloaterSettings;
};

class LLAvatarRenderMuteListObserver : public LLMuteListObserver
{
    /* virtual */ void onChange()  { LLFloaterAvatarRenderSettings::setNeedsUpdate();}
};

static LLAvatarRenderMuteListObserver sAvatarRenderMuteListObserver;

LLFloaterAvatarRenderSettings::LLFloaterAvatarRenderSettings(const LLSD& key)
:   LLFloater(key),
    mAvatarSettingsList(NULL),
    mNeedsUpdate(false)
{
    mContextMenu = new LLSettingsContextMenu(this);
    LLRenderMuteList::getInstance()->addObserver(&sAvatarRenderMuteListObserver);
    mCommitCallbackRegistrar.add("Settings.AddNewEntry", {boost::bind(&LLFloaterAvatarRenderSettings::onClickAdd, this, _2)});
}

LLFloaterAvatarRenderSettings::~LLFloaterAvatarRenderSettings()
{
    delete mContextMenu;
    LLRenderMuteList::getInstance()->removeObserver(&sAvatarRenderMuteListObserver);
}

bool LLFloaterAvatarRenderSettings::postBuild()
{
    LLFloater::postBuild();
    mAvatarSettingsList = getChild<LLNameListCtrl>("render_settings_list");
    mAvatarSettingsList->setRightMouseDownCallback(boost::bind(&LLFloaterAvatarRenderSettings::onAvatarListRightClick, this, _1, _2, _3));

    return true;
}

void LLFloaterAvatarRenderSettings::draw()
{
    if(mNeedsUpdate)
    {
        updateList();
        mNeedsUpdate = false;
    }

    LLFloater::draw();
}

void LLFloaterAvatarRenderSettings::onAvatarListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
    LLNameListCtrl* list = dynamic_cast<LLNameListCtrl*>(ctrl);
    if (!list) return;
    list->selectItemAt(x, y, MASK_NONE);
    uuid_vec_t selected_uuids;

    if(list->getCurrentID().notNull())
    {
        selected_uuids.push_back(list->getCurrentID());
        mContextMenu->show(ctrl, selected_uuids, x, y);
    }
}

void LLFloaterAvatarRenderSettings::onOpen(const LLSD& key)
{
    updateList();
}

void LLFloaterAvatarRenderSettings::updateList()
{
    mAvatarSettingsList->deleteAllItems();
    LLAvatarName av_name;
    LLNameListCtrl::NameItem item_params;
    for (std::map<LLUUID, S32>::iterator iter = LLRenderMuteList::getInstance()->sVisuallyMuteSettingsMap.begin(); iter != LLRenderMuteList::getInstance()->sVisuallyMuteSettingsMap.end(); iter++)
    {
        item_params.value = iter->first;
        LLAvatarNameCache::get(iter->first, &av_name);
        item_params.columns.add().value(av_name.getCompleteName()).column("name");
        std::string setting = getString(iter->second == 1 ? "av_never_render" : "av_always_render");
        item_params.columns.add().value(setting).column("setting");
        mAvatarSettingsList->addNameItemRow(item_params);
    }
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
    LLViewerObject *obj = gObjectList.findObject(id);
    while (obj && obj->isAttachment())
    {
        obj = (LLViewerObject *)obj->getParent();
    }

    if (obj && obj->isAvatar())
    {
        return (LLVOAvatar*)obj;
    }
    else
    {
        return NULL;
    }
}


void LLFloaterAvatarRenderSettings::onCustomAction (const LLSD& userdata, const LLUUID& av_id)
{
    const std::string command_name = userdata.asString();

    S32 new_setting = 0;
    if ("default" == command_name)
    {
        new_setting = S32(LLVOAvatar::AV_RENDER_NORMALLY);
    }
    else if ("never" == command_name)
    {
        new_setting = S32(LLVOAvatar::AV_DO_NOT_RENDER);
    }
    else if ("always" == command_name)
    {
        new_setting = S32(LLVOAvatar::AV_ALWAYS_RENDER);
    }

    setAvatarRenderSetting(av_id, new_setting);
}


bool LLFloaterAvatarRenderSettings::isActionChecked(const LLSD& userdata, const LLUUID& av_id)
{
    const std::string command_name = userdata.asString();

    S32 visual_setting = LLRenderMuteList::getInstance()->getSavedVisualMuteSetting(av_id);
    if ("default" == command_name)
    {
        return (visual_setting == S32(LLVOAvatar::AV_RENDER_NORMALLY));
    }
    else if ("non_default" == command_name)
    {
        return (visual_setting != S32(LLVOAvatar::AV_RENDER_NORMALLY));
    }
    else if ("never" == command_name)
    {
        return (visual_setting == S32(LLVOAvatar::AV_DO_NOT_RENDER));
    }
    else if ("always" == command_name)
    {
        return (visual_setting == S32(LLVOAvatar::AV_ALWAYS_RENDER));
    }
    return false;
}

void LLFloaterAvatarRenderSettings::setNeedsUpdate()
{
    LLFloaterAvatarRenderSettings* instance = LLFloaterReg::getTypedInstance<LLFloaterAvatarRenderSettings>("avatar_render_settings");
    if(!instance) return;
    instance->mNeedsUpdate = true;
}

void LLFloaterAvatarRenderSettings::onClickAdd(const LLSD& userdata)
{
    const std::string command_name = userdata.asString();
    S32 visual_setting = 0;
    if ("never" == command_name)
    {
        visual_setting = S32(LLVOAvatar::AV_DO_NOT_RENDER);
    }
    else if ("always" == command_name)
    {
        visual_setting = S32(LLVOAvatar::AV_ALWAYS_RENDER);
    }

    LLView * button = findChild<LLButton>("plus_btn", true);
    LLFloater* root_floater = gFloaterView->getParentFloater(this);
    LLFloaterAvatarPicker * picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterAvatarRenderSettings::callbackAvatarPicked, this, _1, visual_setting),
                                                                    false, true, false, root_floater->getName(), button);

    if (root_floater)
    {
        root_floater->addDependentFloater(picker);
    }
}

void LLFloaterAvatarRenderSettings::callbackAvatarPicked(const uuid_vec_t& ids, S32 visual_setting)
{
    if (ids.empty()) return;
    if(ids[0] == gAgentID)
    {
        LLNotificationsUtil::add("AddSelfRenderExceptions");
        return;
    }
    setAvatarRenderSetting(ids[0], visual_setting);
}

void LLFloaterAvatarRenderSettings::setAvatarRenderSetting(const LLUUID& av_id, S32 new_setting)
{
    LLVOAvatar *avatarp = find_avatar(av_id);
    if (avatarp)
    {
        avatarp->setVisualMuteSettings(LLVOAvatar::VisualMuteSettings(new_setting));
    }
    else
    {
        LLRenderMuteList::getInstance()->saveVisualMuteSetting(av_id, new_setting);
    }
}

bool LLFloaterAvatarRenderSettings::handleKeyHere(KEY key, MASK mask )
{
    bool handled = false;

    if (KEY_DELETE == key)
    {
        setAvatarRenderSetting(mAvatarSettingsList->getCurrentID(), (S32)LLVOAvatar::AV_RENDER_NORMALLY);
        handled = true;
    }
    return handled;
}

std::string LLFloaterAvatarRenderSettings::createTimestamp(S32 datetime)
{
    std::string timeStr;
    LLSD substitution;
    substitution["datetime"] = datetime;

    timeStr = "["+LLTrans::getString ("TimeMonth")+"]/["
                 +LLTrans::getString ("TimeDay")+"]/["
                 +LLTrans::getString ("TimeYear")+"]";

    LLStringUtil::format (timeStr, substitution);
    return timeStr;
}
