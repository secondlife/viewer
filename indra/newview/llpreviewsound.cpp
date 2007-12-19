/** 
 * @file llpreviewsound.cpp
 * @brief LLPreviewSound class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "audioengine.h"
#include "llagent.h"          // gAgent
#include "llbutton.h"
#include "llinventory.h"
#include "llinventoryview.h"
#include "lllineeditor.h"
#include "llpreviewsound.h"
#include "llresmgr.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"  // send_guid_sound_trigger
#include "llvieweruictrlfactory.h"

extern LLAudioEngine* gAudiop;
extern LLAgent gAgent;

const F32 SOUND_GAIN = 1.0f;

LLPreviewSound::LLPreviewSound(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const LLUUID& object_uuid)	:
	LLPreview( name, rect, title, item_uuid, object_uuid)
{
	
	gUICtrlFactory->buildFloater(this,"floater_preview_sound.xml");

	childSetAction("Sound play btn",&LLPreviewSound::playSound,this);
	childSetAction("Sound audition btn",&LLPreviewSound::auditionSound,this);

	LLButton* button = LLUICtrlFactory::getButtonByName(this, "Sound play btn");
	button->setSoundFlags(LLView::SILENT);
	
	button = LLUICtrlFactory::getButtonByName(this, "Sound audition btn");
	button->setSoundFlags(LLView::SILENT);

	const LLInventoryItem* item = getItem();
	
	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);	
	
	// preload the sound
	if(item && gAudiop)
	{
		gAudiop->preloadSound(item->getAssetUUID());
	}
	
	setTitle(title);

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}

}

// static
void LLPreviewSound::playSound( void *userdata )
{
	LLPreviewSound* self = (LLPreviewSound*) userdata;
	const LLInventoryItem *item = self->getItem();

	if(item && gAudiop)
	{
		send_sound_trigger(item->getAssetUUID(), SOUND_GAIN);
	}
}

// static
void LLPreviewSound::auditionSound( void *userdata )
{
	LLPreviewSound* self = (LLPreviewSound*) userdata;
	const LLInventoryItem *item = self->getItem();

	if(item && gAudiop)
	{
		LLVector3d lpos_global = gAgent.getPositionGlobal();
		F32 volume = gSavedSettings.getBOOL("MuteSounds") ? 0.f : SOUND_GAIN * gSavedSettings.getF32("AudioLevelSFX");
		gAudiop->triggerSound(item->getAssetUUID(), gAgent.getID(), volume, lpos_global);
	}
}
