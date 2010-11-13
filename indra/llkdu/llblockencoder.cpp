 /** 
 * @file llblockencoder.cpp
 * @brief Image block compression
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

#include "llblockencoder.h"

// KDU core header files
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"

#include "llkdumem.h"

#include "llblockdata.h"
#include "llerror.h"

LLBlockEncoder::LLBlockEncoder()
{
	mBPP = 0.f;
}

U8 *LLBlockEncoder::encode(const LLBlockData &block_data, U32 &output_size) const
{
	switch (block_data.getType())
	{
	case LLBlockData::BLOCK_TYPE_U32:
		{
			LLBlockDataU32 &bd_u32 = (LLBlockDataU32 &)block_data;
			return encodeU32(bd_u32, output_size);
		}
	case LLBlockData::BLOCK_TYPE_F32:
		{
			LLBlockDataF32 &bd_f32 = (LLBlockDataF32 &)block_data;
			return encodeF32(bd_f32, output_size);
		}
	default:
		llerrs << "Unsupported block type!" << llendl;
		output_size = 0;
		return NULL;
	}
}

U8 *LLBlockEncoder::encodeU32(const LLBlockDataU32 &block_data, U32 &output_size) const
{
	// OK, for now, just use the standard KDU encoder, with a single channel
	// integer channel.

	// Collect simple arguments.
	bool allow_rate_prediction, allow_shorts, mem, quiet, no_weights;

	allow_rate_prediction = true;
	allow_shorts = false;
	no_weights = false;
	bool use_absolute = false;
	mem = false;
	quiet = false;

	// Set codestream options
	siz_params siz;
	S16 precision = block_data.getPrecision();

	siz.set(Sdims,0,0,(U16)block_data.getHeight());
	siz.set(Sdims,0,1,(U16)block_data.getWidth());
	siz.set(Ssigned,0,0,false);
	siz.set(Scomponents,0,0,1);
	siz.set(Sprecision,0,0, precision);

	// Construct the `kdu_codestream' object and parse all remaining arguments.
	output_size = block_data.getSize();
	if (output_size < 1000)
	{
		output_size = 1000;
	}

	U8 *output_buffer = new U8[output_size];

	LLKDUMemTarget output(output_buffer, output_size, block_data.getSize());

	kdu_codestream codestream;
	codestream.create(&siz,&output);

	codestream.access_siz()->parse_string("Clayers=1");
	codestream.access_siz()->finalize_all();

	kdu_tile tile = codestream.open_tile(kdu_coords(0,0));

	// Open tile-components and create processing engines and resources
	kdu_dims dims;
	kdu_sample_allocator allocator;
	kdu_tile_comp tile_comp;
	kdu_line_buf line;
	kdu_push_ifc engine;

	tile_comp = tile.access_component(0);
	kdu_resolution res = tile_comp.access_resolution(); // Get top resolution

	res.get_dims(dims);

	line.pre_create(&allocator,dims.size.x, use_absolute, allow_shorts);

	if (res.which() == 0) // No DWT levels (should not occur in this example)
	{
        engine = kdu_encoder(res.access_subband(LL_BAND),&allocator, use_absolute);
	}
	else
	{
        engine = kdu_analysis(res,&allocator, use_absolute);
	}

	allocator.finalize(); // Actually creates buffering resources
    line.create(); // Grabs resources from the allocator.

	// Now walk through the lines of the buffer, pushing them into the
	// relevant tile-component processing engines.

	U32 *source_u32 = NULL;
	F32 scale_inv = 1.f / (1 << precision);

	S32 y;
	for (y = 0; y < dims.size.y; y++)
	{
		source_u32 = (U32*)(block_data.getData() + y * block_data.getRowStride());
		kdu_sample32 *dest = line.get_buf32();
		for (S32 n=dims.size.x; n > 0; n--, dest++, source_u32++)
		{
			// Just pack it in, for now.
			dest->fval = (F32)(*source_u32) * scale_inv - 0.5f;// - 0.5f;
        }
        engine.push(line, true);
    }

	// Cleanup
    engine.destroy(); // engines are interfaces; no default destructors

	// Produce the final compressed output.
	kdu_long layer_bytes[1] = {0};

	layer_bytes[0] = (kdu_long) (mBPP*block_data.getWidth()*block_data.getHeight());
	// Here we are not requesting specific sizes for any of the 12
	// quality layers.  As explained in the description of
	// "kdu_codestream::flush" (see "kdu_compressed.h"), the rate allocator
	// will then assign the layers in such a way as to achieve roughly
	// two quality layers per octave change in bit-rate, with the final
	// layer reaching true lossless quality.

	codestream.flush(layer_bytes,1);
	// You can see how many bytes were assigned
	// to each quality layer by looking at the entries of `layer_bytes' here.
	// The flush function can do a lot of interesting things which you may
	// want to spend some time looking into. In addition to targeting
	// specific bit-rates for each quality layer, it can be configured to
	// use rate-distortion slope thresholds of your choosing and to return
	// the thresholds which it finds to be best for any particular set of
	// target layer sizes.  This opens the door to feedback-oriented rate
	// control for video.  You should also look into the
	// "kdu_codestream::set_max_bytes" and
	// "kdu_codestream::set_min_slope_threshold" functions which can be
	// used to significantly speed up compression.
	codestream.destroy(); // All done: simple as that.

	// Now that we're done encoding, create the new data buffer for the compressed
	// image and stick it there.

	U8 *output_data = new U8[output_size];

	memcpy(output_data, output_buffer, output_size);

	output.close(); // Not really necessary here.

	return output_data;
}

U8 *LLBlockEncoder::encodeF32(const LLBlockDataF32 &block_data, U32 &output_size) const
{
	// OK, for now, just use the standard KDU encoder, with a single channel
	// integer channel.

	// Collect simple arguments.
	bool allow_rate_prediction, allow_shorts, mem, quiet, no_weights;

	allow_rate_prediction = true;
	allow_shorts = false;
	no_weights = false;
	bool use_absolute = false;
	mem = false;
	quiet = false;

	F32 min, max, range, range_inv, offset;
	min = block_data.getMin();
	max = block_data.getMax();
	range = max - min;
	range_inv = 1.f / range;
	offset = 0.5f*(max + min);

	// Set codestream options
	siz_params siz;
	S16 precision = block_data.getPrecision(); // Assume precision is always 32 bits for floating point.

	siz.set(Sdims,0,0,(U16)block_data.getHeight());
	siz.set(Sdims,0,1,(U16)block_data.getWidth());
	siz.set(Ssigned,0,0,false);
	siz.set(Scomponents,0,0,1);
	siz.set(Sprecision,0,0, precision);

	// Construct the `kdu_codestream' object and parse all remaining arguments.
	output_size = block_data.getSize();
	if (output_size < 1000)
	{
		output_size = 1000;
	}

	U8 *output_buffer = new U8[output_size*2];

	LLKDUMemTarget output(output_buffer, output_size, block_data.getSize());

	kdu_codestream codestream;
	codestream.create(&siz,&output);

	codestream.access_siz()->parse_string("Clayers=1");
	codestream.access_siz()->finalize_all();

	kdu_tile tile = codestream.open_tile(kdu_coords(0,0));

	// Open tile-components and create processing engines and resources
	kdu_dims dims;
	kdu_sample_allocator allocator;
	kdu_tile_comp tile_comp;
	kdu_line_buf line;
	kdu_push_ifc engine;

	tile_comp = tile.access_component(0);
	kdu_resolution res = tile_comp.access_resolution(); // Get top resolution

	res.get_dims(dims);

	line.pre_create(&allocator,dims.size.x, use_absolute, allow_shorts);

	if (res.which() == 0) // No DWT levels (should not occur in this example)
	{
        engine = kdu_encoder(res.access_subband(LL_BAND),&allocator, use_absolute);
	}
	else
	{
        engine = kdu_analysis(res,&allocator, use_absolute);
	}

	allocator.finalize(); // Actually creates buffering resources
    line.create(); // Grabs resources from the allocator.

	// Now walk through the lines of the buffer, pushing them into the
	// relevant tile-component processing engines.

	F32 *source_f32 = NULL;

	S32 y;
	for (y = 0; y < dims.size.y; y++)
	{
		source_f32 = (F32*)(block_data.getData() + y * block_data.getRowStride());
		kdu_sample32 *dest = line.get_buf32();
		for (S32 n=dims.size.x; n > 0; n--, dest++, source_f32++)
		{
			dest->fval = ((*source_f32) - offset) * range_inv;
        }
        engine.push(line, true);
    }

	// Cleanup
    engine.destroy(); // engines are interfaces; no default destructors

	// Produce the final compressed output.
	kdu_long layer_bytes[1] = {0};

	layer_bytes[0] = (kdu_long) (mBPP*block_data.getWidth()*block_data.getHeight());
	// Here we are not requesting specific sizes for any of the 12
	// quality layers.  As explained in the description of
	// "kdu_codestream::flush" (see "kdu_compressed.h"), the rate allocator
	// will then assign the layers in such a way as to achieve roughly
	// two quality layers per octave change in bit-rate, with the final
	// layer reaching true lossless quality.

	codestream.flush(layer_bytes,1);
	// You can see how many bytes were assigned
	// to each quality layer by looking at the entries of `layer_bytes' here.
	// The flush function can do a lot of interesting things which you may
	// want to spend some time looking into. In addition to targeting
	// specific bit-rates for each quality layer, it can be configured to
	// use rate-distortion slope thresholds of your choosing and to return
	// the thresholds which it finds to be best for any particular set of
	// target layer sizes.  This opens the door to feedback-oriented rate
	// control for video.  You should also look into the
	// "kdu_codestream::set_max_bytes" and
	// "kdu_codestream::set_min_slope_threshold" functions which can be
	// used to significantly speed up compression.
	codestream.destroy(); // All done: simple as that.

	// Now that we're done encoding, create the new data buffer for the compressed
	// image and stick it there.

	U8 *output_data = new U8[output_size];

	memcpy(output_data, output_buffer, output_size);

	output.close(); // Not really necessary here.

	delete[] output_buffer;

	return output_data;
}


void LLBlockEncoder::setBPP(const F32 bpp)
{
	mBPP = bpp;
}
