 /** 
 * @file llimagej2ckdu.cpp
 * @brief This is an implementation of JPEG2000 encode/decode using Kakadu
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
#include "llimagej2ckdu.h"

#include "lltimer.h"
#include "llpointer.h"
#include "llmath.h"
#include "llkdumem.h"

#include "kdu_block_coding.h"

class kdc_flow_control {
	
public:
	kdc_flow_control(kdu_image_in_base *img_in, kdu_codestream codestream);
	~kdc_flow_control();
	bool advance_components();
	void process_components();
	
private:
	
	struct kdc_component_flow_control {
	public:
		kdu_image_in_base *reader;
		int vert_subsampling;
		int ratio_counter;  /*  Initialized to 0, decremented by `count_delta';
                                when < 0, a new line must be processed, after
                                which it is incremented by `vert_subsampling'.  */
		int initial_lines;
		int remaining_lines;
		kdu_line_buf *line;
	};
	
	kdu_codestream codestream;
	kdu_dims valid_tile_indices;
	kdu_coords tile_idx;
	kdu_tile tile;
	int num_components;
	kdc_component_flow_control *components;
	int count_delta; // Holds the minimum of the `vert_subsampling' fields
	kdu_multi_analysis engine;
	kdu_long max_buffer_memory;
};

//
// Kakadu specific implementation
//
void set_default_colour_weights(kdu_params *siz);

const char* engineInfoLLImageJ2CKDU()
{
	static std::string version = llformat("KDU %s", KDU_CORE_VERSION);
	return version.c_str();
}

LLImageJ2CKDU* createLLImageJ2CKDU()
{
	return new LLImageJ2CKDU();
}

void destroyLLImageJ2CKDU(LLImageJ2CKDU* kdu)
{
	delete kdu;
	kdu = NULL;
}

LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl()
{
	return new LLImageJ2CKDU();
}

void fallbackDestroyLLImageJ2CImpl(LLImageJ2CImpl* impl)
{
	delete impl;
	impl = NULL;
}

const char* fallbackEngineInfoLLImageJ2CImpl()
{
	return engineInfoLLImageJ2CKDU();
}

class LLKDUDecodeState
{
public:
	LLKDUDecodeState(kdu_tile tile, kdu_byte *buf, S32 row_gap);
	~LLKDUDecodeState();
	BOOL processTileDecode(F32 decode_time, BOOL limit_time = TRUE);

private:
	S32 mNumComponents;
	BOOL mUseYCC;
	kdu_dims mDims;
	kdu_sample_allocator mAllocator;
	kdu_tile_comp mComps[4];
	kdu_line_buf mLines[4];
	kdu_pull_ifc mEngines[4];
	bool mReversible[4]; // Some components may be reversible and others not
	int mBitDepths[4];   // Original bit-depth may be quite different from 8
	
	kdu_tile mTile;
	kdu_byte *mBuf;
	S32 mRowGap;
};

void ll_kdu_error( void )
{
	// *FIX: This exception is bad, bad, bad. It gets thrown from a
	// destructor which can lead to immediate program termination!
	throw "ll_kdu_error() throwing an exception";
}

// Stuff for new kdu error handling
class LLKDUMessageWarning : public kdu_message
{
public:
	/*virtual*/ void put_text(const char *s);
	/*virtual*/ void put_text(const kdu_uint16 *s);

	static LLKDUMessageWarning sDefaultMessage;
};

class LLKDUMessageError : public kdu_message
{
public:
	/*virtual*/ void put_text(const char *s);
	/*virtual*/ void put_text(const kdu_uint16 *s);
	/*virtual*/ void flush(bool end_of_message = false);
	static LLKDUMessageError sDefaultMessage;
};

void LLKDUMessageWarning::put_text(const char *s)
{
	llinfos << "KDU Warning: " << s << llendl;
}

void LLKDUMessageWarning::put_text(const kdu_uint16 *s)
{
	llinfos << "KDU Warning: " << s << llendl;
}

void LLKDUMessageError::put_text(const char *s)
{
	llinfos << "KDU Error: " << s << llendl;
}

void LLKDUMessageError::put_text(const kdu_uint16 *s)
{
	llinfos << "KDU Error: " << s << llendl;
}

void LLKDUMessageError::flush(bool end_of_message)
{
	if (end_of_message) 
	{
		throw "KDU throwing an exception";
	}
}

LLKDUMessageWarning LLKDUMessageWarning::sDefaultMessage;
LLKDUMessageError	LLKDUMessageError::sDefaultMessage;
static bool kdu_message_initialized = false;

LLImageJ2CKDU::LLImageJ2CKDU() : LLImageJ2CImpl(),
mInputp(NULL),
mCodeStreamp(NULL),
mTPosp(NULL),
mTileIndicesp(NULL),
mRawImagep(NULL),
mDecodeState(NULL),
mBlocksSize(-1),
mPrecinctsSize(-1),
mLevels(0)
{
}

LLImageJ2CKDU::~LLImageJ2CKDU()
{
	cleanupCodeStream(); // in case destroyed before decode completed
}

// Stuff for new simple decode
void transfer_bytes(kdu_byte *dest, kdu_line_buf &src, int gap, int precision);

void LLImageJ2CKDU::setupCodeStream(LLImageJ2C &base, BOOL keep_codestream, ECodeStreamMode mode)
{
	S32 data_size = base.getDataSize();
	S32 max_bytes = (base.getMaxBytes() ? base.getMaxBytes() : data_size);

	//
	//  Initialization
	//
	if (!kdu_message_initialized)
	{
		kdu_message_initialized = true;
		kdu_customize_errors(&LLKDUMessageError::sDefaultMessage);
		kdu_customize_warnings(&LLKDUMessageWarning::sDefaultMessage);
	}

	if (mCodeStreamp)
	{
		mCodeStreamp->destroy();
		delete mCodeStreamp;
		mCodeStreamp = NULL;
	}

	if (!mInputp && base.getData())
	{
		// The compressed data has been loaded
		// Setup the source for the codestream
		mInputp = new LLKDUMemSource(base.getData(), data_size);
	}

	if (mInputp)
	{
		mInputp->reset();
	}
	mCodeStreamp = new kdu_codestream;

	mCodeStreamp->create(mInputp);

	// Set the maximum number of bytes to use from the codestream
	// *TODO: This seems to be wrong. The base class should have no idea of how j2c compression works so no
	// good way of computing what's the byte range to be used.
	mCodeStreamp->set_max_bytes(max_bytes,true);

	//	If you want to flip or rotate the image for some reason, change
	// the resolution, or identify a restricted region of interest, this is
	// the place to do it.  You may use "kdu_codestream::change_appearance"
	// and "kdu_codestream::apply_input_restrictions" for this purpose.
	//	If you wish to truncate the code-stream prior to decompression, you
	// may use "kdu_codestream::set_max_bytes".
	//	If you wish to retain all compressed data so that the material
	// can be decompressed multiple times, possibly with different appearance
	// parameters, you should call "kdu_codestream::set_persistent" here.
	//	There are a variety of other features which must be enabled at
	// this point if you want to take advantage of them.  See the
	// descriptions appearing with the "kdu_codestream" interface functions
	// in "kdu_compressed.h" for an itemized account of these capabilities.

	switch (mode)
	{
	case MODE_FAST:
		mCodeStreamp->set_fast();
		break;
	case MODE_RESILIENT:
		mCodeStreamp->set_resilient();
		break;
	case MODE_FUSSY:
		mCodeStreamp->set_fussy();
		break;
	default:
		llassert(0);
		mCodeStreamp->set_fast();
	}

	kdu_dims dims;
	mCodeStreamp->get_dims(0,dims);

	S32 components = mCodeStreamp->get_num_components();

	if (components >= 3)
	{ // Check that components have consistent dimensions (for PPM file)
		kdu_dims dims1; mCodeStreamp->get_dims(1,dims1);
		kdu_dims dims2; mCodeStreamp->get_dims(2,dims2);
		if ((dims1 != dims) || (dims2 != dims))
		{
			llerrs << "Components don't have matching dimensions!" << llendl;
		}
	}

	// Get the number of resolution levels in that image
	mLevels = mCodeStreamp->get_min_dwt_levels();
	
	// Set the base dimensions
	base.setSize(dims.size.x, dims.size.y, components);
	base.setLevels(mLevels);
	
	if (!keep_codestream)
	{
		mCodeStreamp->destroy();
		delete mCodeStreamp;
		mCodeStreamp = NULL;
		delete mInputp;
		mInputp = NULL;
	}
}

void LLImageJ2CKDU::cleanupCodeStream()
{
	delete mInputp;
	mInputp = NULL;

	delete mDecodeState;
	mDecodeState = NULL;

	if (mCodeStreamp)
	{
		mCodeStreamp->destroy();
		delete mCodeStreamp;
		mCodeStreamp = NULL;
	}

	delete mTPosp;
	mTPosp = NULL;

	delete mTileIndicesp;
	mTileIndicesp = NULL;
}

BOOL LLImageJ2CKDU::initDecode(LLImageJ2C &base, LLImageRaw &raw_image, int discard_level, int* region)
{
	return initDecode(base,raw_image,0.0f,MODE_FAST,0,4,discard_level,region);
}

BOOL LLImageJ2CKDU::initEncode(LLImageJ2C &base, LLImageRaw &raw_image, int blocks_size, int precincts_size, int levels)
{
	mPrecinctsSize = precincts_size;
	if (mPrecinctsSize != -1)
	{
		mPrecinctsSize = get_lower_power_two(mPrecinctsSize,MAX_PRECINCT_SIZE);
		mPrecinctsSize = llmax(mPrecinctsSize,MIN_PRECINCT_SIZE);
	}
	mBlocksSize = blocks_size;
	if (mBlocksSize != -1)
	{
		mBlocksSize = get_lower_power_two(mBlocksSize,MAX_BLOCK_SIZE);
		mBlocksSize = llmax(mBlocksSize,MIN_BLOCK_SIZE);
		if (mPrecinctsSize != -1)
		{
			mBlocksSize = llmin(mBlocksSize,mPrecinctsSize);	// blocks *must* be smaller than precincts
		}
	}
	mLevels = levels;
	if (mLevels != 0)
	{
		mLevels = llclamp(mLevels,MIN_DECOMPOSITION_LEVELS,MAX_DECOMPOSITION_LEVELS);
		base.setLevels(mLevels);
	}
	return TRUE;
}

BOOL LLImageJ2CKDU::initDecode(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, ECodeStreamMode mode, S32 first_channel, S32 max_channel_count, int discard_level, int* region)
{
	base.resetLastError();

	// *FIX: kdu calls our callback function if there's an error, and then bombs.
	// To regain control, we throw an exception, and catch it here.
	try
	{
		// Merov : Test!! DO NOT COMMIT!!
		//findDiscardLevelsBoundaries(base);

		base.updateRawDiscardLevel();
		setupCodeStream(base, TRUE, mode);

		mRawImagep = &raw_image;
		mCodeStreamp->change_appearance(false, true, false);

		// Apply loading discard level and cropping if required
		kdu_dims* region_kdu = NULL;
		if (region != NULL)
		{
			region_kdu = new kdu_dims;
			region_kdu->pos.x  = region[0];
			region_kdu->pos.y  = region[1];
			region_kdu->size.x = region[2] - region[0];
			region_kdu->size.y = region[3] - region[1];
		}
		int discard = (discard_level != -1 ? discard_level : base.getRawDiscardLevel());
		//llinfos << "Merov debug : initDecode, discard used = " << discard << ", asked = " << discard_level << llendl;
		// Apply loading restrictions
		mCodeStreamp->apply_input_restrictions( first_channel, max_channel_count, discard, 0, region_kdu);
		
		// Clean-up
		if (region_kdu)
		{
			delete region_kdu;
			region_kdu = NULL;
		}

		// Resize raw_image according to the image to be decoded
		kdu_dims dims; mCodeStreamp->get_dims(0,dims);
		S32 channels = base.getComponents() - first_channel;
		channels = llmin(channels,max_channel_count);
		raw_image.resize(dims.size.x, dims.size.y, channels);

		if (!mTileIndicesp)
		{
			mTileIndicesp = new kdu_dims;
		}
		mCodeStreamp->get_valid_tiles(*mTileIndicesp);
		if (!mTPosp)
		{
			mTPosp = new kdu_coords;
			mTPosp->y = 0;
			mTPosp->x = 0;
		}
	}
	catch (const char* msg)
	{
		base.setLastError(ll_safe_string(msg));
		return FALSE;
	}
	catch (...)
	{
		base.setLastError("Unknown J2C error");
		return FALSE;
	}

	return TRUE;
}


// Returns TRUE to mean done, whether successful or not.
BOOL LLImageJ2CKDU::decodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count)
{
	ECodeStreamMode mode = MODE_FAST;

	LLTimer decode_timer;

	if (!mCodeStreamp)
	{
		if (!initDecode(base, raw_image, decode_time, mode, first_channel, max_channel_count))
		{
			// Initializing the J2C decode failed, bail out.
			cleanupCodeStream();
			return TRUE; // done
		}
	}

	// These can probably be grabbed from what's saved in the class.
	kdu_dims dims;
	mCodeStreamp->get_dims(0,dims);

	// Now we are ready to walk through the tiles processing them one-by-one.
	kdu_byte *buffer = raw_image.getData();

	while (mTPosp->y < mTileIndicesp->size.y)
	{
		while (mTPosp->x < mTileIndicesp->size.x)
		{
			try
			{
				if (!mDecodeState)
				{
					kdu_tile tile = mCodeStreamp->open_tile(*(mTPosp)+mTileIndicesp->pos);

					// Find the region of the buffer occupied by this
					// tile.  Note that we have no control over
					// sub-sampling factors which might have been used
					// during compression and so it can happen that tiles
					// (at the image component level) actually have
					// different dimensions.  For this reason, we cannot
					// figure out the buffer region occupied by a tile
					// directly from the tile indices.  Instead, we query
					// the highest resolution of the first tile-component
					// concerning its location and size on the canvas --
					// the `dims' object already holds the location and
					// size of the entire image component on the same
					// canvas coordinate system.  Comparing the two tells
					// us where the current tile is in the buffer.
					S32 channels = base.getComponents() - first_channel;
					if (channels > max_channel_count)
					{
						channels = max_channel_count;
					}
					kdu_resolution res = tile.access_component(0).access_resolution();
					kdu_dims tile_dims; res.get_dims(tile_dims);
					kdu_coords offset = tile_dims.pos - dims.pos;
					int row_gap = channels*dims.size.x; // inter-row separation
					kdu_byte *buf = buffer + offset.y*row_gap + offset.x*channels;
					mDecodeState = new LLKDUDecodeState(tile, buf, row_gap);
				}
				// Do the actual processing
				F32 remaining_time = decode_time - decode_timer.getElapsedTimeF32();
				// This is where we do the actual decode.  If we run out of time, return false.
				if (mDecodeState->processTileDecode(remaining_time, (decode_time > 0.0f)))
				{
					delete mDecodeState;
					mDecodeState = NULL;
				}
				else
				{
					// Not finished decoding yet.
					//					setLastError("Ran out of time while decoding");
					return FALSE;
				}
			}
			catch (const char* msg)
			{
				base.setLastError(ll_safe_string(msg));
				base.decodeFailed();
				cleanupCodeStream();
				return TRUE; // done
			}
			catch (...)
			{
				base.setLastError( "Unknown J2C error" );
				base.decodeFailed();
				cleanupCodeStream();
				return TRUE; // done
			}


			mTPosp->x++;
		}
		mTPosp->y++;
		mTPosp->x = 0;
	}

	cleanupCodeStream();

	return TRUE;
}


BOOL LLImageJ2CKDU::encodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text, F32 encode_time, BOOL reversible)
{
	// Declare and set simple arguments
	bool transpose = false;
	bool vflip = true;
	bool hflip = false;

	try
	{
		// Set up input image files
		siz_params siz;
		
		// Should set rate someplace here
		LLKDUMemIn mem_in(raw_image.getData(),
			raw_image.getDataSize(),
			raw_image.getWidth(),
			raw_image.getHeight(),
			raw_image.getComponents(),
			&siz);

		base.setSize(raw_image.getWidth(), raw_image.getHeight(), raw_image.getComponents());

		int num_components = raw_image.getComponents();

		siz.set(Scomponents,0,0,num_components);
		siz.set(Sdims,0,0,base.getHeight());  // Height of first image component
		siz.set(Sdims,0,1,base.getWidth());   // Width of first image component
		siz.set(Sprecision,0,0,8);  // Image samples have original bit-depth of 8
		siz.set(Ssigned,0,0,false); // Image samples are originally unsigned

		kdu_params *siz_ref = &siz; 
		siz_ref->finalize();
		siz_params transformed_siz; // Use this one to construct code-stream
		transformed_siz.copy_from(&siz,-1,-1,-1,0,transpose,false,false);

		// Construct the `kdu_codestream' object and parse all remaining arguments
		U32 max_output_size = base.getWidth()*base.getHeight()*base.getComponents();
		max_output_size = (max_output_size < 1000 ? 1000 : max_output_size);
		U8 *output_buffer = new U8[max_output_size];
		U32 output_size = 0; // Address updated by LLKDUMemTarget to give the final compressed buffer size
		LLKDUMemTarget output(output_buffer, output_size, max_output_size);

		kdu_codestream codestream;
		codestream.create(&transformed_siz,&output);

		if (comment_text)
		{
			// Set the comments for the codestream
			kdu_codestream_comment comment = codestream.add_comment();
			comment.put_text(comment_text);
		}

		if (num_components >= 3)
		{
			// Note that we always use YCC and not YUV
			// *TODO: Verify this doesn't screws up reversible textures (like sculpties) as YCC is not reversible but YUV is...
			set_default_colour_weights(codestream.access_siz());
		}

		// Set codestream options
		int nb_layers = 0;
		kdu_long layer_bytes[MAX_NB_LAYERS];
		U32 max_bytes = (U32)(base.getWidth() * base.getHeight() * base.getComponents());

		// Rate is the argument passed into the LLImageJ2C which specifies the target compression rate. The default is 8:1.
		// *TODO: mRate is actually always 8:1 in the viewer. Test different values.
		llassert (base.mRate > 0.f);
		max_bytes = (U32)((F32)(max_bytes) * base.mRate);
		
		// This code is where we specify the target number of bytes for each quality layer.
		// We're using a logarithmic spacing rule that fits with our way of fetching texture data.
		// Note: For more info on this layers business, read kdu_codestream::flush() doc in kdu_compressed.h
		layer_bytes[nb_layers++] = FIRST_PACKET_SIZE;
		U32 i = MIN_LAYER_SIZE;
		while ((i < max_bytes) && (nb_layers < (MAX_NB_LAYERS-1)))
		{
			layer_bytes[nb_layers++] = i;
			i *= 4;
		}
		// Note: for small images, we can have (max_bytes < FIRST_PACKET_SIZE), hence the test
		if (layer_bytes[nb_layers-1] < max_bytes)
		{
			// Set the last quality layer so to fit the preset compression ratio
			layer_bytes[nb_layers++] = max_bytes;
		}

		if (reversible)
		{
			// Use 0 for a last quality layer for reversible images so all remaining code blocks will be flushed
			// Hack: KDU encoding for reversible images has a bug for small images that leads to j2c images that 
			// cannot be open or are very blurry. Avoiding that last layer prevents the problem to happen.
			if ((base.getWidth() >= 32) || (base.getHeight() >= 32))
			{
				layer_bytes[nb_layers++] = 0;
			}
			codestream.access_siz()->parse_string("Creversible=yes");
			// *TODO: we should use yuv in reversible mode
			// Don't turn this on now though as it creates problems on decoding for the moment
			//codestream.access_siz()->parse_string("Cycc=no");
		}
		
		std::string layer_string = llformat("Clayers=%d",nb_layers);
		codestream.access_siz()->parse_string(layer_string.c_str());
		
		// Set up data ordering, markers, etc... if precincts or blocks specified
		if ((mBlocksSize != -1) || (mPrecinctsSize != -1))
		{
			if (mPrecinctsSize != -1)
			{
				std::string precincts_string = llformat("Cprecincts={%d,%d}",mPrecinctsSize,mPrecinctsSize);
				codestream.access_siz()->parse_string(precincts_string.c_str());
			}
			if (mBlocksSize != -1)
			{
				std::string blocks_string = llformat("Cblk={%d,%d}",mBlocksSize,mBlocksSize);
				codestream.access_siz()->parse_string(blocks_string.c_str());
			}
			std::string ordering_string = llformat("Corder=LRCP");
			codestream.access_siz()->parse_string(ordering_string.c_str());
			std::string PLT_string = llformat("ORGgen_plt=yes");
			codestream.access_siz()->parse_string(PLT_string.c_str());
			std::string Parts_string = llformat("ORGtparts=R");
			codestream.access_siz()->parse_string(Parts_string.c_str());
		}
		
		// Set the number of wavelets subresolutions (aka levels) 
		if (mLevels != 0)
		{
			std::string levels_string = llformat("Clevels=%d",mLevels);
			codestream.access_siz()->parse_string(levels_string.c_str());
		}
		
		// Complete the encode settings
		codestream.access_siz()->finalize_all();
		codestream.change_appearance(transpose,vflip,hflip);

		// Now we are ready for sample data processing
		kdc_flow_control *tile = new kdc_flow_control(&mem_in,codestream);
		bool done = false;
		while (!done)
		{ 
			// Process line by line
			if (tile->advance_components())
			{
				tile->process_components();
			}
			else
			{
				done = true;
			}
		}

		// Produce the compressed output
		codestream.flush(layer_bytes,nb_layers);

		// Cleanup
		delete tile;
		codestream.destroy();

		// Now that we're done encoding, create the new data buffer for the compressed
		// image and stick it there.
		base.copyData(output_buffer, output_size);
		base.updateData(); // set width, height
		delete[] output_buffer;
	}
	catch(const char* msg)
	{
		base.setLastError(ll_safe_string(msg));
		return FALSE;
	}
	catch( ... )
	{
		base.setLastError( "Unknown J2C error" );
		return FALSE;
	}

	return TRUE;
}

BOOL LLImageJ2CKDU::getMetadata(LLImageJ2C &base)
{
	// *FIX: kdu calls our callback function if there's an error, and
	// then bombs. To regain control, we throw an exception, and
	// catch it here.
	try
	{
		setupCodeStream(base, FALSE, MODE_FAST);
		return TRUE;
	}
	catch (const char* msg)
	{
		base.setLastError(ll_safe_string(msg));
		return FALSE;
	}
	catch (...)
	{
		base.setLastError( "Unknown J2C error" );
		return FALSE;
	}
}

/*****************************************************************************/
/* STATIC                        copy_block                                  */
/*****************************************************************************/

static void copy_block(kdu_block *in, kdu_block *out)
{
	if (in->K_max_prime != out->K_max_prime)
    { 
		std::cout << "Cannot copy blocks belonging to subbands with different quantization parameters." << std::endl; 
		return;
	}
	if ((in->size.x != out->size.x) || (in->size.y != out->size.y))  
    { 
		std::cout << "Cannot copy code-blocks with different dimensions." << std::endl; 
		return;
	}
	out->missing_msbs = in->missing_msbs;
	if (out->max_passes < (in->num_passes+2))        // Gives us enough to round up
		out->set_max_passes(in->num_passes+2,false); // to the next whole bit-plane
	out->num_passes = in->num_passes;
	int num_bytes = 0;
	for (int z=0; z < in->num_passes; z++)
    {
		num_bytes += (out->pass_lengths[z] = in->pass_lengths[z]);
		out->pass_slopes[z] = in->pass_slopes[z];
    }
	
    // Just copy compressed code-bytes. Block transcoding not supported.
	if (out->max_bytes < num_bytes)
		out->set_max_bytes(num_bytes,false);
	memcpy(out->byte_buffer,in->byte_buffer,(size_t) num_bytes);
}

/*****************************************************************************/
/* STATIC                        copy_tile                                   */
/*****************************************************************************/

static void
copy_tile(kdu_tile tile_in, kdu_tile tile_out, int tnum_in, int tnum_out,
		  kdu_params *siz_in, kdu_params *siz_out, int skip_components,
		  int &num_blocks)
{
	int num_components = tile_out.get_num_components();
	int new_tpart=0, next_tpart = 1;
	
	for (int c=0; c < num_components; c++)
    {
		kdu_tile_comp comp_in, comp_out;
		comp_in = tile_in.access_component(c);
		comp_out = tile_out.access_component(c);
		int num_resolutions = comp_out.get_num_resolutions();
		//std::cout << "    Copying tile : num_resolutions = " << num_resolutions << std::endl;
		for (int r=0; r < num_resolutions; r++)
        {
			kdu_resolution res_in;  res_in = comp_in.access_resolution(r);
			kdu_resolution res_out; res_out = comp_out.access_resolution(r);
			int b, min_band;
			int num_bands = res_in.get_valid_band_indices(min_band);
			std::cout << "        Copying tile : num_bands = " << num_bands << std::endl;
			for (b=min_band; num_bands > 0; num_bands--, b++)
            {
				kdu_subband band_in;  band_in = res_in.access_subband(b);
				kdu_subband band_out; band_out = res_out.access_subband(b);
				kdu_dims blocks_in;  band_in.get_valid_blocks(blocks_in);
				kdu_dims blocks_out; band_out.get_valid_blocks(blocks_out);
				if ((blocks_in.size.x != blocks_out.size.x) ||
					(blocks_in.size.y != blocks_out.size.y))
                { 
					std::cout << "Transcoding operation cannot proceed: Code-block partitions for the input and output code-streams do not agree." << std::endl;
					return;
				}
				kdu_coords idx;
				//std::cout << "            Copying tile : block indices, x = " << blocks_out.size.x << " and y = " << blocks_out.size.y << std::endl;
				for (idx.y=0; idx.y < blocks_out.size.y; idx.y++)
				{
					for (idx.x=0; idx.x < blocks_out.size.x; idx.x++)
					{
						kdu_block *in =
						band_in.open_block(idx+blocks_in.pos,&new_tpart);
						for (; next_tpart <= new_tpart; next_tpart++)
							siz_out->copy_from(siz_in,tnum_in,tnum_out,next_tpart,
											   skip_components);
						kdu_block *out = band_out.open_block(idx+blocks_out.pos);
						copy_block(in,out);
						band_in.close_block(in);
						band_out.close_block(out);
						num_blocks++;
					}
				}
            }
        }
    }
}

// Find the block boundary for each discard level in the input image.
// We parse the input blocks and copy them in a temporary output stream.
// For the moment, we do nothing more that parsing the raw list of blocks and outputing result.
void LLImageJ2CKDU::findDiscardLevelsBoundaries(LLImageJ2C &base)
{
	// We need the number of levels in that image before starting.
	getMetadata(base);
	
	for (int discard_level = 0; discard_level < mLevels; discard_level++)
	{
		//std::cout << "Parsing discard level = " << discard_level << std::endl;
		// Create the input codestream object.
		setupCodeStream(base, TRUE, MODE_FAST);
		mCodeStreamp->apply_input_restrictions(0, 4, discard_level, 0, NULL);
		mCodeStreamp->set_max_bytes(KDU_LONG_MAX,true);
		siz_params *siz_in = mCodeStreamp->access_siz();
	
		// Create the output codestream object.
		siz_params siz;
		siz.copy_from(siz_in,-1,-1,-1,0,discard_level,false,false,false);
		siz.set(Scomponents,0,0,mCodeStreamp->get_num_components());
	
		U32 max_output_size = base.getWidth()*base.getHeight()*base.getComponents();
		max_output_size = (max_output_size < 1000 ? 1000 : max_output_size);
		U8 *output_buffer = new U8[max_output_size];
		U32 output_size = 0; // Address updated by LLKDUMemTarget to give the final compressed buffer size
		LLKDUMemTarget output(output_buffer, output_size, max_output_size);
		kdu_codestream codestream_out; 
		codestream_out.create(&siz,&output);
		//codestream_out.share_buffering(*mCodeStreamp);
		siz_params *siz_out = codestream_out.access_siz();
		siz_out->copy_from(siz_in,-1,-1,-1,0,discard_level,false,false,false);
		codestream_out.access_siz()->finalize_all(-1);
	
		// Set up rate control variables
		kdu_long max_bytes = KDU_LONG_MAX;
		kdu_params *cod = siz_out->access_cluster(COD_params);
		int total_layers;  cod->get(Clayers,0,0,total_layers);
		kdu_long *layer_bytes = new kdu_long[total_layers];
		int nel, non_empty_layers = 0;
	
		// Now ready to perform the transfer of compressed data between streams
		int flush_counter = INT_MAX;
		kdu_dims tile_indices_in;  
		mCodeStreamp->get_valid_tiles(tile_indices_in);
		kdu_dims tile_indices_out; 
		codestream_out.get_valid_tiles(tile_indices_out);
		assert((tile_indices_in.size.x == tile_indices_out.size.x) &&
			   (tile_indices_in.size.y == tile_indices_out.size.y));
		int num_blocks=0;
	
		kdu_coords idx;
		//std::cout << "Parsing tiles : x = " << tile_indices_out.size.x << " to y = " << tile_indices_out.size.y << std::endl;
		for (idx.y=0; idx.y < tile_indices_out.size.y; idx.y++)
		{
			for (idx.x=0; idx.x < tile_indices_out.size.x; idx.x++)
			{
				kdu_tile tile_in = mCodeStreamp->open_tile(idx+tile_indices_in.pos);
				int tnum_in = tile_in.get_tnum();
				int tnum_out = idx.x + idx.y*tile_indices_out.size.x;
				siz_out->copy_from(siz_in,tnum_in,tnum_out,0,0,discard_level,false,false,false);
				siz_out->finalize_all(tnum_out);
				// Note: do not open the output tile without first copying any tile-specific code-stream parameters
				kdu_tile tile_out = codestream_out.open_tile(idx+tile_indices_out.pos);
				assert(tnum_out == tile_out.get_tnum());
				copy_tile(tile_in,tile_out,tnum_in,tnum_out,siz_in,siz_out,0,num_blocks);
				tile_in.close();
				tile_out.close();
				flush_counter--;
				if ((flush_counter <= 0) && codestream_out.ready_for_flush())
				{
					flush_counter = INT_MAX;
					nel = codestream_out.trans_out(max_bytes,layer_bytes,total_layers);
					non_empty_layers = (nel > non_empty_layers)?nel:non_empty_layers;
				}
			}
		}
	
		// Generate the output code-stream
		if (codestream_out.ready_for_flush())
		{
			nel = codestream_out.trans_out(max_bytes,layer_bytes,total_layers);
			non_empty_layers = (nel > non_empty_layers)?nel:non_empty_layers;
		}
		if (non_empty_layers > total_layers)
			non_empty_layers = total_layers; // Can happen if a tile has more layers
	
		// Print out stats
		std::cout << "Code stream parsing for discard level = " << discard_level << std::endl;
		std::cout << "    Total compressed memory in  = " << mCodeStreamp->get_compressed_data_memory() << " bytes" << std::endl;
		std::cout << "    Total compressed memory out = " << codestream_out.get_compressed_data_memory() << " bytes" << std::endl;
		//std::cout << "    Output contains " << total_layers << " quality layers" << std::endl;		
		std::cout << "    Transferred " << num_blocks << " code-blocks from in to out" << std::endl;
		//std::cout << "    Read " << mCodeStreamp->get_num_tparts() << " tile-part(s) from a total of " << (int) tile_indices_in.area() << " tile(s)" << std::endl;
		std::cout << "    Total bytes read = " << mCodeStreamp->get_total_bytes() << std::endl;
		//std::cout << "    Wrote " << codestream_out.get_num_tparts() << " tile-part(s) in a total of " << (int) tile_indices_out.area() << " tile(s)" << std::endl;
		std::cout << "    Total bytes written = " << codestream_out.get_total_bytes() << std::endl;
		std::cout << "-------------" << std::endl;
	
		// Clean-up
		cleanupCodeStream();
		codestream_out.destroy();
		delete[] output_buffer;	
	}
	return;
}

void set_default_colour_weights(kdu_params *siz)
{
	kdu_params *cod = siz->access_cluster(COD_params);
	assert(cod != NULL);

	bool can_use_ycc = true;
	bool rev0 = false;
	int depth0 = 0, sub_x0 = 1, sub_y0 = 1;
	for (int c = 0; c < 3; c++)
	{
		int depth = 0; siz->get(Sprecision,c,0,depth);
		int sub_y = 1; siz->get(Ssampling,c,0,sub_y);
		int sub_x = 1; siz->get(Ssampling,c,1,sub_x);
		kdu_params *coc = cod->access_relation(-1,c);
		bool rev = false; coc->get(Creversible,0,0,rev);
		if (c == 0)
		{
			rev0 = rev; depth0 = depth; sub_x0 = sub_x; sub_y0 = sub_y;
		}
		else if ((rev != rev0) || (depth != depth0) || 
				 (sub_x != sub_x0) || (sub_y != sub_y0))
		{
			can_use_ycc = false;
		}
	}
	if (!can_use_ycc)
	{
		return;
	}

	bool use_ycc;
	if (!cod->get(Cycc,0,0,use_ycc))
	{
		cod->set(Cycc,0,0,use_ycc=true);
	}
	if (!use_ycc)
	{
		return;
	}
	float weight;
	if (cod->get(Clev_weights,0,0,weight) || cod->get(Cband_weights,0,0,weight))
	{
		// Weights already specified explicitly -> nothing to do
		return; 
	}

	// These example weights are adapted from numbers generated by Marcus Nadenau
	// at EPFL, for a viewing distance of 15 cm and a display resolution of
	// 300 DPI.

	cod->parse_string("Cband_weights:C0="
		"{0.0901},{0.2758},{0.2758},"
		"{0.7018},{0.8378},{0.8378},{1}");
	cod->parse_string("Cband_weights:C1="
		"{0.0263},{0.0863},{0.0863},"
		"{0.1362},{0.2564},{0.2564},"
		"{0.3346},{0.4691},{0.4691},"
		"{0.5444},{0.6523},{0.6523},"
		"{0.7078},{0.7797},{0.7797},{1}");
	cod->parse_string("Cband_weights:C2="
		"{0.0773},{0.1835},{0.1835},"
		"{0.2598},{0.4130},{0.4130},"
		"{0.5040},{0.6464},{0.6464},"
		"{0.7220},{0.8254},{0.8254},"
		"{0.8769},{0.9424},{0.9424},{1}");
}

/******************************************************************************/
/*                              transfer_bytes                                */
/******************************************************************************/

void transfer_bytes(kdu_byte *dest, kdu_line_buf &src, int gap, int precision)
/* Transfers source samples from the supplied line buffer into the output
byte buffer, spacing successive output samples apart by `gap' bytes
(to allow for interleaving of colour components).  The function performs
all necessary level shifting, type conversion, rounding and truncation. */
{
	int width = src.get_width();
	if (src.get_buf32() != NULL)
	{ // Decompressed samples have a 32-bit representation (integer or float)
		assert(precision >= 8); // Else would have used 16 bit representation
		kdu_sample32 *sp = src.get_buf32();
		if (!src.is_absolute())
		{ // Transferring normalized floating point data.
			float scale16 = (float)(1<<16);
			kdu_int32 val;

			for (; width > 0; width--, sp++, dest+=gap)
			{
				val = (kdu_int32)(sp->fval*scale16);
				val = (val+128)>>8; // May be faster than true rounding
				val += 128;
				if (val & ((-1)<<8))
				{
					val = (val < 0 ? 0 : 255);
				}
				*dest = (kdu_byte) val;
			}
		}
		else
		{ // Transferring 32-bit absolute integers.
			kdu_int32 val;
			kdu_int32 downshift = precision-8;
			kdu_int32 offset = (1<<downshift)>>1;

			for (; width > 0; width--, sp++, dest+=gap)
			{
				val = sp->ival;
				val = (val+offset)>>downshift;
				val += 128;
				if (val & ((-1)<<8))
				{
					val = (val < 0 ? 0 : 255);
				}
				*dest = (kdu_byte) val;
			}
		}
	}
	else
	{ // Source data is 16 bits.
		kdu_sample16 *sp = src.get_buf16();
		if (!src.is_absolute())
		{ // Transferring 16-bit fixed point quantities
			kdu_int16 val;

			if (precision >= 8)
			{ // Can essentially ignore the bit-depth.
				for (; width > 0; width--, sp++, dest+=gap)
				{
					val = sp->ival;
					val += (1<<(KDU_FIX_POINT-8))>>1;
					val >>= (KDU_FIX_POINT-8);
					val += 128;
					if (val & ((-1)<<8))
					{
						val = (val < 0 ? 0 : 255);
					}
					*dest = (kdu_byte) val;
				}
			}
			else
			{ // Need to force zeros into one or more least significant bits.
				kdu_int16 downshift = KDU_FIX_POINT-precision;
				kdu_int16 upshift = 8-precision;
				kdu_int16 offset = 1<<(downshift-1);

				for (; width > 0; width--, sp++, dest+=gap)
				{
					val = sp->ival;
					val = (val+offset)>>downshift;
					val <<= upshift;
					val += 128;
					if (val & ((-1)<<8))
					{
						val = (val < 0 ? 0 : 256 - (1<<upshift));
					}
					*dest = (kdu_byte) val;
				}
			}
		}
		else
		{ // Transferring 16-bit absolute integers.
			kdu_int16 val;

			if (precision >= 8)
			{
				kdu_int16 downshift = precision-8;
				kdu_int16 offset = (1<<downshift)>>1;

				for (; width > 0; width--, sp++, dest+=gap)
				{
					val = sp->ival;
					val = (val+offset)>>downshift;
					val += 128;
					if (val & ((-1)<<8))
					{
						val = (val < 0 ? 0 : 255);
					}
					*dest = (kdu_byte) val;
				}
			}
			else
			{
				kdu_int16 upshift = 8-precision;

				for (; width > 0; width--, sp++, dest+=gap)
				{
					val = sp->ival;
					val <<= upshift;
					val += 128;
					if (val & ((-1)<<8))
					{
						val = (val < 0 ? 0 : 256 - (1<<upshift));
					}
					*dest = (kdu_byte) val;
				}
			}
		}
	}
}

LLKDUDecodeState::LLKDUDecodeState(kdu_tile tile, kdu_byte *buf, S32 row_gap)
{
	S32 c;

	mTile = tile;
	mBuf = buf;
	mRowGap = row_gap;

	mNumComponents = tile.get_num_components();

	llassert(mNumComponents <= 4);
	mUseYCC = tile.get_ycc();

	for (c = 0; c < 4; ++c)
	{
		mReversible[c] = false;
		mBitDepths[c] = 0;
	}

	// Open tile-components and create processing engines and resources
	for (c = 0; c < mNumComponents; c++)
	{
		mComps[c] = mTile.access_component(c);
		mReversible[c] = mComps[c].get_reversible();
		mBitDepths[c] = mComps[c].get_bit_depth();
		kdu_resolution res = mComps[c].access_resolution(); // Get top resolution
		kdu_dims comp_dims; res.get_dims(comp_dims);
		if (c == 0)
		{
			mDims = comp_dims;
		}
		else
		{
			llassert(mDims == comp_dims); // Safety check; the caller has ensured this
		}
		bool use_shorts = (mComps[c].get_bit_depth(true) <= 16);
		mLines[c].pre_create(&mAllocator,mDims.size.x,mReversible[c],use_shorts);
		if (res.which() == 0) // No DWT levels used
		{
			mEngines[c] = kdu_decoder(res.access_subband(LL_BAND),&mAllocator,use_shorts);
		}
		else
		{
			mEngines[c] = kdu_synthesis(res,&mAllocator,use_shorts);
		}
	}
	mAllocator.finalize(); // Actually creates buffering resources
	for (c = 0; c < mNumComponents; c++)
	{
		mLines[c].create(); // Grabs resources from the allocator.
	}
}

LLKDUDecodeState::~LLKDUDecodeState()
{
	// Cleanup
	for (S32 c = 0; c < mNumComponents; c++)
	{
		mEngines[c].destroy(); // engines are interfaces; no default destructors
	}
	mTile.close();
}

BOOL LLKDUDecodeState::processTileDecode(F32 decode_time, BOOL limit_time)
/* Decompresses a tile, writing the data into the supplied byte buffer.
The buffer contains interleaved image components, if there are any.
Although you may think of the buffer as belonging entirely to this tile,
the `buf' pointer may actually point into a larger buffer representing
multiple tiles.  For this reason, `row_gap' is needed to identify the
separation between consecutive rows in the real buffer. */
{
	S32 c;
	// Now walk through the lines of the buffer, recovering them from the
	// relevant tile-component processing engines.

	LLTimer decode_timer;
	while (mDims.size.y--)
	{
		for (c = 0; c < mNumComponents; c++)
		{
			mEngines[c].pull(mLines[c],true);
		}
		if ((mNumComponents >= 3) && mUseYCC)
		{
			kdu_convert_ycc_to_rgb(mLines[0],mLines[1],mLines[2]);
		}
		for (c = 0; c < mNumComponents; c++)
		{
			transfer_bytes(mBuf+c,mLines[c],mNumComponents,mBitDepths[c]);
		}
		mBuf += mRowGap;
		if (mDims.size.y % 10)
		{
			if (limit_time && decode_timer.getElapsedTimeF32() > decode_time)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

// kdc_flow_control 

kdc_flow_control::kdc_flow_control (kdu_image_in_base *img_in, kdu_codestream codestream)
{
	int n;
	
	this->codestream = codestream;
	codestream.get_valid_tiles(valid_tile_indices);
	tile_idx = valid_tile_indices.pos;
	tile = codestream.open_tile(tile_idx,NULL);
	
	// Set up the individual components
	num_components = codestream.get_num_components(true);
	components = new kdc_component_flow_control[num_components];
	count_delta = 0;
	kdc_component_flow_control *comp = components;
	for (n = 0; n < num_components; n++, comp++)
	{
		comp->line = NULL;
		comp->reader = img_in;
		kdu_coords subsampling;  
		codestream.get_subsampling(n,subsampling,true);
		kdu_dims dims;  
		codestream.get_tile_dims(tile_idx,n,dims,true);
		comp->vert_subsampling = subsampling.y;
		if ((n == 0) || (comp->vert_subsampling < count_delta))
		{
			count_delta = comp->vert_subsampling;
		}
		comp->ratio_counter = 0;
		comp->remaining_lines = comp->initial_lines = dims.size.y;
	}
	assert(num_components >= 0);
	
	tile.set_components_of_interest(num_components);
	max_buffer_memory = engine.create(codestream,tile,false,NULL,false,1,NULL,NULL,false);
}

kdc_flow_control::~kdc_flow_control()
{
	if (components != NULL)
	{
		delete[] components;
	}
	if (engine.exists())
	{
		engine.destroy();
	}
}

bool kdc_flow_control::advance_components()
{
	bool found_line = false;
	while (!found_line)
	{
		bool all_done = true;
		kdc_component_flow_control *comp = components;
		for (int n = 0; n < num_components; n++, comp++)
		{
			assert(comp->ratio_counter >= 0);
			if (comp->remaining_lines > 0)
			{
				all_done = false;
				comp->ratio_counter -= count_delta;
				if (comp->ratio_counter < 0)
				{
					found_line = true;
					comp->line = engine.exchange_line(n,NULL,NULL);
					assert(comp->line != NULL);
					if (comp->line->get_width())
					{
						comp->reader->get(n,*(comp->line),0);
					}
				}
			}
		}
		if (all_done)
		{
			return false;
		}
	}
	return true;
}

void kdc_flow_control::process_components()
{
	kdc_component_flow_control *comp = components;
	for (int n = 0; n < num_components; n++, comp++)
	{
		if (comp->ratio_counter < 0)
		{
			comp->ratio_counter += comp->vert_subsampling;
			assert(comp->ratio_counter >= 0);
			assert(comp->remaining_lines > 0);
			comp->remaining_lines--;
			assert(comp->line != NULL);
			engine.exchange_line(n,comp->line,NULL);
			comp->line = NULL;
		}
	}
}
