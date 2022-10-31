/** 
 * @file llpanelvoiceeffect.h
 * @author Aimee
 * @brief Panel to select Voice Effects.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_PANELVOICEEFFECT_H
#define LL_PANELVOICEEFFECT_H

#include "llpanel.h"
#include "llvoiceclient.h"

class LLComboBox;

class LLPanelVoiceEffect
    : public LLPanel
    , public LLVoiceEffectObserver
{
public:
    LOG_CLASS(LLPanelVoiceEffect);

    LLPanelVoiceEffect();
    virtual ~LLPanelVoiceEffect();

    virtual BOOL postBuild();

private:
    void onCommitVoiceEffect();
    void update(bool list_updated);

    /// Called by voice effect provider when voice effect list is changed.
    virtual void onVoiceEffectChanged(bool effect_list_updated);

    // Fixed entries in the Voice Morph list
    typedef enum e_voice_effect_combo_items
    {
        NO_VOICE_EFFECT = 0,
        PREVIEW_VOICE_EFFECTS = 1,
        GET_VOICE_EFFECTS = 2
    } EVoiceEffectComboItems;

    LLComboBox* mVoiceEffectCombo;
};


#endif //LL_PANELVOICEEFFECT_H
