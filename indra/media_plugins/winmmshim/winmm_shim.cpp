/** 
 * @file winmmshim.cpp
 * @brief controls volume level of process by intercepting calls to winmm.dll
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
#include "forwarding_api.h"
#include <xmmintrin.h>
#include <map>
#include <math.h>

using std::wstring;

static float sVolumeLevel = 1.f;
static bool sMute = false;
static CRITICAL_SECTION sCriticalSection;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	InitializeCriticalSection(&sCriticalSection);
	return TRUE;
}

void ll_winmm_shim_initialize(){
	static bool initialized = false;
	// do this only once
	EnterCriticalSection(&sCriticalSection);
	if (!initialized)
	{	// bind to original winmm.dll
		TCHAR system_path[MAX_PATH];
		TCHAR dll_path[MAX_PATH];
		::GetSystemDirectory(system_path, MAX_PATH);

		// grab winmm.dll from system path, where it should live
		wsprintf(dll_path, "%s\\winmm.dll", system_path);
		HMODULE winmm_handle = ::LoadLibrary(dll_path);
		
		if (winmm_handle != NULL)
		{	// we have a dll, let's get out pointers!
			initialized = true;
			init_function_pointers(winmm_handle);
			::OutputDebugStringA("WINMM_SHIM.DLL: real winmm.dll initialized successfully\n");
		}
		else
		{
			// failed to initialize real winmm.dll
			::OutputDebugStringA("WINMM_SHIM.DLL: Failed to initialize real winmm.dll\n");
		}
	}
	LeaveCriticalSection(&sCriticalSection);
}


extern "C" 
{
	// tracks the requested format for a given waveout buffer
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
		ll_winmm_shim_initialize();
		if (pwfx->wFormatTag != WAVE_FORMAT_PCM
			|| (pwfx->wBitsPerSample != 8 && pwfx->wBitsPerSample != 16))
		{ // uncompressed 8 and 16 bit sound are the only types we support
			return WAVERR_BADFORMAT;
		}

		MMRESULT result = waveOutOpen_orig(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
		if (result == MMSYSERR_NOERROR 
			&& ((fdwOpen & WAVE_FORMAT_QUERY) == 0)) // not just querying for format support
		{	// remember the requested bits per sample, and associate with the given handle
			WaveOutFormat* wave_outp = new WaveOutFormat(pwfx->wBitsPerSample);
			sWaveOuts.insert(std::make_pair(*phwo, wave_outp));
		}
		return result;
	}

	MMRESULT WINAPI waveOutClose( HWAVEOUT hwo)
	{
		ll_winmm_shim_initialize();
		wave_out_map_t::iterator found_it = sWaveOuts.find(hwo);
		if (found_it != sWaveOuts.end())
		{	// forget what we know about this handle
			delete found_it->second;
			sWaveOuts.erase(found_it);
		}
		return waveOutClose_orig( hwo);
	}

	MMRESULT WINAPI waveOutWrite( HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
	{
		ll_winmm_shim_initialize();
		MMRESULT result = MMSYSERR_NOERROR;

		if (sMute)
		{ // zero out the audio buffer when muted
			memset(pwh->lpData, 0, pwh->dwBufferLength);
		}
		else if (sVolumeLevel != 1.f) 
		{ // need to apply volume level
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
							// unsigned multiply doesn't use most significant bit, so shift by 7 bits
							// to get resulting value back into 8 bits
							pwh->lpData[i] = (pwh->lpData[i] * volume) >> 7;
						}
						break;
					}
				case 16:
					{
						short volume_16 = (short)(sVolumeLevel * 32767.f);

						// copy volume level 4 times into 64 bit MMX register
						__m64 volume_64 = _mm_set_pi16(volume_16, volume_16, volume_16, volume_16);
						__m64* sample_64;
						__m64* last_sample_64 =  (__m64*)(pwh->lpData + pwh->dwBufferLength - sizeof(__m64));
						// for everything that can be addressed in 64 bit multiples...
						for (sample_64 = (__m64*)pwh->lpData;
							sample_64 <= last_sample_64;
							++sample_64)
						{
							//...multiply the samples by the volume...
							__m64 scaled_sample = _mm_mulhi_pi16(*sample_64, volume_64);
							// ...and shift left 1 bit since an unsigned multiple loses the most significant bit
							// 0x7FFF * 0x7FFF = 0x3fff0001
							// 0x3fff0001 << 1 = 0x7ffe0002
							// notice that the LSB is always 0...should consider dithering
							*sample_64 =  _mm_slli_pi16(scaled_sample, 1); 
						}

						// the captain has turned off the MMX sign, you are now free to use floating point registers
						_mm_empty();

						// finish remaining samples that didn't fit into 64 bit register
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
