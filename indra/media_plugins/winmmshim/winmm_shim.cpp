/** 
 * @file winmmshim.cpp
 * @brief controls volume level of process by intercepting calls to winmm.dll
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
#include <xmmintrin.h>
#include <map>
#include <math.h>

using std::wstring;

static float sVolumeLevel = 1.f;
static bool sMute = false;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	static bool initialized = false;
	if (!initialized)
	{
		TCHAR system_path[MAX_PATH];
		TCHAR dll_path[MAX_PATH];
		::GetSystemDirectory(system_path, MAX_PATH);

		wsprintf(dll_path, "%s\\winmm.dll", system_path);
		HMODULE winmm_handle = ::LoadLibrary(dll_path);
		
		if (winmm_handle != NULL)
		{
			initialized = true;
			init_function_pointers(winmm_handle);
			return true;
		}
		return false;
	}
	return true;
}


extern "C" 
{
	struct WaveOutFormat
	{
		WaveOutFormat(int bits_per_sample)
		:	mBitsPerSample(bits_per_sample)
		{}
		int	mBitsPerSample;
	};
	typedef std::map<HWAVEOUT, WaveOutFormat*> wave_out_map_t;
	static wave_out_map_t sWaveOuts;

	MMRESULT WINAPI waveOutOpen( LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
	{
		//OutputDebugString(L"waveOutOpen\n");
		if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
		{
			return WAVERR_BADFORMAT;
		}
		MMRESULT result = waveOutOpen_orig(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
		if (result == MMSYSERR_NOERROR 
			&& ((fdwOpen & WAVE_FORMAT_QUERY) == 0)) // not just querying for format support
		{
			WaveOutFormat* wave_outp = new WaveOutFormat(pwfx->wBitsPerSample);
			sWaveOuts.insert(std::make_pair(*phwo, wave_outp));
		}
		return result;
	}

	MMRESULT WINAPI waveOutClose( HWAVEOUT hwo)
	{
		//OutputDebugString(L"waveOutClose\n");
		wave_out_map_t::iterator found_it = sWaveOuts.find(hwo);
		if (found_it != sWaveOuts.end())
		{
			delete found_it->second;
			sWaveOuts.erase(found_it);
		}
		return waveOutClose_orig( hwo);
	}

	MMRESULT WINAPI waveOutWrite( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
	{
		//OutputDebugString(L"waveOutWrite\n");
		MMRESULT result = MMSYSERR_NOERROR;

		if (sMute)
		{
			memset(pwh->lpData, 0, pwh->dwBufferLength);
		}
		else
		{
			wave_out_map_t::iterator found_it = sWaveOuts.find(hwo);
			if (found_it != sWaveOuts.end())
			{
				WaveOutFormat* formatp = found_it->second;
				switch (formatp->mBitsPerSample){
				case 8:
					{
						char volume = (char)(sVolumeLevel * 127.f);
						for (unsigned int i = 0; i < pwh->dwBufferLength; i++)
						{
							pwh->lpData[i] = (pwh->lpData[i] * volume) >> 7;
						}
						break;
					}
				case 16:
					{
						short volume_16 = (short)(sVolumeLevel * 32767.f);

						__m64 volume_64 = _mm_set_pi16(volume_16, volume_16, volume_16, volume_16);
						__m64 *sample_64;
						for (sample_64 = (__m64*)pwh->lpData;
							sample_64 < (__m64*)(pwh->lpData + pwh->dwBufferLength);
							++sample_64)
						{
							__m64 scaled_sample = _mm_mulhi_pi16(*sample_64, volume_64);
							*sample_64 =  _mm_slli_pi16(scaled_sample, 1); //lose 1 bit of precision here
						}

						_mm_empty();

						for (short* sample_16 = (short*)sample_64;
							sample_16 < (short*)(pwh->lpData + pwh->dwBufferLength);
							++sample_16)
						{
							*sample_16 = (*sample_16 * volume_16) >> 15;
						}

						break;
					}
				default:
					// don't do anything
					break;
				}
			}
		}
		return waveOutWrite_orig( hwo, pwh, cbwh);
	}

	void WINAPI setPluginVolume(float volume)
	{
		sVolumeLevel = volume;
	}

	void WINAPI setPluginMute(bool mute)
	{
		sMute = mute;
	}
}