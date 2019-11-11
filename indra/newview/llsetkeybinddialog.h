/** 
 * @file llsetkeybinddialog.h
 * @brief LLSetKeyBindDialog class definition
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


#ifndef LL_LLSETKEYBINDDIALOG_H
#define LL_LLSETKEYBINDDIALOG_H

#include "llmodaldialog.h"

class LLCheckBoxCtrl;
class LLTextBase;

// Filters for LLSetKeyBindDialog
static const U32 ALLOW_MOUSE = 1;
static const U32 ALLOW_MASK_MOUSE = 2;
static const U32 ALLOW_KEYS = 4; //keyboard
static const U32 ALLOW_MASK_KEYS = 8;
static const U32 ALLOW_MASKS = 16;
static const U32 DEFAULT_KEY_FILTER = ALLOW_MOUSE | ALLOW_MASK_MOUSE | ALLOW_KEYS | ALLOW_MASK_KEYS;


class LLKeyBindResponderInterface
{
public:
    virtual ~LLKeyBindResponderInterface() {};

    virtual void onCancelKeyBind() = 0;
    virtual void onDefaultKeyBind(bool all_modes) = 0;
    // returns true if parent failed to set key due to key being in use
    virtual bool onSetKeyBind(EMouseClickType click, KEY key, MASK mask, bool all_modes) = 0;
};

class LLSetKeyBindDialog : public LLModalDialog
{
public:
    LLSetKeyBindDialog(const LLSD& key);
    ~LLSetKeyBindDialog();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& data);
    /*virtual*/ void onClose(bool app_quiting);
    /*virtual*/ void draw();

    void setParent(LLKeyBindResponderInterface* parent, LLView* frustum_origin, U32 key_mask = DEFAULT_KEY_FILTER);

    // Wrapper around recordAndHandleKey
    // It does not record, it handles, but handleKey function is already in use
    static bool recordKey(KEY key, MASK mask);

    BOOL handleAnyMouseClick(S32 x, S32 y, MASK mask, EMouseClickType clicktype, BOOL down);
    static void onCancel(void* user_data);
    static void onBlank(void* user_data);
    static void onDefault(void* user_data);
    static void onClickTimeout(void* user_data, MASK mask);

    class Updater;

private:
    bool recordAndHandleKey(KEY key, MASK mask);
    void setKeyBind(EMouseClickType click, KEY key, MASK mask, bool all_modes);
    LLKeyBindResponderInterface *pParent;
    LLCheckBoxCtrl *pCheckBox;
    LLTextBase *pDesription;

    U32 mKeyFilterMask;
    Updater *pUpdater;

    static bool sRecordKeys; // for convinience and not to check instance each time

    // drawFrustum
private:
    void drawFrustum();

    LLHandle <LLView>   mFrustumOrigin;
    F32                 mContextConeOpacity;
    F32                 mContextConeInAlpha;
    F32                 mContextConeOutAlpha;
    F32                 mContextConeFadeTime;
};


#endif  // LL_LLSETKEYBINDDIALOG_H
