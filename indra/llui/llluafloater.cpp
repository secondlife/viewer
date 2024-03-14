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

#include <filesystem>
#include "llevents.h"

#include "llcheckboxctrl.h"
#include "llcombobox.h"

const std::string LISTENER_NAME("LLLuaFloater");

const std::string COMMIT_EVENT("commit");
const std::string MOUSE_ENTER_EVENT("mouse_enter");
const std::string MOUSE_LEAVE_EVENT("mouse_leave");
const std::string MOUSE_DOWN_EVENT("mouse_down");
const std::string MOUSE_UP_EVENT("mouse_up");
const std::string RIGHT_MOUSE_DOWN_EVENT("right_mouse_down");
const std::string RIGHT_MOUSE_UP_EVENT("right_mouse_up");
const std::string DOUBLE_CLICK_EVENT("double_click");

LLLuaFloater::LLLuaFloater(const LLSD &key) :
    LLFloater(key), 
    mCommandPumpName(key["command_pump"].asString()),
    mDispatcher("LLLuaFloater", "action"),
    mReqID(key)
{
    mListenerPumpName = LLUUID::generateNewID().asString();

    mBoundListener = LLEventPumps::instance().obtain(mListenerPumpName).listen(LISTENER_NAME, [this](const LLSD &event) 
    { 
        if (event.has("action")) 
        {
            mDispatcher.try_call(event);
        }
        else 
        {
            LL_WARNS("LuaFloater") << "Unknown message: " << event << LL_ENDL;
        }
        return false;
    });

    LLSD requiredParams;
    requiredParams["ctrl_name"] = LLSD();
    requiredParams["value"] = LLSD();
    mDispatcher.add("set_enabled", "", [this](const LLSD &event) 
    { 
        LLUICtrl *ctrl = getChild<LLUICtrl>(event["ctrl_name"].asString());
        if(ctrl) 
        {
            ctrl->setEnabled(event["value"].asBoolean());
        }
    }, requiredParams);
    mDispatcher.add("set_visible", "", [this](const LLSD &event) 
    { 
        LLUICtrl *ctrl = getChild<LLUICtrl>(event["ctrl_name"].asString());
        if(ctrl) 
        {
            ctrl->setVisible(event["value"].asBoolean());
        }
    }, requiredParams);
    mDispatcher.add("set_value", "", [this](const LLSD &event) 
    { 
        LLUICtrl *ctrl = getChild<LLUICtrl>(event["ctrl_name"].asString());
        if(ctrl) 
        {
            ctrl->setValue(event["value"]);
        }
    }, requiredParams);

    requiredParams = LLSD().with("value", LLSD());
    mDispatcher.add("set_title", "", [this](const LLSD &event) 
    { 
        setTitle(event["value"].asString());
    });
}

BOOL LLLuaFloater::postBuild() 
{
    child_list_t::const_iterator iter = getChildList()->begin();
    child_list_t::const_iterator end = getChildList()->end();
    for (; iter != end; ++iter)
    {
        LLView *view  = *iter;
        if (!view) continue;

        LLSD data;
        data["ctrl_name"] = view->getName();
        data["event"] = COMMIT_EVENT;

        LLButton* btn = dynamic_cast<LLButton*>(view);
        if (btn && !isDefaultBtnName(btn->getName())) 
        {
            data["ctrl_type"] = "LLButton";
            btn->setCommitCallback([this, data](LLUICtrl *ctrl, const LLSD &param) { post(data); });
        }
        LLCheckBoxCtrl* check = dynamic_cast<LLCheckBoxCtrl*>(view);
        if (check)
        {
            data["ctrl_type"] = "LLCheckBoxCtrl";
            check->setCommitCallback([this, data](LLUICtrl *ctrl, const LLSD &param)
            {
                LLSD event(data);
                event["value"] = ctrl->getValue();
                post(event); 
            });
        }
        LLComboBox *combo = dynamic_cast<LLComboBox*>(view);
        if (combo)
        {
            data["ctrl_type"] = "LLComboBox";
            combo->setCommitCallback([this, data](LLUICtrl* ctrl, const LLSD &param)
            {
                LLSD event(data);
                event["value"] = dynamic_cast<LLComboBox*>(ctrl)->getValue();
                post(event); 
            });
        }
    }

    //optional field to send additional specified events to the script
    if (mKey.has("extra_events"))
    {
        //the first value is ctrl name, the second contains array of events to send
        const LLSD &events_map = mKey["extra_events"];
        for (LLSD::map_const_iterator it = events_map.beginMap(); it != events_map.endMap(); ++it)
        {
            std::string name = (*it).first;
            LLSD data = (*it).second;
            for (LLSD::array_const_iterator events_it = data.beginArray(); events_it != data.endArray(); ++events_it)
            {
                registerCallback(name, events_it->asString());
            }
        }
    }

    post(LLSD().with("command_name", mListenerPumpName).with("event", "post_build"));

    return true;
}

void LLLuaFloater::onClose(bool app_quitting)
{
    post(LLSD().with("event", "floater_close"));
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

    if (event == MOUSE_ENTER_EVENT)
    {
        ctrl->setMouseEnterCallback(mouse_event_cb);
    }
    else if (event == MOUSE_LEAVE_EVENT)
    {
        ctrl->setMouseLeaveCallback(mouse_event_cb);
    }
    else if (event == MOUSE_DOWN_EVENT)
    {
        ctrl->setMouseDownCallback(mouse_event_coords_cb);
    }
    else if (event == MOUSE_UP_EVENT)
    {
        ctrl->setMouseUpCallback(mouse_event_coords_cb);
    }
    else if (event == RIGHT_MOUSE_DOWN_EVENT)
    {
        ctrl->setRightMouseDownCallback(mouse_event_coords_cb);
    }
    else if (event == RIGHT_MOUSE_UP_EVENT)
    {
        ctrl->setRightMouseUpCallback(mouse_event_coords_cb);
    }
    else if (event == DOUBLE_CLICK_EVENT)
    {
        ctrl->setDoubleClickCallback(mouse_event_coords_cb);
    }
    else 
    {
        LL_WARNS("LuaFloater") << "Can't register callback for unknown event: " << event << " ,control: " << ctrl_name << LL_ENDL;
    }
}

void LLLuaFloater::post(const LLSD &data)
{
    LLSD stamped_data(data);
    mReqID.stamp(stamped_data);
    LLEventPumps::instance().obtain(mCommandPumpName).post(stamped_data);
}

void LLLuaFloater::showLuaFloater(const LLSD &data)
{
    std::filesystem::path fs_path(data["xml_path"].asString());

    LLLuaFloater *floater = new LLLuaFloater(data);
    floater->buildFromFile(fs_path.lexically_normal().string());
    floater->openFloater(floater->getKey());
}


