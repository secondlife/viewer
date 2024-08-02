/**
 * @file   lluilistener.cpp
 * @author Nat Goodspeed
 * @date   2009-08-18
 * @brief  Implementation for lluilistener.
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

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "lluilistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llmenugl.h"
#include "lltoolbarview.h"
#include "llui.h" // getRootView(), resolvePath()
#include "lluictrl.h"
#include "llerror.h"

extern LLMenuBarGL* gMenuBarView;

#define THROTTLE_PERIOD 1.5 // required seconds between throttled functions
#define MIN_THROTTLE 0.5

LLUIListener::LLUIListener():
    LLEventAPI("UI",
               "LLUICtrl::CommitCallbackRegistry listener.\n"
               "Capable of invoking any function (with parameter) you can specify in XUI.")
{
    add("call",
        "Invoke the operation named by [\"function\"], passing [\"parameter\"],\n"
        "as if from a user gesture on a menu -- or a button click.",
        &LLUIListener::call,
        llsd::map("function", LLSD(), "reply", LLSD()));

    add("callables",
        "Return a list [\"callables\"] of dicts {name, access} of functions registered to\n"
        "invoke with \"call\".\n"
        "access has values \"allow\", \"block\" or \"throttle\".",
        &LLUIListener::callables,
        llsd::map("reply", LLSD::String()));

    add("getValue",
        "For the UI control identified by the path in [\"path\"], return the control's\n"
        "current value as [\"value\"] reply.",
        &LLUIListener::getValue,
        llsd::map("path", LLSD(), "reply", LLSD()));

    add("getParents",
        "List names of Top menus suitable for passing as \"parent_menu\"",
        &LLUIListener::getParents,
        llsd::map("reply", LLSD::String()));

    LLSD required_args = llsd::map("name", LLSD(), "label", LLSD(), "reply", LLSD());
    add("addMenu",
        "Add new drop-down menu [\"name\"] with displayed [\"label\"] to the Top menu.",
        &LLUIListener::addMenu,
        required_args);

    required_args.insert("parent_menu", LLSD());
    add("addMenuBranch",
        "Add new menu branch [\"name\"] with displayed [\"label\"]\n"
        "to the [\"parent_menu\"] within the Top menu.",
        &LLUIListener::addMenuBranch,
        required_args);

    add("addMenuItem",
        "Add new menu item [\"name\"] with displayed [\"label\"]\n"
        "and call-on-click UI function [\"func\"] with optional [\"param\"]\n"
        "to the [\"parent_menu\"] within the Top menu.",
        &LLUIListener::addMenuItem,
        required_args.with("func", LLSD()));
    
    add("addMenuSeparator",
        "Add menu separator to the [\"parent_menu\"] within the Top menu.",
        &LLUIListener::addMenuSeparator,
        llsd::map("parent_menu", LLSD(), "reply", LLSD()));

    add("defaultToolbars",
        "todo: defaultToolbars desc",
        &LLUIListener::restoreDefaultToolbars);

    add("clearToolbars",
        "Clear all buttons off the toolbars",
        &LLUIListener::clearAllToolbars);

    add("addToolbarBtn",
        "Add [\"btn_name\"] toolbar button to the [\"toolbar\"]\n"
        "[1 (left toolbar), 2 (right toolbar), 3 (bottom toolbar)]\n"
        "Position of the command in the original list can be specified as [\"rank\"]",
        &LLUIListener::addToolbarBtn,
        llsd::map("btn_name", LLSD(), "reply", LLSD()));

    add("removeToolbarBtn",
        "Remove [\"btn_name\"] toolbar button off the toolbar,\n"
        "return [\"rank\"] (old position) of the command in the original list",
        &LLUIListener::removeToolbarBtn,
        llsd::map("btn_name", LLSD(), "reply", LLSD()));

    add("getToolbarBtnNames",
        "Return the table of Toolbar buttons names",
        &LLUIListener::getToolbarBtnNames,
        llsd::map("reply", LLSD()));
}

typedef LLUICtrl::CommitCallbackInfo cb_info;
void LLUIListener::call(const LLSD& event)
{
    Response response(LLSD(), event);
    LLUICtrl::CommitCallbackInfo *info = LLUICtrl::CommitCallbackRegistry::getValue(event["function"]);
    if (!info || !info->callback_func)
    {
        return response.error(stringize("Function ", std::quoted(event["function"].asString()), " was not found"));
    }
    if (info->handle_untrusted == cb_info::UNTRUSTED_BLOCK) 
    {
        return response.error(stringize("Function ", std::quoted(event["function"].asString()), " may not be called from the script"));
    }

    //Separate UNTRUSTED_THROTTLE and UNTRUSTED_ALLOW functions to have different timeout
    F64 *throttlep, period;
    if (info->handle_untrusted == cb_info::UNTRUSTED_THROTTLE)
    {
        throttlep = &mLastUntrustedThrottle;
        period = THROTTLE_PERIOD;
    }
    else
    {
        throttlep = &mLastMinThrottle;
        period = MIN_THROTTLE;
    }

    F64 cur_time = LLTimer::getElapsedSeconds();
    F64 time_delta = *throttlep + period;
    if (cur_time < time_delta)
    {
        LL_WARNS("LLUIListener") << "Throttled function " << std::quoted(event["function"].asString()) << LL_ENDL;
        return;
    }
    *throttlep = cur_time;

    // Interestingly, view_listener_t::addMenu() (addCommit(),
    // addEnable()) constructs a commit_callback_t callable that accepts
    // two parameters but discards the first. Only the second is passed to
    // handleEvent(). Therefore we feel completely safe passing NULL for
    // the first parameter.
    (info->callback_func)(NULL, event["parameter"]);
}

void LLUIListener::callables(const LLSD& event) const
{
    Response response(LLSD(), event);

    using Registry = LLUICtrl::CommitCallbackRegistry;
    using Method = Registry::Registrar& (*)();
    static Method registrars[] =
    {
        &Registry::defaultRegistrar,
        &Registry::currentRegistrar,
    };
    LLSD list;
    for (auto method : registrars)
    {
        auto& registrar{ (*method)() };
        for (auto it = registrar.beginItems(), end = registrar.endItems(); it != end; ++it)
        {
            LLSD entry{ llsd::map("name", it->first) };
            switch (it->second.handle_untrusted)
            {
            case LLUICtrl::CommitCallbackInfo::UNTRUSTED_ALLOW:
                entry["access"] = "allow";
                break;
            case LLUICtrl::CommitCallbackInfo::UNTRUSTED_BLOCK:
                entry["access"] = "block";
                break;
            case LLUICtrl::CommitCallbackInfo::UNTRUSTED_THROTTLE:
                entry["access"] = "throttle";
                break;
            }
            list.append(entry);
        }
    }
    response["callables"] = list;
}

void LLUIListener::getValue(const LLSD& event) const
{
    Response response(LLSD(), event);

    const LLView* root = LLUI::getInstance()->getRootView();
    const LLView* view = LLUI::getInstance()->resolvePath(root, event["path"].asString());
    const LLUICtrl* ctrl(dynamic_cast<const LLUICtrl*>(view));

    if (ctrl) 
    {
        response["value"] = ctrl->getValue();
    }
    else
    {
        response.error(stringize("UI control ", std::quoted(event["path"].asString()), " was not found"));
    }
}

void LLUIListener::getParents(const LLSD& event) const
{
    Response response(LLSD(), event);
    response["parents"] = llsd::toArray(
        *gMenuBarView->getChildList(),
        [](auto childp) {return childp->getName(); });
}

LLMenuGL::Params get_params(const LLSD&event)
{
    LLMenuGL::Params item_params;
    item_params.name  = event["name"];
    item_params.label = event["label"];
    item_params.can_tear_off = true;
    return item_params;
}

LLMenuGL* get_parent_menu(LLEventAPI::Response& response, const LLSD&event)
{
    LLMenuGL* parent_menu = gMenuBarView->findChildMenuByName(event["parent_menu"], true);
    if(!parent_menu)
    {
        response.error(stringize("Parent menu ", std::quoted(event["parent_menu"].asString()), " was not found"));
    }
    return parent_menu;
}

void LLUIListener::addMenu(const LLSD&event) const
{
    Response response(LLSD(), event);
    LLMenuGL::Params item_params = get_params(event);
    if(!gMenuBarView->appendMenu(LLUICtrlFactory::create<LLMenuGL>(item_params)))
    {
        response.error(stringize("Menu ", std::quoted(event["name"].asString()), " was not added"));
    }
}

void LLUIListener::addMenuBranch(const LLSD&event) const
{
    Response response(LLSD(), event);
    if(LLMenuGL* parent_menu = get_parent_menu(response, event))
    {
        LLMenuGL::Params item_params = get_params(event);
        if(!parent_menu->appendMenu(LLUICtrlFactory::create<LLMenuGL>(item_params)))
        {
            response.error(stringize("Menu branch ", std::quoted(event["name"].asString()), " was not added"));
        }
    }
}

void LLUIListener::addMenuItem(const LLSD&event) const
{
    Response response(LLSD(), event);
    LLMenuItemCallGL::Params item_params;
    item_params.name  = event["name"];
    item_params.label = event["label"];
    LLUICtrl::CommitCallbackParam item_func;
    item_func.function_name = event["func"];
    if (event.has("param"))
    {
        item_func.parameter = event["param"];
    }
    item_params.on_click = item_func;
    if(LLMenuGL* parent_menu = get_parent_menu(response, event))
    {
        if(!parent_menu->append(LLUICtrlFactory::create<LLMenuItemCallGL>(item_params)))
        {
            response.error(stringize("Menu item ", std::quoted(event["name"].asString()), " was not added"));
        }
    }
}

void LLUIListener::addMenuSeparator(const LLSD&event) const
{
    Response response(LLSD(), event);
    if(LLMenuGL* parent_menu = get_parent_menu(response, event))
    {
        if(!parent_menu->addSeparator())
        {
            response.error("Separator was not added");
        }
    }
}

void LLUIListener::restoreDefaultToolbars(const LLSD &event) const
{
    LLToolBarView::loadDefaultToolbars();
}

void LLUIListener::clearAllToolbars(const LLSD &event) const
{
    LLToolBarView::clearAllToolbars();
}

void LLUIListener::addToolbarBtn(const LLSD &event) const
{
    Response response(LLSD(), event);

    typedef LLToolBarEnums::EToolBarLocation ToolBarLocation;
    ToolBarLocation toolbar = ToolBarLocation::TOOLBAR_BOTTOM;
    if (event.has("toolbar"))
    {
        toolbar = llclamp((ToolBarLocation)event["toolbar"].asInteger(), ToolBarLocation::TOOLBAR_NONE, ToolBarLocation::TOOLBAR_BOTTOM);
    }
    S32 rank = event.has("rank") ? event["rank"].asInteger() : - 1;
    if(!gToolBarView->addCommand(event["btn_name"].asString(), toolbar, rank))
    {
        response.error(stringize("Toolbar button ", std::quoted(event["btn_name"].asString()), " was not found"));
    }
}

void LLUIListener::removeToolbarBtn(const LLSD &event) const
{
    Response response(LLSD(), event);

    S32 old_rank = LLToolBar::RANK_NONE;
    gToolBarView->removeCommand(event["btn_name"].asString(), old_rank);
    response["rank"] = old_rank;
}

void LLUIListener::getToolbarBtnNames(const LLSD &event) const
{
    Response response(llsd::map("cmd_names", LLCommandManager::instance().getCommandNames()), event);
}
