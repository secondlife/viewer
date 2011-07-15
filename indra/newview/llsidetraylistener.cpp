/**
 * @file   llsidetraylistener.cpp
 * @author Nat Goodspeed
 * @date   2011-02-15
 * @brief  Implementation for llsidetraylistener.
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llsidetraylistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llsidetray.h"
#include "llsdutil.h"

LLSideTrayListener::LLSideTrayListener(const Getter& getter):
    LLEventAPI("LLSideTray",
               "Operations on side tray (e.g. query state, query tabs)"),
    mGetter(getter)
{
    add("getCollapsed", "Send on [\"reply\"] an [\"open\"] Boolean",
        &LLSideTrayListener::getCollapsed, LLSDMap("reply", LLSD()));
    add("getTabs",
        "Send on [\"reply\"] a map of tab names and info about them",
        &LLSideTrayListener::getTabs, LLSDMap("reply", LLSD()));
    add("getPanels",
        "Send on [\"reply\"] data about panels available with SideTray.ShowPanel",
        &LLSideTrayListener::getPanels, LLSDMap("reply", LLSD()));
}

void LLSideTrayListener::getCollapsed(const LLSD& event) const
{
    sendReply(LLSDMap("open", ! mGetter()->getCollapsed()), event);
}

void LLSideTrayListener::getTabs(const LLSD& event) const
{
    LLSD reply;

    LLSideTray* tray = mGetter();
    LLSD::Integer ord(0);
    for (LLSideTray::child_list_const_iter_t chi(tray->beginChild()), chend(tray->endChild());
         chi != chend; ++chi, ++ord)
    {
        LLView* child = *chi;
        // How much info is important? Toss in as much as seems reasonable for
        // each tab. But to me, at least for the moment, the most important
        // item is the tab name.
        LLSD info;
        // I like the idea of returning a map keyed by tab name. But as
        // compared to an array of maps, that loses sequence information.
        // Address that by indicating the original order in each map entry.
        info["ord"] = ord;
        info["visible"] = bool(child->getVisible());
        info["enabled"] = bool(child->getEnabled());
        info["available"] = child->isAvailable();
        reply[child->getName()] = info;
    }

    sendReply(reply, event);
}

static LLSD getTabInfo(LLPanel* tab)
{
    LLSD panels;
    for (LLPanel::tree_iterator_t ti(tab->beginTreeDFS()), tend(tab->endTreeDFS());
         ti != tend; ++ti)
    {
        // *ti is actually an LLView*, which had better not be NULL
        LLView* view(*ti);
        if (! view)
        {
            LL_ERRS("LLSideTrayListener") << "LLSideTrayTab '" << tab->getName()
                                          << "' has a NULL child LLView*" << LL_ENDL;
        }

        // The logic we use to decide what "panel" names to return is heavily
        // based on LLSideTray::showPanel(): the function that actually
        // implements the "SideTray.ShowPanel" operation. showPanel(), in
        // turn, depends on LLSideTray::openChildPanel(): when
        // openChildPanel() returns non-NULL, showPanel() stops searching
        // attached and detached LLSideTrayTab tabs.

        // For each LLSideTrayTab, openChildPanel() first calls
        // findChildView(panel_name, true). In other words, panel_name need
        // not be a direct LLSideTrayTab child, it's sought recursively.
        // That's why we use (begin|end)TreeDFS() in this loop.

        // But this tree_iterator_t loop will actually traverse every widget
        // in every panel. Returning all those names will not help our caller:
        // passing most such names to openChildPanel() would not do what we
        // want. Even though the code suggests that passing ANY valid
        // side-panel widget name to openChildPanel() will open the tab
        // containing that widget, results could get confusing since followup
        // (onOpen()) logic wouldn't be invoked, and showPanel() wouldn't stop
        // searching because openChildPanel() would return NULL.

        // We must filter these LLView items, using logic that (sigh!) mirrors
        // openChildPanel()'s own.

        // openChildPanel() returns a non-NULL LLPanel* when either:
        // - the LLView is a direct child of an LLSideTrayPanelContainer
        // - the LLView is itself an LLPanel.
        // But as LLSideTrayPanelContainer can directly contain LLView items
        // that are NOT themselves LLPanels (e.g. "sidebar_me" contains an
        // LLButton called "Jump Right Arrow"), we'd better focus only on
        // LLSideTrayPanelContainer children that are themselves LLPanel
        // items. Which means that the second test completely subsumes the
        // first.
        LLPanel* panel(dynamic_cast<LLPanel*>(view));
        if (panel)
        {
            // Maybe it's overkill to construct an LLSD::Map for each panel, but
            // the possibility remains that we might want to deliver more info
            // about each panel than just its name.
            panels.append(LLSDMap("name", panel->getName()));
        }
    }

    return LLSDMap("panels", panels);
}

void LLSideTrayListener::getPanels(const LLSD& event) const
{
    LLSD reply;

    LLSideTray* tray = mGetter();
    // Iterate through the attached tabs.
    LLSD::Integer ord(0);
    for (LLSideTray::child_vector_t::const_iterator
             ati(tray->mTabs.begin()), atend(tray->mTabs.end());
         ati != atend; ++ati)
    {
        // We don't have access to LLSideTrayTab: the class definition is
        // hidden in llsidetray.cpp. But as LLSideTrayTab isa LLPanel, use the
        // LLPanel API. Unfortunately, without the LLSideTrayTab definition,
        // the compiler doesn't even know this LLSideTrayTab* is an LLPanel*.
        // Persuade it.
        LLPanel* tab(tab_cast<LLPanel*>(*ati));
        reply[tab->getName()] = getTabInfo(tab).with("attached", true).with("ord", ord);
    }

    // Now iterate over the detached tabs. These can also be opened via
    // SideTray.ShowPanel.
    ord = 0;
    for (LLSideTray::child_vector_t::const_iterator
             dti(tray->mDetachedTabs.begin()), dtend(tray->mDetachedTabs.end());
         dti != dtend; ++dti)
    {
        LLPanel* tab(tab_cast<LLPanel*>(*dti));
        reply[tab->getName()] = getTabInfo(tab).with("attached", false).with("ord", ord);
    }

    sendReply(reply, event);
}
