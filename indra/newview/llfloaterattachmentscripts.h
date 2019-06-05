/**
 * @file llfloaterconversationlog.h
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLFLOATERATTACHMENTSCRIPTS_H_
#define LL_LLFLOATERATTACHMENTSCRIPTS_H_

#include "llfloater.h"
#include "llaccordionctrl.h"
#include "llscrolllistctrl.h"

class LLFloaterAttachmentScripts : public LLFloater
{
public:

    LLFloaterAttachmentScripts(const LLSD& key);
    virtual ~LLFloaterAttachmentScripts(){};

	virtual BOOL postBuild() override;
    virtual void onOpen(const LLSD& key) override;

    virtual void refresh() override;

private:
    void            handleScriptData(const LLSD &results, U32 status);
    void            handleCheckToggle(LLScrollListItem *item);

    LLScrollListCtrl *    mScrollList;
};


#endif /* LL_LLFLOATERATTACHMENTSCRIPTS_H_ */
