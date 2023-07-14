/** 
 * @file llkeybindparsing.h
 * @brief Structs needed for parsing saved keybindings.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#ifndef LL_KEYBINDPARSING_H
#define LL_KEYBINDPARSING_H

#include "indra_constants.h"

#include "llinitparam.h"

namespace LLKeyBindParsing
{
    struct KeyBinding: public LLInitParam::Block<KeyBinding>
    {
        Mandatory<std::string>	key,
            mask,
            command;
        Optional<std::string>	mouse; // Note, not mandatory for the sake of backward campatibility with keys.xml

        KeyBinding();
    };

    struct KeyMode: public LLInitParam::Block<KeyMode>
    {
        Multiple<KeyBinding>		bindings;

        KeyMode();
    };

    struct Keys: public LLInitParam::Block<Keys>
    {
        Optional<KeyMode>	first_person,
            third_person,
            sitting,
            edit_avatar;
        Optional<S32> xml_version; // 'xml', because 'version' appears to be reserved
        Keys();
    };

    bool modeFromString(const std::string& string, S32* mode);
}

#endif // LL_KEYBINDPARSING_H
