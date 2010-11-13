 /** 
 * @file llblockdecoder.cpp
 * @brief Image block decompression
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

#include "linden_common.h"

#include "llblockdecoder.h"

// KDU core header files
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"

#include "llkdumem.h"

#include "llblockdata.h"
#include "llerror.h"


BOOL LLBlockDecoder::decodeU32(LLBlockDataU32 &block_data, U8 *source_data, const U32 source_size) const
{
	U32 width, height;

	llassert(source_data);
	
	LLKDUMemSource source(source_data, source_size);

	source.reset();

	kdu_codestream codestream;

	codestream.create(&source);
	codestream.set_fast();

	kdu_dims dims;
	codestream.get_dims(0,dims);
	llassert(codestream.get_num_components() == 1);

	width = dims.size.x;
	height = dims.size.y;

	// Assumes U32 data.
	U8 *output = block_data.getData();

	kdu_dims tile_indices;
	codestream.get_valid_tiles(tile_indices);

	kdu_coords tpos;
	tpos.x = 0;
	tpos.y = 0;

	// Now we are ready to walk through the tiles processing them one-by-one.
	while (tpos.y < tile_indices.size.y)
	{
		while (tpos.x < tile_indices.size.x)
		{
			kdu_tile tile = codestream.open_tile(tpos+tile_indices.pos);

			kdu_resolution res = tile.access_component(0).access_resolution();
			kdu_dims tile_dims;
			res.get_dims(tile_dims);
			kdu_coords offset = tile_dims.pos - dims.pos;
			int row_gap = block_data.getRowStride(); // inter-row separation
			kdu_byte *buf = output + offset.y*row_gap + offset.x*4;

			kdu_tile_comp tile_comp = tile.access_component(0);
			bool reversible = tile_comp.get_reversible();
			U32 precision = tile_comp.get_bit_depth();
			U32 precision_scale = 1 << precision;
			llassert(precision >= 8); // Else would have used 16 bit representation

			kdu_resolution comp_res = tile_comp.access_resolution(); // Get top resolution
			kdu_dims comp_dims;
			comp_res.get_dims(comp_dims);

			bool use_shorts = (tile_comp.get_bit_depth(true) <= 16);

			kdu_line_buf line;
			kdu_sample_allocator allocator;
			kdu_pull_ifc engine;

			line.pre_create(&allocator, comp_dims.size.x, reversible, use_shorts);
			if (res.which() == 0) // No DWT levels used
			{
				engine = kdu_decoder(res.access_subband(LL_BAND), &allocator, use_shorts);
			}
			else
			{
				engine = kdu_synthesis(res, &allocator, use_shorts);
			}
			allocator.finalize(); // Actually creates buffering resources

			line.create(); // Grabs resources from the allocator.

			// Do the actual processing
			while (tile_dims.size.y--)
			{
				engine.pull(line, true);
				int width = line.get_width();

				llassert(line.get_buf32());
				llassert(!line.is_absolute());
				// Decompressed samples have a 32-bit representation (integer or float)
				kdu_sample32 *sp = line.get_buf32();
				// Transferring normalized floating point data.
				U32 *dest_u32 = (U32 *)buf;
				for (; width > 0; width--, sp++, dest_u32++)
				{
					if (sp->fval < -0.5f)
					{
						*dest_u32 = 0;
					}
					else
					{
						*dest_u32 = (U32)((sp->fval + 0.5f)*precision_scale);
					}
				}
				buf += row_gap;
			}
			engine.destroy();
			tile.close();
			tpos.x++;
		}
		tpos.y++;
		tpos.x = 0;
	}
	codestream.destroy();

	return TRUE;
}

BOOL LLBlockDecoder::decodeF32(LLBlockDataF32 &block_data, U8 *source_data, const U32 source_size, const F32 min, const F32 max) const
{
	U32 width, height;
	F32 range, range_inv, float_offset;
	bool use_shorts = false;

	range = max - min;
	range_inv = 1.f / range;
	float_offset = 0.5f*(max + min);

	llassert(source_data);
	
	LLKDUMemSource source(source_data, source_size);

	source.reset();

	kdu_codestream codestream;

	codestream.create(&source);
	codestream.set_fast();

	kdu_dims dims;
	codestream.get_dims(0,dims);
	llassert(codestream.get_num_components() == 1);

	width = dims.size.x;
	height = dims.size.y;

	// Assumes F32 data.
	U8 *output = block_data.getData();

	kdu_dims tile_indices;
	codestream.get_valid_tiles(tile_indices);

	kdu_coords tpos;
	tpos.x = 0;
	tpos.y = 0;

	// Now we are ready to walk through the tiles processing them one-by-one.
	while (tpos.y < tile_indices.size.y)
	{
		while (tpos.x < tile_indices.size.x)
		{
			kdu_tile tile = codestream.open_tile(tpos+tile_indices.pos);

			kdu_resolution res = tile.access_component(0).access_resolution();
			kdu_dims tile_dims;
			res.get_dims(tile_dims);
			kdu_coords offset = tile_dims.pos - dims.pos;
			int row_gap = block_data.getRowStride(); // inter-row separation
			kdu_byte *buf = output + offset.y*row_gap + offset.x*4;

			kdu_tile_comp tile_comp = tile.access_component(0);
			bool reversible = tile_comp.get_reversible();

			kdu_resolution comp_res = tile_comp.access_resolution(); // Get top resolution
			kdu_dims comp_dims;
			comp_res.get_dims(comp_dims);

			kdu_line_buf line;
			kdu_sample_allocator allocator;
			kdu_pull_ifc engine;

			line.pre_create(&allocator, comp_dims.size.x, reversible, use_shorts);
			if (res.which() == 0) // No DWT levels used
			{
				engine = kdu_decoder(res.access_subband(LL_BAND), &allocator, use_shorts);
			}
			else
			{
				engine = kdu_synthesis(res, &allocator, use_shorts);
			}
			allocator.finalize(); // Actually creates buffering resources

			line.create(); // Grabs resources from the allocator.

			// Do the actual processing
			while (tile_dims.size.y--)
			{
				engine.pull(line, true);
				int width = line.get_width();

				llassert(line.get_buf32());
				llassert(!line.is_absolute());
				// Decompressed samples have a 32-bit representation (integer or float)
				kdu_sample32 *sp = line.get_buf32();
				// Transferring normalized floating point data.
				F32 *dest_f32 = (F32 *)buf;
				for (; width > 0; width--, sp++, dest_f32++)
				{
					if (sp->fval < -0.5f)
					{
						*dest_f32 = min;
					}
					else if (sp->fval > 0.5f)
					{
						*dest_f32 = max;
					}
					else
					{
						*dest_f32 = (sp->fval) * range + float_offset;
					}
				}
				buf += row_gap;
			}
			engine.destroy();
			tile.close();
			tpos.x++;
		}
		tpos.y++;
		tpos.x = 0;
	}
	codestream.destroy();
	return TRUE;
}
