/** 
 * @file llvieweraudio.cpp
 * @brief Audio functions that used to be in viewer.cpp
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
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llvieweraudio.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llvoiceclient.h"
#include "llviewermedia.h"

/////////////////////////////////////////////////////////

void init_audio() 
{
	if (!gAudiop) 
	{
		llwarns << "Failed to create an appropriate Audio Engine" << llendl;
		return;
	}
	LLVector3d lpos_global = gAgentCamera.getCameraPositionGlobal();
	LLVector3 lpos_global_f;

	lpos_global_f.setVec(lpos_global);
					
	gAudiop->setListener(lpos_global_f,
						  LLVector3::zero,	// LLViewerCamera::getInstance()->getVelocity(),    // !!! BUG need to replace this with smoothed velocity!
						  LLViewerCamera::getInstance()->getUpAxis(),
						  LLViewerCamera::getInstance()->getAtAxis());

// load up our initial set of sounds we'll want so they're in memory and ready to be played

	BOOL mute_audio = gSavedSettings.getBOOL("MuteAudio");

	if (!mute_audio && FALSE == gSavedSettings.getBOOL("NoPreload"))
	{
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndAlert")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndBadKeystroke")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndChatFromObject")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndClick")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndClickRelease")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndHealthReductionF")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndHealthReductionM")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndIncomingChat")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndIncomingIM")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndInvApplyToObject")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndInvalidOp")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndInventoryCopyToInv")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndMoneyChangeDown")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndMoneyChangeUp")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndObjectCopyToInv")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndObjectCreate")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndObjectDelete")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndObjectRezIn")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndObjectRezOut")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndSnapshot")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndStartAutopilot")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndStartFollowpilot")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndStartIM")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndStopAutopilot")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndTeleportOut")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndTextureApplyToObject")));
		//gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndTextureCopyToInv")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndTyping")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndWindowClose")));
		gAudiop->preloadSound(LLUUID(gSavedSettings.getString("UISndWindowOpen")));
	}

	audio_update_volume(true);
}

void audio_update_volume(bool force_update)
{
	F32 master_volume = gSavedSettings.getF32("AudioLevelMaster");
	BOOL mute_audio = gSavedSettings.getBOOL("MuteAudio");
	if (!gViewerWindow->getActive() && (gSavedSettings.getBOOL("MuteWhenMinimized")))
	{
		mute_audio = TRUE;
	}
	F32 mute_volume = mute_audio ? 0.0f : 1.0f;

	// Sound Effects
	if (gAudiop) 
	{
		gAudiop->setMasterGain ( master_volume );

		gAudiop->setDopplerFactor(gSavedSettings.getF32("AudioLevelDoppler"));
		gAudiop->setRolloffFactor(gSavedSettings.getF32("AudioLevelRolloff"));
#ifdef kAUDIO_ENABLE_WIND
		gAudiop->enableWind(!mute_audio);
#endif

		gAudiop->setMuted(mute_audio);
		
		if (force_update)
		{
			audio_update_wind(true);
		}

		// handle secondary gains
		gAudiop->setSecondaryGain(LLAudioEngine::AUDIO_TYPE_SFX,
								  gSavedSettings.getBOOL("MuteSounds") ? 0.f : gSavedSettings.getF32("AudioLevelSFX"));
		gAudiop->setSecondaryGain(LLAudioEngine::AUDIO_TYPE_UI,
								  gSavedSettings.getBOOL("MuteUI") ? 0.f : gSavedSettings.getF32("AudioLevelUI"));
		gAudiop->setSecondaryGain(LLAudioEngine::AUDIO_TYPE_AMBIENT,
								  gSavedSettings.getBOOL("MuteAmbient") ? 0.f : gSavedSettings.getF32("AudioLevelAmbient"));
	}

	// Streaming Music
	if (gAudiop) 
	{		
		F32 music_volume = gSavedSettings.getF32("AudioLevelMusic");
		BOOL music_muted = gSavedSettings.getBOOL("MuteMusic");
		music_volume = mute_volume * master_volume * music_volume;
		gAudiop->setInternetStreamGain ( music_muted ? 0.f : music_volume );
	
	}

	// Streaming Media
	F32 media_volume = gSavedSettings.getF32("AudioLevelMedia");
	BOOL media_muted = gSavedSettings.getBOOL("MuteMedia");
	media_volume = mute_volume * master_volume * media_volume;
	LLViewerMedia::setVolume( media_muted ? 0.0f : media_volume );

	// Voice
	if (gVoiceClient)
	{
		F32 voice_volume = gSavedSettings.getF32("AudioLevelVoice");
		voice_volume = mute_volume * master_volume * voice_volume;
		BOOL voice_mute = gSavedSettings.getBOOL("MuteVoice");
		gVoiceClient->setVoiceVolume(voice_mute ? 0.f : voice_volume);
		gVoiceClient->setMicGain(voice_mute ? 0.f : gSavedSettings.getF32("AudioLevelMic"));

		if (!gViewerWindow->getActive() && (gSavedSettings.getBOOL("MuteWhenMinimized")))
		{
			gVoiceClient->setMuteMic(true);
		}
		else
		{
			gVoiceClient->setMuteMic(false);
		}
	}
}

void audio_update_listener()
{
	if (gAudiop)
	{
		// update listener position because agent has moved	
		LLVector3d lpos_global = gAgentCamera.getCameraPositionGlobal();		
		LLVector3 lpos_global_f;
		lpos_global_f.setVec(lpos_global);
	
		gAudiop->setListener(lpos_global_f,
							 // LLViewerCamera::getInstance()VelocitySmoothed, 
							 // LLVector3::zero,	
							 gAgent.getVelocity(),    // !!! *TODO: need to replace this with smoothed velocity!
							 LLViewerCamera::getInstance()->getUpAxis(),
							 LLViewerCamera::getInstance()->getAtAxis());
	}
}

void audio_update_wind(bool force_update)
{
#ifdef kAUDIO_ENABLE_WIND
	//
	//  Extract height above water to modulate filter by whether above/below water 
	// 
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		static F32 last_camera_water_height = -1000.f;
		LLVector3 camera_pos = gAgentCamera.getCameraPositionAgent();
		F32 camera_water_height = camera_pos.mV[VZ] - region->getWaterHeight();
		
		//
		//  Don't update rolloff factor unless water surface has been crossed
		//
		if (force_update || (last_camera_water_height * camera_water_height) < 0.f)
		{
            static LLUICachedControl<F32> rolloff("AudioLevelRolloff", 1.0f);
			if (camera_water_height < 0.f)
			{
				gAudiop->setRolloffFactor(rolloff * LL_ROLLOFF_MULTIPLIER_UNDER_WATER);
			}
			else 
			{
				gAudiop->setRolloffFactor(rolloff);
			}
		}
        
        // Scale down the contribution of weather-simulation wind to the
        // ambient wind noise.  Wind velocity averages 3.5 m/s, with gusts to 7 m/s
        // whereas steady-state avatar walk velocity is only 3.2 m/s.
        // Without this the world feels desolate on first login when you are
        // standing still.
        static LLUICachedControl<F32> wind_level("AudioLevelWind", 0.5f);
        LLVector3 scaled_wind_vec = gWindVec * wind_level;
        
        // Mix in the avatar's motion, subtract because when you walk north,
        // the apparent wind moves south.
        LLVector3 final_wind_vec = scaled_wind_vec - gAgent.getVelocity();
        
		// rotate the wind vector to be listener (agent) relative
		gRelativeWindVec = gAgent.getFrameAgent().rotateToLocal( final_wind_vec );

		// don't use the setter setMaxWindGain() because we don't
		// want to screw up the fade-in on startup by setting actual source gain
		// outside the fade-in.
		F32 master_volume  = gSavedSettings.getBOOL("MuteAudio") ? 0.f : gSavedSettings.getF32("AudioLevelMaster");
		F32 ambient_volume = gSavedSettings.getBOOL("MuteAmbient") ? 0.f : gSavedSettings.getF32("AudioLevelAmbient");
		F32 max_wind_volume = master_volume * ambient_volume;

		const F32 WIND_SOUND_TRANSITION_TIME = 2.f;
		// amount to change volume this frame
		F32 volume_delta = (LLFrameTimer::getFrameDeltaTimeF32() / WIND_SOUND_TRANSITION_TIME) * max_wind_volume;
		if (force_update) 
		{
			// initialize wind volume (force_update) by using large volume_delta
			// which is sufficient to completely turn off or turn on wind noise
			volume_delta = 1.f;
		}

		// mute wind when not flying
		if (gAgent.getFlying())
		{
			// volume increases by volume_delta, up to no more than max_wind_volume
			gAudiop->mMaxWindGain = llmin(gAudiop->mMaxWindGain + volume_delta, max_wind_volume);
		}
		else
		{
			// volume decreases by volume_delta, down to no less than 0
			gAudiop->mMaxWindGain = llmax(gAudiop->mMaxWindGain - volume_delta, 0.f);
		}
		
		last_camera_water_height = camera_water_height;
		gAudiop->updateWind(gRelativeWindVec, camera_water_height);
	}
#endif
}
