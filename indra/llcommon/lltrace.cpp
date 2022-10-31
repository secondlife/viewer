/** 
 * @file lltrace.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "lltrace.h"
#include "lltracerecording.h"
#include "lltracethreadrecorder.h"
#include "llfasttimer.h"

namespace LLTrace
{

MemStatHandle gTraceMemStat("LLTrace");

StatBase::StatBase( const char* name, const char* description ) 
:   mName(name),
    mDescription(description ? description : "")
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
    if (LLTrace::get_thread_recorder().notNull())
    {
        LL_ERRS() << "Attempting to declare trace object after program initialization.  Trace objects should be statically initialized." << LL_ENDL;
    }
#endif
}

const char* StatBase::getUnitLabel() const
{
    return "";
}

TimeBlockTreeNode::TimeBlockTreeNode() 
:   mBlock(NULL),
    mParent(NULL),
    mNeedsSorting(false),
    mCollapsed(true)
{}

void TimeBlockTreeNode::setParent( BlockTimerStatHandle* parent )
{
    LL_PROFILE_ZONE_SCOPED;
    llassert_always(parent != mBlock);
    llassert_always(parent != NULL);

    TimeBlockTreeNode* parent_tree_node = get_thread_recorder()->getTimeBlockTreeNode(parent->getIndex());
    if (!parent_tree_node) return;

    if (mParent)
    {
        std::vector<BlockTimerStatHandle*>& children = mParent->getChildren();
        std::vector<BlockTimerStatHandle*>::iterator found_it = std::find(children.begin(), children.end(), mBlock);
        if (found_it != children.end())
        {
            children.erase(found_it);
        }
    }

    mParent = parent;
    mBlock->getCurrentAccumulator().mParent = parent;
    parent_tree_node->mChildren.push_back(mBlock);
    parent_tree_node->mNeedsSorting = true;
}

}
