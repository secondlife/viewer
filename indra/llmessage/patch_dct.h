/** 
 * @file patch_dct.h
 * @brief Function declarations for DCT and IDCT routines
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

#ifndef LL_PATCH_DCT_H
#define LL_PATCH_DCT_H

class LLVector3;

// Code Values
const U8 ZERO_CODE	= 0x0;
const U8 ZERO_EOB = 0x2;
const U8 POSITIVE_VALUE = 0x6;
const U8 NEGATIVE_VALUE = 0x7;

const S8 NORMAL_PATCH_SIZE	= 16;
const S8 LARGE_PATCH_SIZE = 32;

const U8 END_OF_PATCHES = 97;

#define _PATCH_SIZE_16_AND_32_ONLY

// Top level header for group of headers
//typedef struct LL_Group_Header
//{
//	U16  stride;		// 2 = 2
//	U8  patch_size;		// 1 = 3
//	U8  layer_type;		// 1 = 4
//} LLGroupHeader;

class LLGroupHeader
{
public:
	U16	stride;			// 2 = 2
	U8	patch_size;		// 1 = 3
	U8	layer_type;		// 1 = 4
};

// Individual patch header

//typedef struct LL_Patch_Header
//{
//	F32 dc_offset;		// 4 bytes
//	U16 range;			// 2 = 7 ((S16) FP range (breaks if we need > 32K meters in 1 patch)
//	U8  quant_wbits;	// 1 = 8 (upper 4 bits is quant - 2, lower 4 bits is word bits - 2)
//	U16	patchids;		// 2 = 10 (actually only uses 10 bits, 5 for each)
//} LLPatchHeader;
class LLPatchHeader
{
public:
	F32	dc_offset;		// 4 bytes
	U16	range;			// 2 = 7 ((S16) FP range (breaks if we need > 32K meters in 1 patch)
	U8	quant_wbits;	// 1 = 8 (upper 4 bits is quant - 2, lower 4 bits is word bits - 2)
	U16	patchids;		// 2 = 10 (actually only uses 10 bits, 5 for each)
};

// Compression routines
void init_patch_compressor(S32 patch_size, S32 patch_stride, S32 layer_type);
void prescan_patch(F32 *patch, LLPatchHeader *php, F32 &zmax, F32 &zmin);
void compress_patch(F32 *patch, S32 *cpatch, LLPatchHeader *php, S32 prequant);
void get_patch_group_header(LLGroupHeader *gopp);

// Decompression routines
void set_group_of_patch_header(LLGroupHeader *gopp);
void init_patch_decompressor(S32 size);
void decompress_patch(F32 *patch, S32 *cpatch, LLPatchHeader *ph);
void decompress_patchv(LLVector3 *v, S32 *cpatch, LLPatchHeader *ph);

#endif
