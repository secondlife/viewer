/** 
 * @file patch_code.h
 * @brief Function declarations for encoding and decoding patches.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_PATCH_CODE_H
#define LL_PATCH_CODE_H

class LLBitPack;
class LLGroupHeader;
class LLPatchHeader;

void	init_patch_coding(LLBitPack &bitpack);
void	code_patch_group_header(LLBitPack &bitpack, LLGroupHeader *gopp);
void	code_patch_header(LLBitPack &bitpack, LLPatchHeader *ph, S32 *patch);
void	code_end_of_data(LLBitPack &bitpack);
void	code_patch(LLBitPack &bitpack, S32 *patch, S32 postquant);
void	end_patch_coding(LLBitPack &bitpack);

void	init_patch_decoding(LLBitPack &bitpack);
void	decode_patch_group_header(LLBitPack &bitpack, LLGroupHeader *gopp);
void	decode_patch_header(LLBitPack &bitpack, LLPatchHeader *ph);
void	decode_patch(LLBitPack &bitpack, S32 *patches);

#endif
