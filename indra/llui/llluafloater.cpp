/**
 * @file llluafloater.cpp
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
 *
 */

#include "llluafloater.h"

#include "fsyspath.h"
#include "llevents.h"

#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llscrolllistctrl.h"
#include "lltexteditor.h"

const std::string LISTENER_NAME("LLLuaFloater");

std::set<std::string> EVENT_LIST = {
    "commit",
    "double_click",
    "mouse_enter",
    "mouse_leave",
    "mouse_down",
    "mouse_up",
    "right_mouse_down",
    "right_mouse_up",
    "post_build",
    "floater_close",
    "keystroke"
};

LLLuaFloater::LLLuaFloater(const LLSD &key) :
    LLFloater(key),
    mDispatchListener(LLUUID::generateNewID().asString(), "action"),
    mReplyPumpName(key["reply"].asString()),
    mReqID(key)
{
    auto ctrl_lookup = [this](const LLSD &event, std::function<LLSD(LLUICtrl*,const LLSD&)> cb)
    {
        LLUICtrl *ctrl = getChild<LLUICtrl>(event["ctrl_name"].asString());
        if (!ctrl)
        {
            LL_WARNS("LuaFloater") << "Control not found: " << event["ctrl_name"] << LL_ENDL;
            return LLSD();
        }
        return cb(ctrl, event);
    };

    LLSD requiredParams = llsd::map("ctrl_name", LLSD(), "value", LLSD());
    mDispatchListener.add("set_enabled", "", [ctrl_lookup](const LLSD &event)
    {
        return ctrl_lookup(event, [](LLUICtrl *ctrl, const LLSD &event) { ctrl->setEnabled(event["value"].asBoolean()); return LLSD(); });
    }, requiredParams);
    mDispatchListener.add("set_visible", "", [ctrl_lookup](const LLSD &event)
    {
        return ctrl_lookup(event, [](LLUICtrl *ctrl, const LLSD &event) { ctrl->setVisible(event["value"].asBoolean()); return LLSD(); });
    }, requiredParams);
    mDispatchListener.add("set_value", "", [ctrl_lookup](const LLSD &event)
    {
        return ctrl_lookup(event, [](LLUICtrl *ctrl, const LLSD &event) { ctrl->setValue(event["value"]); return LLSD(); });
    }, requiredParams);

    mDispatchListener.add("add_list_element", "", [this](const LLSD &event)
    {
        LLScrollListCtrl *ctrl = getChild<LLScrollListCtrl>(event["ctrl_name"].asString());
        if(ctrl)
        {
            LLSD element_data = event["value"];
            if (element_data.isArray())
            {
                for (const auto &row : llsd::inArray(element_data))
                {
                    ctrl->addElement(row);
                }
            }
            else
            {
                ctrl->addElement(element_data);
            }
        }
    }, requiredParams);

    mDispatchListener.add("clear_list", "", [this](const LLSD &event)
    {
        LLScrollListCtrl *ctrl = getChild<LLScrollListCtrl>(event["ctrl_name"].asString());
        if(ctrl)
        {
            ctrl->deleteAllItems();
        }
    }, llsd::map("ctrl_name", LLSD()));

    mDispatchListener.add("add_text", "", [this](const LLSD &event)
    {
        LLTextEditor *editor = getChild<LLTextEditor>(event["ctrl_name"].asString());
        if (editor)
        {
            editor->pasteTextWithLinebreaks(stringize(event["value"]));
            editor->addLineBreakChar(true);
        }
    }, requiredParams);

    mDispatchListener.add("set_label", "", [this](const LLSD &event)
    {
        LLButton *btn = getChild<LLButton>(event["ctrl_name"].asString());
        if (btn)
        {
            btn->setLabel((event["value"]).asString());
        }
    }, requiredParams);

    mDispatchListener.add("set_title", "", [this](const LLSD &event)
    {
        setTitle(event["value"].asString());
    }, llsd::map("value", LLSD()));

    mDispatchListener.add("get_value", "", [ctrl_lookup](const LLSD &event)
    {
        return ctrl_lookup(event, [](LLUICtrl *ctrl, const LLSD &event) { return llsd::map("value", ctrl->getValue()); });
    }, llsd::map("ctrl_name", LLSD(), "reqid", LLSD()));

    mDispatchListener.add("get_selected_id", "", [this](const LLSD &event)
    {
        LLScrollListCtrl *ctrl = getChild<LLScrollListCtrl>(event["ctrl_name"].asString());
        if (!ctrl)
        {
            LL_WARNS("LuaFloater") << "Control not found: " << event["ctrl_name"] << LL_ENDL;
            return LLSD();
        }
        return llsd::map("value", ctrl->getCurrentID());
    }, llsd::map("ctrl_name", LLSD(), "reqid", LLSD()));
}

LLLuaFloater::~LLLuaFloater()
{
    //post empty LLSD() to indicate done, in case it wasn't handled by the script after CLOSE_EVENT
    post(LLSD());
}

bool LLLuaFloater::postBuild()
{
    for (LLView *view : *getChildList())
    {
        LLUICtrl *ctrl = dynamic_cast<LLUICtrl*>(view);
        if (ctrl)
        {
            LLSD data;
            data["ctrl_name"] = view->getName();

            ctrl->setCommitCallback([this, data](LLUICtrl *ctrl, const LLSD &param)
            {
                LLSD event(data);
                event["value"] = ctrl->getValue();
                postEvent(event, "commit");
            });
        }
    }

    //optional field to send additional specified events to the script
    if (mKey.has("extra_events"))
    {
        //the first value is ctrl name, the second contains array of events to send
        for (const auto &[name, data] : llsd::inMap(mKey["extra_events"]))
        {
            for (const auto &event : llsd::inArray(data))
            {
                registerCallback(name, event);
            }
        }
    }

    //send pump name to the script after the floater is built
    postEvent(llsd::map("command_name", mDispatchListener.getPumpName()), "post_build");

    return true;
}

void LLLuaFloater::onClose(bool app_quitting)
{
    postEvent(llsd::map("app_quitting", app_quitting), "floater_close");
}

bool event_is(const std::string &event_name, const std::string &list_event)
{
    llassert(EVENT_LIST.find(list_event) != EVENT_LIST.end());
    return (event_name == list_event);
}

void LLLuaFloater::registerCallback(const std::string &ctrl_name, const std::string &event)
{
    LLUICtrl *ctrl = getChild<LLUICtrl>(ctrl_name);
    if (!ctrl) return;

    LLSD data;
    data["ctrl_name"] = ctrl_name;
    data["event"] = event;

    auto mouse_event_cb = [this, data](LLUICtrl *ctrl, const LLSD &param) { post(data); };

    auto mouse_event_coords_cb = [this, data](LLUICtrl *ctrl, S32 x, S32 y, MASK mask)
    {
        LLSD event(data);
        post(event.with("x", x).with("y", y));
    };

    auto post_with_value = [this, data](LLSD value)
    {
        LLSD event(data);
        post(event.with("value", value));
    };

    if (event_is(event, "mouse_enter"))
    {
        ctrl->setMouseEnterCallback(mouse_event_cb);
    }
    else if (event_is(event, "mouse_leave"))
    {
        ctrl->setMouseLeaveCallback(mouse_event_cb);
    }
    else if (event_is(event, "mouse_down"))
    {
        ctrl->setMouseDownCallback(mouse_event_coords_cb);
    }
    else if (event_is(event, "mouse_up"))
    {
        ctrl->setMouseUpCallback(mouse_event_coords_cb);
    }
    else if (event_is(event, "right_mouse_down"))
    {
        ctrl->setRightMouseDownCallback(mouse_event_coords_cb);
    }
    else if (event_is(event, "right_mouse_up"))
    {
        ctrl->setRightMouseUpCallback(mouse_event_coords_cb);
    }
    else if (event_is(event, "double_click"))
    {
        LLScrollListCtrl *list = dynamic_cast<LLScrollListCtrl *>(ctrl);
        if (list)
        {
            list->setDoubleClickCallback( [post_with_value, list](){ post_with_value(LLSD(list->getCurrentID())); });
        }
        else
        {
            ctrl->setDoubleClickCallback(mouse_event_coords_cb);
        }
    }
    else if (event_is(event, "keystroke"))
    {
        LLTextEditor* text_editor = dynamic_cast<LLTextEditor*>(ctrl);
        if (text_editor)
        {
            text_editor->setKeystrokeCallback([post_with_value](LLTextEditor *editor) { post_with_value(editor->getValue()); });
        }
        LLLineEditor* line_editor = dynamic_cast<LLLineEditor*>(ctrl);
        if (line_editor)
        {
            line_editor->setKeystrokeCallback([post_with_value](LLLineEditor *editor, void* userdata) { post_with_value(editor->getValue()); }, NULL);
        }
    }
    else
    {
        LL_WARNS("LuaFloater") << "Can't register callback for unknown event: " << event << " , control: " << ctrl_name << LL_ENDL;
    }
}

void LLLuaFloater::post(const LLSD &data)
{
    // send event data to the script signed with ["reqid"] key
    LLSD stamped_data(data);
    mReqID.stamp(stamped_data);
    LLEventPumps::instance().obtain(mReplyPumpName).post(stamped_data);
}

void LLLuaFloater::postEvent(LLSD data, const std::string &event_name)
{
    llassert(EVENT_LIST.find(event_name) != EVENT_LIST.end());
    post(data.with("event", event_name));
}

void LLLuaFloater::showLuaFloater(const LLSD &data)
{
    fsyspath fs_path(data["xml_path"].asString());
    fsyspath path = fs_path.lexically_normal();
    if (!fs_path.is_absolute())
    {
        std::string lib_path = gDirUtilp->getExpandedFilename(LL_PATH_SCRIPTS, "lua");
        path = fsyspath(fsyspath(lib_path) / path);
    }

    LLLuaFloater *floater = new LLLuaFloater(data);
    floater->buildFromFile(path);
    floater->openFloater(floater->getKey());
}

LLSD LLLuaFloater::getEventsData()
{
    LLSD event_data;
    for (auto &it : EVENT_LIST)
    {
        event_data.append(it);
    }
    return event_data;
}
