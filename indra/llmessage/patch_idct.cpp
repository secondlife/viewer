/** 
 * @file patch_idct.cpp
 * @brief IDCT patch.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llmath.h"
//#include "vmath.h"
#include "v3math.h"
#include "patch_dct.h"

LLGroupHeader	*gGOPP;

void set_group_of_patch_header(LLGroupHeader *gopp)
{
	gGOPP = gopp;
}

F32 gPatchDequantizeTable[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];
void build_patch_dequantize_table(S32 size)
{
	S32 i, j;
	for (j = 0; j < size; j++)
	{
		for (i = 0; i < size; i++)
		{
			gPatchDequantizeTable[j*size + i] = (1.f + 2.f*(i+j));
		}
	}
}

S32	gCurrentDeSize = 0;

F32	gPatchICosines[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

void setup_patch_icosines(S32 size)
{
	S32 n, u;
	F32 oosob = F_PI*0.5f/size;

	for (u = 0; u < size; u++)
	{
		for (n = 0; n < size; n++)
		{
			gPatchICosines[u*size+n] = cosf((2.f*n+1.f)*u*oosob);
		}
	}
}

S32	gDeCopyMatrix[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

void build_decopy_matrix(S32 size)
{
	S32 i, j, count;
	BOOL	b_diag = FALSE;
	BOOL	b_right = TRUE;

	i = 0;
	j = 0;
	count = 0;

	while (  (i < size)
		   &&(j < size))
	{
		gDeCopyMatrix[j*size + i] = count;

		count++;

		if (!b_diag)
		{
			if (b_right)
			{
				if (i < size - 1)
					i++;
				else
					j++;
				b_right = FALSE;
				b_diag = TRUE;
			}
			else
			{
				if (j < size - 1)
					j++;
				else
					i++;
				b_right = TRUE;
				b_diag = TRUE;
			}
		}
		else
		{
			if (b_right)
			{
				i++;
				j--;
				if (  (i == size - 1)
					||(j == 0))
				{
					b_diag = FALSE;
				}
			}
			else
			{
				i--;
				j++;
				if (  (i == 0)
					||(j == size - 1))
				{
					b_diag = FALSE;
				}
			}
		}
	}
}

void init_patch_decompressor(S32 size)
{
	if (size != gCurrentDeSize)
	{
		gCurrentDeSize = size;
		build_patch_dequantize_table(size);
		setup_patch_icosines(size);
		build_decopy_matrix(size);
	}
}

inline void idct_line(F32 *linein, F32 *lineout, S32 line)
{
	S32 n;
	F32 total;
	F32 *pcp = gPatchICosines;

#ifdef _PATCH_SIZE_16_AND_32_ONLY
	F32 oosob = 2.f/16.f;
	S32	line_size = line*NORMAL_PATCH_SIZE;
	F32 *tlinein, *tpcp;


	for (n = 0; n < NORMAL_PATCH_SIZE; n++)
	{
		tpcp = pcp + n;
		tlinein = linein + line_size;
	
		total = OO_SQRT2*(*(tlinein++));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein)*(*(tpcp += NORMAL_PATCH_SIZE));

		*(lineout + line_size + n) = total*oosob;
	}
#else
	F32 oosob = 2.f/size;
	S32	size = gGOPP->patch_size;
	S32	line_size = line*size;
	S32 u;
	for (n = 0; n < size; n++)
	{
		total = OO_SQRT2*linein[line_size];
		for (u = 1; u < size; u++)
		{
			total += linein[line_size + u]*pcp[u*size+n];
		}
		lineout[line_size + n] = total*oosob;
	}
#endif
}

inline void idct_line_large_slow(F32 *linein, F32 *lineout, S32 line)
{
	S32 n;
	F32 total;
	F32 *pcp = gPatchICosines;

	F32 oosob = 2.f/32.f;
	S32	line_size = line*LARGE_PATCH_SIZE;
	F32 *tlinein, *tpcp;


	for (n = 0; n < LARGE_PATCH_SIZE; n++)
	{
		tpcp = pcp + n;
		tlinein = linein + line_size;
	
		total = OO_SQRT2*(*(tlinein++));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein)*(*(tpcp += LARGE_PATCH_SIZE));

		*(lineout + line_size + n) = total*oosob;
	}
}

// Nota Bene: assumes that coefficients beyond 128 are 0!

void idct_line_large(F32 *linein, F32 *lineout, S32 line)
{
	S32 n;
	F32 total;
	F32 *pcp = gPatchICosines;

	F32 oosob = 2.f/32.f;
	S32	line_size = line*LARGE_PATCH_SIZE;
	F32 *tlinein, *tpcp;
	F32 *baselinein = linein + line_size;
	F32 *baselineout = lineout + line_size;


	for (n = 0; n < LARGE_PATCH_SIZE; n++)
	{
		tpcp = pcp++;
		tlinein = baselinein;
	
		total = OO_SQRT2*(*(tlinein++));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein++)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein)*(*(tpcp));

		*baselineout++ = total*oosob;
	}
}

inline void idct_column(F32 *linein, F32 *lineout, S32 column)
{
	S32 n;
	F32 total;
	F32 *pcp = gPatchICosines;

#ifdef _PATCH_SIZE_16_AND_32_ONLY
	F32 *tlinein, *tpcp;

	for (n = 0; n < NORMAL_PATCH_SIZE; n++)
	{
		tpcp = pcp + n;
		tlinein = linein + column;

		total = OO_SQRT2*(*tlinein);
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));

		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));

		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));

		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));
		total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp += NORMAL_PATCH_SIZE));

		*(lineout + (n<<4) + column) = total;
	}

#else
	S32	size = gGOPP->patch_size;
	S32 u;
	S32 u_size;

	for (n = 0; n < size; n++)
	{
		total = OO_SQRT2*linein[column];
		for (u = 1; u < size; u++)
		{
			u_size = u*size;
			total += linein[u_size + column]*pcp[u_size+n];
		}
		lineout[size*n + column] = total;
	}
#endif
}

inline void idct_column_large_slow(F32 *linein, F32 *lineout, S32 column)
{
	S32 n;
	F32 total;
	F32 *pcp = gPatchICosines;

	F32 *tlinein, *tpcp;

	for (n = 0; n < LARGE_PATCH_SIZE; n++)
	{
		tpcp = pcp + n;
		tlinein = linein + column;

		total = OO_SQRT2*(*tlinein);
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));
		total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		*(lineout + (n<<5) + column) = total;
	}
}

// Nota Bene: assumes that coefficients beyond 128 are 0!

void idct_column_large(F32 *linein, F32 *lineout, S32 column)
{
	S32 n, m;
	F32 total;
	F32 *pcp = gPatchICosines;

	F32 *tlinein, *tpcp;
	F32 *baselinein = linein + column;
	F32 *baselineout = lineout + column;

	for (n = 0; n < LARGE_PATCH_SIZE; n++)
	{
		tpcp = pcp++;
		tlinein = baselinein;

		total = OO_SQRT2*(*tlinein);
		for (m = 1; m < NORMAL_PATCH_SIZE; m++)
			total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp += LARGE_PATCH_SIZE));

		*(baselineout + (n<<5)) = total;
	}
}

inline void idct_patch(F32 *block)
{
	F32 temp[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

#ifdef _PATCH_SIZE_16_AND_32_ONLY
	idct_column(block, temp, 0);	
	idct_column(block, temp, 1);	
	idct_column(block, temp, 2);	
	idct_column(block, temp, 3);	

	idct_column(block, temp, 4);	
	idct_column(block, temp, 5);	
	idct_column(block, temp, 6);	
	idct_column(block, temp, 7);	

	idct_column(block, temp, 8);	
	idct_column(block, temp, 9);	
	idct_column(block, temp, 10);	
	idct_column(block, temp, 11);	

	idct_column(block, temp, 12);	
	idct_column(block, temp, 13);	
	idct_column(block, temp, 14);	
	idct_column(block, temp, 15);	

	idct_line(temp, block, 0);	
	idct_line(temp, block, 1);	
	idct_line(temp, block, 2);	
	idct_line(temp, block, 3);	

	idct_line(temp, block, 4);	
	idct_line(temp, block, 5);	
	idct_line(temp, block, 6);	
	idct_line(temp, block, 7);	

	idct_line(temp, block, 8);	
	idct_line(temp, block, 9);	
	idct_line(temp, block, 10);	
	idct_line(temp, block, 11);	

	idct_line(temp, block, 12);	
	idct_line(temp, block, 13);	
	idct_line(temp, block, 14);	
	idct_line(temp, block, 15);	
#else
	S32 i;
	S32	size = gGOPP->patch_size;
	for (i = 0; i < size; i++)
	{
		idct_column(block, temp, i);	
	}
	for (i = 0; i < size; i++)
	{
		idct_line(temp, block, i);	
	}
#endif
}

inline void idct_patch_large(F32 *block)
{
	F32 temp[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

	idct_column_large_slow(block, temp, 0);	
	idct_column_large_slow(block, temp, 1);	
	idct_column_large_slow(block, temp, 2);	
	idct_column_large_slow(block, temp, 3);	

	idct_column_large_slow(block, temp, 4);	
	idct_column_large_slow(block, temp, 5);	
	idct_column_large_slow(block, temp, 6);	
	idct_column_large_slow(block, temp, 7);	

	idct_column_large_slow(block, temp, 8);	
	idct_column_large_slow(block, temp, 9);	
	idct_column_large_slow(block, temp, 10);	
	idct_column_large_slow(block, temp, 11);	

	idct_column_large_slow(block, temp, 12);	
	idct_column_large_slow(block, temp, 13);	
	idct_column_large_slow(block, temp, 14);	
	idct_column_large_slow(block, temp, 15);	

	idct_column_large_slow(block, temp, 16);	
	idct_column_large_slow(block, temp, 17);	
	idct_column_large_slow(block, temp, 18);	
	idct_column_large_slow(block, temp, 19);	

	idct_column_large_slow(block, temp, 20);	
	idct_column_large_slow(block, temp, 21);	
	idct_column_large_slow(block, temp, 22);	
	idct_column_large_slow(block, temp, 23);	

	idct_column_large_slow(block, temp, 24);	
	idct_column_large_slow(block, temp, 25);	
	idct_column_large_slow(block, temp, 26);	
	idct_column_large_slow(block, temp, 27);	

	idct_column_large_slow(block, temp, 28);	
	idct_column_large_slow(block, temp, 29);	
	idct_column_large_slow(block, temp, 30);	
	idct_column_large_slow(block, temp, 31);	

	idct_line_large_slow(temp, block, 0);	
	idct_line_large_slow(temp, block, 1);	
	idct_line_large_slow(temp, block, 2);	
	idct_line_large_slow(temp, block, 3);	

	idct_line_large_slow(temp, block, 4);	
	idct_line_large_slow(temp, block, 5);	
	idct_line_large_slow(temp, block, 6);	
	idct_line_large_slow(temp, block, 7);	

	idct_line_large_slow(temp, block, 8);	
	idct_line_large_slow(temp, block, 9);	
	idct_line_large_slow(temp, block, 10);	
	idct_line_large_slow(temp, block, 11);	

	idct_line_large_slow(temp, block, 12);	
	idct_line_large_slow(temp, block, 13);	
	idct_line_large_slow(temp, block, 14);	
	idct_line_large_slow(temp, block, 15);	

	idct_line_large_slow(temp, block, 16);	
	idct_line_large_slow(temp, block, 17);	
	idct_line_large_slow(temp, block, 18);	
	idct_line_large_slow(temp, block, 19);	

	idct_line_large_slow(temp, block, 20);	
	idct_line_large_slow(temp, block, 21);	
	idct_line_large_slow(temp, block, 22);	
	idct_line_large_slow(temp, block, 23);	

	idct_line_large_slow(temp, block, 24);	
	idct_line_large_slow(temp, block, 25);	
	idct_line_large_slow(temp, block, 26);	
	idct_line_large_slow(temp, block, 27);	

	idct_line_large_slow(temp, block, 28);	
	idct_line_large_slow(temp, block, 29);	
	idct_line_large_slow(temp, block, 30);	
	idct_line_large_slow(temp, block, 31);	
}

S32	gDitherNoise = 128;

void decompress_patch(F32 *patch, S32 *cpatch, LLPatchHeader *ph)
{
	S32		i, j;

	F32		block[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE], *tblock = block;
	F32		*tpatch;

	LLGroupHeader	*gopp = gGOPP;
	S32		size = gopp->patch_size;
	F32		range = ph->range;
	S32		prequant = (ph->quant_wbits >> 4) + 2;
	S32		quantize = 1<<prequant;
	F32		hmin = ph->dc_offset;
	S32		stride = gopp->stride;

	F32		ooq = 1.f/(F32)quantize;
	F32     *dq = gPatchDequantizeTable;
	S32		*decopy_matrix = gDeCopyMatrix;

	F32		mult = ooq*range;
	F32		addval = mult*(F32)(1<<(prequant - 1))+hmin;

	for (i = 0; i < size*size; i++)
	{
		*(tblock++) = *(cpatch + *(decopy_matrix++))*(*dq++);
	}

	if (size == 16)
	{
		idct_patch(block);
	}
	else
	{
		idct_patch_large(block);
	}

	for (j = 0; j < size; j++)
	{
		tpatch = patch + j*stride;
		tblock = block + j*size;
		for (i = 0; i < size; i++)
		{
			*(tpatch++) = *(tblock++)*mult+addval;
		}
	}
}


void decompress_patchv(LLVector3 *v, S32 *cpatch, LLPatchHeader *ph)
{
	S32		i, j;

	F32			block[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE], *tblock = block;
	LLVector3	*tvec;

	LLGroupHeader	*gopp = gGOPP;
	S32		size = gopp->patch_size;
	F32		range = ph->range;
	S32		prequant = (ph->quant_wbits >> 4) + 2;
	S32		quantize = 1<<prequant;
	F32		hmin = ph->dc_offset;
	S32		stride = gopp->stride;

	F32		ooq = 1.f/(F32)quantize;
	F32     *dq = gPatchDequantizeTable;
	S32		*decopy_matrix = gDeCopyMatrix;

	F32		mult = ooq*range;
	F32		addval = mult*(F32)(1<<(prequant - 1))+hmin;

//	BOOL	b_diag = FALSE;
//	BOOL	b_right = TRUE;

	for (i = 0; i < size*size; i++)
	{
		*(tblock++) = *(cpatch + *(decopy_matrix++))*(*dq++);
	}

	if (size == 16)
		idct_patch(block);
	else
		idct_patch_large(block);

	for (j = 0; j < size; j++)
	{
		tvec = v + j*stride;
		tblock = block + j*size;
		for (i = 0; i < size; i++)
		{
			(*tvec++).mV[VZ] = *(tblock++)*mult+addval;
		}
	}
}

