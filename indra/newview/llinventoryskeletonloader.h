/**
 * @file llinventoryskeletonloader.h
 * @brief LLInventorySkeletonLoader class header file
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

#ifndef LL_LLINVENTORYSKELETONLOADER_H
#define LL_LLINVENTORYSKELETONLOADER_H

#include "llviewerinventory.h"
#include "llinventorymodel.h"

class LLInventorySkeletonLoader
{
public:
    struct InventoryIDPtrLess
    {
        bool operator()(const LLViewerInventoryCategory *i1, const LLViewerInventoryCategory *i2) const
        {
            return (i1->getUUID() < i2->getUUID());
        }
    };

    typedef std::unique_ptr<LLInventorySkeletonLoader> ptr_t;
    typedef std::set<LLPointer<LLViewerInventoryCategory>, InventoryIDPtrLess> cat_set_t;

    LLInventorySkeletonLoader(const LLSD &options, const LLUUID &owner_id);

    void loadChunk();
    bool loadFromFile(LLInventoryModel::cat_array_t     &categories,
                      LLInventoryModel::item_array_t    &items,
                      LLInventoryModel::changed_items_t &cats_to_update,
                      bool &is_cache_obsolete);

  private:

    llifstream file;
};


#endif  // LL_LLINVENTORYSKELETONLOADER_H
