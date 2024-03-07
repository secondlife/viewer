/** 
 * @file llproductinforequest.h
 * @author Kent Quirk
 * @brief Get region type descriptions (translation from SKU to description)
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPRODUCTINFOREQUEST_H
#define LL_LLPRODUCTINFOREQUEST_H

#include "llmemory.h"
#include "lleventcoro.h"
#include "llcoros.h"

/** 
 * This is a singleton to manage a cache of information about land types.
 * The land system provides a capability to get information about the
 * set of possible land sku, name, and description information.
 * We use description in the UI, but the sku is provided in the various
 * messages; this tool provides translation between the systems.
 */
class LLProductInfoRequestManager : public LLSingleton<LLProductInfoRequestManager>
{
	LLSINGLETON(LLProductInfoRequestManager);
public:
	std::string getDescriptionForSku(const std::string& sku);

private:
	/* virtual */ void initSingleton() override;

    void getLandDescriptionsCoro(std::string url);
    LLSD mSkuDescriptions;
};

#endif // LL_LLPRODUCTINFOREQUEST_H
