/** 
 * @file forwarding_api.h
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

// this turns off __declspec(dllimport) for the functions declared in mmsystem.h
#define _WINMM_
#include <windows.h>
#include <mmsystem.h>

void init_function_pointers(HMODULE winmm_handle);

typedef VOID (*LPTASKCALLBACK)(DWORD_PTR dwInst);

typedef LRESULT   (WINAPI *CloseDriver_type)( HDRVR hDriver, LPARAM lParam1, LPARAM lParam2);
extern CloseDriver_type CloseDriver_orig;
typedef HDRVR     (WINAPI *OpenDriver_type)( LPCWSTR szDriverName, LPCWSTR szSectionName, LPARAM lParam2);
extern OpenDriver_type OpenDriver_orig;
typedef LRESULT   (WINAPI *SendDriverMessage_type)( HDRVR hDriver, UINT message, LPARAM lParam1, LPARAM lParam2);
extern SendDriverMessage_type SendDriverMessage_orig;
typedef HMODULE   (WINAPI *DrvGetModuleHandle_type)( HDRVR hDriver);
extern DrvGetModuleHandle_type DrvGetModuleHandle_orig;
typedef HMODULE   (WINAPI *GetDriverModuleHandle_type)( HDRVR hDriver);
extern GetDriverModuleHandle_type GetDriverModuleHandle_orig;
typedef LRESULT   (WINAPI *DefDriverProc_type)( DWORD_PTR dwDriverIdentifier, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
extern DefDriverProc_type DefDriverProc_orig;
typedef BOOL (WINAPI *DriverCallback_type)(DWORD dwCallBack, DWORD dwFlags, HDRVR hdrvr, DWORD msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
extern DriverCallback_type DriverCallback_orig;
typedef UINT (WINAPI *mmsystemGetVersion_type)(void);
extern mmsystemGetVersion_type mmsystemGetVersion_orig;
typedef BOOL (WINAPI *sndPlaySoundA_type)( LPCSTR pszSound, UINT fuSound);
extern sndPlaySoundA_type sndPlaySoundA_orig;
typedef BOOL (WINAPI *sndPlaySoundW_type)( LPCWSTR pszSound, UINT fuSound);
extern sndPlaySoundW_type sndPlaySoundW_orig;
typedef BOOL (WINAPI *PlaySoundA_type)( LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
extern PlaySoundA_type PlaySoundA_orig;
typedef BOOL (WINAPI *PlaySoundW_type)( LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);
extern PlaySoundW_type PlaySoundW_orig;
typedef UINT (WINAPI *waveOutGetNumDevs_type)(void);
extern waveOutGetNumDevs_type waveOutGetNumDevs_orig;
typedef MMRESULT (WINAPI *waveOutGetDevCapsA_type)( UINT_PTR uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc);
extern waveOutGetDevCapsA_type waveOutGetDevCapsA_orig;
typedef MMRESULT (WINAPI *waveOutGetDevCapsW_type)( UINT_PTR uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc);
extern waveOutGetDevCapsW_type waveOutGetDevCapsW_orig;
typedef MMRESULT (WINAPI *waveOutGetVolume_type)( HWAVEOUT hwo, LPDWORD pdwVolume);
extern waveOutGetVolume_type waveOutGetVolume_orig;
typedef MMRESULT (WINAPI *waveOutSetVolume_type)( HWAVEOUT hwo, DWORD dwVolume);
extern waveOutSetVolume_type waveOutSetVolume_orig;
typedef MMRESULT (WINAPI *waveOutGetErrorTextA_type)( MMRESULT mmrError, LPSTR pszText, UINT cchText);
extern waveOutGetErrorTextA_type waveOutGetErrorTextA_orig;
typedef MMRESULT (WINAPI *waveOutGetErrorTextW_type)( MMRESULT mmrError, LPWSTR pszText, UINT cchText);
extern waveOutGetErrorTextW_type waveOutGetErrorTextW_orig;
typedef MMRESULT (WINAPI *waveOutOpen_type)( LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
extern waveOutOpen_type waveOutOpen_orig;
typedef MMRESULT (WINAPI *waveOutClose_type)( HWAVEOUT hwo);
extern waveOutClose_type waveOutClose_orig;
typedef MMRESULT (WINAPI *waveOutPrepareHeader_type)( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
extern waveOutPrepareHeader_type waveOutPrepareHeader_orig;
typedef MMRESULT (WINAPI *waveOutUnprepareHeader_type)( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
extern waveOutUnprepareHeader_type waveOutUnprepareHeader_orig;
typedef MMRESULT (WINAPI *waveOutWrite_type)( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
extern waveOutWrite_type waveOutWrite_orig;
typedef MMRESULT (WINAPI *waveOutPause_type)( HWAVEOUT hwo);
extern waveOutPause_type waveOutPause_orig;
typedef MMRESULT (WINAPI *waveOutRestart_type)( HWAVEOUT hwo);
extern waveOutRestart_type waveOutRestart_orig;
typedef MMRESULT (WINAPI *waveOutReset_type)( HWAVEOUT hwo);
extern waveOutReset_type waveOutReset_orig;
typedef MMRESULT (WINAPI *waveOutBreakLoop_type)( HWAVEOUT hwo);
extern waveOutBreakLoop_type waveOutBreakLoop_orig;
typedef MMRESULT (WINAPI *waveOutGetPosition_type)( HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt);
extern waveOutGetPosition_type waveOutGetPosition_orig;
typedef MMRESULT (WINAPI *waveOutGetPitch_type)( HWAVEOUT hwo, LPDWORD pdwPitch);
extern waveOutGetPitch_type waveOutGetPitch_orig;
typedef MMRESULT (WINAPI *waveOutSetPitch_type)( HWAVEOUT hwo, DWORD dwPitch);
extern waveOutSetPitch_type waveOutSetPitch_orig;
typedef MMRESULT (WINAPI *waveOutGetPlaybackRate_type)( HWAVEOUT hwo, LPDWORD pdwRate);
extern waveOutGetPlaybackRate_type waveOutGetPlaybackRate_orig;
typedef MMRESULT (WINAPI *waveOutSetPlaybackRate_type)( HWAVEOUT hwo, DWORD dwRate);
extern waveOutSetPlaybackRate_type waveOutSetPlaybackRate_orig;
typedef MMRESULT (WINAPI *waveOutGetID_type)( HWAVEOUT hwo, LPUINT puDeviceID);
extern waveOutGetID_type waveOutGetID_orig;
typedef MMRESULT (WINAPI *waveOutMessage_type)( HWAVEOUT hwo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2);
extern waveOutMessage_type waveOutMessage_orig;
typedef UINT (WINAPI *waveInGetNumDevs_type)(void);
extern waveInGetNumDevs_type waveInGetNumDevs_orig;
typedef MMRESULT (WINAPI *waveInGetDevCapsA_type)( UINT_PTR uDeviceID, LPWAVEINCAPSA pwic, UINT cbwic);
extern waveInGetDevCapsA_type waveInGetDevCapsA_orig;
typedef MMRESULT (WINAPI *waveInGetDevCapsW_type)( UINT_PTR uDeviceID, LPWAVEINCAPSW pwic, UINT cbwic);
extern waveInGetDevCapsW_type waveInGetDevCapsW_orig;
typedef MMRESULT (WINAPI *waveInGetErrorTextA_type)(MMRESULT mmrError, LPSTR pszText, UINT cchText);
extern waveInGetErrorTextA_type waveInGetErrorTextA_orig;
typedef MMRESULT (WINAPI *waveInGetErrorTextW_type)(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
extern waveInGetErrorTextW_type waveInGetErrorTextW_orig;
typedef MMRESULT (WINAPI *waveInOpen_type)( LPHWAVEIN phwi, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
extern waveInOpen_type waveInOpen_orig;
typedef MMRESULT (WINAPI *waveInClose_type)( HWAVEIN hwi);
extern waveInClose_type waveInClose_orig;
typedef MMRESULT (WINAPI *waveInPrepareHeader_type)( HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh);
extern waveInPrepareHeader_type waveInPrepareHeader_orig;
typedef MMRESULT (WINAPI *waveInUnprepareHeader_type)( HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh);
extern waveInUnprepareHeader_type waveInUnprepareHeader_orig;
typedef MMRESULT (WINAPI *waveInAddBuffer_type)( HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh);
extern waveInAddBuffer_type waveInAddBuffer_orig;
typedef MMRESULT (WINAPI *waveInStart_type)( HWAVEIN hwi);
extern waveInStart_type waveInStart_orig;
typedef MMRESULT (WINAPI *waveInStop_type)( HWAVEIN hwi);
extern waveInStop_type waveInStop_orig;
typedef MMRESULT (WINAPI *waveInReset_type)( HWAVEIN hwi);
extern waveInReset_type waveInReset_orig;
typedef MMRESULT (WINAPI *waveInGetPosition_type)( HWAVEIN hwi, LPMMTIME pmmt, UINT cbmmt);
extern waveInGetPosition_type waveInGetPosition_orig;
typedef MMRESULT (WINAPI *waveInGetID_type)( HWAVEIN hwi, LPUINT puDeviceID);
extern waveInGetID_type waveInGetID_orig;
typedef MMRESULT (WINAPI *waveInMessage_type)( HWAVEIN hwi, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2);
extern waveInMessage_type waveInMessage_orig;
typedef UINT (WINAPI *midiOutGetNumDevs_type)(void);
extern midiOutGetNumDevs_type midiOutGetNumDevs_orig;
typedef MMRESULT (WINAPI *midiStreamOpen_type)( LPHMIDISTRM phms, LPUINT puDeviceID, DWORD cMidi, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
extern midiStreamOpen_type midiStreamOpen_orig;
typedef MMRESULT (WINAPI *midiStreamClose_type)( HMIDISTRM hms);
extern midiStreamClose_type midiStreamClose_orig;
typedef MMRESULT (WINAPI *midiStreamProperty_type)( HMIDISTRM hms, LPBYTE lppropdata, DWORD dwProperty);
extern midiStreamProperty_type midiStreamProperty_orig;
typedef MMRESULT (WINAPI *midiStreamPosition_type)( HMIDISTRM hms, LPMMTIME lpmmt, UINT cbmmt);
extern midiStreamPosition_type midiStreamPosition_orig;
typedef MMRESULT (WINAPI *midiStreamOut_type)( HMIDISTRM hms, LPMIDIHDR pmh, UINT cbmh);
extern midiStreamOut_type midiStreamOut_orig;
typedef MMRESULT (WINAPI *midiStreamPause_type)( HMIDISTRM hms);
extern midiStreamPause_type midiStreamPause_orig;
typedef MMRESULT (WINAPI *midiStreamRestart_type)( HMIDISTRM hms);
extern midiStreamRestart_type midiStreamRestart_orig;
typedef MMRESULT (WINAPI *midiStreamStop_type)( HMIDISTRM hms);
extern midiStreamStop_type midiStreamStop_orig;
typedef MMRESULT (WINAPI *midiConnect_type)( HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved);
extern midiConnect_type midiConnect_orig;
typedef MMRESULT (WINAPI *midiDisconnect_type)( HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved);
extern midiDisconnect_type midiDisconnect_orig;
typedef MMRESULT (WINAPI *midiOutGetDevCapsA_type)( UINT_PTR uDeviceID, LPMIDIOUTCAPSA pmoc, UINT cbmoc);
extern midiOutGetDevCapsA_type midiOutGetDevCapsA_orig;
typedef MMRESULT (WINAPI *midiOutGetDevCapsW_type)( UINT_PTR uDeviceID, LPMIDIOUTCAPSW pmoc, UINT cbmoc);
extern midiOutGetDevCapsW_type midiOutGetDevCapsW_orig;
typedef MMRESULT (WINAPI *midiOutGetVolume_type)( HMIDIOUT hmo, LPDWORD pdwVolume);
extern midiOutGetVolume_type midiOutGetVolume_orig;
typedef MMRESULT (WINAPI *midiOutSetVolume_type)( HMIDIOUT hmo, DWORD dwVolume);
extern midiOutSetVolume_type midiOutSetVolume_orig;
typedef MMRESULT (WINAPI *midiOutGetErrorTextA_type)( MMRESULT mmrError, LPSTR pszText, UINT cchText);
extern midiOutGetErrorTextA_type midiOutGetErrorTextA_orig;
typedef MMRESULT (WINAPI *midiOutGetErrorTextW_type)( MMRESULT mmrError, LPWSTR pszText, UINT cchText);
extern midiOutGetErrorTextW_type midiOutGetErrorTextW_orig;
typedef MMRESULT (WINAPI *midiOutOpen_type)( LPHMIDIOUT phmo, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
extern midiOutOpen_type midiOutOpen_orig;
typedef MMRESULT (WINAPI *midiOutClose_type)( HMIDIOUT hmo);
extern midiOutClose_type midiOutClose_orig;
typedef MMRESULT (WINAPI *midiOutPrepareHeader_type)( HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh);
extern midiOutPrepareHeader_type midiOutPrepareHeader_orig;
typedef MMRESULT (WINAPI *midiOutUnprepareHeader_type)(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh);
extern midiOutUnprepareHeader_type midiOutUnprepareHeader_orig;
typedef MMRESULT (WINAPI *midiOutShortMsg_type)( HMIDIOUT hmo, DWORD dwMsg);
extern midiOutShortMsg_type midiOutShortMsg_orig;
typedef MMRESULT (WINAPI *midiOutLongMsg_type)(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh);
extern midiOutLongMsg_type midiOutLongMsg_orig;
typedef MMRESULT (WINAPI *midiOutReset_type)( HMIDIOUT hmo);
extern midiOutReset_type midiOutReset_orig;
typedef MMRESULT (WINAPI *midiOutCachePatches_type)( HMIDIOUT hmo, UINT uBank, LPWORD pwpa, UINT fuCache);
extern midiOutCachePatches_type midiOutCachePatches_orig;
typedef MMRESULT (WINAPI *midiOutCacheDrumPatches_type)( HMIDIOUT hmo, UINT uPatch, LPWORD pwkya, UINT fuCache);
extern midiOutCacheDrumPatches_type midiOutCacheDrumPatches_orig;
typedef MMRESULT (WINAPI *midiOutGetID_type)( HMIDIOUT hmo, LPUINT puDeviceID);
extern midiOutGetID_type midiOutGetID_orig;
typedef MMRESULT (WINAPI *midiOutMessage_type)( HMIDIOUT hmo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2);
extern midiOutMessage_type midiOutMessage_orig;
typedef UINT (WINAPI *midiInGetNumDevs_type)(void);
extern midiInGetNumDevs_type midiInGetNumDevs_orig;
typedef MMRESULT (WINAPI *midiInGetDevCapsA_type)( UINT_PTR uDeviceID, LPMIDIINCAPSA pmic, UINT cbmic);
extern midiInGetDevCapsA_type midiInGetDevCapsA_orig;
typedef MMRESULT (WINAPI *midiInGetDevCapsW_type)( UINT_PTR uDeviceID, LPMIDIINCAPSW pmic, UINT cbmic);
extern midiInGetDevCapsW_type midiInGetDevCapsW_orig;
typedef MMRESULT (WINAPI *midiInGetErrorTextA_type)( MMRESULT mmrError, LPSTR pszText, UINT cchText);
extern midiInGetErrorTextA_type midiInGetErrorTextA_orig;
typedef MMRESULT (WINAPI *midiInGetErrorTextW_type)( MMRESULT mmrError, LPWSTR pszText, UINT cchText);
extern midiInGetErrorTextW_type midiInGetErrorTextW_orig;
typedef MMRESULT (WINAPI *midiInOpen_type)( LPHMIDIIN phmi, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
extern midiInOpen_type midiInOpen_orig;
typedef MMRESULT (WINAPI *midiInClose_type)( HMIDIIN hmi);
extern midiInClose_type midiInClose_orig;
typedef MMRESULT (WINAPI *midiInPrepareHeader_type)( HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh);
extern midiInPrepareHeader_type midiInPrepareHeader_orig;
typedef MMRESULT (WINAPI *midiInUnprepareHeader_type)( HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh);
extern midiInUnprepareHeader_type midiInUnprepareHeader_orig;
typedef MMRESULT (WINAPI *midiInAddBuffer_type)( HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh);
extern midiInAddBuffer_type midiInAddBuffer_orig;
typedef MMRESULT (WINAPI *midiInStart_type)( HMIDIIN hmi);
extern midiInStart_type midiInStart_orig;
typedef MMRESULT (WINAPI *midiInStop_type)( HMIDIIN hmi);
extern midiInStop_type midiInStop_orig;
typedef MMRESULT (WINAPI *midiInReset_type)( HMIDIIN hmi);
extern midiInReset_type midiInReset_orig;
typedef MMRESULT (WINAPI *midiInGetID_type)( HMIDIIN hmi, LPUINT puDeviceID);
extern midiInGetID_type midiInGetID_orig;
typedef MMRESULT (WINAPI *midiInMessage_type)( HMIDIIN hmi, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2);
extern midiInMessage_type midiInMessage_orig;
typedef UINT (WINAPI *auxGetNumDevs_type)(void);
extern auxGetNumDevs_type auxGetNumDevs_orig;
typedef MMRESULT (WINAPI *auxGetDevCapsA_type)( UINT_PTR uDeviceID, LPAUXCAPSA pac, UINT cbac);
extern auxGetDevCapsA_type auxGetDevCapsA_orig;
typedef MMRESULT (WINAPI *auxGetDevCapsW_type)( UINT_PTR uDeviceID, LPAUXCAPSW pac, UINT cbac);
extern auxGetDevCapsW_type auxGetDevCapsW_orig;
typedef MMRESULT (WINAPI *auxSetVolume_type)( UINT uDeviceID, DWORD dwVolume);
extern auxSetVolume_type auxSetVolume_orig;
typedef MMRESULT (WINAPI *auxGetVolume_type)( UINT uDeviceID, LPDWORD pdwVolume);
extern auxGetVolume_type auxGetVolume_orig;
typedef MMRESULT (WINAPI *auxOutMessage_type)( UINT uDeviceID, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2);
extern auxOutMessage_type auxOutMessage_orig;
typedef UINT (WINAPI *mixerGetNumDevs_type)(void);
extern mixerGetNumDevs_type mixerGetNumDevs_orig;
typedef MMRESULT (WINAPI *mixerGetDevCapsA_type)( UINT_PTR uMxId, LPMIXERCAPSA pmxcaps, UINT cbmxcaps);
extern mixerGetDevCapsA_type mixerGetDevCapsA_orig;
typedef MMRESULT (WINAPI *mixerGetDevCapsW_type)( UINT_PTR uMxId, LPMIXERCAPSW pmxcaps, UINT cbmxcaps);
extern mixerGetDevCapsW_type mixerGetDevCapsW_orig;
typedef MMRESULT (WINAPI *mixerOpen_type)( LPHMIXER phmx, UINT uMxId, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
extern mixerOpen_type mixerOpen_orig;
typedef MMRESULT (WINAPI *mixerClose_type)( HMIXER hmx);
extern mixerClose_type mixerClose_orig;
typedef DWORD (WINAPI *mixerMessage_type)( HMIXER hmx, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
extern mixerMessage_type mixerMessage_orig;
typedef MMRESULT (WINAPI *mixerGetLineInfoA_type)( HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo);
extern mixerGetLineInfoA_type mixerGetLineInfoA_orig;
typedef MMRESULT (WINAPI *mixerGetLineInfoW_type)( HMIXEROBJ hmxobj, LPMIXERLINEW pmxl, DWORD fdwInfo);
extern mixerGetLineInfoW_type mixerGetLineInfoW_orig;
typedef MMRESULT (WINAPI *mixerGetID_type)( HMIXEROBJ hmxobj, UINT FAR *puMxId, DWORD fdwId);
extern mixerGetID_type mixerGetID_orig;
typedef MMRESULT (WINAPI *mixerGetLineControlsA_type)( HMIXEROBJ hmxobj, LPMIXERLINECONTROLSA pmxlc, DWORD fdwControls);
extern mixerGetLineControlsA_type mixerGetLineControlsA_orig;
typedef MMRESULT (WINAPI *mixerGetLineControlsW_type)( HMIXEROBJ hmxobj, LPMIXERLINECONTROLSW pmxlc, DWORD fdwControls);
extern mixerGetLineControlsW_type mixerGetLineControlsW_orig;
typedef MMRESULT (WINAPI *mixerGetControlDetailsA_type)( HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
extern mixerGetControlDetailsA_type mixerGetControlDetailsA_orig;
typedef MMRESULT (WINAPI *mixerGetControlDetailsW_type)( HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
extern mixerGetControlDetailsW_type mixerGetControlDetailsW_orig;
typedef MMRESULT (WINAPI *mixerSetControlDetails_type)( HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
extern mixerSetControlDetails_type mixerSetControlDetails_orig;
typedef DWORD    (WINAPI *mmGetCurrentTask_type)(void);
extern mmGetCurrentTask_type mmGetCurrentTask_orig;
typedef void (WINAPI *mmTaskBlock_type)(DWORD);
extern mmTaskBlock_type mmTaskBlock_orig;
typedef UINT (WINAPI *mmTaskCreate_type)(LPTASKCALLBACK, HANDLE*, DWORD_PTR);
extern mmTaskCreate_type mmTaskCreate_orig;
typedef BOOL (WINAPI *mmTaskSignal_type)(DWORD);
extern mmTaskSignal_type mmTaskSignal_orig;
typedef VOID (WINAPI *mmTaskYield_type)(VOID);
extern mmTaskYield_type mmTaskYield_orig;
typedef MMRESULT (WINAPI *timeGetSystemTime_type)( LPMMTIME pmmt, UINT cbmmt);
extern timeGetSystemTime_type timeGetSystemTime_orig;
typedef DWORD (WINAPI *timeGetTime_type)(void);
extern timeGetTime_type timeGetTime_orig;
typedef MMRESULT (WINAPI *timeSetEvent_type)( UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc, DWORD_PTR dwUser, UINT fuEvent);
extern timeSetEvent_type timeSetEvent_orig;
typedef MMRESULT (WINAPI *timeKillEvent_type)( UINT uTimerID);
extern timeKillEvent_type timeKillEvent_orig;
typedef MMRESULT (WINAPI *timeGetDevCaps_type)( LPTIMECAPS ptc, UINT cbtc);
extern timeGetDevCaps_type timeGetDevCaps_orig;
typedef MMRESULT (WINAPI *timeBeginPeriod_type)( UINT uPeriod);
extern timeBeginPeriod_type timeBeginPeriod_orig;
typedef MMRESULT (WINAPI *timeEndPeriod_type)( UINT uPeriod);
extern timeEndPeriod_type timeEndPeriod_orig;
typedef UINT (WINAPI *joyGetNumDevs_type)(void);
extern joyGetNumDevs_type joyGetNumDevs_orig;
typedef MMRESULT (WINAPI *joyConfigChanged_type)(DWORD dwFlags);
extern joyConfigChanged_type joyConfigChanged_orig;
typedef MMRESULT (WINAPI *joyGetDevCapsA_type)( UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc);
extern joyGetDevCapsA_type joyGetDevCapsA_orig;
typedef MMRESULT (WINAPI *joyGetDevCapsW_type)( UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc);
extern joyGetDevCapsW_type joyGetDevCapsW_orig;
typedef MMRESULT (WINAPI *joyGetPos_type)( UINT uJoyID, LPJOYINFO pji);
extern joyGetPos_type joyGetPos_orig;
typedef MMRESULT (WINAPI *joyGetPosEx_type)( UINT uJoyID, LPJOYINFOEX pji);
extern joyGetPosEx_type joyGetPosEx_orig;
typedef MMRESULT (WINAPI *joyGetThreshold_type)( UINT uJoyID, LPUINT puThreshold);
extern joyGetThreshold_type joyGetThreshold_orig;
typedef MMRESULT (WINAPI *joyReleaseCapture_type)( UINT uJoyID);
extern joyReleaseCapture_type joyReleaseCapture_orig;
typedef MMRESULT (WINAPI *joySetCapture_type)( HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged);
extern joySetCapture_type joySetCapture_orig;
typedef MMRESULT (WINAPI *joySetThreshold_type)( UINT uJoyID, UINT uThreshold);
extern joySetThreshold_type joySetThreshold_orig;
typedef BOOL (WINAPI  *mciDriverNotify_type)(HWND hwndCallback, UINT uDeviceID, UINT uStatus);
extern mciDriverNotify_type mciDriverNotify_orig;
typedef UINT (WINAPI  *mciDriverYield_type)(UINT uDeviceID);
extern mciDriverYield_type mciDriverYield_orig;
typedef FOURCC (WINAPI *mmioStringToFOURCCA_type)( LPCSTR sz, UINT uFlags);
extern mmioStringToFOURCCA_type mmioStringToFOURCCA_orig;
typedef FOURCC (WINAPI *mmioStringToFOURCCW_type)( LPCWSTR sz, UINT uFlags);
extern mmioStringToFOURCCW_type mmioStringToFOURCCW_orig;
typedef LPMMIOPROC (WINAPI *mmioInstallIOProcA_type)( FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags);
extern mmioInstallIOProcA_type mmioInstallIOProcA_orig;
typedef LPMMIOPROC (WINAPI *mmioInstallIOProcW_type)( FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags);
extern mmioInstallIOProcW_type mmioInstallIOProcW_orig;
typedef HMMIO (WINAPI *mmioOpenA_type)( LPSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen);
extern mmioOpenA_type mmioOpenA_orig;
typedef HMMIO (WINAPI *mmioOpenW_type)( LPWSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen);
extern mmioOpenW_type mmioOpenW_orig;
typedef MMRESULT (WINAPI *mmioRenameA_type)( LPCSTR pszFileName, LPCSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename);
extern mmioRenameA_type mmioRenameA_orig;
typedef MMRESULT (WINAPI *mmioRenameW_type)( LPCWSTR pszFileName, LPCWSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename);
extern mmioRenameW_type mmioRenameW_orig;
typedef MMRESULT (WINAPI *mmioClose_type)( HMMIO hmmio, UINT fuClose);
extern mmioClose_type mmioClose_orig;
typedef LONG (WINAPI *mmioRead_type)( HMMIO hmmio, HPSTR pch, LONG cch);
extern mmioRead_type mmioRead_orig;
typedef LONG (WINAPI *mmioWrite_type)( HMMIO hmmio, const char _huge* pch, LONG cch);
extern mmioWrite_type mmioWrite_orig;
typedef LONG (WINAPI *mmioSeek_type)( HMMIO hmmio, LONG lOffset, int iOrigin);
extern mmioSeek_type mmioSeek_orig;
typedef MMRESULT (WINAPI *mmioGetInfo_type)( HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuInfo);
extern mmioGetInfo_type mmioGetInfo_orig;
typedef MMRESULT (WINAPI *mmioSetInfo_type)( HMMIO hmmio, LPCMMIOINFO pmmioinfo, UINT fuInfo);
extern mmioSetInfo_type mmioSetInfo_orig;
typedef MMRESULT (WINAPI *mmioSetBuffer_type)( HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer, UINT fuBuffer);
extern mmioSetBuffer_type mmioSetBuffer_orig;
typedef MMRESULT (WINAPI *mmioFlush_type)( HMMIO hmmio, UINT fuFlush);
extern mmioFlush_type mmioFlush_orig;
typedef MMRESULT (WINAPI *mmioAdvance_type)( HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuAdvance);
extern mmioAdvance_type mmioAdvance_orig;
typedef LRESULT (WINAPI *mmioSendMessage_type)( HMMIO hmmio, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
extern mmioSendMessage_type mmioSendMessage_orig;
typedef MMRESULT (WINAPI *mmioDescend_type)( HMMIO hmmio, LPMMCKINFO pmmcki, const MMCKINFO FAR* pmmckiParent, UINT fuDescend);
extern mmioDescend_type mmioDescend_orig;
typedef MMRESULT (WINAPI *mmioAscend_type)( HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuAscend);
extern mmioAscend_type mmioAscend_orig;
typedef MMRESULT (WINAPI *mmioCreateChunk_type)(HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuCreate);
extern mmioCreateChunk_type mmioCreateChunk_orig;
typedef MCIERROR (WINAPI *mciSendCommandA_type)( MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
extern mciSendCommandA_type mciSendCommandA_orig;
typedef MCIERROR (WINAPI *mciSendCommandW_type)( MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
extern mciSendCommandW_type mciSendCommandW_orig;
typedef MCIERROR  (WINAPI *mciSendStringA_type)( LPCSTR lpstrCommand, LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback);
extern mciSendStringA_type mciSendStringA_orig;
typedef MCIERROR  (WINAPI *mciSendStringW_type)( LPCWSTR lpstrCommand, LPWSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback);
extern mciSendStringW_type mciSendStringW_orig;
typedef MCIDEVICEID (WINAPI *mciGetDeviceIDA_type)( LPCSTR pszDevice);
extern mciGetDeviceIDA_type mciGetDeviceIDA_orig;
typedef MCIDEVICEID (WINAPI *mciGetDeviceIDW_type)( LPCWSTR pszDevice);
extern mciGetDeviceIDW_type mciGetDeviceIDW_orig;
typedef MCIDEVICEID (WINAPI *mciGetDeviceIDFromElementIDA_type)( DWORD dwElementID, LPCSTR lpstrType );
extern mciGetDeviceIDFromElementIDA_type mciGetDeviceIDFromElementIDA_orig;
typedef MCIDEVICEID (WINAPI *mciGetDeviceIDFromElementIDW_type)( DWORD dwElementID, LPCWSTR lpstrType );
extern mciGetDeviceIDFromElementIDW_type mciGetDeviceIDFromElementIDW_orig;
typedef DWORD_PTR (WINAPI  *mciGetDriverData_type)(UINT uDeviceID);
extern mciGetDriverData_type mciGetDriverData_orig;
typedef BOOL (WINAPI *mciGetErrorStringA_type)( MCIERROR mcierr, LPSTR pszText, UINT cchText);
extern mciGetErrorStringA_type mciGetErrorStringA_orig;
typedef BOOL (WINAPI *mciGetErrorStringW_type)( MCIERROR mcierr, LPWSTR pszText, UINT cchText);
extern mciGetErrorStringW_type mciGetErrorStringW_orig;
typedef BOOL (WINAPI  *mciSetDriverData_type)(UINT uDeviceID, DWORD_PTR dwData);
extern mciSetDriverData_type mciSetDriverData_orig;
typedef BOOL (WINAPI *mciSetYieldProc_type)( MCIDEVICEID mciId, YIELDPROC fpYieldProc, DWORD dwYieldData);
extern mciSetYieldProc_type mciSetYieldProc_orig;
typedef BOOL (WINAPI  *mciFreeCommandResource_type)(UINT uTable);
extern mciFreeCommandResource_type mciFreeCommandResource_orig;
typedef HTASK (WINAPI *mciGetCreatorTask_type)( MCIDEVICEID mciId);
extern mciGetCreatorTask_type mciGetCreatorTask_orig;
typedef YIELDPROC (WINAPI *mciGetYieldProc_type)( MCIDEVICEID mciId, LPDWORD pdwYieldData);
extern mciGetYieldProc_type mciGetYieldProc_orig;
typedef UINT (WINAPI *mciLoadCommandResource_type)(HINSTANCE hInstance, LPCWSTR lpResName, UINT uType);
extern mciLoadCommandResource_type mciLoadCommandResource_orig;
typedef BOOL (WINAPI *mciExecute_type)(LPCSTR pszCommand);
extern mciExecute_type mciExecute_orig;
