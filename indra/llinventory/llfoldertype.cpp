/**
 * @file llfoldertype.cpp
 * @brief Implementatino of LLFolderType functionality.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llfoldertype.h"
#include "lldictionary.h"
#include "llmemory.h"
#include "llsd.h"
#include "llsingleton.h"

///----------------------------------------------------------------------------
/// Class LLFolderType
///----------------------------------------------------------------------------
struct FolderEntry : public LLDictionaryEntry
{
    FolderEntry(const std::string &type_name, // 8 character limit!
                bool is_protected, // can the viewer change categories of this type?
                bool is_automatic, // always made before first login?
                bool is_singleton  // should exist as a unique copy under root
        )
        :
    LLDictionaryEntry(type_name),
    mIsProtected(is_protected),
    mIsAutomatic(is_automatic),
    mIsSingleton(is_singleton)
    {
        llassert(type_name.length() <= 8);
    }

    const bool mIsProtected;
    const bool mIsAutomatic;
    const bool mIsSingleton;
};

class LLFolderDictionary : public LLSingleton<LLFolderDictionary>,
                           public LLDictionary<LLFolderType::EType, FolderEntry>
{
    LLSINGLETON(LLFolderDictionary);
protected:
    virtual LLFolderType::EType notFound() const override
    {
        return LLFolderType::FT_NONE;
    }
};

// Folder types
//
// PROTECTED means that folders of this type can't be moved, deleted
// or otherwise modified by the viewer.
//
// SINGLETON means that there should always be exactly one folder of
// this type, and it should be the root or a child of the root. This
// is true for most types of folders.
//
// AUTOMATIC means that a copy of this folder should be created under
// the root before the user ever logs in, and should never be created
// from the viewer. A missing AUTOMATIC folder should be treated as a
// fatal error by the viewer, since it indicates either corrupted
// inventory or a failure in the inventory services.
//
LLFolderDictionary::LLFolderDictionary()
{
    //                                                              TYPE NAME, PROTECTED, AUTOMATIC, SINGLETON
    addEntry(LLFolderType::FT_TEXTURE,              new FolderEntry("texture",  true, true, true));
    addEntry(LLFolderType::FT_SOUND,                new FolderEntry("sound",    true, true, true));
    addEntry(LLFolderType::FT_CALLINGCARD,          new FolderEntry("callcard", true, true, false));
    addEntry(LLFolderType::FT_LANDMARK,             new FolderEntry("landmark", true, false, false));
    addEntry(LLFolderType::FT_CLOTHING,             new FolderEntry("clothing", true, true, true));
    addEntry(LLFolderType::FT_OBJECT,               new FolderEntry("object",   true, true, true));
    addEntry(LLFolderType::FT_NOTECARD,             new FolderEntry("notecard", true, true, true));
    addEntry(LLFolderType::FT_ROOT_INVENTORY,       new FolderEntry("root_inv", true, true, true));
    addEntry(LLFolderType::FT_LSL_TEXT,             new FolderEntry("lsltext",  true, true, true));
    addEntry(LLFolderType::FT_BODYPART,             new FolderEntry("bodypart", true, true, true));
    addEntry(LLFolderType::FT_TRASH,                new FolderEntry("trash",    true, false, true));
    addEntry(LLFolderType::FT_SNAPSHOT_CATEGORY,    new FolderEntry("snapshot", true, true, true));
    addEntry(LLFolderType::FT_LOST_AND_FOUND,       new FolderEntry("lstndfnd", true, true, true));
    addEntry(LLFolderType::FT_ANIMATION,            new FolderEntry("animatn",  true, true, true));
    addEntry(LLFolderType::FT_GESTURE,              new FolderEntry("gesture",  true, true, true));
    addEntry(LLFolderType::FT_FAVORITE,             new FolderEntry("favorite", true, false, true));

    for (S32 ensemble_num = S32(LLFolderType::FT_ENSEMBLE_START); ensemble_num <= S32(LLFolderType::FT_ENSEMBLE_END); ensemble_num++)
    {
        addEntry(LLFolderType::EType(ensemble_num), new FolderEntry("ensemble", false, false, false)); // Not used
    }

    addEntry(LLFolderType::FT_CURRENT_OUTFIT,       new FolderEntry("current",  true, false, true));
    addEntry(LLFolderType::FT_OUTFIT,               new FolderEntry("outfit",   false, false, false));
    addEntry(LLFolderType::FT_MY_OUTFITS,           new FolderEntry("my_otfts", true, false, true));

    addEntry(LLFolderType::FT_MESH,                 new FolderEntry("mesh",     true, false, false)); // Not used?

    addEntry(LLFolderType::FT_INBOX,                new FolderEntry("inbox",    true, false, true));
    addEntry(LLFolderType::FT_OUTBOX,               new FolderEntry("outbox",   true, false, false));

    addEntry(LLFolderType::FT_BASIC_ROOT,           new FolderEntry("basic_rt", true, false, false));

    addEntry(LLFolderType::FT_MARKETPLACE_LISTINGS, new FolderEntry("merchant", false, false, false));
    addEntry(LLFolderType::FT_MARKETPLACE_STOCK,    new FolderEntry("stock",    false, false, false));
    addEntry(LLFolderType::FT_MARKETPLACE_VERSION,  new FolderEntry("version",  false, false, false));

    addEntry(LLFolderType::FT_SETTINGS,             new FolderEntry("settings", true, false, true));
    addEntry(LLFolderType::FT_MATERIAL,             new FolderEntry("material", true, false, true));

    addEntry(LLFolderType::FT_NONE,                 new FolderEntry("-1",       false, false, false));
};

// static
LLFolderType::EType LLFolderType::lookup(const std::string& name)
{
    return LLFolderDictionary::getInstance()->lookup(name);
}

// static
const std::string &LLFolderType::lookup(LLFolderType::EType folder_type)
{
    const FolderEntry *entry = LLFolderDictionary::getInstance()->lookup(folder_type);
    if (entry)
    {
        return entry->mName;
    }
    else
    {
        return badLookup();
    }
}

// static
// Only plain folders and a few other types aren't protected.  "Protected" means
// you can't move, deleted, or change certain properties such as their type.
bool LLFolderType::lookupIsProtectedType(EType folder_type)
{
    const LLFolderDictionary *dict = LLFolderDictionary::getInstance();
    const FolderEntry *entry = dict->lookup(folder_type);
    if (entry)
    {
        return entry->mIsProtected;
    }
    return true;
}

// static
// Is this folder type automatically created outside the viewer?
bool LLFolderType::lookupIsAutomaticType(EType folder_type)
{
    const LLFolderDictionary *dict = LLFolderDictionary::getInstance();
    const FolderEntry *entry = dict->lookup(folder_type);
    if (entry)
    {
        return entry->mIsAutomatic;
    }
    return true;
}

// static
// Should this folder always exist as a single copy under (or as) the root?
bool LLFolderType::lookupIsSingletonType(EType folder_type)
{
    const LLFolderDictionary *dict = LLFolderDictionary::getInstance();
    const FolderEntry *entry = dict->lookup(folder_type);
    if (entry)
    {
        return entry->mIsSingleton;
    }
    return true;
}

// static
bool LLFolderType::lookupIsEnsembleType(EType folder_type)
{
    return (folder_type >= FT_ENSEMBLE_START &&
            folder_type <= FT_ENSEMBLE_END);
}

// static
LLAssetType::EType LLFolderType::folderTypeToAssetType(LLFolderType::EType folder_type)
{
    if (LLAssetType::lookup(LLAssetType::EType(folder_type)) == LLAssetType::BADLOOKUP)
    {
        LL_WARNS() << "Converting to unknown asset type " << folder_type << LL_ENDL;
    }
    return (LLAssetType::EType)folder_type;
}

// static
LLFolderType::EType LLFolderType::assetTypeToFolderType(LLAssetType::EType asset_type)
{
    if (LLFolderType::lookup(LLFolderType::EType(asset_type)) == LLFolderType::badLookup())
    {
        LL_WARNS() << "Converting to unknown folder type " << asset_type << LL_ENDL;
    }
    return (LLFolderType::EType)asset_type;
}

// static
const std::string &LLFolderType::badLookup()
{
    static const std::string sBadLookup = "llfoldertype_bad_lookup";
    return sBadLookup;
}

LLSD LLFolderType::getTypeNames()
{
    LLSD type_names;
    const LLFolderDictionary *dict = LLFolderDictionary::getInstance();
    for (S32 type = 0; type < FT_COUNT; ++type)
    {
        if (lookupIsEnsembleType(LLFolderType::EType(type))) continue;

        const FolderEntry *entry = dict->lookup(LLFolderType::EType(type));
        //skip llfoldertype_bad_lookup
        if (entry)
        {
            type_names.append(entry->mName);
        }
    }
    return type_names;
}
