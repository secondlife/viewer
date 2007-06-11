/** 
 * @file llpreviewsound.cpp
 * @brief LLPreviewSound class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpreviewsound.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llinventory.h"
#include "llinventoryview.h"
#include "audioengine.h"
#include "llviewermessage.h"  // send_guid_sound_trigger
#include "llagent.h"          // gAgent
#include "lllineeditor.h"
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
				
		gAudiop->triggerSound(item->getAssetUUID(), gAgent.getID(), SOUND_GAIN, lpos_global);
	}
}
