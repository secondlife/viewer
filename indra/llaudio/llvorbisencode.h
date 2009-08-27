/** 
 * @file vorbisencode.h
 * @brief Vorbis encoding routine routine for Indra.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_VORBISENCODE_H
#define LL_VORBISENCODE_H

const S32 LLVORBISENC_NOERR                        = 0; // no error
const S32 LLVORBISENC_SOURCE_OPEN_ERR              = 1; // error opening source
const S32 LLVORBISENC_DEST_OPEN_ERR                = 2; // error opening destination
const S32 LLVORBISENC_WAV_FORMAT_ERR               = 3; // not a WAV
const S32 LLVORBISENC_PCM_FORMAT_ERR               = 4; // not a PCM
const S32 LLVORBISENC_MONO_ERR                     = 5; // can't do mono
const S32 LLVORBISENC_STEREO_ERR                   = 6; // can't do stereo
const S32 LLVORBISENC_MULTICHANNEL_ERR             = 7; // can't do stereo
const S32 LLVORBISENC_UNSUPPORTED_SAMPLE_RATE      = 8; // unsupported sample rate
const S32 LLVORBISENC_UNSUPPORTED_WORD_SIZE        = 9; // unsupported word size
const S32 LLVORBISENC_CLIP_TOO_LONG                = 10; // source file is too long


S32 check_for_invalid_wav_formats(const std::string& in_fname, std::string& error_msg);
S32 encode_vorbis_file(const std::string& in_fname, const std::string& out_fname);

#endif

