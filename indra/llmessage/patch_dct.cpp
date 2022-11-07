/** 
 * @file patch_dct.cpp
 * @brief DCT patch.
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

typedef struct s_patch_compress_global_data
{
    S32 patch_size;
    S32 patch_stride;
    U32 charptr;
    S32 layer_type;
} PCGD;

PCGD    gPatchCompressGlobalData;

void reset_patch_compressor(void)
{
    PCGD *pcp = &gPatchCompressGlobalData;

    pcp->charptr = 0;
}

S32 gCurrentSize = 0;

F32 gPatchQuantizeTable[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

void build_patch_quantize_table(S32 size)
{
    S32 i, j;
    for (j = 0; j < size; j++)
    {
        for (i = 0; i < size; i++)
        {
            gPatchQuantizeTable[j*size + i] = 1.f/(1.f + 2.f*(i+j));
        }
    }
}

F32 gPatchCosines[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

void setup_patch_cosines(S32 size)
{
    S32 n, u;
    F32 oosob = F_PI*0.5f/size;

    for (u = 0; u < size; u++)
    {
        for (n = 0; n < size; n++)
        {
            gPatchCosines[u*size+n] = cosf((2.f*n+1.f)*u*oosob);
        }
    }
}

S32 gCopyMatrix[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

void build_copy_matrix(S32 size)
{
    S32 i, j, count;
    BOOL    b_diag = FALSE;
    BOOL    b_right = TRUE;

    i = 0;
    j = 0;
    count = 0;

    while (  (i < size)
           &&(j < size))
    {
        gCopyMatrix[j*size + i] = count;

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


void init_patch_compressor(S32 patch_size, S32 patch_stride, S32 layer_type)
{
    PCGD *pcp = &gPatchCompressGlobalData;

    pcp->charptr = 0;

    pcp->patch_size = patch_size;
    pcp->patch_stride = patch_stride;
    pcp->layer_type = layer_type;

    if (patch_size != gCurrentSize)
    {
        gCurrentSize = patch_size;
        build_patch_quantize_table(patch_size);
        setup_patch_cosines(patch_size);
        build_copy_matrix(patch_size);
    }
}

void prescan_patch(F32 *patch, LLPatchHeader *php, F32 &zmax, F32 &zmin)
{
    S32     i, j;
    PCGD    *pcp = &gPatchCompressGlobalData;
    S32     stride = pcp->patch_stride;
    S32     size = pcp->patch_size;
    S32     jstride;

    zmax = -99999999.f;
    zmin = 99999999.f;

    for (j = 0; j < size; j++)
    {
        jstride = j*stride;
        for (i = 0; i < size; i++)
        {
            if (*(patch + jstride + i) > zmax)
            {
                zmax = *(patch + jstride + i);
            }
            if (*(patch + jstride + i) < zmin)
            {
                zmin = *(patch + jstride + i);
            }
        }
    }

    php->dc_offset = zmin;
    php->range = (U16) ((zmax - zmin) + 1.f);
}

void dct_line(F32 *linein, F32 *lineout, S32 line)
{
    S32 u;
    F32 total;
    F32 *pcp = gPatchCosines;
    S32 line_size = line*NORMAL_PATCH_SIZE;

#ifdef _PATCH_SIZE_16_AND_32_ONLY
    F32 *tlinein, *tpcp;

    tlinein = linein + line_size;

    total = *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein);

    *(lineout + line_size) = OO_SQRT2*total;

    for (u = 1; u < NORMAL_PATCH_SIZE; u++)
    {
        tlinein = linein + line_size;
        tpcp = pcp + (u<<4);

        total = *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein)*(*tpcp);

        *(lineout + line_size + u) = total;
    }
#else
    S32 n;
    S32 size = gPatchCompressGlobalData.patch_size;
    total = 0.f;
    for (n = 0; n < size; n++)
    {
        total += linein[line_size + n];
    }
    lineout[line_size] = OO_SQRT2*total;

    for (u = 1; u < size; u++)
    {
        total = 0.f;
        for (n = 0; n < size; n++)
        {
            total += linein[line_size + n]*pcp[u*size+n];
        }
        lineout[line_size + u] = total;
    }
#endif
}

void dct_line_large(F32 *linein, F32 *lineout, S32 line)
{
    S32 u;
    F32 total;
    F32 *pcp = gPatchCosines;
    S32 line_size = line*LARGE_PATCH_SIZE;

    F32 *tlinein, *tpcp;

    tlinein = linein + line_size;

    total = *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);

    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein++);
    total += *(tlinein);

    *(lineout + line_size) = OO_SQRT2*total;

    for (u = 1; u < LARGE_PATCH_SIZE; u++)
    {
        tlinein = linein + line_size;
        tpcp = pcp + (u<<5);

        total = *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));

        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein++)*(*(tpcp++));
        total += *(tlinein)*(*tpcp);

        *(lineout + line_size + u) = total;
    }
}

inline void dct_column(F32 *linein, S32 *lineout, S32 column)
{
    S32 u;
    F32 total;
    F32 oosob = 2.f/16.f;
    F32 *pcp = gPatchCosines;
    S32 *copy_matrix = gCopyMatrix;
    F32 *qt = gPatchQuantizeTable;

#ifdef _PATCH_SIZE_16_AND_32_ONLY
    F32 *tlinein, *tpcp;
    S32 sizeu;

    tlinein = linein + column;

    total = *(tlinein);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);

    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);

    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);

    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);
    total += *(tlinein += NORMAL_PATCH_SIZE);

    *(lineout + *(copy_matrix + column)) = (S32)(OO_SQRT2*total*oosob*(*(qt + column)));

    for (u = 1; u < NORMAL_PATCH_SIZE; u++)
    {
        tlinein = linein + column;
        tpcp = pcp + (u<<4);

        total = *(tlinein)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += NORMAL_PATCH_SIZE)*(*(tpcp));

        sizeu = NORMAL_PATCH_SIZE*u + column;

        *(lineout + *(copy_matrix + sizeu)) = (S32)(total*oosob*(*(qt+sizeu)));
    }
#else
    S32 size = gPatchCompressGlobalData.patch_size;
    F32 oosob = 2.f/size;
    S32 n;
    total = 0.f;
    for (n = 0; n < size; n++)
    {
        total += linein[size*n + column];
    }
    lineout[copy_matrix[column]] = OO_SQRT2*total*oosob*qt[column];

    for (u = 1; u < size; u++)
    {
        total = 0.f;
        for (n = 0; n < size; n++)
        {
            total += linein[size*n + column]*pcp[u*size+n];
        }
        lineout[copy_matrix[size*u + column]] = total*oosob*qt[size*u + column];
    }
#endif
}

inline void dct_column_large(F32 *linein, S32 *lineout, S32 column)
{
    S32 u;
    F32 total;
    F32 oosob = 2.f/32.f;
    F32 *pcp = gPatchCosines;
    S32 *copy_matrix = gCopyMatrix;
    F32 *qt = gPatchQuantizeTable;

    F32 *tlinein, *tpcp;
    S32 sizeu;

    tlinein = linein + column;

    total = *(tlinein);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);
    total += *(tlinein += LARGE_PATCH_SIZE);

    *(lineout + *(copy_matrix + column)) = (S32)(OO_SQRT2*total*oosob*(*(qt + column)));

    for (u = 1; u < LARGE_PATCH_SIZE; u++)
    {
        tlinein = linein + column;
        tpcp = pcp + (u<<5);

        total = *(tlinein)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));

        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp++));
        total += *(tlinein += LARGE_PATCH_SIZE)*(*(tpcp));

        sizeu = LARGE_PATCH_SIZE*u + column;

        *(lineout + *(copy_matrix + sizeu)) = (S32)(total*oosob*(*(qt+sizeu)));
    }
}

inline void dct_patch(F32 *block, S32 *cpatch)
{
    F32 temp[NORMAL_PATCH_SIZE*NORMAL_PATCH_SIZE];

#ifdef _PATCH_SIZE_16_AND_32_ONLY
    dct_line(block, temp, 0);
    dct_line(block, temp, 1);
    dct_line(block, temp, 2);
    dct_line(block, temp, 3);

    dct_line(block, temp, 4);
    dct_line(block, temp, 5);
    dct_line(block, temp, 6);
    dct_line(block, temp, 7);

    dct_line(block, temp, 8);
    dct_line(block, temp, 9);
    dct_line(block, temp, 10);
    dct_line(block, temp, 11);

    dct_line(block, temp, 12);
    dct_line(block, temp, 13);
    dct_line(block, temp, 14);
    dct_line(block, temp, 15);

    dct_column(temp, cpatch, 0);
    dct_column(temp, cpatch, 1);
    dct_column(temp, cpatch, 2);
    dct_column(temp, cpatch, 3);

    dct_column(temp, cpatch, 4);
    dct_column(temp, cpatch, 5);
    dct_column(temp, cpatch, 6);
    dct_column(temp, cpatch, 7);

    dct_column(temp, cpatch, 8);
    dct_column(temp, cpatch, 9);
    dct_column(temp, cpatch, 10);
    dct_column(temp, cpatch, 11);

    dct_column(temp, cpatch, 12);
    dct_column(temp, cpatch, 13);
    dct_column(temp, cpatch, 14);
    dct_column(temp, cpatch, 15);
#else
    S32 i;
    S32 size = gPatchCompressGlobalData.patch_size;
    for (i = 0; i < size; i++)
    {
        dct_line(block, temp, i);
    }
    for (i = 0; i < size; i++)
    {
        dct_column(temp, cpatch, i);
    }
#endif
}

inline void dct_patch_large(F32 *block, S32 *cpatch)
{
    F32 temp[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];

    dct_line_large(block, temp, 0);
    dct_line_large(block, temp, 1);
    dct_line_large(block, temp, 2);
    dct_line_large(block, temp, 3);

    dct_line_large(block, temp, 4);
    dct_line_large(block, temp, 5);
    dct_line_large(block, temp, 6);
    dct_line_large(block, temp, 7);

    dct_line_large(block, temp, 8);
    dct_line_large(block, temp, 9);
    dct_line_large(block, temp, 10);
    dct_line_large(block, temp, 11);

    dct_line_large(block, temp, 12);
    dct_line_large(block, temp, 13);
    dct_line_large(block, temp, 14);
    dct_line_large(block, temp, 15);

    dct_line_large(block, temp, 16);
    dct_line_large(block, temp, 17);
    dct_line_large(block, temp, 18);
    dct_line_large(block, temp, 19);

    dct_line_large(block, temp, 20);
    dct_line_large(block, temp, 21);
    dct_line_large(block, temp, 22);
    dct_line_large(block, temp, 23);

    dct_line_large(block, temp, 24);
    dct_line_large(block, temp, 25);
    dct_line_large(block, temp, 26);
    dct_line_large(block, temp, 27);

    dct_line_large(block, temp, 28);
    dct_line_large(block, temp, 29);
    dct_line_large(block, temp, 30);
    dct_line_large(block, temp, 31);

    dct_column_large(temp, cpatch, 0);
    dct_column_large(temp, cpatch, 1);
    dct_column_large(temp, cpatch, 2);
    dct_column_large(temp, cpatch, 3);

    dct_column_large(temp, cpatch, 4);
    dct_column_large(temp, cpatch, 5);
    dct_column_large(temp, cpatch, 6);
    dct_column_large(temp, cpatch, 7);

    dct_column_large(temp, cpatch, 8);
    dct_column_large(temp, cpatch, 9);
    dct_column_large(temp, cpatch, 10);
    dct_column_large(temp, cpatch, 11);

    dct_column_large(temp, cpatch, 12);
    dct_column_large(temp, cpatch, 13);
    dct_column_large(temp, cpatch, 14);
    dct_column_large(temp, cpatch, 15);

    dct_column_large(temp, cpatch, 16);
    dct_column_large(temp, cpatch, 17);
    dct_column_large(temp, cpatch, 18);
    dct_column_large(temp, cpatch, 19);

    dct_column_large(temp, cpatch, 20);
    dct_column_large(temp, cpatch, 21);
    dct_column_large(temp, cpatch, 22);
    dct_column_large(temp, cpatch, 23);

    dct_column_large(temp, cpatch, 24);
    dct_column_large(temp, cpatch, 25);
    dct_column_large(temp, cpatch, 26);
    dct_column_large(temp, cpatch, 27);

    dct_column_large(temp, cpatch, 28);
    dct_column_large(temp, cpatch, 29);
    dct_column_large(temp, cpatch, 30);
    dct_column_large(temp, cpatch, 31);
}

void compress_patch(F32 *patch, S32 *cpatch, LLPatchHeader *php, S32 prequant)
{
    S32     i, j;
    PCGD    *pcp = &gPatchCompressGlobalData;
    S32     stride = pcp->patch_stride;
    S32     size = pcp->patch_size;
    F32     block[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE], *tblock;
    F32     *tpatch;

    S32     wordsize = prequant;
    F32     oozrange = 1.f/php->range;

    F32     dc = php->dc_offset;

    S32     range = (1<<prequant);
    F32     premult = oozrange*range;
//  F32     sub = (F32)(1<<(prequant - 1));
    F32     sub = (F32)(1<<(prequant - 1)) + dc*premult;

    php->quant_wbits = wordsize - 2;
    php->quant_wbits |= (prequant - 2)<<4;

    for (j = 0; j < size; j++)
    {
        tblock = block + j*size;
        tpatch = patch + j*stride;
        for (i = 0; i < size; i++)
        {
//          block[j*size + i] = (patch[j*stride + i] - dc)*premult - sub;
            *(tblock++) = *(tpatch++)*premult - sub;
        }
    }

    if (size == 16)
        dct_patch(block, cpatch);
    else
        dct_patch_large(block, cpatch);
}

void get_patch_group_header(LLGroupHeader *gopp)
{
    PCGD    *pcp = &gPatchCompressGlobalData;
    gopp->stride = pcp->patch_stride;
    gopp->patch_size = pcp->patch_size;
    gopp->layer_type = pcp->layer_type;
}
