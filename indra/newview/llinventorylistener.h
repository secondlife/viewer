/**
 * @file llinventorylistener.h
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


#ifndef LL_LLINVENTORYLISTENER_H
#define LL_LLINVENTORYLISTENER_H

#include "lleventapi.h"
#include "llinventoryfunctions.h"

class LLInventoryListener : public LLEventAPI
{
public:
    LLInventoryListener();

private:
    void getItemsInfo(LLSD const &data);
    void getFolderTypeNames(LLSD const &data);
    void getAssetTypeNames(LLSD const &data);
    void getBasicFolderID(LLSD const &data);
    void getDirectDescendents(LLSD const &data);
    void collectDescendentsIf(LLSD const &data);
};

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
    virtual bool operator()(LLInventoryCategory *cat, LLInventoryItem *item);

  protected:
    bool checkagainstType(LLInventoryCategory *cat, LLInventoryItem *item);
    bool checkagainstNameDesc(LLInventoryCategory *cat, LLInventoryItem *item);
    bool checkagainstLinks(LLInventoryCategory *cat, LLInventoryItem *item);

    LLAssetType::EType mType;
    std::string mName;
    std::string mDesc;
    EFilterLink mLinkFilter;
};

#endif // LL_LLINVENTORYLISTENER_H

