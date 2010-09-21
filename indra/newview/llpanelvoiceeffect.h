/** 
 * @file llpanelvoiceeffect.h
 * @author Aimee
 * @brief Panel to select Voice Effects.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
