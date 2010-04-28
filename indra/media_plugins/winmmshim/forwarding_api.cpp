/** 
 * @file forwarding_api.cpp
 * @brief forwards winmm API calls to real winmm.dll
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

#include "forwarding_api.h"

CloseDriver_type CloseDriver_orig;
OpenDriver_type OpenDriver_orig;
SendDriverMessage_type SendDriverMessage_orig;
DrvGetModuleHandle_type DrvGetModuleHandle_orig;
GetDriverModuleHandle_type GetDriverModuleHandle_orig;
DefDriverProc_type DefDriverProc_orig;
DriverCallback_type DriverCallback_orig;
mmsystemGetVersion_type mmsystemGetVersion_orig;
sndPlaySoundA_type sndPlaySoundA_orig;
sndPlaySoundW_type sndPlaySoundW_orig;
PlaySoundA_type PlaySoundA_orig;
PlaySoundW_type PlaySoundW_orig;
waveOutGetNumDevs_type waveOutGetNumDevs_orig;
waveOutGetDevCapsA_type waveOutGetDevCapsA_orig;
waveOutGetDevCapsW_type waveOutGetDevCapsW_orig;
waveOutGetVolume_type waveOutGetVolume_orig;
waveOutSetVolume_type waveOutSetVolume_orig;
waveOutGetErrorTextA_type waveOutGetErrorTextA_orig;
waveOutGetErrorTextW_type waveOutGetErrorTextW_orig;
waveOutOpen_type waveOutOpen_orig;
waveOutClose_type waveOutClose_orig;
waveOutPrepareHeader_type waveOutPrepareHeader_orig;
waveOutUnprepareHeader_type waveOutUnprepareHeader_orig;
waveOutWrite_type waveOutWrite_orig;
waveOutPause_type waveOutPause_orig;
waveOutRestart_type waveOutRestart_orig;
waveOutReset_type waveOutReset_orig;
waveOutBreakLoop_type waveOutBreakLoop_orig;
waveOutGetPosition_type waveOutGetPosition_orig;
waveOutGetPitch_type waveOutGetPitch_orig;
waveOutSetPitch_type waveOutSetPitch_orig;
waveOutGetPlaybackRate_type waveOutGetPlaybackRate_orig;
waveOutSetPlaybackRate_type waveOutSetPlaybackRate_orig;
waveOutGetID_type waveOutGetID_orig;
waveOutMessage_type waveOutMessage_orig;
waveInGetNumDevs_type waveInGetNumDevs_orig;
waveInGetDevCapsA_type waveInGetDevCapsA_orig;
waveInGetDevCapsW_type waveInGetDevCapsW_orig;
waveInGetErrorTextA_type waveInGetErrorTextA_orig;
waveInGetErrorTextW_type waveInGetErrorTextW_orig;
waveInOpen_type waveInOpen_orig;
waveInClose_type waveInClose_orig;
waveInPrepareHeader_type waveInPrepareHeader_orig;
waveInUnprepareHeader_type waveInUnprepareHeader_orig;
waveInAddBuffer_type waveInAddBuffer_orig;
waveInStart_type waveInStart_orig;
waveInStop_type waveInStop_orig;
waveInReset_type waveInReset_orig;
waveInGetPosition_type waveInGetPosition_orig;
waveInGetID_type waveInGetID_orig;
waveInMessage_type waveInMessage_orig;
midiOutGetNumDevs_type midiOutGetNumDevs_orig;
midiStreamOpen_type midiStreamOpen_orig;
midiStreamClose_type midiStreamClose_orig;
midiStreamProperty_type midiStreamProperty_orig;
midiStreamPosition_type midiStreamPosition_orig;
midiStreamOut_type midiStreamOut_orig;
midiStreamPause_type midiStreamPause_orig;
midiStreamRestart_type midiStreamRestart_orig;
midiStreamStop_type midiStreamStop_orig;
midiConnect_type midiConnect_orig;
midiDisconnect_type midiDisconnect_orig;
midiOutGetDevCapsA_type midiOutGetDevCapsA_orig;
midiOutGetDevCapsW_type midiOutGetDevCapsW_orig;
midiOutGetVolume_type midiOutGetVolume_orig;
midiOutSetVolume_type midiOutSetVolume_orig;
midiOutGetErrorTextA_type midiOutGetErrorTextA_orig;
midiOutGetErrorTextW_type midiOutGetErrorTextW_orig;
midiOutOpen_type midiOutOpen_orig;
midiOutClose_type midiOutClose_orig;
midiOutPrepareHeader_type midiOutPrepareHeader_orig;
midiOutUnprepareHeader_type midiOutUnprepareHeader_orig;
midiOutShortMsg_type midiOutShortMsg_orig;
midiOutLongMsg_type midiOutLongMsg_orig;
midiOutReset_type midiOutReset_orig;
midiOutCachePatches_type midiOutCachePatches_orig;
midiOutCacheDrumPatches_type midiOutCacheDrumPatches_orig;
midiOutGetID_type midiOutGetID_orig;
midiOutMessage_type midiOutMessage_orig;
midiInGetNumDevs_type midiInGetNumDevs_orig;
midiInGetDevCapsA_type midiInGetDevCapsA_orig;
midiInGetDevCapsW_type midiInGetDevCapsW_orig;
midiInGetErrorTextA_type midiInGetErrorTextA_orig;
midiInGetErrorTextW_type midiInGetErrorTextW_orig;
midiInOpen_type midiInOpen_orig;
midiInClose_type midiInClose_orig;
midiInPrepareHeader_type midiInPrepareHeader_orig;
midiInUnprepareHeader_type midiInUnprepareHeader_orig;
midiInAddBuffer_type midiInAddBuffer_orig;
midiInStart_type midiInStart_orig;
midiInStop_type midiInStop_orig;
midiInReset_type midiInReset_orig;
midiInGetID_type midiInGetID_orig;
midiInMessage_type midiInMessage_orig;
auxGetNumDevs_type auxGetNumDevs_orig;
auxGetDevCapsA_type auxGetDevCapsA_orig;
auxGetDevCapsW_type auxGetDevCapsW_orig;
auxSetVolume_type auxSetVolume_orig;
auxGetVolume_type auxGetVolume_orig;
auxOutMessage_type auxOutMessage_orig;
mixerGetNumDevs_type mixerGetNumDevs_orig;
mixerGetDevCapsA_type mixerGetDevCapsA_orig;
mixerGetDevCapsW_type mixerGetDevCapsW_orig;
mixerOpen_type mixerOpen_orig;
mixerClose_type mixerClose_orig;
mixerMessage_type mixerMessage_orig;
mixerGetLineInfoA_type mixerGetLineInfoA_orig;
mixerGetLineInfoW_type mixerGetLineInfoW_orig;
mixerGetID_type mixerGetID_orig;
mixerGetLineControlsA_type mixerGetLineControlsA_orig;
mixerGetLineControlsW_type mixerGetLineControlsW_orig;
mixerGetControlDetailsA_type mixerGetControlDetailsA_orig;
mixerGetControlDetailsW_type mixerGetControlDetailsW_orig;
mixerSetControlDetails_type mixerSetControlDetails_orig;
mmGetCurrentTask_type mmGetCurrentTask_orig;
mmTaskBlock_type mmTaskBlock_orig;
mmTaskCreate_type mmTaskCreate_orig;
mmTaskSignal_type mmTaskSignal_orig;
mmTaskYield_type mmTaskYield_orig;
timeGetSystemTime_type timeGetSystemTime_orig;
timeGetTime_type timeGetTime_orig;
timeSetEvent_type timeSetEvent_orig;
timeKillEvent_type timeKillEvent_orig;
timeGetDevCaps_type timeGetDevCaps_orig;
timeBeginPeriod_type timeBeginPeriod_orig;
timeEndPeriod_type timeEndPeriod_orig;
joyGetNumDevs_type joyGetNumDevs_orig;
joyConfigChanged_type joyConfigChanged_orig;
joyGetDevCapsA_type joyGetDevCapsA_orig;
joyGetDevCapsW_type joyGetDevCapsW_orig;
joyGetPos_type joyGetPos_orig;
joyGetPosEx_type joyGetPosEx_orig;
joyGetThreshold_type joyGetThreshold_orig;
joyReleaseCapture_type joyReleaseCapture_orig;
joySetCapture_type joySetCapture_orig;
joySetThreshold_type joySetThreshold_orig;
mmioStringToFOURCCA_type mmioStringToFOURCCA_orig;
mmioStringToFOURCCW_type mmioStringToFOURCCW_orig;
mmioInstallIOProcA_type mmioInstallIOProcA_orig;
mmioInstallIOProcW_type mmioInstallIOProcW_orig;
mmioOpenA_type mmioOpenA_orig;
mmioOpenW_type mmioOpenW_orig;
mmioRenameA_type mmioRenameA_orig;
mmioRenameW_type mmioRenameW_orig;
mmioClose_type mmioClose_orig;
mmioRead_type mmioRead_orig;
mmioWrite_type mmioWrite_orig;
mmioSeek_type mmioSeek_orig;
mmioGetInfo_type mmioGetInfo_orig;
mmioSetInfo_type mmioSetInfo_orig;
mmioSetBuffer_type mmioSetBuffer_orig;
mmioFlush_type mmioFlush_orig;
mmioAdvance_type mmioAdvance_orig;
mmioSendMessage_type mmioSendMessage_orig;
mmioDescend_type mmioDescend_orig;
mmioAscend_type mmioAscend_orig;
mmioCreateChunk_type mmioCreateChunk_orig;
mciSendCommandA_type mciSendCommandA_orig;
mciSendCommandW_type mciSendCommandW_orig;
mciSendStringA_type mciSendStringA_orig;
mciSendStringW_type mciSendStringW_orig;
mciGetDeviceIDA_type mciGetDeviceIDA_orig;
mciGetDeviceIDW_type mciGetDeviceIDW_orig;
mciGetDeviceIDFromElementIDA_type mciGetDeviceIDFromElementIDA_orig;
mciGetDeviceIDFromElementIDW_type mciGetDeviceIDFromElementIDW_orig;
mciGetDriverData_type mciGetDriverData_orig;
mciGetErrorStringA_type mciGetErrorStringA_orig;
mciGetErrorStringW_type mciGetErrorStringW_orig;
mciSetDriverData_type mciSetDriverData_orig;
mciDriverNotify_type mciDriverNotify_orig;
mciDriverYield_type mciDriverYield_orig;
mciSetYieldProc_type mciSetYieldProc_orig;
mciFreeCommandResource_type mciFreeCommandResource_orig;
mciGetCreatorTask_type mciGetCreatorTask_orig;
mciGetYieldProc_type mciGetYieldProc_orig;
mciLoadCommandResource_type mciLoadCommandResource_orig;
mciExecute_type mciExecute_orig;

// grab pointers to function calls in the real DLL
void init_function_pointers(HMODULE winmm_handle)
{	
	CloseDriver_orig = (CloseDriver_type)::GetProcAddress(winmm_handle, "CloseDriver");
	OpenDriver_orig = (OpenDriver_type)::GetProcAddress(winmm_handle, "OpenDriver");
	SendDriverMessage_orig = (SendDriverMessage_type)::GetProcAddress(winmm_handle, "SendDriverMessage");
	DrvGetModuleHandle_orig = (DrvGetModuleHandle_type)::GetProcAddress(winmm_handle, "DrvGetModuleHandle");
	GetDriverModuleHandle_orig = (GetDriverModuleHandle_type)::GetProcAddress(winmm_handle, "GetDriverModuleHandle");
	DefDriverProc_orig = (DefDriverProc_type)::GetProcAddress(winmm_handle, "DefDriverProc");
	DriverCallback_orig = (DriverCallback_type)::GetProcAddress(winmm_handle, "DriverCallback");
	mmsystemGetVersion_orig = (mmsystemGetVersion_type)::GetProcAddress(winmm_handle, "mmsystemGetVersion");
	sndPlaySoundA_orig = (sndPlaySoundA_type)::GetProcAddress(winmm_handle, "sndPlaySoundA");
	sndPlaySoundW_orig = (sndPlaySoundW_type)::GetProcAddress(winmm_handle, "sndPlaySoundW");
	PlaySoundA_orig = (PlaySoundA_type)::GetProcAddress(winmm_handle, "PlaySoundA");
	PlaySoundW_orig = (PlaySoundW_type)::GetProcAddress(winmm_handle, "PlaySoundW");
	waveOutGetNumDevs_orig = (waveOutGetNumDevs_type)::GetProcAddress(winmm_handle, "waveOutGetNumDevs");
	waveOutGetDevCapsA_orig = (waveOutGetDevCapsA_type)::GetProcAddress(winmm_handle, "waveOutGetDevCapsA");
	waveOutGetDevCapsW_orig = (waveOutGetDevCapsW_type)::GetProcAddress(winmm_handle, "waveOutGetDevCapsW");
	waveOutGetVolume_orig = (waveOutGetVolume_type)::GetProcAddress(winmm_handle, "waveOutGetVolume");
	waveOutSetVolume_orig = (waveOutSetVolume_type)::GetProcAddress(winmm_handle, "waveOutSetVolume");
	waveOutGetErrorTextA_orig = (waveOutGetErrorTextA_type)::GetProcAddress(winmm_handle, "waveOutGetErrorTextA");
	waveOutGetErrorTextW_orig = (waveOutGetErrorTextW_type)::GetProcAddress(winmm_handle, "waveOutGetErrorTextW");
	waveOutOpen_orig = (waveOutOpen_type)::GetProcAddress(winmm_handle, "waveOutOpen");
	waveOutClose_orig = (waveOutClose_type)::GetProcAddress(winmm_handle, "waveOutClose");
	waveOutPrepareHeader_orig = (waveOutPrepareHeader_type)::GetProcAddress(winmm_handle, "waveOutPrepareHeader");
	waveOutUnprepareHeader_orig = (waveOutUnprepareHeader_type)::GetProcAddress(winmm_handle, "waveOutUnprepareHeader");
	waveOutWrite_orig = (waveOutWrite_type)::GetProcAddress(winmm_handle, "waveOutWrite");
	waveOutPause_orig = (waveOutPause_type)::GetProcAddress(winmm_handle, "waveOutPause");
	waveOutRestart_orig = (waveOutRestart_type)::GetProcAddress(winmm_handle, "waveOutRestart");
	waveOutReset_orig = (waveOutReset_type)::GetProcAddress(winmm_handle, "waveOutReset");
	waveOutBreakLoop_orig = (waveOutBreakLoop_type)::GetProcAddress(winmm_handle, "waveOutBreakLoop");
	waveOutGetPosition_orig = (waveOutGetPosition_type)::GetProcAddress(winmm_handle, "waveOutGetPosition");
	waveOutGetPitch_orig = (waveOutGetPitch_type)::GetProcAddress(winmm_handle, "waveOutGetPitch");
	waveOutSetPitch_orig = (waveOutSetPitch_type)::GetProcAddress(winmm_handle, "waveOutSetPitch");
	waveOutGetPlaybackRate_orig = (waveOutGetPlaybackRate_type)::GetProcAddress(winmm_handle, "waveOutGetPlaybackRate");
	waveOutSetPlaybackRate_orig = (waveOutSetPlaybackRate_type)::GetProcAddress(winmm_handle, "waveOutSetPlaybackRate");
	waveOutGetID_orig = (waveOutGetID_type)::GetProcAddress(winmm_handle, "waveOutGetID");
	waveOutMessage_orig = (waveOutMessage_type)::GetProcAddress(winmm_handle, "waveOutMessage");
	waveInGetNumDevs_orig = (waveInGetNumDevs_type)::GetProcAddress(winmm_handle, "waveInGetNumDevs");
	waveInGetDevCapsA_orig = (waveInGetDevCapsA_type)::GetProcAddress(winmm_handle, "waveInGetDevCapsA");
	waveInGetDevCapsW_orig = (waveInGetDevCapsW_type)::GetProcAddress(winmm_handle, "waveInGetDevCapsW");
	waveInGetErrorTextA_orig = (waveInGetErrorTextA_type)::GetProcAddress(winmm_handle, "waveInGetErrorTextA");
	waveInGetErrorTextW_orig = (waveInGetErrorTextW_type)::GetProcAddress(winmm_handle, "waveInGetErrorTextW");
	waveInOpen_orig = (waveInOpen_type)::GetProcAddress(winmm_handle, "waveInOpen");
	waveInClose_orig = (waveInClose_type)::GetProcAddress(winmm_handle, "waveInClose");
	waveInPrepareHeader_orig = (waveInPrepareHeader_type)::GetProcAddress(winmm_handle, "waveInPrepareHeader");
	waveInUnprepareHeader_orig = (waveInUnprepareHeader_type)::GetProcAddress(winmm_handle, "waveInUnprepareHeader");
	waveInAddBuffer_orig = (waveInAddBuffer_type)::GetProcAddress(winmm_handle, "waveInAddBuffer");
	waveInStart_orig = (waveInStart_type)::GetProcAddress(winmm_handle, "waveInStart");
	waveInStop_orig = (waveInStop_type)::GetProcAddress(winmm_handle, "waveInStop");
	waveInReset_orig = (waveInReset_type)::GetProcAddress(winmm_handle, "waveInReset");
	waveInGetPosition_orig = (waveInGetPosition_type)::GetProcAddress(winmm_handle, "waveInGetPosition");
	waveInGetID_orig = (waveInGetID_type)::GetProcAddress(winmm_handle, "waveInGetID");
	waveInMessage_orig = (waveInMessage_type)::GetProcAddress(winmm_handle, "waveInMessage");
	midiOutGetNumDevs_orig = (midiOutGetNumDevs_type)::GetProcAddress(winmm_handle, "midiOutGetNumDevs");
	midiStreamOpen_orig = (midiStreamOpen_type)::GetProcAddress(winmm_handle, "midiStreamOpen");
	midiStreamClose_orig = (midiStreamClose_type)::GetProcAddress(winmm_handle, "midiStreamClose");
	midiStreamProperty_orig = (midiStreamProperty_type)::GetProcAddress(winmm_handle, "midiStreamProperty");
	midiStreamPosition_orig = (midiStreamPosition_type)::GetProcAddress(winmm_handle, "midiStreamPosition");
	midiStreamOut_orig = (midiStreamOut_type)::GetProcAddress(winmm_handle, "midiStreamOut");
	midiStreamPause_orig = (midiStreamPause_type)::GetProcAddress(winmm_handle, "midiStreamPause");
	midiStreamRestart_orig = (midiStreamRestart_type)::GetProcAddress(winmm_handle, "midiStreamRestart");
	midiStreamStop_orig = (midiStreamStop_type)::GetProcAddress(winmm_handle, "midiStreamStop");
	midiConnect_orig = (midiConnect_type)::GetProcAddress(winmm_handle, "midiConnect");
	midiDisconnect_orig = (midiDisconnect_type)::GetProcAddress(winmm_handle, "midiDisconnect");
	midiOutGetDevCapsA_orig = (midiOutGetDevCapsA_type)::GetProcAddress(winmm_handle, "midiOutGetDevCapsA");
	midiOutGetDevCapsW_orig = (midiOutGetDevCapsW_type)::GetProcAddress(winmm_handle, "midiOutGetDevCapsW");
	midiOutGetVolume_orig = (midiOutGetVolume_type)::GetProcAddress(winmm_handle, "midiOutGetVolume");
	midiOutSetVolume_orig = (midiOutSetVolume_type)::GetProcAddress(winmm_handle, "midiOutSetVolume");
	midiOutGetErrorTextA_orig = (midiOutGetErrorTextA_type)::GetProcAddress(winmm_handle, "midiOutGetErrorTextA");
	midiOutGetErrorTextW_orig = (midiOutGetErrorTextW_type)::GetProcAddress(winmm_handle, "midiOutGetErrorTextW");
	midiOutOpen_orig = (midiOutOpen_type)::GetProcAddress(winmm_handle, "midiOutOpen");
	midiOutClose_orig = (midiOutClose_type)::GetProcAddress(winmm_handle, "midiOutClose");
	midiOutPrepareHeader_orig = (midiOutPrepareHeader_type)::GetProcAddress(winmm_handle, "midiOutPrepareHeader");
	midiOutUnprepareHeader_orig = (midiOutUnprepareHeader_type)::GetProcAddress(winmm_handle, "midiOutUnprepareHeader");
	midiOutShortMsg_orig = (midiOutShortMsg_type)::GetProcAddress(winmm_handle, "midiOutShortMsg");
	midiOutLongMsg_orig = (midiOutLongMsg_type)::GetProcAddress(winmm_handle, "midiOutLongMsg");
	midiOutReset_orig = (midiOutReset_type)::GetProcAddress(winmm_handle, "midiOutReset");
	midiOutCachePatches_orig = (midiOutCachePatches_type)::GetProcAddress(winmm_handle, "midiOutCachePatches");
	midiOutCacheDrumPatches_orig = (midiOutCacheDrumPatches_type)::GetProcAddress(winmm_handle, "midiOutCacheDrumPatches");
	midiOutGetID_orig = (midiOutGetID_type)::GetProcAddress(winmm_handle, "midiOutGetID");
	midiOutMessage_orig = (midiOutMessage_type)::GetProcAddress(winmm_handle, "midiOutMessage");
	midiInGetNumDevs_orig = (midiInGetNumDevs_type)::GetProcAddress(winmm_handle, "midiInGetNumDevs");
	midiInGetDevCapsA_orig = (midiInGetDevCapsA_type)::GetProcAddress(winmm_handle, "midiInGetDevCapsA");
	midiInGetDevCapsW_orig = (midiInGetDevCapsW_type)::GetProcAddress(winmm_handle, "midiInGetDevCapsW");
	midiInGetErrorTextA_orig = (midiInGetErrorTextA_type)::GetProcAddress(winmm_handle, "midiInGetErrorTextA");
	midiInGetErrorTextW_orig = (midiInGetErrorTextW_type)::GetProcAddress(winmm_handle, "midiInGetErrorTextW");
	midiInOpen_orig = (midiInOpen_type)::GetProcAddress(winmm_handle, "midiInOpen");
	midiInClose_orig = (midiInClose_type)::GetProcAddress(winmm_handle, "midiInClose");
	midiInPrepareHeader_orig = (midiInPrepareHeader_type)::GetProcAddress(winmm_handle, "midiInPrepareHeader");
	midiInUnprepareHeader_orig = (midiInUnprepareHeader_type)::GetProcAddress(winmm_handle, "midiInUnprepareHeader");
	midiInAddBuffer_orig = (midiInAddBuffer_type)::GetProcAddress(winmm_handle, "midiInAddBuffer");
	midiInStart_orig = (midiInStart_type)::GetProcAddress(winmm_handle, "midiInStart");
	midiInStop_orig = (midiInStop_type)::GetProcAddress(winmm_handle, "midiInStop");
	midiInReset_orig = (midiInReset_type)::GetProcAddress(winmm_handle, "midiInReset");
	midiInGetID_orig = (midiInGetID_type)::GetProcAddress(winmm_handle, "midiInGetID");
	midiInMessage_orig = (midiInMessage_type)::GetProcAddress(winmm_handle, "midiInMessage");
	auxGetNumDevs_orig = (auxGetNumDevs_type)::GetProcAddress(winmm_handle, "auxGetNumDevs");
	auxGetDevCapsA_orig = (auxGetDevCapsA_type)::GetProcAddress(winmm_handle, "auxGetDevCapsA");
	auxGetDevCapsW_orig = (auxGetDevCapsW_type)::GetProcAddress(winmm_handle, "auxGetDevCapsW");
	auxSetVolume_orig = (auxSetVolume_type)::GetProcAddress(winmm_handle, "auxSetVolume");
	auxGetVolume_orig = (auxGetVolume_type)::GetProcAddress(winmm_handle, "auxGetVolume");
	auxOutMessage_orig = (auxOutMessage_type)::GetProcAddress(winmm_handle, "auxOutMessage");
	mixerGetNumDevs_orig = (mixerGetNumDevs_type)::GetProcAddress(winmm_handle, "mixerGetNumDevs");
	mixerGetDevCapsA_orig = (mixerGetDevCapsA_type)::GetProcAddress(winmm_handle, "mixerGetDevCapsA");
	mixerGetDevCapsW_orig = (mixerGetDevCapsW_type)::GetProcAddress(winmm_handle, "mixerGetDevCapsW");
	mixerOpen_orig = (mixerOpen_type)::GetProcAddress(winmm_handle, "mixerOpen");
	mixerClose_orig = (mixerClose_type)::GetProcAddress(winmm_handle, "mixerClose");
	mixerMessage_orig = (mixerMessage_type)::GetProcAddress(winmm_handle, "mixerMessage");
	mixerGetLineInfoA_orig = (mixerGetLineInfoA_type)::GetProcAddress(winmm_handle, "mixerGetLineInfoA");
	mixerGetLineInfoW_orig = (mixerGetLineInfoW_type)::GetProcAddress(winmm_handle, "mixerGetLineInfoW");
	mixerGetID_orig = (mixerGetID_type)::GetProcAddress(winmm_handle, "mixerGetID");
	mixerGetLineControlsA_orig = (mixerGetLineControlsA_type)::GetProcAddress(winmm_handle, "mixerGetLineControlsA");
	mixerGetLineControlsW_orig = (mixerGetLineControlsW_type)::GetProcAddress(winmm_handle, "mixerGetLineControlsW");
	mixerGetControlDetailsA_orig = (mixerGetControlDetailsA_type)::GetProcAddress(winmm_handle, "mixerGetControlDetailsA");
	mixerGetControlDetailsW_orig = (mixerGetControlDetailsW_type)::GetProcAddress(winmm_handle, "mixerGetControlDetailsW");
	mixerSetControlDetails_orig = (mixerSetControlDetails_type)::GetProcAddress(winmm_handle, "mixerSetControlDetails");
	mmGetCurrentTask_orig = (mmGetCurrentTask_type)::GetProcAddress(winmm_handle, "mmGetCurrentTask");
	mmTaskBlock_orig = (mmTaskBlock_type)::GetProcAddress(winmm_handle, "mmTaskBlock");
	mmTaskCreate_orig = (mmTaskCreate_type)::GetProcAddress(winmm_handle, "mmTaskCreate");
	mmTaskSignal_orig = (mmTaskSignal_type)::GetProcAddress(winmm_handle, "mmTaskSignal");
	mmTaskYield_orig = (mmTaskYield_type)::GetProcAddress(winmm_handle, "mmTaskYield");
	timeGetSystemTime_orig = (timeGetSystemTime_type)::GetProcAddress(winmm_handle, "timeGetSystemTime");
	timeGetTime_orig = (timeGetTime_type)::GetProcAddress(winmm_handle, "timeGetTime");
	timeSetEvent_orig = (timeSetEvent_type)::GetProcAddress(winmm_handle, "timeSetEvent");
	timeKillEvent_orig = (timeKillEvent_type)::GetProcAddress(winmm_handle, "timeKillEvent");
	timeGetDevCaps_orig = (timeGetDevCaps_type)::GetProcAddress(winmm_handle, "timeGetDevCaps");
	timeBeginPeriod_orig = (timeBeginPeriod_type)::GetProcAddress(winmm_handle, "timeBeginPeriod");
	timeEndPeriod_orig = (timeEndPeriod_type)::GetProcAddress(winmm_handle, "timeEndPeriod");
	joyGetNumDevs_orig = (joyGetNumDevs_type)::GetProcAddress(winmm_handle, "joyGetNumDevs");
	joyConfigChanged_orig = (joyConfigChanged_type)::GetProcAddress(winmm_handle, "joyConfigChanged");
	joyGetDevCapsA_orig = (joyGetDevCapsA_type)::GetProcAddress(winmm_handle, "joyGetDevCapsA");
	joyGetDevCapsW_orig = (joyGetDevCapsW_type)::GetProcAddress(winmm_handle, "joyGetDevCapsW");
	joyGetPos_orig = (joyGetPos_type)::GetProcAddress(winmm_handle, "joyGetPos");
	joyGetPosEx_orig = (joyGetPosEx_type)::GetProcAddress(winmm_handle, "joyGetPosEx");
	joyGetThreshold_orig = (joyGetThreshold_type)::GetProcAddress(winmm_handle, "joyGetThreshold");
	joyReleaseCapture_orig = (joyReleaseCapture_type)::GetProcAddress(winmm_handle, "joyReleaseCapture");
	joySetCapture_orig = (joySetCapture_type)::GetProcAddress(winmm_handle, "joySetCapture");
	joySetThreshold_orig = (joySetThreshold_type)::GetProcAddress(winmm_handle, "joySetThreshold");
	mciDriverNotify_orig = (mciDriverNotify_type)::GetProcAddress(winmm_handle, "mciDriverNotify");
	mciDriverYield_orig = (mciDriverYield_type)::GetProcAddress(winmm_handle, "mciDriverYield");
	mmioStringToFOURCCA_orig = (mmioStringToFOURCCA_type)::GetProcAddress(winmm_handle, "mmioStringToFOURCCA");
	mmioStringToFOURCCW_orig = (mmioStringToFOURCCW_type)::GetProcAddress(winmm_handle, "mmioStringToFOURCCW");
	mmioInstallIOProcA_orig = (mmioInstallIOProcA_type)::GetProcAddress(winmm_handle, "mmioInstallIOProcA");
	mmioInstallIOProcW_orig = (mmioInstallIOProcW_type)::GetProcAddress(winmm_handle, "mmioInstallIOProcW");
	mmioOpenA_orig = (mmioOpenA_type)::GetProcAddress(winmm_handle, "mmioOpenA");
	mmioOpenW_orig = (mmioOpenW_type)::GetProcAddress(winmm_handle, "mmioOpenW");
	mmioRenameA_orig = (mmioRenameA_type)::GetProcAddress(winmm_handle, "mmioRenameA");
	mmioRenameW_orig = (mmioRenameW_type)::GetProcAddress(winmm_handle, "mmioRenameW");
	mmioClose_orig = (mmioClose_type)::GetProcAddress(winmm_handle, "mmioClose");
	mmioRead_orig = (mmioRead_type)::GetProcAddress(winmm_handle, "mmioRead");
	mmioWrite_orig = (mmioWrite_type)::GetProcAddress(winmm_handle, "mmioWrite");
	mmioSeek_orig = (mmioSeek_type)::GetProcAddress(winmm_handle, "mmioSeek");
	mmioGetInfo_orig = (mmioGetInfo_type)::GetProcAddress(winmm_handle, "mmioGetInfo");
	mmioSetInfo_orig = (mmioSetInfo_type)::GetProcAddress(winmm_handle, "mmioSetInfo");
	mmioSetBuffer_orig = (mmioSetBuffer_type)::GetProcAddress(winmm_handle, "mmioSetBuffer");
	mmioFlush_orig = (mmioFlush_type)::GetProcAddress(winmm_handle, "mmioFlush");
	mmioAdvance_orig = (mmioAdvance_type)::GetProcAddress(winmm_handle, "mmioAdvance");
	mmioSendMessage_orig = (mmioSendMessage_type)::GetProcAddress(winmm_handle, "mmioSendMessage");
	mmioDescend_orig = (mmioDescend_type)::GetProcAddress(winmm_handle, "mmioDescend");
	mmioAscend_orig = (mmioAscend_type)::GetProcAddress(winmm_handle, "mmioAscend");
	mmioCreateChunk_orig = (mmioCreateChunk_type)::GetProcAddress(winmm_handle, "mmioCreateChunk");
	mciSendCommandA_orig = (mciSendCommandA_type)::GetProcAddress(winmm_handle, "mciSendCommandA");
	mciSendCommandW_orig = (mciSendCommandW_type)::GetProcAddress(winmm_handle, "mciSendCommandW");
	mciSendStringA_orig = (mciSendStringA_type)::GetProcAddress(winmm_handle, "mciSendStringA");
	mciSendStringW_orig = (mciSendStringW_type)::GetProcAddress(winmm_handle, "mciSendStringW");
	mciGetDeviceIDA_orig = (mciGetDeviceIDA_type)::GetProcAddress(winmm_handle, "mciGetDeviceIDA");
	mciGetDeviceIDW_orig = (mciGetDeviceIDW_type)::GetProcAddress(winmm_handle, "mciGetDeviceIDW");
	mciGetDeviceIDFromElementIDA_orig = (mciGetDeviceIDFromElementIDA_type)::GetProcAddress(winmm_handle, "mciGetDeviceIDFromElementIDA");
	mciGetDeviceIDFromElementIDW_orig = (mciGetDeviceIDFromElementIDW_type)::GetProcAddress(winmm_handle, "mciGetDeviceIDFromElementIDW");
	mciGetDriverData_orig = (mciGetDriverData_type)::GetProcAddress(winmm_handle, "mciGetDriverData");
	mciGetErrorStringA_orig = (mciGetErrorStringA_type)::GetProcAddress(winmm_handle, "mciGetErrorStringA");
	mciGetErrorStringW_orig = (mciGetErrorStringW_type)::GetProcAddress(winmm_handle, "mciGetErrorStringW");
	mciSetDriverData_orig = (mciSetDriverData_type)::GetProcAddress(winmm_handle, "mciSetDriverData");
	mciSetYieldProc_orig = (mciSetYieldProc_type)::GetProcAddress(winmm_handle, "mciSetYieldProc");
	mciFreeCommandResource_orig = (mciFreeCommandResource_type)::GetProcAddress(winmm_handle, "mciFreeCommandResource");
	mciGetCreatorTask_orig = (mciGetCreatorTask_type)::GetProcAddress(winmm_handle, "mciGetCreatorTask");
	mciGetYieldProc_orig = (mciGetYieldProc_type)::GetProcAddress(winmm_handle, "mciGetYieldProc");
	mciLoadCommandResource_orig = (mciLoadCommandResource_type)::GetProcAddress(winmm_handle, "mciLoadCommandResource");
	mciExecute_orig = (mciExecute_type)::GetProcAddress(winmm_handle, "mciExecute");
}

extern "C" {
	LRESULT   WINAPI CloseDriver( HDRVR hDriver, LPARAM lParam1, LPARAM lParam2)
	{
		//OutputDebugString(L"CloseDriver\n");
		return CloseDriver_orig( hDriver, lParam1, lParam2);
	}

	HDRVR     WINAPI OpenDriver( LPCWSTR szDriverName, LPCWSTR szSectionName, LPARAM lParam2)
	{
		//OutputDebugString(L"OpenDriver\n");
		return OpenDriver_orig( szDriverName, szSectionName, lParam2);
	}

	LRESULT   WINAPI SendDriverMessage( HDRVR hDriver, UINT message, LPARAM lParam1, LPARAM lParam2)
	{
		//OutputDebugString(L"SendDriverMessage\n");
		return SendDriverMessage_orig( hDriver, message, lParam1, lParam2);
	}

	HMODULE   WINAPI DrvGetModuleHandle( HDRVR hDriver)
	{
		//OutputDebugString(L"DrvGetModuleHandle\n");
		return DrvGetModuleHandle_orig( hDriver);
	}

	HMODULE   WINAPI GetDriverModuleHandle( HDRVR hDriver)
	{
		//OutputDebugString(L"GetDriverModuleHandle\n");
		return GetDriverModuleHandle_orig( hDriver);
	}

	LRESULT   WINAPI DefDriverProc( DWORD_PTR dwDriverIdentifier, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
	{
		//OutputDebugString(L"DefDriverProc\n");
		return DefDriverProc_orig( dwDriverIdentifier, hdrvr, uMsg, lParam1, lParam2);
	}

	BOOL WINAPI DriverCallback( DWORD dwCallBack, DWORD dwFlags, HDRVR hdrvr, DWORD msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
	{
		//OutputDebugString(L"DriverCallback\n");
		return DriverCallback_orig(dwCallBack, dwFlags, hdrvr, msg, dwUser, dwParam1, dwParam2);
	}

	UINT WINAPI mmsystemGetVersion(void)
	{
		//OutputDebugString(L"mmsystemGetVersion\n");
		return mmsystemGetVersion_orig();
	}

	BOOL WINAPI sndPlaySoundA( LPCSTR pszSound, UINT fuSound)
	{
		//OutputDebugString(L"sndPlaySoundA\n");
		return sndPlaySoundA_orig( pszSound, fuSound);
	}

	BOOL WINAPI sndPlaySoundW( LPCWSTR pszSound, UINT fuSound)
	{
		//OutputDebugString(L"sndPlaySoundW\n");
		return sndPlaySoundW_orig( pszSound, fuSound);
	}

	BOOL WINAPI PlaySoundA( LPCSTR pszSound, HMODULE hmod, DWORD fdwSound)
	{
		//OutputDebugString(L"PlaySoundA\n");
		return PlaySoundA_orig( pszSound, hmod, fdwSound);
	}

	BOOL WINAPI PlaySoundW( LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
	{
		//OutputDebugString(L"PlaySoundW\n");
		return PlaySoundW_orig( pszSound, hmod, fdwSound);
	}

	UINT WINAPI waveOutGetNumDevs(void)
	{
		//OutputDebugString(L"waveOutGetNumDevs\n");
		return waveOutGetNumDevs_orig();
	}

	MMRESULT WINAPI waveOutGetDevCapsA( UINT_PTR uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc)
	{
		//OutputDebugString(L"waveOutGetDevCapsA\n");
		return waveOutGetDevCapsA_orig( uDeviceID, pwoc, cbwoc);
	}

	MMRESULT WINAPI waveOutGetDevCapsW( UINT_PTR uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc)
	{
		//OutputDebugString(L"waveOutGetDevCapsW\n");
		return waveOutGetDevCapsW_orig( uDeviceID, pwoc, cbwoc);
	}


	MMRESULT WINAPI waveOutGetVolume( HWAVEOUT hwo, LPDWORD pdwVolume)
	{
		//OutputDebugString(L"waveOutGetVolume\n");
		return waveOutGetVolume_orig( hwo, pdwVolume);
	}

	MMRESULT WINAPI waveOutSetVolume( HWAVEOUT hwo, DWORD dwVolume)
	{
		//OutputDebugString(L"waveOutSetVolume\n");
		return waveOutSetVolume_orig( hwo, dwVolume);
	}

	MMRESULT WINAPI waveOutGetErrorTextA( MMRESULT mmrError, LPSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"waveOutGetErrorTextA\n");
		return waveOutGetErrorTextA_orig( mmrError, pszText, cchText);
	}

	MMRESULT WINAPI waveOutGetErrorTextW( MMRESULT mmrError, LPWSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"waveOutGetErrorTextW\n");
		return waveOutGetErrorTextW_orig( mmrError, pszText, cchText);
	}

	//MMRESULT WINAPI waveOutOpen( LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
	//{
	//	//OutputDebugString(L"waveOutGetErrorTextW\n");
	//	return waveOutOpen_orig( phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
	//}

	//MMRESULT WINAPI waveOutClose( HWAVEOUT hwo)
	//{
	//	//OutputDebugString(L"waveOutGetErrorTextW\n");
	//	return waveOutClose_orig( hwo );
	//}

	MMRESULT WINAPI waveOutPrepareHeader( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
	{
		//OutputDebugString(L"waveOutPrepareHeader\n");
		return waveOutPrepareHeader_orig( hwo, pwh, cbwh);
	}

	MMRESULT WINAPI waveOutUnprepareHeader( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
	{
		//OutputDebugString(L"waveOutUnprepareHeader\n");
		return waveOutUnprepareHeader_orig( hwo, pwh, cbwh);
	}


	//MMRESULT WINAPI waveOutWrite( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
	//{
	//	//OutputDebugString(L"waveOutUnprepareHeader\n");
	//	return waveOutWrite_orig( hwo, pwh, cbwh);
	//}

	MMRESULT WINAPI waveOutPause( HWAVEOUT hwo)
	{
		//OutputDebugString(L"waveOutPause\n");
		return waveOutPause_orig( hwo);
	}

	MMRESULT WINAPI waveOutRestart( HWAVEOUT hwo)
	{
		//OutputDebugString(L"waveOutRestart\n");
		return waveOutRestart_orig( hwo);
	}

	MMRESULT WINAPI waveOutReset( HWAVEOUT hwo)
	{
		//OutputDebugString(L"waveOutReset\n");
		return waveOutReset_orig( hwo);
	}

	MMRESULT WINAPI waveOutBreakLoop( HWAVEOUT hwo)
	{
		//OutputDebugString(L"waveOutBreakLoop\n");
		return waveOutBreakLoop_orig( hwo);
	}

	MMRESULT WINAPI waveOutGetPosition( HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt)
	{
		//OutputDebugString(L"waveOutGetPosition\n");
		return waveOutGetPosition_orig( hwo, pmmt, cbmmt);
	}

	MMRESULT WINAPI waveOutGetPitch( HWAVEOUT hwo, LPDWORD pdwPitch)
	{
		//OutputDebugString(L"waveOutGetPitch\n");
		return waveOutGetPitch_orig( hwo, pdwPitch);
	}

	MMRESULT WINAPI waveOutSetPitch( HWAVEOUT hwo, DWORD dwPitch)
	{
		//OutputDebugString(L"waveOutSetPitch\n");
		return waveOutSetPitch_orig( hwo, dwPitch);
	}

	MMRESULT WINAPI waveOutGetPlaybackRate( HWAVEOUT hwo, LPDWORD pdwRate)
	{
		//OutputDebugString(L"waveOutGetPlaybackRate\n");
		return waveOutGetPlaybackRate_orig( hwo, pdwRate);
	}

	MMRESULT WINAPI waveOutSetPlaybackRate( HWAVEOUT hwo, DWORD dwRate)
	{
		//OutputDebugString(L"waveOutSetPlaybackRate\n");
		return waveOutSetPlaybackRate_orig( hwo, dwRate);
	}

	MMRESULT WINAPI waveOutGetID( HWAVEOUT hwo, LPUINT puDeviceID)
	{
		//OutputDebugString(L"waveOutGetID\n");
		return waveOutGetID_orig( hwo, puDeviceID);
	}

	MMRESULT WINAPI waveOutMessage( HWAVEOUT hwo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		//OutputDebugString(L"waveOutMessage\n");
		return waveOutMessage_orig( hwo, uMsg, dw1, dw2);
	}

	UINT WINAPI waveInGetNumDevs(void)
	{
		//OutputDebugString(L"waveInGetNumDevs\n");
		return waveInGetNumDevs_orig();
	}

	MMRESULT WINAPI waveInGetDevCapsA( UINT_PTR uDeviceID, LPWAVEINCAPSA pwic, UINT cbwic)
	{
		//OutputDebugString(L"waveInGetDevCapsA\n");
		return waveInGetDevCapsA_orig( uDeviceID, pwic, cbwic);
	}

	MMRESULT WINAPI waveInGetDevCapsW( UINT_PTR uDeviceID, LPWAVEINCAPSW pwic, UINT cbwic)
	{
		//OutputDebugString(L"waveInGetDevCapsW\n");
		return waveInGetDevCapsW_orig( uDeviceID, pwic, cbwic);
	}

	MMRESULT WINAPI waveInGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"waveInGetErrorTextA\n");
		return waveInGetErrorTextA_orig(mmrError, pszText, cchText);
	}

	MMRESULT WINAPI waveInGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"waveInGetErrorTextW\n");
		return waveInGetErrorTextW_orig(mmrError, pszText, cchText);
	}

	MMRESULT WINAPI waveInOpen( LPHWAVEIN phwi, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
	{
		//OutputDebugString(L"waveInOpen\n");
		return waveInOpen_orig(phwi, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
	}

	MMRESULT WINAPI waveInClose( HWAVEIN hwi)
	{
		//OutputDebugString(L"waveInClose\n");
		return waveInClose_orig( hwi);
	}

	MMRESULT WINAPI waveInPrepareHeader( HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh)
	{
		//OutputDebugString(L"waveInPrepareHeader\n");
		return waveInPrepareHeader_orig( hwi, pwh, cbwh);
	}

	MMRESULT WINAPI waveInUnprepareHeader( HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh)
	{
		//OutputDebugString(L"waveInUnprepareHeader\n");
		return waveInUnprepareHeader_orig( hwi, pwh, cbwh);
	}

	MMRESULT WINAPI waveInAddBuffer( HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh)
	{
		//OutputDebugString(L"waveInAddBuffer\n");
		return waveInAddBuffer_orig( hwi, pwh, cbwh);
	}

	MMRESULT WINAPI waveInStart( HWAVEIN hwi)
	{
		//OutputDebugString(L"waveInStart\n");
		return waveInStart_orig( hwi);
	}

	MMRESULT WINAPI waveInStop( HWAVEIN hwi)
	{
		//OutputDebugString(L"waveInStop\n");
		return waveInStop_orig(hwi);
	}

	MMRESULT WINAPI waveInReset( HWAVEIN hwi)
	{
		//OutputDebugString(L"waveInReset\n");
		return waveInReset_orig(hwi);
	}

	MMRESULT WINAPI waveInGetPosition( HWAVEIN hwi, LPMMTIME pmmt, UINT cbmmt)
	{
		//OutputDebugString(L"waveInGetPosition\n");
		return waveInGetPosition_orig( hwi, pmmt, cbmmt);
	}

	MMRESULT WINAPI waveInGetID( HWAVEIN hwi, LPUINT puDeviceID)
	{
		//OutputDebugString(L"waveInGetID\n");
		return waveInGetID_orig( hwi, puDeviceID);
	}

	MMRESULT WINAPI waveInMessage( HWAVEIN hwi, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		//OutputDebugString(L"waveInMessage\n");
		return waveInMessage_orig( hwi, uMsg, dw1, dw2);
	}

	UINT WINAPI midiOutGetNumDevs(void)
	{
		//OutputDebugString(L"midiOutGetNumDevs\n");
		return midiOutGetNumDevs_orig();
	}

	MMRESULT WINAPI midiStreamOpen( LPHMIDISTRM phms, LPUINT puDeviceID, DWORD cMidi, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
	{
		//OutputDebugString(L"midiStreamOpen\n");
		return midiStreamOpen_orig( phms, puDeviceID, cMidi, dwCallback, dwInstance, fdwOpen);
	}

	MMRESULT WINAPI midiStreamClose( HMIDISTRM hms)
	{
		//OutputDebugString(L"midiStreamClose\n");
		return midiStreamClose_orig( hms);
	}

	MMRESULT WINAPI midiStreamProperty( HMIDISTRM hms, LPBYTE lppropdata, DWORD dwProperty)
	{
		//OutputDebugString(L"midiStreamProperty\n");
		return midiStreamProperty_orig( hms, lppropdata, dwProperty);
	}

	MMRESULT WINAPI midiStreamPosition( HMIDISTRM hms, LPMMTIME lpmmt, UINT cbmmt)
	{
		//OutputDebugString(L"midiStreamPosition\n");
		return midiStreamPosition_orig( hms, lpmmt, cbmmt);
	}

	MMRESULT WINAPI midiStreamOut( HMIDISTRM hms, LPMIDIHDR pmh, UINT cbmh)
	{
		//OutputDebugString(L"midiStreamOut\n");
		return midiStreamOut_orig( hms, pmh, cbmh);
	}

	MMRESULT WINAPI midiStreamPause( HMIDISTRM hms)
	{
		//OutputDebugString(L"midiStreamPause\n");
		return midiStreamPause_orig( hms);
	}

	MMRESULT WINAPI midiStreamRestart( HMIDISTRM hms)
	{
		//OutputDebugString(L"midiStreamRestart\n");
		return midiStreamRestart_orig( hms);
	}

	MMRESULT WINAPI midiStreamStop( HMIDISTRM hms)
	{
		//OutputDebugString(L"midiStreamStop\n");
		return midiStreamStop_orig( hms);
	}

	MMRESULT WINAPI midiConnect( HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved)
	{
		//OutputDebugString(L"midiConnect\n");
		return midiConnect_orig( hmi, hmo, pReserved);
	}

	MMRESULT WINAPI midiDisconnect( HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved)
	{
		//OutputDebugString(L"midiDisconnect\n");
		return midiDisconnect_orig( hmi, hmo, pReserved);
	}

	MMRESULT WINAPI midiOutGetDevCapsA( UINT_PTR uDeviceID, LPMIDIOUTCAPSA pmoc, UINT cbmoc)
	{
		//OutputDebugString(L"midiOutGetDevCapsA\n");
		return midiOutGetDevCapsA_orig( uDeviceID, pmoc, cbmoc);
	}

	MMRESULT WINAPI midiOutGetDevCapsW( UINT_PTR uDeviceID, LPMIDIOUTCAPSW pmoc, UINT cbmoc)
	{
		//OutputDebugString(L"midiOutGetDevCapsW\n");
		return midiOutGetDevCapsW_orig( uDeviceID, pmoc, cbmoc);
	}

	MMRESULT WINAPI midiOutGetVolume( HMIDIOUT hmo, LPDWORD pdwVolume)
	{
		//OutputDebugString(L"midiOutGetVolume\n");
		return midiOutGetVolume_orig( hmo, pdwVolume);
	}

	MMRESULT WINAPI midiOutSetVolume( HMIDIOUT hmo, DWORD dwVolume)
	{
		//OutputDebugString(L"midiOutSetVolume\n");
		return midiOutSetVolume_orig( hmo, dwVolume);
	}

	MMRESULT WINAPI midiOutGetErrorTextA( MMRESULT mmrError, LPSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"midiOutGetErrorTextA\n");
		return midiOutGetErrorTextA_orig( mmrError, pszText, cchText);
	}

	MMRESULT WINAPI midiOutGetErrorTextW( MMRESULT mmrError, LPWSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"midiOutGetErrorTextW\n");
		return midiOutGetErrorTextW_orig( mmrError, pszText, cchText);
	}

	MMRESULT WINAPI midiOutOpen( LPHMIDIOUT phmo, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
	{
		//OutputDebugString(L"midiOutOpen\n");
		return midiOutOpen_orig(phmo, uDeviceID, dwCallback, dwInstance, fdwOpen);
	}

	MMRESULT WINAPI midiOutClose( HMIDIOUT hmo)
	{
		//OutputDebugString(L"midiOutClose\n");
		return midiOutClose_orig( hmo);
	}

	MMRESULT WINAPI midiOutPrepareHeader( HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh)
	{
		//OutputDebugString(L"midiOutPrepareHeader\n");
		return midiOutPrepareHeader_orig( hmo, pmh, cbmh);
	}

	MMRESULT WINAPI midiOutUnprepareHeader(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh)
	{
		//OutputDebugString(L"midiOutUnprepareHeader\n");
		return midiOutUnprepareHeader_orig(hmo, pmh, cbmh);
	}

	MMRESULT WINAPI midiOutShortMsg( HMIDIOUT hmo, DWORD dwMsg)
	{
		//OutputDebugString(L"midiOutShortMsg\n");
		return midiOutShortMsg_orig( hmo, dwMsg);
	}

	MMRESULT WINAPI midiOutLongMsg(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh)
	{
		//OutputDebugString(L"midiOutLongMsg\n");
		return midiOutLongMsg_orig(hmo, pmh, cbmh);
	}

	MMRESULT WINAPI midiOutReset( HMIDIOUT hmo)
	{
		//OutputDebugString(L"midiOutReset\n");
		return midiOutReset_orig( hmo);
	}

	MMRESULT WINAPI midiOutCachePatches( HMIDIOUT hmo, UINT uBank, LPWORD pwpa, UINT fuCache)
	{
		//OutputDebugString(L"midiOutCachePatches\n");
		return midiOutCachePatches_orig( hmo, uBank, pwpa, fuCache);
	}

	MMRESULT WINAPI midiOutCacheDrumPatches( HMIDIOUT hmo, UINT uPatch, LPWORD pwkya, UINT fuCache)
	{
		//OutputDebugString(L"midiOutCacheDrumPatches\n");
		return midiOutCacheDrumPatches_orig( hmo, uPatch, pwkya, fuCache);
	}

	MMRESULT WINAPI midiOutGetID( HMIDIOUT hmo, LPUINT puDeviceID)
	{
		//OutputDebugString(L"midiOutGetID\n");
		return midiOutGetID_orig( hmo, puDeviceID);
	}

	MMRESULT WINAPI midiOutMessage( HMIDIOUT hmo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		//OutputDebugString(L"midiOutMessage\n");
		return midiOutMessage_orig( hmo, uMsg, dw1, dw2);
	}

	UINT WINAPI midiInGetNumDevs(void)
	{
		//OutputDebugString(L"midiInGetNumDevs\n");
		return midiInGetNumDevs_orig();
	}

	MMRESULT WINAPI midiInGetDevCapsA( UINT_PTR uDeviceID, LPMIDIINCAPSA pmic, UINT cbmic)
	{
		//OutputDebugString(L"midiInGetDevCapsA\n");
		return midiInGetDevCapsA_orig( uDeviceID, pmic, cbmic);
	}

	MMRESULT WINAPI midiInGetDevCapsW( UINT_PTR uDeviceID, LPMIDIINCAPSW pmic, UINT cbmic)
	{
		//OutputDebugString(L"midiInGetDevCapsW\n");
		return midiInGetDevCapsW_orig( uDeviceID, pmic, cbmic);
	}

	MMRESULT WINAPI midiInGetErrorTextA( MMRESULT mmrError, LPSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"midiInGetErrorTextA\n");
		return midiInGetErrorTextA_orig( mmrError, pszText, cchText);
	}

	MMRESULT WINAPI midiInGetErrorTextW( MMRESULT mmrError, LPWSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"midiInGetErrorTextW\n");
		return midiInGetErrorTextW_orig( mmrError, pszText, cchText);
	}

	MMRESULT WINAPI midiInOpen( LPHMIDIIN phmi, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
	{
		//OutputDebugString(L"midiInOpen\n");
		return midiInOpen_orig(phmi, uDeviceID, dwCallback, dwInstance, fdwOpen);
	}

	MMRESULT WINAPI midiInClose( HMIDIIN hmi)
	{
		//OutputDebugString(L"midiInClose\n");
		return midiInClose_orig( hmi);
	}

	MMRESULT WINAPI midiInPrepareHeader( HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh)
	{
		//OutputDebugString(L"midiInPrepareHeader\n");
		return midiInPrepareHeader_orig( hmi, pmh, cbmh);
	}

	MMRESULT WINAPI midiInUnprepareHeader( HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh)
	{
		//OutputDebugString(L"midiInUnprepareHeader\n");
		return midiInUnprepareHeader_orig( hmi, pmh, cbmh);
	}

	MMRESULT WINAPI midiInAddBuffer( HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh)
	{
		//OutputDebugString(L"midiInAddBuffer\n");
		return midiInAddBuffer_orig( hmi, pmh, cbmh);
	}

	MMRESULT WINAPI midiInStart( HMIDIIN hmi)
	{
		//OutputDebugString(L"midiInStart\n");
		return midiInStart_orig( hmi);
	}

	MMRESULT WINAPI midiInStop( HMIDIIN hmi)
	{
		//OutputDebugString(L"midiInStop\n");
		return midiInStop_orig(hmi);
	}

	MMRESULT WINAPI midiInReset( HMIDIIN hmi)
	{
		//OutputDebugString(L"midiInReset\n");
		return midiInReset_orig( hmi);
	}

	MMRESULT WINAPI midiInGetID( HMIDIIN hmi, LPUINT puDeviceID)
	{
		//OutputDebugString(L"midiInGetID\n");
		return midiInGetID_orig( hmi, puDeviceID);
	}

	MMRESULT WINAPI midiInMessage( HMIDIIN hmi, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		//OutputDebugString(L"midiInMessage\n");
		return midiInMessage_orig( hmi, uMsg, dw1, dw2);
	}

	UINT WINAPI auxGetNumDevs(void)
	{
		//OutputDebugString(L"auxGetNumDevs\n");
		return auxGetNumDevs_orig();
	}

	MMRESULT WINAPI auxGetDevCapsA( UINT_PTR uDeviceID, LPAUXCAPSA pac, UINT cbac)
	{
		//OutputDebugString(L"auxGetDevCapsA\n");
		return auxGetDevCapsA_orig( uDeviceID, pac, cbac);
	}

	MMRESULT WINAPI auxGetDevCapsW( UINT_PTR uDeviceID, LPAUXCAPSW pac, UINT cbac)
	{
		//OutputDebugString(L"auxGetDevCapsW\n");
		return auxGetDevCapsW_orig( uDeviceID, pac, cbac);
	}

	MMRESULT WINAPI auxSetVolume( UINT uDeviceID, DWORD dwVolume)
	{
		//OutputDebugString(L"auxSetVolume\n");
		return auxSetVolume_orig( uDeviceID, dwVolume);
	}

	MMRESULT WINAPI auxGetVolume( UINT uDeviceID, LPDWORD pdwVolume)
	{
		//OutputDebugString(L"auxGetVolume\n");
		return auxGetVolume_orig( uDeviceID, pdwVolume);
	}

	MMRESULT WINAPI auxOutMessage( UINT uDeviceID, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		//OutputDebugString(L"auxOutMessage\n");
		return auxOutMessage_orig( uDeviceID, uMsg, dw1, dw2);
	}

	UINT WINAPI mixerGetNumDevs(void)
	{
		//OutputDebugString(L"mixerGetNumDevs\n");
		return mixerGetNumDevs_orig();
	}

	MMRESULT WINAPI mixerGetDevCapsA( UINT_PTR uMxId, LPMIXERCAPSA pmxcaps, UINT cbmxcaps)
	{
		//OutputDebugString(L"mixerGetDevCapsA\n");
		return mixerGetDevCapsA_orig( uMxId, pmxcaps, cbmxcaps);
	}

	MMRESULT WINAPI mixerGetDevCapsW( UINT_PTR uMxId, LPMIXERCAPSW pmxcaps, UINT cbmxcaps)
	{
		//OutputDebugString(L"mixerGetDevCapsW\n");
		return mixerGetDevCapsW_orig( uMxId, pmxcaps, cbmxcaps);
	}

	MMRESULT WINAPI mixerOpen( LPHMIXER phmx, UINT uMxId, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
	{
		//OutputDebugString(L"mixerOpen\n");
		return mixerOpen_orig( phmx, uMxId, dwCallback, dwInstance, fdwOpen);
	}

	MMRESULT WINAPI mixerClose( HMIXER hmx)
	{
		//OutputDebugString(L"mixerClose\n");
		return mixerClose_orig( hmx);
	}

	DWORD WINAPI mixerMessage( HMIXER hmx, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		//OutputDebugString(L"mixerMessage\n");
		return mixerMessage_orig( hmx, uMsg, dwParam1, dwParam2);
	}

	MMRESULT WINAPI mixerGetLineInfoA( HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo)
	{
		//OutputDebugString(L"mixerGetLineInfoA\n");
		return mixerGetLineInfoA_orig( hmxobj, pmxl, fdwInfo);
	}

	MMRESULT WINAPI mixerGetLineInfoW( HMIXEROBJ hmxobj, LPMIXERLINEW pmxl, DWORD fdwInfo)
	{
		//OutputDebugString(L"mixerGetLineInfoW\n");
		return mixerGetLineInfoW_orig( hmxobj, pmxl, fdwInfo);
	}

	MMRESULT WINAPI mixerGetID( HMIXEROBJ hmxobj, UINT FAR *puMxId, DWORD fdwId)
	{
		//OutputDebugString(L"mixerGetID\n");
		return mixerGetID_orig( hmxobj, puMxId, fdwId);
	}

	MMRESULT WINAPI mixerGetLineControlsA( HMIXEROBJ hmxobj, LPMIXERLINECONTROLSA pmxlc, DWORD fdwControls)
	{
		//OutputDebugString(L"mixerGetLineControlsA\n");
		return mixerGetLineControlsA_orig( hmxobj, pmxlc, fdwControls);
	}

	MMRESULT WINAPI mixerGetLineControlsW( HMIXEROBJ hmxobj, LPMIXERLINECONTROLSW pmxlc, DWORD fdwControls)
	{
		//OutputDebugString(L"mixerGetLineControlsW\n");
		return mixerGetLineControlsW_orig( hmxobj, pmxlc, fdwControls);
	}

	MMRESULT WINAPI mixerGetControlDetailsA( HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
	{
		//OutputDebugString(L"mixerGetControlDetailsA\n");
		return mixerGetControlDetailsA_orig( hmxobj, pmxcd, fdwDetails);
	}

	MMRESULT WINAPI mixerGetControlDetailsW( HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
	{
		//OutputDebugString(L"mixerGetControlDetailsW\n");
		return mixerGetControlDetailsW_orig( hmxobj, pmxcd, fdwDetails);
	}

	MMRESULT WINAPI mixerSetControlDetails( HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
	{
		//OutputDebugString(L"mixerSetControlDetails\n");
		return mixerSetControlDetails_orig( hmxobj, pmxcd, fdwDetails);
	}

	DWORD    WINAPI mmGetCurrentTask(void)
	{
		//OutputDebugString(L"mmGetCurrentTask\n");
		return mmGetCurrentTask_orig();
	}

	void WINAPI mmTaskBlock(DWORD val)
	{
		//OutputDebugString(L"mmTaskBlock\n");
		return mmTaskBlock_orig(val);
	}

	UINT WINAPI mmTaskCreate(LPTASKCALLBACK a, HANDLE* b, DWORD_PTR c)
	{
		//OutputDebugString(L"mmTaskCreate\n");
		return mmTaskCreate_orig(a, b, c);
	}

	BOOL WINAPI mmTaskSignal(DWORD a)
	{
		//OutputDebugString(L"mmTaskSignal\n");
		return mmTaskSignal_orig(a);
	}

	VOID WINAPI mmTaskYield()
	{
		//OutputDebugString(L"mmTaskYield\n");
		mmTaskYield_orig();
	}

	MMRESULT WINAPI timeGetSystemTime( LPMMTIME pmmt, UINT cbmmt)
	{
		//OutputDebugString(L"timeGetSystemTime\n");
		return timeGetSystemTime_orig( pmmt, cbmmt);
	}

	DWORD WINAPI timeGetTime(void)
	{
		//OutputDebugString(L"timeGetTime\n");
		return timeGetTime_orig();
	}

	MMRESULT WINAPI timeSetEvent( UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent)
	{
		//OutputDebugString(L"timeSetEvent\n");
		return timeSetEvent_orig(uDelay, uResolution, fptc, dwUser, fuEvent);
	}

	MMRESULT WINAPI timeKillEvent( UINT uTimerID)
	{
		//OutputDebugString(L"timeKillEvent\n");
		return timeKillEvent_orig( uTimerID);
	}

	MMRESULT WINAPI timeGetDevCaps( LPTIMECAPS ptc, UINT cbtc)
	{
		//OutputDebugString(L"timeGetDevCaps\n");
		return timeGetDevCaps_orig( ptc, cbtc);
	}

	MMRESULT WINAPI timeBeginPeriod( UINT uPeriod)
	{
		//OutputDebugString(L"timeBeginPeriod\n");
		return timeBeginPeriod_orig( uPeriod);
	}

	MMRESULT WINAPI timeEndPeriod( UINT uPeriod)
	{
		//OutputDebugString(L"timeEndPeriod\n");
		return timeEndPeriod_orig( uPeriod);
	}

	UINT WINAPI joyGetNumDevs(void)
	{
		//OutputDebugString(L"joyGetNumDevs\n");
		return joyGetNumDevs_orig();
	}

	MMRESULT WINAPI joyConfigChanged(DWORD dwFlags)
	{
		//OutputDebugString(L"joyConfigChanged\n");
		return joyConfigChanged_orig(dwFlags);
	}

	MMRESULT WINAPI joyGetDevCapsA( UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc)
	{
		//OutputDebugString(L"joyGetDevCapsA\n");
		return joyGetDevCapsA_orig( uJoyID, pjc, cbjc);
	}

	MMRESULT WINAPI joyGetDevCapsW( UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc)
	{
		//OutputDebugString(L"joyGetDevCapsW\n");
		return joyGetDevCapsW_orig( uJoyID, pjc, cbjc);
	}

	MMRESULT WINAPI joyGetPos( UINT uJoyID, LPJOYINFO pji)
	{
		//OutputDebugString(L"joyGetPos\n");
		return joyGetPos_orig( uJoyID, pji);
	}

	MMRESULT WINAPI joyGetPosEx( UINT uJoyID, LPJOYINFOEX pji)
	{
		//OutputDebugString(L"joyGetPosEx\n");
		return joyGetPosEx_orig( uJoyID, pji);
	}

	MMRESULT WINAPI joyGetThreshold( UINT uJoyID, LPUINT puThreshold)
	{
		//OutputDebugString(L"joyGetThreshold\n");
		return joyGetThreshold_orig( uJoyID, puThreshold);
	}

	MMRESULT WINAPI joyReleaseCapture( UINT uJoyID)
	{
		//OutputDebugString(L"joyReleaseCapture\n");
		return joyReleaseCapture_orig( uJoyID);
	}

	MMRESULT WINAPI joySetCapture( HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged)
	{
		//OutputDebugString(L"joySetCapture\n");
		return joySetCapture_orig(hwnd, uJoyID, uPeriod, fChanged);
	}

	MMRESULT WINAPI joySetThreshold( UINT uJoyID, UINT uThreshold)
	{
		//OutputDebugString(L"joySetThreshold\n");
		return joySetThreshold_orig( uJoyID, uThreshold);
	}

	BOOL WINAPI  mciDriverNotify(HWND hwndCallback, UINT uDeviceID, UINT uStatus)
	{
		//OutputDebugString(L"mciDriverNotify\n");
		return mciDriverNotify_orig(hwndCallback, uDeviceID, uStatus);
	}

	UINT WINAPI  mciDriverYield(UINT uDeviceID)
	{
		//OutputDebugString(L"mciDriverYield\n");
		return mciDriverYield_orig(uDeviceID);
	}	

	FOURCC WINAPI mmioStringToFOURCCA( LPCSTR sz, UINT uFlags)
	{
		//OutputDebugString(L"mmioStringToFOURCCA\n");
		return mmioStringToFOURCCA_orig( sz, uFlags);
	}

	FOURCC WINAPI mmioStringToFOURCCW( LPCWSTR sz, UINT uFlags)
	{
		//OutputDebugString(L"mmioStringToFOURCCW\n");
		return mmioStringToFOURCCW_orig( sz, uFlags);
	}

	LPMMIOPROC WINAPI mmioInstallIOProcA( FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags)
	{
		//OutputDebugString(L"mmioInstallIOProcA\n");
		return mmioInstallIOProcA_orig( fccIOProc, pIOProc, dwFlags);
	}

	LPMMIOPROC WINAPI mmioInstallIOProcW( FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags)
	{
		//OutputDebugString(L"mmioInstallIOProcW\n");
		return mmioInstallIOProcW_orig( fccIOProc, pIOProc, dwFlags);
	}

	HMMIO WINAPI mmioOpenA( LPSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen)
	{
		//OutputDebugString(L"mmioOpenA\n");
		return mmioOpenA_orig( pszFileName, pmmioinfo, fdwOpen);
	}

	HMMIO WINAPI mmioOpenW( LPWSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen)
	{
		//OutputDebugString(L"mmioOpenW\n");
		return mmioOpenW_orig( pszFileName, pmmioinfo, fdwOpen);
	}

	MMRESULT WINAPI mmioRenameA( LPCSTR pszFileName, LPCSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename)
	{
		//OutputDebugString(L"mmioRenameA\n");
		return mmioRenameA_orig( pszFileName, pszNewFileName, pmmioinfo, fdwRename);
	}

	MMRESULT WINAPI mmioRenameW( LPCWSTR pszFileName, LPCWSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename)
	{
		//OutputDebugString(L"mmioRenameW\n");
		return mmioRenameW_orig( pszFileName, pszNewFileName, pmmioinfo, fdwRename);
	}

	MMRESULT WINAPI mmioClose( HMMIO hmmio, UINT fuClose)
	{
		//OutputDebugString(L"mmioClose\n");
		return mmioClose_orig( hmmio, fuClose);
	}

	LONG WINAPI mmioRead( HMMIO hmmio, HPSTR pch, LONG cch)
	{
		//OutputDebugString(L"mmioRead\n");
		return mmioRead_orig( hmmio, pch, cch);
	}

	LONG WINAPI mmioWrite( HMMIO hmmio, const char _huge* pch, LONG cch)
	{
		//OutputDebugString(L"mmioWrite\n");
		return mmioWrite_orig( hmmio, pch, cch);
	}

	LONG WINAPI mmioSeek( HMMIO hmmio, LONG lOffset, int iOrigin)
	{
		//OutputDebugString(L"mmioSeek\n");
		return mmioSeek_orig(hmmio, lOffset, iOrigin);
	}

	MMRESULT WINAPI mmioGetInfo( HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuInfo)
	{
		//OutputDebugString(L"mmioGetInfo\n");
		return mmioGetInfo_orig( hmmio, pmmioinfo, fuInfo);
	}

	MMRESULT WINAPI mmioSetInfo( HMMIO hmmio, LPCMMIOINFO pmmioinfo, UINT fuInfo)
	{
		//OutputDebugString(L"mmioSetInfo\n");
		return mmioSetInfo_orig( hmmio, pmmioinfo, fuInfo);
	}

	MMRESULT WINAPI mmioSetBuffer( HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer, UINT fuBuffer)
	{
		//OutputDebugString(L"mmioSetBuffer\n");
		return mmioSetBuffer_orig(hmmio, pchBuffer, cchBuffer, fuBuffer);
	}

	MMRESULT WINAPI mmioFlush( HMMIO hmmio, UINT fuFlush)
	{
		//OutputDebugString(L"mmioFlush\n");
		return mmioFlush_orig( hmmio, fuFlush);
	}

	MMRESULT WINAPI mmioAdvance( HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuAdvance)
	{
		//OutputDebugString(L"mmioAdvance\n");
		return mmioAdvance_orig( hmmio, pmmioinfo, fuAdvance);
	}

	LRESULT WINAPI mmioSendMessage( HMMIO hmmio, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
	{
		//OutputDebugString(L"mmioSendMessage\n");
		return mmioSendMessage_orig(hmmio, uMsg, lParam1, lParam2);
	}

	MMRESULT WINAPI mmioDescend( HMMIO hmmio, LPMMCKINFO pmmcki, const MMCKINFO FAR* pmmckiParent, UINT fuDescend)
	{
		//OutputDebugString(L"mmioDescend\n");
		return mmioDescend_orig(hmmio, pmmcki, pmmckiParent, fuDescend);
	}

	MMRESULT WINAPI mmioAscend( HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuAscend)
	{
		//OutputDebugString(L"mmioAscend\n");
		return mmioAscend_orig( hmmio, pmmcki, fuAscend);
	}

	MMRESULT WINAPI mmioCreateChunk(HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuCreate)
	{
		//OutputDebugString(L"mmioCreateChunk\n");
		return mmioCreateChunk_orig(hmmio, pmmcki, fuCreate);
	}

	MCIERROR WINAPI mciSendCommandA( MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		//OutputDebugString(L"mciSendCommandA\n");
		return mciSendCommandA_orig( mciId, uMsg, dwParam1, dwParam2);
	}

	MCIERROR WINAPI mciSendCommandW( MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		//OutputDebugString(L"mciSendCommandW\n");
		return mciSendCommandW_orig( mciId, uMsg, dwParam1, dwParam2);
	}

	MCIERROR  WINAPI mciSendStringA( LPCSTR lpstrCommand, LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback)
	{
		//OutputDebugString(L"mciSendStringA\n");
		return mciSendStringA_orig( lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);
	}

	MCIERROR  WINAPI mciSendStringW( LPCWSTR lpstrCommand, LPWSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback)
	{
		//OutputDebugString(L"mciSendStringW\n");
		return mciSendStringW_orig( lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);
	}

	MCIDEVICEID WINAPI mciGetDeviceIDA( LPCSTR pszDevice)
	{
		//OutputDebugString(L"mciGetDeviceIDA\n");
		return mciGetDeviceIDA_orig( pszDevice);
	}

	MCIDEVICEID WINAPI mciGetDeviceIDW( LPCWSTR pszDevice)
	{
		//OutputDebugString(L"mciGetDeviceIDW\n");
		return mciGetDeviceIDW_orig( pszDevice);
	}

	MCIDEVICEID WINAPI mciGetDeviceIDFromElementIDA( DWORD dwElementID, LPCSTR lpstrType )
	{
		//OutputDebugString(L"mciGetDeviceIDFromElementIDA\n");
		return mciGetDeviceIDFromElementIDA_orig( dwElementID, lpstrType );
	}

	MCIDEVICEID WINAPI mciGetDeviceIDFromElementIDW( DWORD dwElementID, LPCWSTR lpstrType )
	{
		//OutputDebugString(L"mciGetDeviceIDFromElementIDW\n");
		return mciGetDeviceIDFromElementIDW_orig( dwElementID, lpstrType );
	}

	DWORD_PTR WINAPI  mciGetDriverData(UINT uDeviceID)
	{
		//OutputDebugString(L"mciGetDriverData\n");
		return mciGetDriverData_orig(uDeviceID);
	}

	BOOL WINAPI mciGetErrorStringA( MCIERROR mcierr, LPSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"mciGetErrorStringA\n");
		return mciGetErrorStringA_orig( mcierr, pszText, cchText);
	}

	BOOL WINAPI mciGetErrorStringW( MCIERROR mcierr, LPWSTR pszText, UINT cchText)
	{
		//OutputDebugString(L"mciGetErrorStringW\n");
		return mciGetErrorStringW_orig( mcierr, pszText, cchText);
	}

	BOOL WINAPI  mciSetDriverData(UINT uDeviceID, DWORD_PTR dwData)
	{
		//OutputDebugString(L"mciSetDriverData_type\n");
		return mciSetDriverData_orig( uDeviceID, dwData );
	}

	BOOL WINAPI mciSetYieldProc( MCIDEVICEID mciId, YIELDPROC fpYieldProc, DWORD dwYieldData)
	{
		//OutputDebugString(L"mciSetYieldProc\n");
		return mciSetYieldProc_orig(mciId, fpYieldProc, dwYieldData);
	}

	BOOL WINAPI  mciFreeCommandResource(UINT uTable)
	{
		//OutputDebugString(L"mciFreeCommandResource\n");
		return mciFreeCommandResource_orig(uTable);
	}

	HTASK WINAPI mciGetCreatorTask( MCIDEVICEID mciId)
	{
		//OutputDebugString(L"mciGetCreatorTask\n");
		return mciGetCreatorTask_orig( mciId);
	}

	YIELDPROC WINAPI mciGetYieldProc( MCIDEVICEID mciId, LPDWORD pdwYieldData)
	{
		//OutputDebugString(L"mciGetYieldProc\n");
		return mciGetYieldProc_orig( mciId, pdwYieldData);
	}

	UINT WINAPI mciLoadCommandResource(HINSTANCE hInstance, LPCWSTR lpResName, UINT uType)
	{
		//OutputDebugString(L"mciLoadCommandResource");
		return mciLoadCommandResource_orig(hInstance, lpResName, uType);
	}
	

	BOOL WINAPI mciExecute(LPCSTR pszCommand)
	{
		//OutputDebugString(L"mciExecute\n");
		return mciExecute_orig(pszCommand);
	}
}
