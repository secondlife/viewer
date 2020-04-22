/** 
* @author AndreyK Productengine
* @brief LLTrackPicker class header file including related functions
*
* $LicenseInfo:firstyear=2018&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2018, Linden Research, Inc.
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

#include "lltrackpicker.h"

#include "llradiogroup.h"
#include "llviewercontrol.h"


//=========================================================================
namespace
{
    const std::string FLOATER_DEFINITION_XML("floater_pick_track.xml");

    const std::string BTN_SELECT("btn_select");
    const std::string BTN_CANCEL("btn_cancel");
    const std::string RDO_TRACK_SELECTION("track_selection");
    const std::string RDO_TRACK_PREFIX("radio_sky");
}
//=========================================================================

LLFloaterTrackPicker::LLFloaterTrackPicker(LLView * owner, const LLSD &params) :
    LLFloater(params),
    mContextConeOpacity(0.0f),
    mOwnerHandle()
{
    mOwnerHandle = owner->getHandle();
    buildFromFile(FLOATER_DEFINITION_XML);
}

LLFloaterTrackPicker::~LLFloaterTrackPicker()
{
}

BOOL LLFloaterTrackPicker::postBuild()
{
    childSetAction(BTN_CANCEL, [this](LLUICtrl*, const LLSD& param){ onButtonCancel(); });
    childSetAction(BTN_SELECT, [this](LLUICtrl*, const LLSD& param){ onButtonSelect(); });
    return TRUE;
}

void LLFloaterTrackPicker::onClose(bool app_quitting)
{
    if (app_quitting)
        return;

    LLView *owner = mOwnerHandle.get();
    if (owner)
    {
        owner->setFocus(TRUE);
    }
}

void LLFloaterTrackPicker::showPicker(const LLSD &args)
{
    LLSD::array_const_iterator iter;
    LLSD::array_const_iterator end = args.endArray();

    bool select_item = true;
    for (iter = args.beginArray(); iter != end; ++iter)
    {
        S32 track_id = (*iter)["id"].asInteger();
        bool can_enable = (*iter)["enabled"].asBoolean();
        LLCheckBoxCtrl *view = getChild<LLCheckBoxCtrl>(RDO_TRACK_PREFIX + llformat("%d", track_id), true);
        view->setEnabled(can_enable);
        view->setLabelArg("[ALT]", (*iter).has("altitude") ? ((*iter)["altitude"].asString() + "m") : " ");

        // Mark first avaliable item as selected
        if (can_enable && select_item)
        {
            select_item = false;
            getChild<LLRadioGroup>(RDO_TRACK_SELECTION, true)->setSelectedByValue(LLSD(track_id), TRUE);
        }
    }

    openFloater(getKey());
    setFocus(TRUE);
}

void LLFloaterTrackPicker::draw()
{
    LLView *owner = mOwnerHandle.get();
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, owner);

    LLFloater::draw();
}

void LLFloaterTrackPicker::onButtonCancel()
{
    closeFloater();
}

void LLFloaterTrackPicker::onButtonSelect()
{
    if (mCommitSignal)
    {
        (*mCommitSignal)(this, getChild<LLRadioGroup>(RDO_TRACK_SELECTION, true)->getSelectedValue());
    }
    closeFloater();
}

void LLFloaterTrackPicker::onFocusLost()
{
    if (isInVisibleChain())
    {
        closeFloater();
    }
}
