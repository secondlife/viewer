/** 
 * @file llpreviewsound.cpp
 * @brief LLPreviewSound class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llaudioengine.h"
#include "llagent.h"          // gAgent
#include "llbutton.h"
#include "llinventory.h"
#include "lllineeditor.h"
#include "llpreviewsound.h"
#include "llresmgr.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"  // send_guid_sound_trigger
#include "lluictrlfactory.h"

extern LLAudioEngine* gAudiop;
extern LLAgent gAgent;

const F32 SOUND_GAIN = 1.0f;

LLPreviewSound::LLPreviewSound(const LLSD& key)
  : LLPreview( key )
{
}

// virtual
BOOL	LLPreviewSound::postBuild()
{
	const LLInventoryItem* item = getItem();
	if (item)
	{
		getChild<LLUICtrl>("desc")->setValue(item->getDescription());
		if (gAudiop)
			gAudiop->preloadSound(item->getAssetUUID()); // preload the sound
	}
	
	childSetAction("Sound play btn",&LLPreviewSound::playSound,this);
	childSetAction("Sound audition btn",&LLPreviewSound::auditionSound,this);

	LLButton* button = getChild<LLButton>("Sound play btn");
	button->setSoundFlags(LLView::SILENT);
	
	button = getChild<LLButton>("Sound audition btn");
	button->setSoundFlags(LLView::SILENT);

	childSetCommitCallback("desc", LLPreview::onText, this);
	getChild<LLLineEditor>("desc")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);	

	return LLPreview::postBuild();
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
		gAudiop->triggerSound(item->getAssetUUID(), gAgent.getID(), SOUND_GAIN, LLAudioEngine::AUDIO_TYPE_UI, lpos_global);
	}
}
