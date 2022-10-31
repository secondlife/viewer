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

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lleventtimer.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llkeyconflict.h"
#include "llviewercontrol.h"

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

bool LLSetKeyBindDialog::sRecordKeys = false;

LLSetKeyBindDialog::LLSetKeyBindDialog(const LLSD& key)
    : LLModalDialog(key),
    pParent(NULL),
    mKeyFilterMask(DEFAULT_KEY_FILTER),
    pUpdater(NULL),
    mLastMaskKey(0),
    mContextConeOpacity(0.f),
    mContextConeInAlpha(0.f),
    mContextConeOutAlpha(0.f),
    mContextConeFadeTime(0.f)
{
    mContextConeInAlpha = gSavedSettings.getF32("ContextConeInAlpha");
    mContextConeOutAlpha = gSavedSettings.getF32("ContextConeOutAlpha");
    mContextConeFadeTime = gSavedSettings.getF32("ContextConeFadeTime");
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

    pCheckBox = getChild<LLCheckBoxCtrl>("apply_all");
    pDescription = getChild<LLTextBase>("description");

    gFocusMgr.setKeystrokesOnly(TRUE);

    return TRUE;
}

//virtual
void LLSetKeyBindDialog::onOpen(const LLSD& data)
{
     sRecordKeys = true;
     LLModalDialog::onOpen(data);
}

//virtual
void LLSetKeyBindDialog::onClose(bool app_quiting)
{
    sRecordKeys = false;
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

void LLSetKeyBindDialog::drawFrustum()
{
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, mFrustumOrigin.get(), mContextConeFadeTime, mContextConeInAlpha, mContextConeOutAlpha);
}

//virtual
void LLSetKeyBindDialog::draw()
{
    drawFrustum();
    LLModalDialog::draw();
}

void LLSetKeyBindDialog::setParent(LLKeyBindResponderInterface* parent, LLView* frustum_origin, U32 key_mask)
{
    pParent = parent;
    mFrustumOrigin = frustum_origin->getHandle();
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
    pDescription->setText(getString("basic_description"));
    pDescription->setTextArg("[INPUT]", input);
}

// static
bool LLSetKeyBindDialog::recordKey(KEY key, MASK mask, BOOL down)
{
    if (sRecordKeys)
    {
        LLSetKeyBindDialog* dialog = LLFloaterReg::getTypedInstance<LLSetKeyBindDialog>("keybind_dialog", LLSD());
        if (dialog && dialog->getVisible())
        {
            return dialog->recordAndHandleKey(key, mask, down);
        }
        else
        {
            LL_WARNS() << "Key recording was set despite no open dialog" << LL_ENDL;
            sRecordKeys = false;
        }
    }
    return false;
}

bool LLSetKeyBindDialog::recordAndHandleKey(KEY key, MASK mask, BOOL down)
{
    if ((key == 'Q' && mask == MASK_CONTROL)
        || key == KEY_ESCAPE)
    {
        sRecordKeys = false;
        closeFloater();
        return true;
    }

    if (key == KEY_DELETE)
    {
        setKeyBind(CLICK_NONE, KEY_NONE, MASK_NONE, false);
        sRecordKeys = false;
        closeFloater();
        return false;
    }

    // forbidden keys
    if (key == KEY_NONE
        || key == KEY_RETURN
        || key == KEY_BACKSPACE)
    {
        return false;
    }

    if (key == KEY_CONTROL || key == KEY_SHIFT || key == KEY_ALT)
    {
        // Mask keys get special treatment
        if ((mKeyFilterMask & ALLOW_MASKS) == 0)
        {
            // Masks by themself are not allowed
            return false;
        }
        if (down == TRUE)
        {
            // Most keys are handled on 'down' event because menu is handled on 'down'
            // masks are exceptions to let other keys be handled
            mLastMaskKey = key;
            return false;
        }
        if (mLastMaskKey != key)
        {
            // This was mask+key combination that got rejected, don't handle mask's key
            // Or user did something like: press shift, press ctrl, release shift
            return false;
        }
        // Mask up event often generates things like 'shift key + shift mask', filter it out.
        if (key == KEY_CONTROL)
        {
            mask &= ~MASK_CONTROL;
        }
        if (key == KEY_SHIFT)
        {
            mask &= ~MASK_SHIFT;
        }
        if (key == KEY_ALT)
        {
            mask &= ~MASK_ALT;
        }
    }
    if ((mKeyFilterMask & ALLOW_KEYS) == 0)
    {
        // basic keys not allowed
        return false;
    }
    else if ((mKeyFilterMask & ALLOW_MASK_KEYS) == 0 && mask != 0)
    {
        // masked keys not allowed
        return false;
    }

    if (LLKeyConflictHandler::isReservedByMenu(key, mask))
    {
        pDescription->setText(getString("reserved_by_menu"));
        pDescription->setTextArg("[KEYSTR]", LLKeyboard::stringFromAccelerator(mask,key));
        mLastMaskKey = 0;
        return true;
    }

    setKeyBind(CLICK_NONE, key, mask, pCheckBox->getValue().asBoolean());
    // Note/Todo: To warranty zero interference we should also consume
    // an 'up' event if we recorded on 'down', not just close floater
    // on first recorded combination.
    sRecordKeys = false;
    closeFloater();
    return true;
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
        self->pParent->onDefaultKeyBind(self->pCheckBox->getValue().asBoolean());
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

void LLSetKeyBindDialog::setKeyBind(EMouseClickType click, KEY key, MASK mask, bool all_modes)
{
    if (pParent)
    {
        pParent->onSetKeyBind(click, key, mask, all_modes);
        pParent = NULL;
    }
}

