/**
* @file llinventorysettings.h
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

#ifndef LL_INVENTORY_SETTINGS_H
#define LL_INVENTORY_SETTINGS_H

#include "llinventorytype.h"
#include "llinvtranslationbrdg.h"
#include "llsingleton.h"

class LLSettingsType : public LLParamSingleton<LLSettingsType>
{
    LLSINGLETON(LLSettingsType, LLTranslationBridge::ptr_t &trans);
    ~LLSettingsType();

    friend struct SettingsEntry;

public:
    enum type_e
    {
        ST_SKY = 0,
        ST_WATER = 1,
        ST_DAYCYCLE = 2,

        ST_INVALID = 255,
        ST_NONE = -1
    };

    static type_e fromInventoryFlags(U32 flags);
    static LLInventoryType::EIconName getIconName(type_e type);
    static std::string getDefaultName(type_e type);

protected:

    LLTranslationBridge::ptr_t mTranslator;
};


#endif
