/**
 * @file llviewerinput.h
 * @brief LLViewerInput class header file
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLVIEWERINPUT_H
#define LL_LLVIEWERINPUT_H

#include "llkeyboard.h" // For EKeystate

const S32 MAX_KEY_BINDINGS = 128; // was 60
const S32 keybindings_xml_version = 1;
const std::string script_mouse_handler_name = "script_trigger_lbutton";

class LLWindow;

class LLNamedFunction
{
public:
    LLNamedFunction() : mFunction(NULL) { };
    ~LLNamedFunction() { };

    std::string mName;
    LLKeyFunc   mFunction;
};

class LLKeyboardBinding
{
public:
    KEY             mKey;
    MASK            mMask;

    LLKeyFunc       mFunction;
    std::string     mFunctionName;
};

class LLMouseBinding
{
public:
    EMouseClickType mMouse;
    MASK            mMask;

    LLKeyFunc       mFunction;
    std::string     mFunctionName;
};


typedef enum e_keyboard_mode
{
    MODE_FIRST_PERSON,
    MODE_THIRD_PERSON,
    MODE_EDIT_AVATAR,
    MODE_SITTING,
    MODE_COUNT
} EKeyboardMode;

class LLViewerInput : public LLKeyBindingToStringHandler
{
public:
    struct KeyBinding : public LLInitParam::Block<KeyBinding>
    {
        Mandatory<std::string>  key,
                                mask,
                                command;
        Optional<std::string>   mouse; // Note, not mandatory for the sake of backward campatibility with keys.xml

        KeyBinding();
    };

    struct KeyMode : public LLInitParam::Block<KeyMode>
    {
        Multiple<KeyBinding>        bindings;

        KeyMode();
    };

    struct Keys : public LLInitParam::Block<Keys>
    {
        Optional<KeyMode>   first_person,
                            third_person,
                            sitting,
                            edit_avatar;
        Optional<S32> xml_version; // 'xml', because 'version' appears to be reserved
        Keys();
    };

    LLViewerInput();
    virtual ~LLViewerInput();

    bool            handleKey(KEY key, MASK mask, bool repeated);
    bool            handleKeyUp(KEY key, MASK mask);

    // Handle 'global' keybindings that do not consume event,
    // yet need to be processed early
    // Example: we want voice to toggle even if some floater is focused
    bool            handleGlobalBindsKeyDown(KEY key, MASK mask);
    bool            handleGlobalBindsKeyUp(KEY key, MASK mask);
    bool            handleGlobalBindsMouse(EMouseClickType clicktype, MASK mask, bool down);

    S32             loadBindingsXML(const std::string& filename);                                       // returns number bound, 0 on error
    EKeyboardMode   getMode() const;

    static bool     modeFromString(const std::string& string, S32 *mode);           // False on failure
    static bool     mouseFromString(const std::string& string, EMouseClickType *mode);// False on failure

    bool            scanKey(KEY key,
                            bool key_down,
                            bool key_up,
                            bool key_level) const;

    // handleMouse() records state, scanMouse() goes through states, scanMouse(click) processes individual saved states after UI is done with them
    bool            handleMouse(LLWindow *window_impl, LLCoordGL pos, MASK mask, EMouseClickType clicktype, bool down);
    void            scanMouse();

    bool            isMouseBindUsed(const EMouseClickType mouse, const MASK mask, const S32 mode) const;
    bool            isLMouseHandlingDefault(const S32 mode) const { return mLMouseDefaultHandling[mode]; }

    // inherited from LLKeyBindingToStringHandler
    virtual std::string getKeyBindingAsString(const std::string& mode, const std::string& control) const override;

private:
    bool            scanKey(const std::vector<LLKeyboardBinding> &binding,
                            S32 binding_count,
                            KEY key,
                            MASK mask,
                            bool key_down,
                            bool key_up,
                            bool key_level,
                            bool repeat) const;

    enum EMouseState
    {
        MOUSE_STATE_DOWN, // key down this frame
        MOUSE_STATE_CLICK, // key went up and down in scope of same frame
        MOUSE_STATE_LEVEL, // clicked again fast, or never released
        MOUSE_STATE_UP, // went up this frame
        MOUSE_STATE_SILENT // notified about 'up', do not notify again
    };
    bool            scanMouse(EMouseClickType click, EMouseState state) const;
    bool            scanMouse(const std::vector<LLMouseBinding> &binding,
                          S32 binding_count,
                          EMouseClickType mouse,
                          MASK mask,
                          EMouseState state,
                          bool ignore_additional_masks) const;

    S32             loadBindingMode(const LLViewerInput::KeyMode& keymode, S32 mode);
    bool            bindKey(const S32 mode, const KEY key, const MASK mask, const std::string& function_name);
    bool            bindMouse(const S32 mode, const EMouseClickType mouse, const MASK mask, const std::string& function_name);
    void            resetBindings();

    // Hold all the ugly stuff torn out to make LLKeyboard non-viewer-specific here

    // TODO: at some point it is better to remake this, especially keyaboard part
    // would be much better to send to functions actual state of the button than
    // to send what we think function wants based on collection of bools (mKeyRepeated, mKeyLevel, mKeyDown)
    std::vector<LLKeyboardBinding>  mKeyBindings[MODE_COUNT];
    std::vector<LLMouseBinding>     mMouseBindings[MODE_COUNT];
    bool                            mLMouseDefaultHandling[MODE_COUNT]; // Due to having special priority

    // keybindings that do not consume event and are handled earlier, before floaters
    std::vector<LLKeyboardBinding>  mGlobalKeyBindings[MODE_COUNT];
    std::vector<LLMouseBinding>     mGlobalMouseBindings[MODE_COUNT];

    typedef std::map<U32, U32> key_remap_t;
    key_remap_t     mRemapKeys[MODE_COUNT];
    std::set<KEY>   mKeysSkippedByUI;
    bool            mKeyHandledByUI[KEY_COUNT];     // key processed successfully by UI

    // This is indentical to what llkeyboard does (mKeyRepeated, mKeyLevel, mKeyDown e t c),
    // just instead of remembering individually as bools,  we record state as enum
    EMouseState     mMouseLevel[CLICK_COUNT];   // records of key state
};

extern LLViewerInput gViewerInput;

#endif // LL_LLVIEWERINPUT_H
