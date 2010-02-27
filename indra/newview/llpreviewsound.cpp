/** 
 * @file llpreviewsound.cpp
 * @brief LLPreviewSound class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
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
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this,"floater_preview_sound.xml", FALSE);
}

// virtual
BOOL	LLPreviewSound::postBuild()
{
	const LLInventoryItem* item = getItem();
	if (item)
	{
		childSetText("desc", item->getDescription());
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
	childSetPrevalidate("desc", &LLTextValidate::validateASCIIPrintableNoPipe);	

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
