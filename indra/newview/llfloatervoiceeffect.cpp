/** 
 * @file llfloatervoiceeffect.cpp
 * @brief Selection and preview of voice effect.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llfloatervoiceeffect.h"

#include "llscrolllistctrl.h"
#include "lltrans.h"

LLFloaterVoiceEffect::LLFloaterVoiceEffect(const LLSD& key)
	: LLFloater(key)
{
	mCommitCallbackRegistrar.add("VoiceEffect.Record",	boost::bind(&LLFloaterVoiceEffect::onClickRecord, this));
	mCommitCallbackRegistrar.add("VoiceEffect.Play",	boost::bind(&LLFloaterVoiceEffect::onClickPlay, this));
}

// virtual
LLFloaterVoiceEffect::~LLFloaterVoiceEffect()
{
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
BOOL LLFloaterVoiceEffect::postBuild()
{
	setDefaultBtn("record_btn");

	mVoiceEffectList = getChild<LLScrollListCtrl>("voice_effect_list");

	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		effect_interface->addObserver(this);
	}

	update();

	return TRUE;
}

// virtual
void LLFloaterVoiceEffect::onClose(bool app_quitting)
{
	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		effect_interface->clearPreviewBuffer();
	}
}

void LLFloaterVoiceEffect::update()
{
	if (!mVoiceEffectList)
	{
		return;
	}

	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (!effect_interface) // || !LLVoiceClient::instance().isVoiceWorking())
	{
		mVoiceEffectList->setEnabled(false);
		return;
	}

	LL_DEBUGS("Voice")<< "Rebuilding voice effect list."<< LL_ENDL;

	// Preserve selected items and scroll position
	S32 scroll_pos = mVoiceEffectList->getScrollPos();
	uuid_vec_t selected_items;
	std::vector<LLScrollListItem*> items = mVoiceEffectList->getAllSelected();
	for(std::vector<LLScrollListItem*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		selected_items.push_back((*it)->getUUID());
	}

	mVoiceEffectList->deleteAllItems();

	{
		// Add the "No Voice Effect" entry
		LLSD element;

		element["id"] = LLUUID::null;
		element["columns"][0]["column"] = "name";
		element["columns"][0]["value"] = getString("no_voice_effect");
		element["columns"][0]["font"]["name"] = "SANSSERIF";
		element["columns"][0]["font"]["style"] = "BOLD";

		LLScrollListItem* sl_item = mVoiceEffectList->addElement(element, ADD_BOTTOM);
		// *HACK: Copied from llfloatergesture.cpp : ["font"]["style"] does not affect font style :(
		if(sl_item)
		{
			((LLScrollListText*)sl_item->getColumn(0))->setFontStyle(LLFontGL::BOLD);
		}
	}

	const voice_effect_list_t& template_list = effect_interface->getVoiceEffectTemplateList();
	if (!template_list.empty())
	{
		for (voice_effect_list_t::const_iterator it = template_list.begin(); it != template_list.end(); ++it)
		{
			const LLUUID& effect_id = it->second;
			std::string effect_name = it->first;

			LLSD effect_properties = effect_interface->getVoiceEffectProperties(effect_id);
			bool is_template_only = effect_properties["template_only"].asBoolean();
			bool is_new = effect_properties["is_new"].asBoolean();
			std::string expiry_date = effect_properties["expiry_date"].asString();

			std::string font_style = "NORMAL";
			if (!is_template_only)
			{
				font_style = "BOLD";
			}
			LLSD element;
			element["id"] = effect_id;

			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = effect_name;
			element["columns"][0]["font"]["name"] = "SANSSERIF";
			element["columns"][0]["font"]["style"] = font_style;
			element["columns"][1]["column"] = "new";
			element["columns"][1]["value"] = is_new ? getString("new_voice_effect") : "";
			element["columns"][1]["font"]["name"] = "SANSSERIF";
			element["columns"][1]["font"]["style"] = font_style;

			element["columns"][2]["column"] = "expires";
			element["columns"][2]["value"] = !is_template_only ? expiry_date : "";
			element["columns"][2]["font"]["name"] = "SANSSERIF";
			element["columns"][2]["font"]["style"] = font_style;

			LLScrollListItem* sl_item = mVoiceEffectList->addElement(element, ADD_BOTTOM);
			// *HACK: Copied from llfloatergesture.cpp : ["font"]["style"] does not affect font style :(
			if(sl_item)
			{
				LLFontGL::StyleFlags style = is_template_only ? LLFontGL::NORMAL : LLFontGL::BOLD;
				((LLScrollListText*)sl_item->getColumn(0))->setFontStyle(style);
			}
		}
	}

	// Re-select items that were selected before, and restore the scroll position
	for(uuid_vec_t::iterator it = selected_items.begin(); it != selected_items.end(); it++)
	{
		mVoiceEffectList->selectByID(*it);
	}
	mVoiceEffectList->setScrollPos(scroll_pos);

	mVoiceEffectList->setValue(effect_interface->getVoiceEffect());
	mVoiceEffectList->setEnabled(true);

	// Update button states
	// *TODO: Should separate this from rebuilding the effects list, to avoid rebuilding it unnecessarily
	bool recording = effect_interface->isPreviewRecording();
	getChild<LLButton>("record_btn")->setVisible(!recording);
	getChild<LLButton>("record_stop_btn")->setVisible(recording);

	getChild<LLButton>("play_btn")->setEnabled(effect_interface->isPreviewReady());
	bool playing = effect_interface->isPreviewPlaying();
	getChild<LLButton>("play_btn")->setVisible(!playing);
	getChild<LLButton>("play_stop_btn")->setVisible(playing);
}

// virtual
void LLFloaterVoiceEffect::onVoiceEffectChanged(bool new_effects)
{
	update();
}

void LLFloaterVoiceEffect::onClickRecord()
{
	LL_DEBUGS("Voice") << "Record clicked" << LL_ENDL;
	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface) // && LLVoiceClient::instance().isVoiceWorking())
	{
		bool record = !effect_interface->isPreviewRecording();
		effect_interface->recordPreviewBuffer(record);
		getChild<LLButton>("record_btn")->setVisible(!record);
		getChild<LLButton>("record_stop_btn")->setVisible(record);
	}
}

void LLFloaterVoiceEffect::onClickPlay()
{
	LL_DEBUGS("Voice") << "Play clicked" << LL_ENDL;
	if (!mVoiceEffectList)
	{
		return;
	}

	const LLUUID& effect_id = mVoiceEffectList->getCurrentID();

	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface) // && LLVoiceClient::instance().isVoiceWorking())
	{
		bool play = !effect_interface->isPreviewPlaying();
		effect_interface->playPreviewBuffer(play, effect_id);
		getChild<LLButton>("play_btn")->setVisible(!play);
		getChild<LLButton>("play_stop_btn")->setVisible(play);
	}
}
