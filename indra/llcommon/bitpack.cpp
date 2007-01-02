/** 
 * @file bitpack.cpp
 * @brief Convert data to packed bit stream
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "bitpack.h"

#if 0
#include <stdio.h>
#include <stdlib.h>
#include "stdtypes.h"

U8 gLoad, gUnLoad;
U32 gLoadSize, gUnLoadSize, gTotalBits;

const U32 gMaxDataBits = 8;

/////////////////////////////////////////////////////////////////////////////////////////
#if 0
void bit_pack(U8 *outbase, U32 *outptr, U8 *total_data, U32 total_dsize) 
{
	U32 max_data_bits = gMaxDataBits;
	U32 load_size = gLoadSize, total_bits = gTotalBits;
	U32 dsize;

	U8 data;
	U8 load = gLoad;

	while (total_dsize > 0)
	{
		if (total_dsize > max_data_bits)
		{
			dsize = max_data_bits;
			total_dsize -= max_data_bits;
		}
		else
		{
			dsize = total_dsize;
			total_dsize = 0;
		}

		data = *total_data++;

		data <<= (max_data_bits - dsize);
		while (dsize > 0) 
		{
			if (load_size == max_data_bits) 
			{
				*(outbase + (*outptr)++) = load;
				load_size = 0;
				load = 0x00;
			}
			load <<= 1;			
			load |= (data >> (max_data_bits - 1));
			data <<= 1;
			load_size++;
			total_bits++;
			dsize--;
		}
	}

	gLoad = load;
	gLoadSize = load_size;
	gTotalBits = total_bits;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////

void bit_pack_reset()
{
	gLoad = 0x0;
	gLoadSize = 0;
	gTotalBits = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

void bit_pack_flush(U8 *outbase, U32 *outptr) 
{
	if (gLoadSize) 
	{
		gTotalBits += gLoadSize;
		gLoad <<= (gMaxDataBits - gLoadSize);
		outbase[(*outptr)++] = gLoad;
		gLoadSize = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
#if 0
void bit_unpack(U8 *total_retval, U32 total_dsize, U8 *indata, U32 *inptr) 
{
	U32 max_data_bits = gMaxDataBits;
	U32 unload_size = gUnLoadSize;
	U32 dsize;
	U8	*retval;
	U8 unload = gUnLoad;

	while (total_dsize > 0)
	{
		if (total_dsize > max_data_bits)
		{
			dsize = max_data_bits;
			total_dsize -= max_data_bits;
		}
		else
		{
			dsize = total_dsize;
			total_dsize = 0;
		}

		retval = total_data++;
		*retval = 0x00;
		while (dsize > 0) 
		{
			if (unload_size == 0) 
			{
				unload = indata[(*inptr)++];
				unload_size = max_data_bits;
			}
			*retval <<= 1;
			*retval |= (unload >> (max_data_bits - 1));
			unload_size--;
			unload <<= 1;
			dsize--;
		}
	}

	gUnLoad = unload;
	gUnLoadSize = unload_size;
}
#endif


///////////////////////////////////////////////////////////////////////////////////////

void bit_unpack_reset() 
{
	gUnLoad = 0;
	gUnLoadSize = 0;
}
#endif
