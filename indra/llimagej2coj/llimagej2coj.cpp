/** 
 * @file llimagej2coj.cpp
 * @brief This is an implementation of JPEG2000 encode/decode using OpenJPEG.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#include "llimagej2coj.h"

// this is defined so that we get static linking.
#include "openjpeg.h"
#include "event.h"
#include "cio.h"

#include "lltimer.h"
#include "llmemory.h"

#define MAX_ENCODED_DISCARD_LEVELS 5

#pragma optimize("", off)

// Return string from message, eliminating final \n if present
static std::string chomp(const char* msg)
{
	// stomp trailing \n
	std::string message = msg;
	if (!message.empty())
	{
		size_t last = message.size() - 1;
		if (message[last] == '\n')
		{
			message.resize( last );
		}
	}
	return message;
}

/**
sample error callback expecting a LLFILE* client object
*/
void error_callback(const char* msg, void*)
{
	LL_DEBUGS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
}
/**
sample warning callback expecting a LLFILE* client object
*/
void warning_callback(const char* msg, void*)
{
	LL_DEBUGS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
}
/**
sample debug callback expecting no client object
*/
void info_callback(const char* msg, void*)
{
	LL_DEBUGS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
}

// Divide a by 2 to the power of b and round upwards
int ceildivpow2(int a, int b)
{
	return (a + (1 << b) - 1) >> b;
}

class JPEG2KDecode
{
public:

    JPEG2KDecode(S8 discardLevel)
    {
        memset(&event_mgr,  0, sizeof(opj_event_mgr_t));
        memset(&parameters, 0, sizeof(opj_dparameters_t));
        event_mgr.error_handler = error_callback;
	    event_mgr.warning_handler = warning_callback;
	    event_mgr.info_handler = info_callback;
        opj_set_default_decoder_parameters(&parameters);
	    parameters.cp_reduce = discardLevel;
    }

    ~JPEG2KDecode()
    {
        if(decoder)
	    {
		    opj_destroy_codec(decoder);
	    }
        decoder = nullptr;

        if (image)
        {
            opj_image_destroy(image);
        }
        image = nullptr;

        if (stream)
        {
            delete stream;
        }
        stream = nullptr;

        if (codestream_info)
        {
            opj_destroy_cstr_info(&codestream_info);
        }
        codestream_info = nullptr;
    }

     bool readHeader(
            U8* data,
            U32 dataSize,
            S32& widthOut,
            S32& heightOut,
            S32& components,
            S32& discard_level)
    {
        parameters.flags |= OPJ_DPARAMETERS_DUMP_FLAG;

        decoder = opj_create_decompress(OPJ_CODEC_J2K);

	    if (!opj_setup_decoder(decoder, &parameters))
        {
            return false;
        }

        if (stream)
        {
            delete stream;
        }
        stream = new opj_stream_private_t;

        if (!stream)
        {
            return false;
        }

        stream->m_buffer_size = dataSize;
        stream->m_stored_data = (OPJ_BYTE *)data;
        stream->m_current_data = stream->m_stored_data;

        /* Read the main header of the codestream and if necessary the JP2 boxes*/
        if (!opj_read_header((opj_stream_t*)stream, decoder, &image))
        {
            return false;
        }

        codestream_info = opj_get_cstr_info(decoder);

        if (!codestream_info)
        {
            return false;
        }

        U32 tileDimX = codestream_info->tdx;
        U32 tileDimY = codestream_info->tdy;
        U32 tilesW   = codestream_info->tw;
        U32 tilesH   = codestream_info->th;

        widthOut   = S32(tilesW * tileDimX);
        heightOut  = S32(tilesH * tileDimY);
        components = codestream_info->nbcomps;

		discard_level = 0;
		while (tilesW > 1 && tilesH > 1 && discard_level < MAX_DISCARD_LEVEL)
		{
			discard_level++;
			tilesW >>= 1;
			tilesH >>= 1;
		}

        return true;
    }

    bool decode(U8* data, U32 dataSize, U32* channels)
    {
        parameters.flags &= ~OPJ_DPARAMETERS_DUMP_FLAG;

        decoder = opj_create_decompress(OPJ_CODEC_J2K);
	    opj_setup_decoder(decoder, &parameters);

        if (stream)
        {
            delete stream;
        }
        stream = new opj_stream_private_t;

        if (!stream)
        {
            return false;
        }

        stream->m_buffer_size = dataSize;
        stream->m_stored_data = (OPJ_BYTE *)data;
        stream->m_current_data = stream->m_stored_data;

        if (image)
        {
            opj_image_destroy(image);
            image = nullptr;
        }

        if (!opj_read_header((opj_stream_t*)stream, decoder, &image))
        {
            return false;
        }

    	OPJ_BOOL decoded = opj_decode(decoder, (opj_stream_t*)stream, image);

        // count was zero.  The latter is just a sanity check before we
	    // dereference the array.
	    if(!decoded || !image || !image->numcomps)
	    {
            opj_end_decompress(decoder, (opj_stream_t*)stream);
		    return false;
	    }

        if (channels)
        {
            *channels = image->numcomps;
        }

        opj_end_decompress(decoder, (opj_stream_t*)stream);

        return true;
    }

    opj_image_t* getImage() { return image; }

private:
    opj_dparameters_t         parameters;
	opj_event_mgr_t           event_mgr;
	opj_image_t*              image           = nullptr;
	opj_codec_t*              decoder         = nullptr;
	opj_stream_private_t*     stream          = nullptr;
    opj_codestream_info_v2_t* codestream_info = nullptr;
};


class JPEG2KEncode
{
public:
    const OPJ_UINT32 TILE_SIZE = 64 * 64 * 3;

    JPEG2KEncode(const char* comment_text_in, bool reversible)
    {
        memset(&parameters, 0, sizeof(opj_cparameters_t));
        memset(&event_mgr,  0, sizeof(opj_event_mgr_t));
        event_mgr.error_handler   = error_callback;
	    event_mgr.warning_handler = warning_callback;
	    event_mgr.info_handler    = info_callback;

        opj_set_default_encoder_parameters(&parameters);
        parameters.cod_format = 0;
	    parameters.cp_disto_alloc = 1;

	    if (reversible)
	    {
		    parameters.tcp_numlayers = 1;
		    parameters.tcp_rates[0]  = 1.0f;
	    }
	    else
	    {
		    parameters.tcp_numlayers = 5;
            parameters.tcp_rates[0]  = 1920.0f;
            parameters.tcp_rates[1]  = 480.0f;
            parameters.tcp_rates[2]  = 120.0f;
            parameters.tcp_rates[3]  = 30.0f;
		    parameters.tcp_rates[4]  = 1.0f;
		    parameters.irreversible  = 1;
			parameters.tcp_mct       = 1;
	    }

        parameters.cp_tx0 = 0;
        parameters.cp_ty0 = 0;
        parameters.tile_size_on = OPJ_TRUE;
        parameters.cp_tdx = 64;
        parameters.cp_tdy = 64;

        if (comment_text)
        {
            free(comment_text);
        }
        comment_text = comment_text_in ? strdup(comment_text_in) : "";

        parameters.cp_comment = comment_text;
        llassert(parameters.cp_comment);
        
        tile_storage = (OPJ_BYTE*)malloc(TILE_SIZE);
    }

    ~JPEG2KEncode()
    {
        if(encoder)
	    {
		    opj_destroy_codec(encoder);
	    }
        encoder = nullptr;

        if (image)
        {
            opj_image_destroy(image);
        }
        image = nullptr;

        if (stream)
        {
            delete stream;
        }
        stream = nullptr;

        if (stream_storage)
        {
            free(stream_storage);
        }
        stream_storage = nullptr;

        if (tile_storage)
        {
            free(tile_storage);
        }
        tile_storage = nullptr;

        if (comment_text)
        {
            free(comment_text);
        }
        comment_text = nullptr;
    }

    bool encode(const LLImageRaw& rawImageIn, LLImageJ2C &compressedImageOut)
    {
        setImage(rawImageIn);

        encoder = opj_create_compress(OPJ_CODEC_J2K);
	    opj_setup_encoder(encoder, &parameters, image);

        if (stream)
        {
            delete stream;
        }

        stream = new opj_stream_private_t;

        if (!stream)
        {
            return false;
        }


        U32 data_size_guess = TILE_SIZE * 2;

        if (stream_storage)
        {
            free(stream_storage);
        }

        stream_storage = (OPJ_BYTE*)malloc(data_size_guess);

        stream->m_stored_data = stream_storage;
        stream->m_buffer_size = data_size_guess;

        OPJ_BOOL started = opj_start_compress(encoder, image, (opj_stream_t*)stream);

        if (!started)
        {
            return false;
        }

        if (!tile_storage)
        {
            return false; // OOM
        }

        U32 tile_count = (rawImageIn.getWidth() >> 6) * (rawImageIn.getHeight() >> 6);
        for (U32 i = 0; i < tile_count; ++i)
        {
            if (!opj_write_tile(encoder, i, tile_storage, TILE_SIZE, (opj_stream_t*)stream))
            {
                return false;
            }
        
    	    // copy compressed image
	        compressedImageOut.appendData(tile_storage, TILE_SIZE);	        
        }

        compressedImageOut.updateData(); // set width, height

        OPJ_BOOL encoded = opj_end_compress(encoder, (opj_stream_t*)stream);

        return encoded;
    }

    void setImage(const LLImageRaw& raw)
    {
	    opj_image_cmptparm_t cmptparm[MAX_ENCODED_DISCARD_LEVELS];
        memset(&cmptparm[0], 0, MAX_ENCODED_DISCARD_LEVELS * sizeof(opj_image_cmptparm_t));

	    S32 numcomps = raw.getComponents();
	    S32 width    = raw.getWidth();
	    S32 height   = raw.getHeight();

	    for(S32 c = 0; c < numcomps; c++)
        {
		    cmptparm[c].prec = 8;
		    cmptparm[c].bpp  = 8;
		    cmptparm[c].sgnd = 0;
		    cmptparm[c].dx   = parameters.subsampling_dx;
		    cmptparm[c].dy   = parameters.subsampling_dy;
		    cmptparm[c].w    = width;
		    cmptparm[c].h    = height;
	    }

	    image = opj_image_create(numcomps, &cmptparm[0], OPJ_CLRSPC_SRGB);

	    image->x1 = width;
	    image->y1 = height;

        const U8 *src = raw.getData();

        // De-interleave to component plane data

        switch (numcomps)
        {
            case 0:
            default:
                break;

            case 1:
            {
                U32 rBitDepth     = image->comps[0].bpp;
                U32 bytesPerPixel = rBitDepth >> 3;
                memcpy(image->comps[0].data, src, width * height * bytesPerPixel);                
            }
            break;

            case 2:
            {
                U32 rBitDepth     = image->comps[0].bpp;
                U32 gBitDepth     = image->comps[1].bpp;
                U32 totalBitDepth = rBitDepth + gBitDepth;
                U32 bytesPerPixel = totalBitDepth >> 3;
                U32 stride        = width * bytesPerPixel;
                U32 offset        = 0;
                for (S32 y = height - 1; y >= 0; y--)
	            {
                    const U8* component = src + (y * stride);
                    for (S32 x = 0; x < width; x++)
	                {
                        image->comps[0].data[offset] = *component++;
                        image->comps[1].data[offset] = *component++;
                        offset++;
                    }
                }
            }
            break;

            case 3:
            {
                U32 rBitDepth     = image->comps[0].bpp;
                U32 gBitDepth     = image->comps[1].bpp;
                U32 bBitDepth     = image->comps[2].bpp;
                U32 totalBitDepth = rBitDepth + gBitDepth + bBitDepth;
                U32 bytesPerPixel = totalBitDepth >> 3;
                U32 stride        = width * bytesPerPixel;
                U32 offset        = 0;
                for (S32 y = height - 1; y >= 0; y--)
	            {
                    const U8* component = src + (y * stride);
                    for (S32 x = 0; x < width; x++)
	                {
                        image->comps[0].data[offset] = *component++;
                        image->comps[1].data[offset] = *component++;
                        image->comps[2].data[offset] = *component++;
                        offset++;
                    }
                }
            }
            break;


            case 4:
            {
                U32 rBitDepth = image->comps[0].bpp;
                U32 gBitDepth = image->comps[1].bpp;
                U32 bBitDepth = image->comps[2].bpp;
                U32 aBitDepth = image->comps[3].bpp;

                U32 totalBitDepth = rBitDepth + gBitDepth + bBitDepth + aBitDepth;
                U32 bytesPerPixel = totalBitDepth >> 3;

                U32 stride        = width * bytesPerPixel;
                U32 offset        = 0;
                for (S32 y = height - 1; y >= 0; y--)
	            {
                    const U8* component = src + (y * stride);
                    for (S32 x = 0; x < width; x++)
	                {
                        image->comps[0].data[offset] = *component++;
                        image->comps[1].data[offset] = *component++;
                        image->comps[2].data[offset] = *component++;
                        image->comps[3].data[offset] = *component++;
                        offset++;
                    }
                }
            }
            break;
        }
    }

    opj_image_t* getImage() { return image; }

private:
    opj_cparameters_t       parameters;
	opj_event_mgr_t         event_mgr;
	opj_image_t*            image           = nullptr;
	opj_codec_t*            encoder         = nullptr;
	opj_stream_private_t*   stream          = nullptr;
    char*                   comment_text    = nullptr;
    OPJ_BYTE*               stream_storage  = nullptr;
    OPJ_BYTE*               tile_storage    = nullptr;
};

// Factory function: see declaration in llimagej2c.cpp
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl()
{
	return new LLImageJ2COJ();
}

std::string LLImageJ2COJ::getEngineInfo() const
{
#ifdef OPJ_PACKAGE_VERSION
	return std::string("OpenJPEG " OPJ_PACKAGE_VERSION ", runtime: ") + opj_version();
#else
	return std::string("OpenJPEG runtime: ") + opj_version();
#endif
}

LLImageJ2COJ::LLImageJ2COJ()
	: LLImageJ2CImpl()
{
}


LLImageJ2COJ::~LLImageJ2COJ()
{
}

bool LLImageJ2COJ::initDecode(LLImageJ2C &base, LLImageRaw &raw_image, int discard_level, int* region)
{
	base.mDiscardLevel = discard_level;
	return true;
}

bool LLImageJ2COJ::initEncode(LLImageJ2C &base, LLImageRaw &raw_image, int blocks_size, int precincts_size, int levels)
{
	// No specific implementation for this method in the OpenJpeg case
	return false;
}

bool LLImageJ2COJ::decodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count)
{
    JPEG2KDecode decoder(0);

    U32 channels = 0;
	bool decoded = decoder.decode(base.getData(), base.getDataSize(), &channels);
    if(!decoded)
	{
		LL_DEBUGS("Texture") << "ERROR -> decodeImpl: failed to decode image!" << LL_ENDL;
		return true; // done
	}

    opj_image_t *image = decoder.getImage();

	// Component buffers are allocated in an image width by height buffer.
	// The image placed in that buffer is ceil(width/2^factor) by
	// ceil(height/2^factor) and if the factor isn't zero it will be at the
	// top left of the buffer with black filled in the rest of the pixels.
	// It is integer math so the formula is written in ceildivpo2.
	// (Assuming all the components have the same width, height and
	// factor.)
	U32 comp_width = image->comps[0].w; // leave this unshifted by 'f' discard factor, the strides are always for the full buffer width
	U32 f          = image->comps[0].factor;

    // do size the texture to the mem we'll acrually use...
	U32 width      = image->comps[0].w >> f;
	U32 height     = image->comps[0].h >> f;

	raw_image.resize(U16(width), U16(height), S8(channels));

	U8 *rawp = raw_image.getData();

	// first_channel is what channel to start copying from
	// dest is what channel to copy to.  first_channel comes from the
	// argument, dest always starts writing at channel zero.
	for (S32 comp = first_channel, dest=0; comp < first_channel + channels; comp++, dest++)
	{
        llassert(image->comps[comp].data);
		if (image->comps[comp].data)
		{
			S32 offset = dest;
			for (S32 y = (height - 1); y >= 0; y--)
			{
				for (S32 x = 0; x < width; x++)
				{
					rawp[offset] = image->comps[comp].data[y*comp_width + x];
					offset += channels;
				}
			}
		}
		else // Some rare OpenJPEG versions have this bug.
		{
			LL_DEBUGS("Texture") << "ERROR -> decodeImpl: failed! (OpenJPEG bug)" << LL_ENDL;
		}
	}

    llassert(f == 0);

    base.setDiscardLevel(f);
    base.updateRawDiscardLevel();

	return true; // done
}


bool LLImageJ2COJ::encodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text, F32 encode_time, bool reversible)
{
    JPEG2KEncode encode(comment_text, reversible);
    return encode.encode(raw_image, base);
}

bool LLImageJ2COJ::getMetadata(LLImageJ2C &base)
{
	JPEG2KDecode decode(0);

    S32 width      = 0;
	S32 height     = 0;
	S32 components = 0;
    S32 discard_level = 0;

    U32 dataSize = base.getDataSize();
    U8* data     = base.getData();
    bool header_read = decode.readHeader(data, dataSize, width, height, components, discard_level);
    if (!header_read)
    {
        return false;
    }

	base.mDiscardLevel = discard_level;
    base.setFullWidth(width);
    base.setFullHeight(height);
    base.setSize(width >> discard_level, height >> discard_level, components);
	return true;
}

#pragma optimize("", on)