/**
 * @file   llviewinject.h
 * @author Nat Goodspeed
 * @date   2011-08-16
 * @brief  Supplemental LLView functionality used for simulating UI events.
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLVIEWINJECT_H)
#define LL_LLVIEWINJECT_H

#include "llview.h"
#include <map>

namespace llview
{

    /**
     * TargetEvent is a callable with state, specifically intended for use as
     * an LLView::TemporaryDrilldownFunc. Instantiate it with the desired
     * target LLView*; pass it to a TemporaryDrilldownFunc instance;
     * TargetEvent::operator() will then attempt to direct subsequent mouse
     * events to the desired target LLView*. (This is an "attempt" because
     * LLView will still balk unless the target LLView and every parent are
     * visible and enabled.)
     */
    class TargetEvent
    {
    public:
        /**
         * Construct TargetEvent with the desired target LLView*. (See
         * LLUI::resolvePath() to obtain an LLView* given a string pathname.)
         * This sets up for operator().
         */
        TargetEvent(LLView* view);

        /**
         * This signature must match LLView::DrilldownFunc. When you install
         * this TargetEvent instance using LLView::TemporaryDrilldownFunc,
         * LLView will call this method to decide whether to propagate an
         * incoming mouse event to the passed child LLView*.
         */
        bool operator()(const LLView*, S32 x, S32 y) const;

    private:
        // For a given parent LLView, identify which child to select.
        typedef std::map<LLView*, LLView*> ChildMap;
        ChildMap mChildMap;
    };

} // llview namespace

#endif /* ! defined(LL_LLVIEWINJECT_H) */
