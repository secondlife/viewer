/** 
 * @file llpanelvoiceeffect.cpp
 * @author Aimee
 * @brief Panel to select Voice Morphs.
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

#include "llviewerprecompiledheaders.h"

#include "llpanelvoiceeffect.h"

#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llpanel.h"
#include "lltrans.h"
#include "lltransientfloatermgr.h"
#include "llvoiceclient.h"

static LLRegisterPanelClassWrapper<LLPanelVoiceEffect> t_panel_voice_effect("panel_voice_effect");

LLPanelVoiceEffect::LLPanelVoiceEffect()
	: mVoiceEffectCombo(NULL)
{
	mCommitCallbackRegistrar.add("Voice.CommitVoiceEffect", boost::bind(&LLPanelVoiceEffect::onCommitVoiceEffect, this));
}

LLPanelVoiceEffect::~LLPanelVoiceEffect()
{
	LLView* combo_list_view = mVoiceEffectCombo->getChildView("ComboBox");
	LLTransientFloaterMgr::getInstance()->removeControlView(combo_list_view);

	if(LLVoiceClient::instanceExists())
	{
		LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
		if (effect_interface)
		{
			effect_interface->removeObserver(this);
		}
	}
}

// virtual
BOOL LLPanelVoiceEffect::postBuild()
{
	mVoiceEffectCombo = getChild<LLComboBox>("voice_effect");

	// Need to tell LLTransientFloaterMgr about the combo list, otherwise it can't
	// be clicked while in a docked floater as it extends outside the floater area.
	LLView* combo_list_view = mVoiceEffectCombo->getChildView("ComboBox");
	LLTransientFloaterMgr::getInstance()->addControlView(combo_list_view);

	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		effect_interface->addObserver(this);
	}

	update(true);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
/// PRIVATE SECTION
//////////////////////////////////////////////////////////////////////////

void LLPanelVoiceEffect::onCommitVoiceEffect()
{
	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (!effect_interface)
	{
		mVoiceEffectCombo->setEnabled(false);
		return;
	}

	LLSD value = mVoiceEffectCombo->getValue();
	if (value.asInteger() == PREVIEW_VOICE_EFFECTS)
	{
		// Open the Voice Morph preview floater
		LLFloaterReg::showInstance("voice_effect");
	}
	else if (value.asInteger() == GET_VOICE_EFFECTS)
	{
		// Open the voice morphing info web page
		LLWeb::loadURL(LLTrans::getString("voice_morphing_url"));
	}
	else
	{
		effect_interface->setVoiceEffect(value.asUUID());
	}

	mVoiceEffectCombo->setValue(effect_interface->getVoiceEffect());
}

// virtual
void LLPanelVoiceEffect::onVoiceEffectChanged(bool effect_list_updated)
{
	update(effect_list_updated);
}

void LLPanelVoiceEffect::update(bool list_updated)
{
	if (mVoiceEffectCombo)
	{
		LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
		llassert(effect_interface);
		if (!effect_interface) return;
		if (list_updated)
		{
			// Add the default "No Voice Morph" entry.
			mVoiceEffectCombo->removeall();
			mVoiceEffectCombo->add(getString("no_voice_effect"), LLUUID::null);
			mVoiceEffectCombo->addSeparator();

			// Add entries for each Voice Morph.
			const voice_effect_list_t& effect_list = effect_interface->getVoiceEffectList();
			if (!effect_list.empty())
			{
				for (voice_effect_list_t::const_iterator it = effect_list.begin(); it != effect_list.end(); ++it)
				{
					mVoiceEffectCombo->add(it->first, it->second, ADD_BOTTOM);
				}

				mVoiceEffectCombo->addSeparator();
			}

			// Add the fixed entries to go to the preview floater or marketing page.
			mVoiceEffectCombo->add(getString("preview_voice_effects"), PREVIEW_VOICE_EFFECTS);
			mVoiceEffectCombo->add(getString("get_voice_effects"), GET_VOICE_EFFECTS);
		}

		if (effect_interface && LLVoiceClient::instance().isVoiceWorking())
		{
			// Select the current Voice Morph.
			mVoiceEffectCombo->setValue(effect_interface->getVoiceEffect());
			mVoiceEffectCombo->setEnabled(true);
		}
		else
		{
			// If voice isn't working or Voice Effects are not supported disable the control.
			mVoiceEffectCombo->setValue(LLUUID::null);
			mVoiceEffectCombo->setEnabled(false);
		}
	}
}
