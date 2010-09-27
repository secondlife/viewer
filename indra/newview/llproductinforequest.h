/** 
 * @file llproductinforequest.h
 * @author Kent Quirk
 * @brief Get region type descriptions (translation from SKU to description)
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLPRODUCTINFOREQUEST_H
#define LL_LLPRODUCTINFOREQUEST_H

#include "llhttpclient.h"
#include "llmemory.h"

/* 
 This is a singleton to manage a cache of information about land types.
 The land system provides a capability to get information about the
 set of possible land sku, name, and description information.
 We use description in the UI, but the sku is provided in the various
 messages; this tool provides translation between the systems.
 */

class LLProductInfoRequestManager : public LLSingleton<LLProductInfoRequestManager>
{
public:
	LLProductInfoRequestManager();
	void setSkuDescriptions(const LLSD& content);
	std::string getDescriptionForSku(const std::string& sku);
private:
	friend class LLSingleton<LLProductInfoRequestManager>;	
	/* virtual */ void initSingleton();
	LLSD mSkuDescriptions;
};

#endif // LL_LLPRODUCTINFOREQUEST_H
