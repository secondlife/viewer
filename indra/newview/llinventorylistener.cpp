/**
 * @file llinventorylistener.cpp
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llinventorylistener.h"

#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "lltransutil.h"
#include "llwearableitemslist.h"
#include "resultset.h"
#include "stringize.h"
#include <algorithm>                // std::min()

constexpr S32 MAX_ITEM_LIMIT = 100;

LLInventoryListener::LLInventoryListener()
  : LLEventAPI("LLInventory",
               "API for interactions with viewer Inventory items")
{
    add("getItemsInfo",
        "Return information about items or folders defined in [\"item_ids\"]:\n"
        "reply will contain [\"items\"] and [\"categories\"] result set keys",
        &LLInventoryListener::getItemsInfo,
        llsd::map("item_ids", LLSD(), "reply", LLSD()));

    add("getFolderTypeNames",
        "Return the table of folder type names, contained in [\"names\"]\n",
        &LLInventoryListener::getFolderTypeNames,
        llsd::map("reply", LLSD()));

    add("getAssetTypeNames",
        "Return the table of asset type names, contained in [\"names\"]\n",
        &LLInventoryListener::getAssetTypeNames,
        llsd::map("reply", LLSD()));

    add("getBasicFolderID",
        "Return the UUID of the folder by specified folder type name, for example:\n"
        "\"Textures\", \"My outfits\", \"Sounds\" and other basic folders which have associated type",
        &LLInventoryListener::getBasicFolderID,
        llsd::map("ft_name", LLSD(), "reply", LLSD()));

    add("getDirectDescendants",
        "Return result set keys [\"categories\"] and [\"items\"] for the direct\n"
        "descendants of the [\"folder_id\"]",
        &LLInventoryListener::getDirectDescendants,
        llsd::map("folder_id", LLSD(), "reply", LLSD()));

    add("collectDescendantsIf",
        "Return result set keys [\"categories\"] and [\"items\"] for the descendants\n"
        "of the [\"folder_id\"], if it passes specified filters:\n"
        "[\"name\"] is a substring of object's name,\n"
        "[\"desc\"] is a substring of object's description,\n"
        "asset [\"type\"] corresponds to the string name of the object's asset type\n"
        "[\"limit\"] sets item count limit in result set (default unlimited)\n"
        "[\"filter_links\"]: EXCLUDE_LINKS - don't show links, ONLY_LINKS - only show links, INCLUDE_LINKS - show links too (default)",
        &LLInventoryListener::collectDescendantsIf,
        llsd::map("folder_id", LLSD(), "reply", LLSD()));

/*==========================================================================*|
    add("getSingle",
        "Return LLSD [\"single\"] for a single folder or item from the specified\n"
        "[\"result\"] key at the specified 0-relative [\"index\"].",
        &LLInventoryListener::getSingle,
        llsd::map("result", LLSD::Integer(), "index", LLSD::Integer(),
                  "reply", LLSD::String()));
|*==========================================================================*/

    add("getSlice",
        stringize(
        "Return an LLSD array [\"slice\"] from the specified [\"result\"] key\n"
        "starting at 0-relative [\"index\"] with (up to) [\"count\"] entries.\n"
        "count is limited to ", MAX_ITEM_LIMIT, " (default and max)."),
        &LLInventoryListener::getSlice,
        llsd::map("result", LLSD::Integer(), "index", LLSD::Integer(),
                  "reply", LLSD::String()));

    add("closeResult",
        "Release resources associated with specified [\"result\"] key,\n"
        "or keys if [\"result\"] is an array.",
        &LLInventoryListener::closeResult,
        llsd::map("result", LLSD()));
}

// This struct captures (possibly large) category results from
// getDirectDescendants() and collectDescendantsIf().
struct CatResultSet: public LL::VectorResultSet<LLInventoryModel::cat_array_t::value_type>
{
    CatResultSet(): super("categories") {}
    LLSD getSingleFrom(const LLPointer<LLViewerInventoryCategory>& cat) const override
    {
        return llsd::map("id", cat->getUUID(),
                         "name", cat->getName(),
                         "parent_id", cat->getParentUUID(),
                         "type", LLFolderType::lookup(cat->getPreferredType()));
    }
};

// This struct captures (possibly large) item results from
// getDirectDescendants() and collectDescendantsIf().
struct ItemResultSet: public LL::VectorResultSet<LLInventoryModel::item_array_t::value_type>
{
    ItemResultSet(): super("items") {}
    LLSD getSingleFrom(const LLPointer<LLViewerInventoryItem>& item) const override
    {
        return llsd::map("id", item->getUUID(),
                         "name", item->getName(),
                         "parent_id", item->getParentUUID(),
                         "desc", item->getDescription(),
                         "inv_type", LLInventoryType::lookup(item->getInventoryType()),
                         "asset_type", LLAssetType::lookup(item->getType()),
                         "creation_date", LLSD::Integer(item->getCreationDate()),
                         "asset_id", item->getAssetUUID(),
                         "is_link", item->getIsLinkType(),
                         "linked_id", item->getLinkedUUID());
    }
};

void LLInventoryListener::getItemsInfo(LLSD const &data)
{
    Response response(LLSD(), data);

    auto catresult = new CatResultSet;
    auto itemresult = new ItemResultSet;

    uuid_vec_t ids = LLSDParam<uuid_vec_t>(data["item_ids"]);
    for (auto &it : ids)
    {
        LLViewerInventoryItem* item = gInventory.getItem(it);
        if (item)
        {
            itemresult->mVector.push_back(item);
        }
        else
        {
            LLViewerInventoryCategory *cat = gInventory.getCategory(it);
            if (cat)
            {
                catresult->mVector.push_back(cat);
            }
        }
    }
    // Each of categories and items is a { result set key, total length } pair.
    response["categories"] = catresult->getKeyLength();
    response["items"] = itemresult->getKeyLength();
}

void LLInventoryListener::getFolderTypeNames(LLSD const &data)
{
    Response response(llsd::map("names", LLFolderType::getTypeNames()), data);
}

void LLInventoryListener::getAssetTypeNames(LLSD const &data)
{
    Response response(llsd::map("names", LLAssetType::getTypeNames()), data);
}

void LLInventoryListener::getBasicFolderID(LLSD const &data)
{
    Response response(llsd::map("id", gInventory.findCategoryUUIDForType(LLFolderType::lookup(data["ft_name"].asString()))), data);
}


void LLInventoryListener::getDirectDescendants(LLSD const &data)
{
    Response response(LLSD(), data);
    LLInventoryModel::cat_array_t* cats;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(data["folder_id"], cats, items);

    auto catresult = new CatResultSet;
    auto itemresult = new ItemResultSet;

    catresult->mVector = *cats;
    itemresult->mVector = *items;

    response["categories"] = catresult->getKeyLength();
    response["items"] = itemresult->getKeyLength();
}

struct LLFilteredCollector : public LLInventoryCollectFunctor
{
    enum EFilterLink
    {
        INCLUDE_LINKS,  // show links too
        EXCLUDE_LINKS,  // don't show links
        ONLY_LINKS      // only show links
    };

    LLFilteredCollector(LLSD const &data);
    virtual ~LLFilteredCollector() {}
    virtual bool operator()(LLInventoryCategory *cat, LLInventoryItem *item) override;
    virtual bool exceedsLimit() override
    {
        // mItemLimit == 0 means unlimited
        return (mItemLimit && mItemLimit <= mItemCount);
    }

  protected:
    bool checkagainstType(LLInventoryCategory *cat, LLInventoryItem *item);
    bool checkagainstNameDesc(LLInventoryCategory *cat, LLInventoryItem *item);
    bool checkagainstLinks(LLInventoryCategory *cat, LLInventoryItem *item);

    LLAssetType::EType mType;
    std::string mName;
    std::string mDesc;
    EFilterLink mLinkFilter;

    S32 mItemLimit;
    S32 mItemCount;
};

void LLInventoryListener::collectDescendantsIf(LLSD const &data)
{
    Response response(LLSD(), data);
    LLUUID folder_id(data["folder_id"].asUUID());
    LLViewerInventoryCategory *cat = gInventory.getCategory(folder_id);
    if (!cat)
    {
        return response.error(stringize("Folder ", std::quoted(data["folder_id"].asString()), " was not found"));
    }
    auto catresult = new CatResultSet;
    auto itemresult = new ItemResultSet;

    LLFilteredCollector collector = LLFilteredCollector(data);

    // Populate results directly into the catresult and itemresult arrays.
    // TODO: sprinkle count-based coroutine yields into the real
    // collectDescendentsIf() method so it doesn't steal too many cycles.
    gInventory.collectDescendentsIf(
        folder_id,
        catresult->mVector,
        itemresult->mVector,
        LLInventoryModel::EXCLUDE_TRASH,
        collector);

    response["categories"] = catresult->getKeyLength();
    response["items"] = itemresult->getKeyLength();
}

void LLInventoryListener::getSlice(LLSD const& data)
{
    auto result = LL::ResultSet::getInstance(data["result"]);
    int count = data.has("count")? data["count"].asInteger() : MAX_ITEM_LIMIT;
    LL_DEBUGS("Lua") << *result << ".getSlice(" << data["index"].asInteger()
                     << ", " << count << ')' << LL_ENDL;
    auto pair{ result->getSliceStart(data["index"], std::min(count, MAX_ITEM_LIMIT)) };
    sendReply(llsd::map("slice", pair.first, "start", pair.second), data);
}

void LLInventoryListener::closeResult(LLSD const& data)
{
    LLSD results = data["result"];
    if (results.isInteger())
    {
        results = llsd::array(results);
    }
    for (const auto& result : llsd::inArray(results))
    {
        auto ptr = LL::ResultSet::getInstance(result);
        if (ptr)
        {
            delete ptr.get();
        }
    }
}

LLFilteredCollector::LLFilteredCollector(LLSD const &data) :
    mType(LLAssetType::EType::AT_UNKNOWN),
    mLinkFilter(INCLUDE_LINKS),
    mItemLimit(0),
    mItemCount(0)
{

    mName = data["name"].asString();
    mDesc = data["desc"].asString();

    if (data.has("type"))
    {
        mType = LLAssetType::lookup(data["type"]);
    }
    if (data.has("filter_links"))
    {
        if (data["filter_links"] == "EXCLUDE_LINKS")
        {
            mLinkFilter = EXCLUDE_LINKS;
        }
        else if (data["filter_links"] == "ONLY_LINKS")
        {
            mLinkFilter = ONLY_LINKS;
        }
    }
    if (data["limit"].isInteger())
    {
        mItemLimit = std::max(data["limit"].asInteger(), 1);
    }
}

bool LLFilteredCollector::operator()(LLInventoryCategory *cat, LLInventoryItem *item)
{
    bool passed = checkagainstType(cat, item);
    passed = passed && checkagainstNameDesc(cat, item);
    passed = passed && checkagainstLinks(cat, item);

    if (passed)
    {
        ++mItemCount;
    }
    return passed;
}

bool LLFilteredCollector::checkagainstNameDesc(LLInventoryCategory *cat, LLInventoryItem *item)
{
    std::string name, desc;
    bool passed(true);
    if (cat)
    {
        if (!mDesc.empty()) return false;
        name = cat->getName();
    }
    if (item)
    {
        name = item->getName();
        passed = (mDesc.empty() || (item->getDescription().find(mDesc) != std::string::npos));
    }

    return passed && (mName.empty() || name.find(mName) != std::string::npos);
}

bool LLFilteredCollector::checkagainstType(LLInventoryCategory *cat, LLInventoryItem *item)
{
    if (mType == LLAssetType::AT_UNKNOWN)
    {
        return true;
    }
    if (cat && (mType == LLAssetType::AT_CATEGORY))
    {
        return true;
    }
    if (item && item->getType() == mType)
    {
        return true;
    }
    return false;
}

bool LLFilteredCollector::checkagainstLinks(LLInventoryCategory *cat, LLInventoryItem *item)
{
    bool is_link = cat ? cat->getIsLinkType() : item->getIsLinkType();
    if (is_link && (mLinkFilter == EXCLUDE_LINKS))
        return false;
    if (!is_link && (mLinkFilter == ONLY_LINKS))
        return false;
    return true;
}
