/**
* @file llinventorysettings.cpp
* @author optional
* @brief A base class for asset based settings groups.
*
* $LicenseInfo:2011&license=viewerlgpl$
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

#include "linden_common.h"
#include "llinventorysettings.h"
#include "llinventorytype.h"
#include "llinventorydefines.h"
#include "lldictionary.h"
#include "llsingleton.h"
#include "llinvtranslationbrdg.h"


//=========================================================================
struct SettingsEntry : public LLDictionaryEntry
{
    SettingsEntry(const std::string &name,
        const std::string& default_new_name,
        LLInventoryType::EIconName iconName) :
        LLDictionaryEntry(name),
        mDefaultNewName(default_new_name),
        mLabel(name),
        mIconName(iconName)
    {
        std::string transdname = LLSettingsType::getInstance()->mTranslator->getString(mLabel);
        if (!transdname.empty())
        {
            mLabel = transdname;
        }
    }

    std::string mLabel;
    std::string mDefaultNewName; //keep mLabel for backward compatibility
    LLInventoryType::EIconName mIconName;
};

class LLSettingsDictionary : public LLSingleton<LLSettingsDictionary>,
    public LLDictionary<LLSettingsType::type_e, SettingsEntry>
{
    LLSINGLETON(LLSettingsDictionary);

    void initSingleton();
};

LLSettingsDictionary::LLSettingsDictionary() 
{
}

void LLSettingsDictionary::initSingleton()
{
    addEntry(LLSettingsType::ST_SKY,        new SettingsEntry("sky",        "New Sky",      LLInventoryType::ICONNAME_SETTINGS_SKY));
    addEntry(LLSettingsType::ST_WATER,      new SettingsEntry("water",      "New Water",    LLInventoryType::ICONNAME_SETTINGS_WATER));
    addEntry(LLSettingsType::ST_DAYCYCLE,   new SettingsEntry("day",        "New Day",      LLInventoryType::ICONNAME_SETTINGS_DAY));
    addEntry(LLSettingsType::ST_NONE,       new SettingsEntry("none",       "New Settings", LLInventoryType::ICONNAME_SETTINGS));
    addEntry(LLSettingsType::ST_INVALID,    new SettingsEntry("invalid",    "New Settings", LLInventoryType::ICONNAME_SETTINGS));
}

//=========================================================================

LLSettingsType::LLSettingsType(LLTranslationBridge::ptr_t &trans)
{
    mTranslator = trans;
}

LLSettingsType::~LLSettingsType()
{
    mTranslator.reset();
}

LLSettingsType::type_e LLSettingsType::fromInventoryFlags(U32 flags)
{
    return  (LLSettingsType::type_e)(flags & LLInventoryItemFlags::II_FLAGS_SUBTYPE_MASK);
}

LLInventoryType::EIconName LLSettingsType::getIconName(LLSettingsType::type_e type)
{
    const SettingsEntry *entry = LLSettingsDictionary::instance().lookup(type);
    if (!entry) 
        return getIconName(ST_INVALID);
    return entry->mIconName;
}

std::string LLSettingsType::getDefaultName(LLSettingsType::type_e type)
{
    const SettingsEntry *entry = LLSettingsDictionary::instance().lookup(type);
    if (!entry)
        return getDefaultName(ST_INVALID);
    return entry->mDefaultNewName;
}
