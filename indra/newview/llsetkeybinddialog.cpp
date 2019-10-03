/** 
 * @file llsetkeybinddialog.cpp
 * @brief LLSetKeyBindDialog class implementation.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#include "llsetkeybinddialog.h"

//#include "llkeyboard.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lleventtimer.h"
#include "llfocusmgr.h"
#include "llkeyconflict.h"

class LLSetKeyBindDialog::Updater : public LLEventTimer
{
public:

    typedef boost::function<void(MASK)> callback_t;

    Updater(callback_t cb, F32 period, MASK mask)
        :LLEventTimer(period),
        mMask(mask),
        mCallback(cb)
    {
        mEventTimer.start();
    }

    virtual ~Updater(){}

protected:
    BOOL tick()
    {
        mCallback(mMask);
        // Deletes itseft after execution
        return TRUE;
    }

private:
    MASK mMask;
    callback_t mCallback;
};

LLSetKeyBindDialog::LLSetKeyBindDialog(const LLSD& key)
    : LLModalDialog(key),
    pParent(NULL),
    mKeyFilterMask(DEFAULT_KEY_FILTER),
    pUpdater(NULL)
{
}

LLSetKeyBindDialog::~LLSetKeyBindDialog()
{
}

//virtual
BOOL LLSetKeyBindDialog::postBuild()
{
    childSetAction("SetEmpty", onBlank, this);
    childSetAction("Default", onDefault, this);
    childSetAction("Cancel", onCancel, this);
    getChild<LLUICtrl>("Cancel")->setFocus(TRUE);

    pCheckBox = getChild<LLCheckBoxCtrl>("ignore_masks");
    pDesription = getChild<LLTextBase>("descritption");

    gFocusMgr.setKeystrokesOnly(TRUE);

    return TRUE;
}

//virtual
void LLSetKeyBindDialog::onClose(bool app_quiting)
{
    if (pParent)
    {
        pParent->onCancelKeyBind();
        pParent = NULL;
    }
    if (pUpdater)
    {
        // Doubleclick timer has't fired, delete it
        delete pUpdater;
        pUpdater = NULL;
    }
    LLModalDialog::onClose(app_quiting);
}

//virtual
void LLSetKeyBindDialog::draw()
{
    LLRect local_rect;
    drawFrustum(local_rect, this, (LLView*)getDragHandle(), hasFocus());
    LLModalDialog::draw();
}

void LLSetKeyBindDialog::setParent(LLKeyBindResponderInterface* parent, LLView* frustum_origin, U32 key_mask)
{
    pParent = parent;
    setFrustumOrigin(frustum_origin);
    mKeyFilterMask = key_mask;

    std::string input;
    if ((key_mask & ALLOW_MOUSE) != 0)
    {
        input = getString("mouse");
    }
    if ((key_mask & ALLOW_KEYS) != 0)
    {
        if (!input.empty())
        {
            input += ", ";
        }
        input += getString("keyboard");
    }
    pDesription->setText(getString("basic_description"));
    pDesription->setTextArg("[INPUT]", input);

    bool can_ignore_masks = (key_mask & CAN_IGNORE_MASKS) != 0;
    pCheckBox->setVisible(can_ignore_masks);
    pCheckBox->setValue(false);
}

BOOL LLSetKeyBindDialog::handleKeyHere(KEY key, MASK mask)
{
    if ((key == 'Q' && mask == MASK_CONTROL)
        || key == KEY_ESCAPE)
    {
        closeFloater();
        return TRUE;
    }

    if (key == KEY_DELETE)
    {
        setKeyBind(CLICK_NONE, KEY_NONE, MASK_NONE, false);
        closeFloater();
        return FALSE;
    }

    // forbidden keys
    if (key == KEY_NONE
        || key == KEY_RETURN
        || key == KEY_BACKSPACE)
    {
        return FALSE;
    }

    if ((mKeyFilterMask & ALLOW_MASKS) == 0
        && (key == KEY_CONTROL || key == KEY_SHIFT || key == KEY_ALT))
    {
        // mask by themself are not allowed
        return FALSE;
    }
    else if ((mKeyFilterMask & ALLOW_KEYS) == 0)
    {
        // basic keys not allowed
        return FALSE;
    }
    else if ((mKeyFilterMask & ALLOW_MASK_KEYS) == 0 && mask != 0)
    {
        // masked keys not allowed
        return FALSE;
    }

    if (LLKeyConflictHandler::isReservedByMenu(key, mask))
    {
        pDesription->setText(getString("reserved_by_menu"));
        pDesription->setTextArg("[KEYSTR]", LLKeyboard::stringFromAccelerator(mask,key));
        return TRUE;
    }

    setKeyBind(CLICK_NONE, key, mask, pCheckBox->getValue().asBoolean());
    closeFloater();
    return TRUE;
}

BOOL LLSetKeyBindDialog::handleAnyMouseClick(S32 x, S32 y, MASK mask, EMouseClickType clicktype, BOOL down)
{
    BOOL result = FALSE;
    if (!pParent)
    {
        // we already processed 'down' event, this is 'up', consume
        closeFloater();
        result = TRUE;
    }
    if (!result && clicktype == CLICK_LEFT)
    {
        // try handling buttons first
        if (down)
        {
            result = LLView::handleMouseDown(x, y, mask);
        }
        else
        {
            result = LLView::handleMouseUp(x, y, mask);
        }
        if (result)
        {
            setFocus(TRUE);
            gFocusMgr.setKeystrokesOnly(TRUE);
        }
        // ignore selection related combinations
        else if (down && (mask & (MASK_SHIFT | MASK_CONTROL)) == 0)
        {
            // this can be a double click, wait a bit;
            if (!pUpdater)
            {
                // Note: default doubleclick time is 500ms, but can stretch up to 5s
                pUpdater = new Updater(boost::bind(&onClickTimeout, this, _1), 0.7f, mask);
                result = TRUE;
            }
        }
    }

    if (!result
        && (clicktype != CLICK_LEFT) // subcases were handled above
        && ((mKeyFilterMask & ALLOW_MOUSE) != 0)
        && (clicktype != CLICK_RIGHT || mask != 0) // reassigning menu button is not supported
        && ((mKeyFilterMask & ALLOW_MASK_MOUSE) != 0 || mask == 0)) // reserved for selection
    {
        setKeyBind(clicktype, KEY_NONE, mask, pCheckBox->getValue().asBoolean());
        result = TRUE;
        if (!down)
        {
            // wait for 'up' event before closing
            // alternative: set pUpdater
            closeFloater();
        }
    }

    return result;
}

//static
void LLSetKeyBindDialog::onCancel(void* user_data)
{
    LLSetKeyBindDialog* self = (LLSetKeyBindDialog*)user_data;
    self->closeFloater();
}

//static
void LLSetKeyBindDialog::onBlank(void* user_data)
{
    LLSetKeyBindDialog* self = (LLSetKeyBindDialog*)user_data;
    // tmp needs 'no key' button
    self->setKeyBind(CLICK_NONE, KEY_NONE, MASK_NONE, false);
    self->closeFloater();
}

//static
void LLSetKeyBindDialog::onDefault(void* user_data)
{
    LLSetKeyBindDialog* self = (LLSetKeyBindDialog*)user_data;
    if (self->pParent)
    {
        self->pParent->onDefaultKeyBind();
        self->pParent = NULL;
    }
    self->closeFloater();
}

//static
void LLSetKeyBindDialog::onClickTimeout(void* user_data, MASK mask)
{
    LLSetKeyBindDialog* self = (LLSetKeyBindDialog*)user_data;

    // timer will delete itself after timeout
    self->pUpdater = NULL;

    self->setKeyBind(CLICK_LEFT, KEY_NONE, mask, self->pCheckBox->getValue().asBoolean());
    self->closeFloater();
}

void LLSetKeyBindDialog::setKeyBind(EMouseClickType click, KEY key, MASK mask, bool ignore)
{
    if (pParent)
    {
        pParent->onSetKeyBind(click, key, mask, ignore);
        pParent = NULL;
    }
}

