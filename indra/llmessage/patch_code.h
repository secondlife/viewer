/** 
 * @file patch_code.h
 * @brief Function declarations for encoding and decoding patches.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
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
