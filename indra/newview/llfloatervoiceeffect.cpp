/** 
 * @file llfloatervoiceeffect.cpp
 * @author Aimee
 * @brief Selection and preview of voice effect.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatervoiceeffect.h"

#include "llscrolllistctrl.h"
#include "lltrans.h"
#include "llweb.h"

LLFloaterVoiceEffect::LLFloaterVoiceEffect(const LLSD& key)
	: LLFloater(key)
{
	mCommitCallbackRegistrar.add("VoiceEffect.Record",	boost::bind(&LLFloaterVoiceEffect::onClickRecord, this));
	mCommitCallbackRegistrar.add("VoiceEffect.Play",	boost::bind(&LLFloaterVoiceEffect::onClickPlay, this));
	mCommitCallbackRegistrar.add("VoiceEffect.Stop",	boost::bind(&LLFloaterVoiceEffect::onClickStop, this));
//	mCommitCallbackRegistrar.add("VoiceEffect.Activate", boost::bind(&LLFloaterVoiceEffect::onClickActivate, this));
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
	getChild<LLButton>("record_btn")->setFocus(true);
	getChild<LLUICtrl>("voice_morphing_link")->setTextArg("[URL]", LLTrans::getString("voice_morphing_url"));

	mVoiceEffectList = getChild<LLScrollListCtrl>("voice_effect_list");
	if (mVoiceEffectList)
	{
		mVoiceEffectList->setCommitCallback(boost::bind(&LLFloaterVoiceEffect::onClickPlay, this));
//		mVoiceEffectList->setDoubleClickCallback(boost::bind(&LLFloaterVoiceEffect::onClickActivate, this));
	}

	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		effect_interface->addObserver(this);

		// Disconnect from the current voice channel ready to record a voice sample for previewing
		effect_interface->enablePreviewBuffer(true);
	}

	refreshEffectList();
	updateControls();

	return TRUE;
}

// virtual
void LLFloaterVoiceEffect::onClose(bool app_quitting)
{
	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		effect_interface->enablePreviewBuffer(false);
	}
}

void LLFloaterVoiceEffect::refreshEffectList()
{
	if (!mVoiceEffectList)
	{
		return;
	}

	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (!effect_interface)
	{
		mVoiceEffectList->setEnabled(false);
		return;
	}

	LL_DEBUGS("Voice")<< "Rebuilding Voice Morph list."<< LL_ENDL;

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
		// Add the "No Voice Morph" entry
		LLSD element;

		element["id"] = LLUUID::null;
		element["columns"][NAME_COLUMN]["column"] = "name";
		element["columns"][NAME_COLUMN]["value"] = getString("no_voice_effect");
		element["columns"][NAME_COLUMN]["font"]["style"] = "BOLD";

		LLScrollListItem* sl_item = mVoiceEffectList->addElement(element, ADD_BOTTOM);
		// *HACK: Copied from llfloatergesture.cpp : ["font"]["style"] does not affect font style :(
		if(sl_item)
		{
			((LLScrollListText*)sl_item->getColumn(0))->setFontStyle(LLFontGL::BOLD);
		}
	}

	// Add each Voice Morph template, if there are any (template list includes all usable effects)
	const voice_effect_list_t& template_list = effect_interface->getVoiceEffectTemplateList();
	if (!template_list.empty())
	{
		for (voice_effect_list_t::const_iterator it = template_list.begin(); it != template_list.end(); ++it)
		{
			const LLUUID& effect_id = it->second;

			std::string localized_effect = "effect_" + it->first;
			std::string effect_name = hasString(localized_effect) ? getString(localized_effect) : it->first;  // XML contains localized effects names

			LLSD effect_properties = effect_interface->getVoiceEffectProperties(effect_id);

			// Tag the active effect.
			if (effect_id == LLVoiceClient::instance().getVoiceEffectDefault())
			{
				effect_name += " " + getString("active_voice_effect");
			}

			// Tag available effects that are new this session
			if (effect_properties["is_new"].asBoolean())
			{
				effect_name += " " + getString("new_voice_effect");
			}

			LLDate expiry_date = effect_properties["expiry_date"].asDate();
			bool is_template_only = effect_properties["template_only"].asBoolean();

			std::string font_style = "NORMAL";
			if (!is_template_only)
			{
				font_style = "BOLD";
			}

			LLSD element;
			element["id"] = effect_id;

			element["columns"][NAME_COLUMN]["column"] = "name";
			element["columns"][NAME_COLUMN]["value"] = effect_name;
			element["columns"][NAME_COLUMN]["font"]["style"] = font_style;

			element["columns"][1]["column"] = "expires";
			if (!is_template_only)
			{
				element["columns"][DATE_COLUMN]["value"] = expiry_date;
				element["columns"][DATE_COLUMN]["type"] = "date";
			}
			else {
				element["columns"][DATE_COLUMN]["value"] = getString("unsubscribed_voice_effect");
			}
//			element["columns"][DATE_COLUMN]["font"]["style"] = "NORMAL";

			LLScrollListItem* sl_item = mVoiceEffectList->addElement(element, ADD_BOTTOM);
			// *HACK: Copied from llfloatergesture.cpp : ["font"]["style"] does not affect font style :(
			if(sl_item)
			{
				LLFontGL::StyleFlags style = is_template_only ? LLFontGL::NORMAL : LLFontGL::BOLD;
				LLScrollListText* slt = dynamic_cast<LLScrollListText*>(sl_item->getColumn(0));
				llassert(slt);
				if (slt)
				{
					slt->setFontStyle(style);
				}
			}
		}
	}

	// Re-select items that were selected before, and restore the scroll position
	for(uuid_vec_t::iterator it = selected_items.begin(); it != selected_items.end(); it++)
	{
		mVoiceEffectList->selectByID(*it);
	}
	mVoiceEffectList->setScrollPos(scroll_pos);
	mVoiceEffectList->setEnabled(true);
}

void LLFloaterVoiceEffect::updateControls()
{
	bool recording = false;

	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		recording = effect_interface->isPreviewRecording();
	}

	getChild<LLButton>("record_btn")->setVisible(!recording);
	getChild<LLButton>("record_stop_btn")->setVisible(recording);
}

// virtual
void LLFloaterVoiceEffect::onVoiceEffectChanged(bool effect_list_updated)
{
	if (effect_list_updated)
	{
		refreshEffectList();
	}
	updateControls();
}

void LLFloaterVoiceEffect::onClickRecord()
{
	LL_DEBUGS("Voice") << "Record clicked" << LL_ENDL;
	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		effect_interface->recordPreviewBuffer();
	}
	updateControls();
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
	if (effect_interface)
	{
		effect_interface->playPreviewBuffer(effect_id);
	}
	updateControls();
}

void LLFloaterVoiceEffect::onClickStop()
{
	LL_DEBUGS("Voice") << "Stop clicked" << LL_ENDL;
	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
	if (effect_interface)
	{
		effect_interface->stopPreviewBuffer();
	}
	updateControls();
}

//void LLFloaterVoiceEffect::onClickActivate()
//{
//	LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
//	if (effect_interface && mVoiceEffectList)
//	{
//		effect_interface->setVoiceEffect(mVoiceEffectList->getCurrentID());
//	}
//}

