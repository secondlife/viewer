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

#include "llavatarnamecache.h"
#include "llfloateravatarpicker.h"
#include "llfiltereditor.h"
#include "llfloaterreg.h"
#include "llnamelistctrl.h"
#include "llmenugl.h"
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
        registrar.add("Settings.SetRendering", boost::bind(&LLFloaterAvatarRenderSettings::onCustomAction, mFloaterSettings, _2, mUUIDs.front()));
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
    mCommitCallbackRegistrar.add("Settings.AddNewEntry", boost::bind(&LLFloaterAvatarRenderSettings::onClickAdd, this, _2));
}

LLFloaterAvatarRenderSettings::~LLFloaterAvatarRenderSettings()
{
    delete mContextMenu;
    LLRenderMuteList::getInstance()->removeObserver(&sAvatarRenderMuteListObserver);
}

BOOL LLFloaterAvatarRenderSettings::postBuild()
{
    LLFloater::postBuild();
    mAvatarSettingsList = getChild<LLNameListCtrl>("render_settings_list");
    mAvatarSettingsList->setRightMouseDownCallback(boost::bind(&LLFloaterAvatarRenderSettings::onAvatarListRightClick, this, _1, _2, _3));
    this->setVisibleCallback(boost::bind(&LLFloaterAvatarRenderSettings::removePicker, this));
    getChild<LLFilterEditor>("people_filter_input")->setCommitCallback(boost::bind(&LLFloaterAvatarRenderSettings::onFilterEdit, this, _2));

	return TRUE;
}

void LLFloaterAvatarRenderSettings::removePicker()
{
    if(mPicker.get())
    {
        mPicker.get()->closeFloater();
    }
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
        if(!isHiddenRow(av_name.getCompleteName()))
        {
            item_params.columns.add().value(av_name.getCompleteName()).column("name");
            std::string setting = getString(iter->second == 1 ? "av_never_render" : "av_always_render");
            item_params.columns.add().value(setting).column("setting");
            mAvatarSettingsList->addNameItemRow(item_params);
        }
    }
}

void LLFloaterAvatarRenderSettings::onFilterEdit(const std::string& search_string)
{
    std::string filter_upper = search_string;
    LLStringUtil::toUpper(filter_upper);
    if (mNameFilter != filter_upper)
    {
        mNameFilter = filter_upper;
        mNeedsUpdate = true;
    }
}

bool LLFloaterAvatarRenderSettings::isHiddenRow(const std::string& av_name)
{
    if (mNameFilter.empty()) return false;
    std::string upper_name = av_name;
    LLStringUtil::toUpper(upper_name);
    return std::string::npos == upper_name.find(mNameFilter);
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


bool LLFloaterAvatarRenderSettings::isActionChecked(const LLSD& userdata, const LLUUID& av_id)
{
    const std::string command_name = userdata.asString();

    S32 visual_setting = LLRenderMuteList::getInstance()->getSavedVisualMuteSetting(av_id);
    if ("default" == command_name)
    {
        return (visual_setting == S32(LLVOAvatar::AV_RENDER_NORMALLY));
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

    LLView * button = findChild<LLButton>("plus_btn", TRUE);
    LLFloater* root_floater = gFloaterView->getParentFloater(this);
    LLFloaterAvatarPicker * picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterAvatarRenderSettings::callbackAvatarPicked, this, _1, visual_setting),
                                                                    FALSE, TRUE, FALSE, root_floater->getName(), button);

    if (root_floater)
    {
        root_floater->addDependentFloater(picker);
    }

    mPicker = picker->getHandle();
}

void LLFloaterAvatarRenderSettings::callbackAvatarPicked(const uuid_vec_t& ids, S32 visual_setting)
{
    if (ids.empty()) return;

    LLVOAvatar *avatarp = find_avatar(ids[0]);
    if (avatarp)
    {
        avatarp->setVisualMuteSettings(LLVOAvatar::VisualMuteSettings(visual_setting));
    }
    else
    {
        LLRenderMuteList::getInstance()->saveVisualMuteSetting(ids[0], visual_setting);
    }
}
