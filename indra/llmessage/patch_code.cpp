/** 
 * @file patch_code.cpp
 * @brief Encode patch DCT data into bitcode.
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

#include "linden_common.h"

#include "llmath.h"
//#include "vmath.h"
#include "v3math.h"
#include "patch_dct.h"
#include "patch_code.h"
#include "llbitpack.h"

U32 gPatchSize, gWordBits;

void	init_patch_coding(LLBitPack &bitpack)
{
	bitpack.resetBitPacking();
}

void	code_patch_group_header(LLBitPack &bitpack, LLGroupHeader *gopp)
{
#ifdef LL_BIG_ENDIAN
	U8 *stride = (U8 *)&gopp->stride;
	bitpack.bitPack(&(stride[1]), 8);
	bitpack.bitPack(&(stride[0]), 8);
#else
	bitpack.bitPack((U8 *)&gopp->stride, 16);
#endif
	bitpack.bitPack((U8 *)&gopp->patch_size, 8);
	bitpack.bitPack((U8 *)&gopp->layer_type, 8);

	gPatchSize = gopp->patch_size; 
}

void	code_patch_header(LLBitPack &bitpack, LLPatchHeader *ph, S32 *patch)
{
	S32		i, j, temp, patch_size = gPatchSize, wbits = (ph->quant_wbits & 0xf) + 2;
	U32     max_wbits = wbits + 5, min_wbits = wbits>>1;

	wbits = min_wbits;

	for (i = 0; i < (int) patch_size*patch_size; i++)
	{
		temp = patch[i];
		if (temp)
		{
			if (temp < 0)
				temp *= -1;
			for (j = max_wbits; j > (int) min_wbits; j--)
			{
				if (temp & (1<<j))
				{
					if (j > wbits)
						wbits = j;
					break;
				}
			}
		}
	}

	wbits += 1;

	ph->quant_wbits &= 0xf0;

	if (  (wbits > 17)
		||(wbits < 2))
	{
		LL_ERRS() << "Bits needed per word in code_patch_header out of legal range.  Adjust compression quatization." << LL_ENDL;
	}

	ph->quant_wbits |= (wbits - 2);

	bitpack.bitPack((U8 *)&ph->quant_wbits, 8);
#ifdef LL_BIG_ENDIAN
	U8 *offset = (U8 *)&ph->dc_offset;
	bitpack.bitPack(&(offset[3]), 8);
	bitpack.bitPack(&(offset[2]), 8);
	bitpack.bitPack(&(offset[1]), 8);
	bitpack.bitPack(&(offset[0]), 8);
#else
	bitpack.bitPack((U8 *)&ph->dc_offset, 32);
#endif
#ifdef LL_BIG_ENDIAN
	U8 *range = (U8 *)&ph->range;
	bitpack.bitPack(&(range[1]), 8);
	bitpack.bitPack(&(range[0]), 8);
#else
	bitpack.bitPack((U8 *)&ph->range, 16);
#endif
#ifdef LL_BIG_ENDIAN
	U8 *ids = (U8 *)&ph->patchids;
	bitpack.bitPack(&(ids[1]), 8);
	bitpack.bitPack(&(ids[0]), 2);
#else
	bitpack.bitPack((U8 *)&ph->patchids, 10);
#endif

	gWordBits = wbits;
}

void	code_end_of_data(LLBitPack &bitpack)
{
	bitpack.bitPack((U8 *)&END_OF_PATCHES, 8);
}

void code_patch(LLBitPack &bitpack, S32 *patch, S32 postquant)
{
	S32		i, j, patch_size = gPatchSize, wbits = gWordBits;
	S32		temp;
	bool	b_eob;

	if (  (postquant > patch_size*patch_size)
		||(postquant < 0))
	{
		LL_ERRS() << "Bad postquant in code_patch!"  << LL_ENDL;
	}

	if (postquant)
		patch[patch_size*patch_size - postquant] = 0;

	for (i = 0; i < patch_size*patch_size; i++)
	{
		b_eob = false;
		temp = patch[i];
		if (!temp)
		{
			b_eob = true;
			for (j = i; j < patch_size*patch_size - postquant; j++)
			{
				if (patch[j])
				{
					b_eob = false;
					break;
				}
			}
			if (b_eob)
			{
				bitpack.bitPack((U8 *)&ZERO_EOB, 2);
				return;
			}
			else
			{
				bitpack.bitPack((U8 *)&ZERO_CODE, 1);
			}
		}
		else
		{
			if (temp < 0)
			{
				temp *= -1;
				if (temp > (1<<wbits))
				{
					temp = (1<<wbits);
//					printf("patch quatization exceeding allowable bits!");
				}
				bitpack.bitPack((U8 *)&NEGATIVE_VALUE, 3);
				bitpack.bitPack((U8 *)&temp, wbits);
			}
			else
			{
				if (temp > (1<<wbits))
				{
					temp = (1<<wbits);
//					printf("patch quatization exceeding allowable bits!");
				}
				bitpack.bitPack((U8 *)&POSITIVE_VALUE, 3);
				bitpack.bitPack((U8 *)&temp, wbits);
			}
		}
	}
}


void	end_patch_coding(LLBitPack &bitpack)
{
	bitpack.flushBitPack();
}

void	init_patch_decoding(LLBitPack &bitpack)
{
	bitpack.resetBitPacking();
}

void	decode_patch_group_header(LLBitPack &bitpack, LLGroupHeader *gopp)
{
	U16 retvalu16;

	retvalu16 = 0;
#ifdef LL_BIG_ENDIAN
	U8 *ret = (U8 *)&retvalu16;
	bitpack.bitUnpack(&(ret[1]), 8);
	bitpack.bitUnpack(&(ret[0]), 8);
#else
	bitpack.bitUnpack((U8 *)&retvalu16, 16);
#endif
	gopp->stride = retvalu16;

	U8 retvalu8 = 0;
	bitpack.bitUnpack(&retvalu8, 8);
	gopp->patch_size = retvalu8;

	retvalu8 = 0;
	bitpack.bitUnpack(&retvalu8, 8);
	gopp->layer_type = retvalu8;

	gPatchSize = gopp->patch_size; 
}

void	decode_patch_header(LLBitPack &bitpack, LLPatchHeader *ph)
{
	U8 retvalu8;

	retvalu8 = 0;
	bitpack.bitUnpack(&retvalu8, 8);
	ph->quant_wbits = retvalu8;

	if (END_OF_PATCHES == ph->quant_wbits)
	{
		// End of data, blitz the rest.
		ph->dc_offset = 0;
		ph->range = 0;
		ph->patchids = 0;
		return;
	}

	U32 retvalu32 = 0;
#ifdef LL_BIG_ENDIAN
	U8 *ret = (U8 *)&retvalu32;
	bitpack.bitUnpack(&(ret[3]), 8);
	bitpack.bitUnpack(&(ret[2]), 8);
	bitpack.bitUnpack(&(ret[1]), 8);
	bitpack.bitUnpack(&(ret[0]), 8);
#else
	bitpack.bitUnpack((U8 *)&retvalu32, 32);
#endif
	ph->dc_offset = *(F32 *)&retvalu32;

	U16 retvalu16 = 0;
#ifdef LL_BIG_ENDIAN
	ret = (U8 *)&retvalu16;
	bitpack.bitUnpack(&(ret[1]), 8);
	bitpack.bitUnpack(&(ret[0]), 8);
#else
	bitpack.bitUnpack((U8 *)&retvalu16, 16);
#endif
	ph->range = retvalu16;

	retvalu16 = 0;
#ifdef LL_BIG_ENDIAN
	ret = (U8 *)&retvalu16;
	bitpack.bitUnpack(&(ret[1]), 8);
	bitpack.bitUnpack(&(ret[0]), 2);
#else
	bitpack.bitUnpack((U8 *)&retvalu16, 10);
#endif
	ph->patchids = retvalu16;

	gWordBits = (ph->quant_wbits & 0xf) + 2;
}

void	decode_patch(LLBitPack &bitpack, S32 *patches)
{
#ifdef LL_BIG_ENDIAN
	S32		i, j, patch_size = gPatchSize, wbits = gWordBits;
	U8		tempu8;
	U16		tempu16;
	U32		tempu32;
	for (i = 0; i < patch_size*patch_size; i++)
	{
		bitpack.bitUnpack((U8 *)&tempu8, 1);
		if (tempu8)
		{
			// either 0 EOB or Value
			bitpack.bitUnpack((U8 *)&tempu8, 1);
			if (tempu8)
			{
				// value
				bitpack.bitUnpack((U8 *)&tempu8, 1);
				if (tempu8)
				{
					// negative
					patches[i] = -1;
				}
				else
				{
					// positive
					patches[i] = 1;
				}
				if (wbits <= 8)
				{
					bitpack.bitUnpack((U8 *)&tempu8, wbits);
					patches[i] *= tempu8;
				}
				else if (wbits <= 16)
				{
					tempu16 = 0;
					U8 *ret = (U8 *)&tempu16;
					bitpack.bitUnpack(&(ret[1]), 8);
					bitpack.bitUnpack(&(ret[0]), wbits - 8);
					patches[i] *= tempu16;
				}
				else if (wbits <= 24)
				{
					tempu32 = 0;
					U8 *ret = (U8 *)&tempu32;
					bitpack.bitUnpack(&(ret[2]), 8);
					bitpack.bitUnpack(&(ret[1]), 8);
					bitpack.bitUnpack(&(ret[0]), wbits - 16);
					patches[i] *= tempu32;
				}
				else if (wbits <= 32)
				{
					tempu32 = 0;
					U8 *ret = (U8 *)&tempu32;
					bitpack.bitUnpack(&(ret[3]), 8);
					bitpack.bitUnpack(&(ret[2]), 8);
					bitpack.bitUnpack(&(ret[1]), 8);
					bitpack.bitUnpack(&(ret[0]), wbits - 24);
					patches[i] *= tempu32;
				}
			}
			else
			{
				for (j = i; j < patch_size*patch_size; j++)
				{
					patches[j] = 0;
				}
				return;
			}
		}
		else
		{
			patches[i] = 0;
		}
	}
#else
	S32		i, j, patch_size = gPatchSize, wbits = gWordBits;
	U32		temp;
	for (i = 0; i < patch_size*patch_size; i++)
	{
		temp = 0;
		bitpack.bitUnpack((U8 *)&temp, 1);
		if (temp)
		{
			// either 0 EOB or Value
			temp = 0;
			bitpack.bitUnpack((U8 *)&temp, 1);
			if (temp)
			{
				// value
				temp = 0;
				bitpack.bitUnpack((U8 *)&temp, 1);
				if (temp)
				{
					// negative
					temp = 0;
					bitpack.bitUnpack((U8 *)&temp, wbits);
					patches[i] = temp;
					patches[i] *= -1;
				}
				else
				{
					// positive
					temp = 0;
					bitpack.bitUnpack((U8 *)&temp, wbits);
					patches[i] = temp;
				}
			}
			else
			{
				for (j = i; j < patch_size*patch_size; j++)
				{
					patches[j] = 0;
				}
				return;
			}
		}
		else
		{
			patches[i] = 0;
		}
	}
#endif
}

