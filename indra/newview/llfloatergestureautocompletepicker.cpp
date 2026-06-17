/**
 * @file llfloatergestureautocompletepicker.cpp
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatergestureautocompletepicker.h"

#include "llgestureautocompletehelper.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"

LLFloaterGestureAutocompletePicker::LLFloaterGestureAutocompletePicker(const LLSD& key)
: LLFloater(key), mGestureList(NULL)
{
    setFocusStealsFrontmost(false);
    setBackgroundVisible(false);
    setAutoFocus(false);
}

bool LLFloaterGestureAutocompletePicker::postBuild()
{
    mGestureList = getChild<LLScrollListCtrl>("gesture_list");
    mGestureList->setCommitOnKeyboardMovement(false);
    mGestureList->setCommitCallback(boost::bind(&LLFloaterGestureAutocompletePicker::commitSelected, this));

    return LLFloater::postBuild();
}

void LLFloaterGestureAutocompletePicker::onOpen(const LLSD& key)
{
    LLGestureAutocompleteHelper& helper = LLGestureAutocompleteHelper::instance();
    mGestureList->clearRows();

    const std::vector<LLGestureAutocompleteHelper::Row>& rows = helper.rows();

    for (const auto& row : rows)
    {
        LLSD element;
        element["value"] = row.value;
        element["columns"][0]["column"] = "trigger";
        element["columns"][0]["value"] = row.trigger;
        element["columns"][1]["column"] = "name";
        element["columns"][1]["value"] = row.name;
        mGestureList->addElement(element);
    }

    if (rows.empty() && !helper.emptyText().empty())
    {
        LLSD element;
        element["enabled"] = false;
        element["columns"][0]["column"] = "trigger";
        element["columns"][0]["value"] = helper.emptyText();
        element["columns"][1]["column"] = "name";
        element["columns"][1]["value"] = LLStringUtil::null;
        mGestureList->addElement(element);
    }

    if (helper.total() > rows.size())
    {
        LLSD element;
        element["enabled"] = false;
        element["columns"][0]["column"] = "trigger";
        element["columns"][0]["value"] = LLStringUtil::null;
        element["columns"][1]["column"] = "name";

        LLStringUtil::format_map_t args;
        args["[COUNT]"] = llformat("%d", (S32)rows.size());
        args["[TOTAL]"] = llformat("%d", (S32)helper.total());
        element["columns"][1]["value"] = getString("showing_count", args);

        mGestureList->addElement(element);
    }

    mGestureList->selectFirstItem();
    gFloaterView->adjustToFitScreen(this, false);
}

bool LLFloaterGestureAutocompletePicker::handleKey(KEY key, MASK mask, bool called_from_parent)
{
    if (mask == MASK_NONE)
    {
        switch (key)
        {
            case KEY_UP:
                mGestureList->selectPrevItem();
                mGestureList->scrollToShowSelected();
                return true;
            case KEY_DOWN:
                mGestureList->selectNextItem();
                mGestureList->scrollToShowSelected();
                return true;
            case KEY_RETURN:
            case KEY_TAB:
                commitSelected();
                return true;
            case KEY_ESCAPE:
                LLGestureAutocompleteHelper::instance().hideHelper();
                return true;
            case KEY_LEFT:
            case KEY_RIGHT:
                return true;
            default:
                break;
        }
    }

    return LLFloater::handleKey(key, mask, called_from_parent);
}

void LLFloaterGestureAutocompletePicker::onClose(bool app_quitting)
{
    if (!app_quitting)
    {
        LLGestureAutocompleteHelper::instance().hideHelper();
    }
}

void LLFloaterGestureAutocompletePicker::goneFromFront()
{
    LLGestureAutocompleteHelper::instance().hideHelper();
}

bool LLFloaterGestureAutocompletePicker::commitSelected()
{
    LLScrollListItem* item = mGestureList->getFirstSelected();

    if (!item || !item->getEnabled())
    {
        return false;
    }

    const std::string value = mGestureList->getSelectedValue().asString();

    if (value.empty())
    {
        return false;
    }

    setValue(value);
    onCommit();

    return true;
}
