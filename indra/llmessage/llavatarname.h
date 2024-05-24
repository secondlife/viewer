/**
 * @file llavatarname.h
 * @brief Represents name-related data for an avatar, such as the
 * username/SLID ("bobsmith123" or "james.linden") and the display
 * name ("James Cook")
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#ifndef LLAVATARNAME_H
#define LLAVATARNAME_H

#include <string>

class LLSD;

class LL_COMMON_API LLAvatarName
{
public:
    LLAvatarName();

    bool operator<(const LLAvatarName& rhs) const;

    // Conversion to and from LLSD (cache file or server response)
    LLSD asLLSD() const;
    void fromLLSD(const LLSD& sd);

    // Used only in legacy mode when the display name capability is not provided server side
    // or to otherwise create a temporary valid item.
    void fromString(const std::string& full_name);

    // Set the name object to become invalid in "expires" seconds from now
    void setExpires(F64 expires);

    // Set and get the display name flag set by the user in preferences.
    static void setUseDisplayNames(bool use);
    static bool useDisplayNames();

    static void setUseUsernames(bool use);
    static bool useUsernames();

    // A name object is valid if not temporary and not yet expired (default is expiration not checked)
    bool isValidName(F64 max_unrefreshed = 0.0f) const { return !mIsTemporaryName && (mExpires >= max_unrefreshed); }

    // Return true if the name is made up from legacy or temporary data
    bool isDisplayNameDefault() const { return mIsDisplayNameDefault; }

    // For normal names, returns "James Linden (james.linden)"
    // When display names are disabled returns just "James Linden"
    std::string getCompleteName(bool use_parentheses = true, bool force_use_complete_name = false) const;

    // Returns "James Linden" or "bobsmith123 Resident" for backwards
    // compatibility with systems like voice and muting
    // *TODO: Eliminate this in favor of username only
    std::string getLegacyName() const;

    // "José Sanchez" or "James Linden", UTF-8 encoded Unicode
    // Takes the display name preference into account. This is truly the name that should
    // be used for all UI where an avatar name has to be used unless we truly want something else (rare)
    std::string getDisplayName(bool force_use_display_name = false) const;

    // Returns "James Linden" or "bobsmith123 Resident"
    // Used where we explicitely prefer or need a non UTF-8 legacy (ASCII) name
    // Also used for backwards compatibility with systems like voice and muting
    std::string getUserName(bool lowercase = false) const;

    // Returns "james.linden" or the legacy name for very old names
    std::string getAccountName() const { return mUsername; }

    // Debug print of the object
    void dump() const;

    // Names can change, so need to keep track of when name was
    // last checked.
    // Unix time-from-epoch seconds for efficiency
    F64 mExpires;

    // You can only change your name every N hours, so record
    // when the next update is allowed
    // Unix time-from-epoch seconds
    F64 mNextUpdate;

private:
    // "bobsmith123" or "james.linden", US-ASCII only
    std::string mUsername;

    // "José Sanchez" or "James Linden", UTF-8 encoded Unicode
    // Contains data whether or not user has explicitly set
    // a display name; may duplicate their username.
    std::string mDisplayName;

    // For "James Linden", "James"
    // For "bobsmith123", "bobsmith123"
    // Used to communicate with legacy systems like voice and muting which
    // rely on old-style names.
    // *TODO: Eliminate this in favor of username only
    std::string mLegacyFirstName;

    // For "James Linden", "Linden"
    // For "bobsmith123", "Resident"
    // see above for rationale
    std::string mLegacyLastName;

    // If true, both display name and SLID were generated from
    // a legacy first and last name, like "James Linden (james.linden)"
    bool mIsDisplayNameDefault;

    // Under error conditions, we may insert "dummy" records with
    // names like "???" into caches as placeholders.  These can be
    // shown in UI, but are not serialized.
    bool mIsTemporaryName;

    // Global flag indicating if display name should be used or not
    // This will affect the output of the high level "get" methods
    static bool sUseDisplayNames;

    // Flag indicating if username should be shown after display name or not
    static bool sUseUsernames;
};

#endif
