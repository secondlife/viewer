/**
 * @file   llviewinject.cpp
 * @author Nat Goodspeed
 * @date   2011-08-16
 * @brief  Implementation for llviewinject.
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llviewinject.h"
// STL headers
// std headers
// external library headers
// other Linden headers

llview::TargetEvent::TargetEvent(LLView* view)
{
    // Walk up the view tree from target LLView to the root (NULL). If
    // passed NULL, iterate 0 times.
    for (; view; view = view->getParent())
    {
        // At each level, operator() is going to ask: for a particular parent
        // LLView*, which of its children should I select? So for this view's
        // parent, select this view.
        mChildMap[view->getParent()] = view;
    }
}

bool llview::TargetEvent::operator()(const LLView* view, S32 /*x*/, S32 /*y*/) const
{
    // We are being called to decide whether to direct an incoming mouse event
    // to this child view. (Normal LLView processing is to check whether the
    // incoming (x, y) is within the view.) Look up the parent to decide
    // whether, for that parent, this is the previously-selected child.
    ChildMap::const_iterator found(mChildMap.find(view->getParent()));
    // If we're looking at a child whose parent isn't even in the map, never
    // mind.
    if (found == mChildMap.end())
    {
        return false;
    }
    // So, is this the predestined child for this parent?
    return (view == found->second);
}
