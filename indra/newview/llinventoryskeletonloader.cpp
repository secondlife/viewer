/**
 * @file llinventoryskeletonloader.cpp
 * @brief LLInventorySkeletonLoader class implementation file
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

#include "llviewerprecompiledheaders.h"

#include "llinventoryskeletonloader.h"

#include "llsdserialize.h"
#include "llsdutil.h"

#include "llinventorymodel.h"

namespace
{
    static constexpr char const * LOG_INV = "Inventory";
}

LLInventorySkeletonLoader::LLInventorySkeletonLoader(const LLSD &options, const LLUUID &owner_id)
    : rv{true}
{
    LL_PROFILE_ZONE_SCOPED;
    LL_DEBUGS(LOG_INV) << "importing inventory skeleton for " << owner_id << LL_ENDL;

    cat_set_t temp_cats;

    for (LLSD const & sd : llsd::inArray(options))
    {
        LLSD name      = sd["name"];
        LLSD folder_id = sd["folder_id"];
        LLSD parent_id = sd["parent_id"];
        LLSD version   = sd["version"];
        if (name.isDefined() && folder_id.isDefined() && parent_id.isDefined() && version.isDefined() &&
            folder_id.asUUID().notNull()  // if an id is null, it locks the viewer.
        )
        {
            LLPointer<LLViewerInventoryCategory> cat = new LLViewerInventoryCategory(owner_id);
            cat->rename(name.asString());
            cat->setUUID(folder_id.asUUID());
            cat->setParent(parent_id.asUUID());

            LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
            LLSD                type_default   = sd["type_default"];
            if (type_default.isDefined())
            {
                preferred_type = (LLFolderType::EType) type_default.asInteger();
            }
            cat->setPreferredType(preferred_type);
            cat->setVersion(version.asInteger());
            temp_cats.insert(cat);
        }
        else
        {
            LL_WARNS(LOG_INV) << "Unable to import near " << name.asString() << LL_ENDL;
            rv = false;
        }
    }

    S32 cached_category_count = 0;
    S32 cached_item_count     = 0;
    if (!temp_cats.empty())
    {
        LLInventoryModel::update_map_t    child_counts;
        LLInventoryModel::cat_array_t  categories;
        LLInventoryModel::item_array_t    items;
        LLInventoryModel::changed_items_t categories_to_update;
        LLInventoryModel::item_array_t    possible_broken_links;
        cat_set_t       invalid_categories;  // Used to mark categories that weren't successfully loaded.
        std::string     inventory_filename = LLInventoryModel::getInvCacheAddres(owner_id);
        const S32       NO_VERSION         = LLViewerInventoryCategory::VERSION_UNKNOWN;
        std::string     gzip_filename(inventory_filename);
        gzip_filename.append(".gz");
        LLFILE *fp                    = LLFile::fopen(gzip_filename, "rb");
        bool    remove_inventory_file = false;
        if (fp)
        {
            fclose(fp);
            fp = NULL;
            if (gunzip_file(gzip_filename, inventory_filename))
            {
                // we only want to remove the inventory file if it was
                // gzipped before we loaded, and we successfully
                // gunziped it.
                remove_inventory_file = true;
            }
            else
            {
                LL_INFOS(LOG_INV) << "Unable to gunzip " << gzip_filename << LL_ENDL;
            }
        }
        bool is_cache_obsolete = false;

        file.open(inventory_filename);

        if (loadFromFile(categories, items, categories_to_update, is_cache_obsolete))
        {
            // We were able to find a cache of files. So, use what we
            // found to generate a set of categories we should add. We
            // will go through each category loaded and if the version
            // does not match, invalidate the version.
            S32                 count      = categories.size();
            cat_set_t::iterator not_cached = temp_cats.end();
            std::set<LLUUID>    cached_ids;
            for (S32 i = 0; i < count; ++i)
            {
                LLViewerInventoryCategory *cat = categories[i];
                cat_set_t::iterator        cit = temp_cats.find(cat);
                if (cit == temp_cats.end())
                {
                    continue;  // cache corruption?? not sure why this happens -SJB
                }
                LLViewerInventoryCategory *tcat = *cit;

                if (categories_to_update.find(tcat->getUUID()) != categories_to_update.end())
                {
                    tcat->setVersion(NO_VERSION);
                    LL_WARNS() << "folder to update: " << tcat->getName() << LL_ENDL;
                }

                // we can safely ignore anything loaded from file, but
                // not sent down in the skeleton. Must have been removed from inventory.
                if (cit == not_cached)
                {
                    continue;
                }
                else if (cat->getVersion() != tcat->getVersion())
                {
                    // if the cached version does not match the server version,
                    // throw away the version we have so we can fetch the
                    // correct contents the next time the viewer opens the folder.
                    tcat->setVersion(NO_VERSION);
                }
                else
                {
                    cached_ids.insert(tcat->getUUID());
                }
            }

            // go ahead and add the cats returned during the download
            std::set<LLUUID>::const_iterator not_cached_id = cached_ids.end();
            cached_category_count                          = cached_ids.size();
            for (auto const & cat : temp_cats)
            {
                if (cached_ids.find(cat->getUUID()) == not_cached_id)
                {
                    // this check is performed so that we do not
                    // mark new folders in the skeleton (and not in cache)
                    // as being cached.
                    LLViewerInventoryCategory *llvic = cat;
                    llvic->setVersion(NO_VERSION);
                }
                gInventory.addCategory(cat);
                ++child_counts[cat->getParentUUID()];
            }

            // Add all the items loaded which are parented to a
            // category with a correctly cached parent
            S32                 bad_link_count       = 0;
            S32                 good_link_count      = 0;
            S32                 recovered_link_count = 0;
            const LLInventoryModel::cat_map_t::const_iterator unparented = gInventory.mCategoryMap.end();
            for (auto const & item_ptr : items)
            {
                LLViewerInventoryItem    *item = item_ptr.get();
                LLInventoryModel::cat_map_t::const_iterator cit  = gInventory.mCategoryMap.find(item->getParentUUID());

                if (cit != unparented)
                {
                    const LLViewerInventoryCategory *cat = cit->second.get();
                    if (cat->getVersion() != NO_VERSION)
                    {
                        // This can happen if the linked object's baseobj is removed from the cache but the linked object is still in the
                        // cache.
                        if (item->getIsBrokenLink())
                        {
                            // bad_link_count++;
                            LL_DEBUGS(LOG_INV) << "Attempted to add cached link item without baseobj present ( name: " << item->getName()
                                               << " itemID: " << item->getUUID() << " assetID: " << item->getAssetUUID()
                                               << " ).  Ignoring and invalidating " << cat->getName() << " . " << LL_ENDL;
                            possible_broken_links.push_back(item);
                            continue;
                        }
                        else if (item->getIsLinkType())
                        {
                            good_link_count++;
                        }
                        gInventory.addItem(item);
                        cached_item_count += 1;
                        ++child_counts[cat->getUUID()];
                    }
                }
            }
            if (possible_broken_links.size() > 0)
            {
                for (auto const & item_ptr : possible_broken_links)
                {
                    LLViewerInventoryItem           *item = item_ptr.get();
                    const LLInventoryModel::cat_map_t::const_iterator        cit  = gInventory.mCategoryMap.find(item->getParentUUID());
                    const LLViewerInventoryCategory *cat  = cit->second.get();
                    if (item->getIsBrokenLink())
                    {
                        bad_link_count++;
                        invalid_categories.insert(cit->second);
                        // LL_INFOS(LOG_INV) << "link still broken: " << item->getName() << " in folder " << cat->getName() << LL_ENDL;
                    }
                    else
                    {
                        // was marked as broken because of loading order, its actually fine to load
                        gInventory.addItem(item);
                        cached_item_count += 1;
                        ++child_counts[cat->getUUID()];
                        recovered_link_count++;
                    }
                }

                LL_DEBUGS(LOG_INV) << "Attempted to add " << bad_link_count << " cached link items without baseobj present. "
                                   << good_link_count << " link items were successfully added. " << recovered_link_count
                                   << " links added in recovery. "
                                   << "The corresponding categories were invalidated." << LL_ENDL;
            }
        }
        else
        {
            // go ahead and add everything after stripping the version
            // information.
            for (auto const & cat : temp_cats)
            {
                LLViewerInventoryCategory *llvic = cat;
                llvic->setVersion(NO_VERSION);
                gInventory.addCategory(cat);
            }
        }

        // Invalidate all categories that failed fetching descendents for whatever
        // reason (e.g. one of the descendents was a broken link).
        for (cat_set_t::iterator invalid_cat_it = invalid_categories.begin(); invalid_cat_it != invalid_categories.end(); invalid_cat_it++)
        {
            LLViewerInventoryCategory *cat = (*invalid_cat_it).get();
            cat->setVersion(NO_VERSION);
            LL_DEBUGS(LOG_INV) << "Invalidating category name: " << cat->getName() << " UUID: " << cat->getUUID()
                               << " due to invalid descendents cache" << LL_ENDL;
        }
        if (invalid_categories.size() > 0)
        {
            LL_DEBUGS(LOG_INV) << "Invalidated " << invalid_categories.size() << " categories due to invalid descendents cache" << LL_ENDL;
        }

        // At this point, we need to set the known descendents for each
        // category which successfully cached so that we do not
        // needlessly fetch descendents for categories which we have.
        LLInventoryModel::update_map_t::const_iterator no_child_counts = child_counts.end();
        for (auto const & cat_ptr : temp_cats)
        {
            LLViewerInventoryCategory *cat = cat_ptr.get();
            if (cat->getVersion() != NO_VERSION)
            {
                LLInventoryModel::update_map_t::const_iterator the_count = child_counts.find(cat->getUUID());
                if (the_count != no_child_counts)
                {
                    const S32 num_descendents = (*the_count).second.mValue;
                    cat->setDescendentCount(num_descendents);
                }
                else
                {
                    cat->setDescendentCount(0);
                }
            }
        }

        if (remove_inventory_file)
        {
            // clean up the gunzipped file.
            LLFile::remove(inventory_filename);
        }
        if (is_cache_obsolete)
        {
            // If out of date, remove the gzipped file too.
            LL_WARNS(LOG_INV) << "Inv cache out of date, removing" << LL_ENDL;
            LLFile::remove(gzip_filename);
        }
        categories.clear();  // will unref and delete entries
    }

    LL_INFOS(LOG_INV) << "Successfully loaded " << cached_category_count << " categories and " << cached_item_count << " items from cache."
                      << LL_ENDL;

}

// static
bool LLInventorySkeletonLoader::loadFromFile(LLInventoryModel::cat_array_t & categories,
                                             LLInventoryModel::item_array_t &items,
                                             LLInventoryModel::changed_items_t &cats_to_update,
                                                               bool            &is_cache_obsolete)
{
    LL_PROFILE_ZONE_NAMED_COLOR("inventory loadFromFile", 0xFF1111);

    if (!file.is_open())
    {
        LL_INFOS(LOG_INV) << "unable to load inventory, file failed to open" << LL_ENDL;
        return false;
    }

    is_cache_obsolete = true;  // Obsolete until proven current

    std::string                   line;
    LLPointer<LLSDNotationParser> parser = new LLSDNotationParser();
    // U32 count  = 0;
    while (std::getline(file, line))
    {
        LLSD               s_item;
        std::istringstream iss(line);
        if (parser->parse(iss, s_item, line.length()) == LLSDParser::PARSE_FAILURE)
        {
            LL_WARNS(LOG_INV) << "Parsing inventory cache failed" << LL_ENDL;
            break;
        }

        if (s_item.has("inv_cache_version"))
        {
            S32 version = s_item["inv_cache_version"].asInteger();
            if (version == LLInventoryModel::sCurrentInvCacheVersion)
            {
                // Cache is up to date
                is_cache_obsolete = false;
                continue;
            }
            else
            {
                LL_WARNS(LOG_INV) << "Inventory cache is out of date" << LL_ENDL;
                break;
            }
        }
        else if (s_item.has("cat_id"))
        {
            if (is_cache_obsolete)
                break;

            LLPointer<LLViewerInventoryCategory> inv_cat = new LLViewerInventoryCategory(LLUUID::null);
            if (inv_cat->importLLSD(s_item))
            {
                categories.push_back(inv_cat);
            }
        }
        else if (s_item.has("item_id"))
        {
            if (is_cache_obsolete)
                break;

            // if (++count > 5000)
            //         {
            //	// oh no. inventory is huge!
            //             break;
            //         }

            LLPointer<LLViewerInventoryItem> inv_item = new LLViewerInventoryItem;
            if (inv_item->fromLLSD(s_item))
            {
                if (inv_item->getUUID().isNull())
                {
                    // LL_WARNS(LOG_INV) << "Ignoring inventory with null item id: "
                    //<< inv_item->getName() << LL_ENDL;
                }
                else
                {
                    if (inv_item->getType() == LLAssetType::AT_UNKNOWN)
                    {
                        cats_to_update.insert(inv_item->getParentUUID());
                    }
                    else
                    {
                        items.push_back(inv_item);
                    }
                }
            }
        }
    }

    file.close();

    return !is_cache_obsolete;
}

ELoaderStatus LLInventorySkeletonLoader::loadChunk()
{
    return rv ? LOAD_SUCCESS : LOAD_FAILURE;
}