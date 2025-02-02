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

#define MAX_ENCODED_DISCARD_LEVELS 5

// Factory function: see declaration in llimagej2c.cpp
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl()
{
    return new LLImageJ2COJ();
}

std::string LLImageJ2COJ::getEngineInfo() const
{
#ifdef OPENJPEG_VERSION
    return std::string("OpenJPEG: " OPENJPEG_VERSION ", Runtime: ")
        + opj_version();
#else
    return std::string("OpenJPEG runtime: ") + opj_version();
#endif
}

#if WANT_VERBOSE_OPJ_SPAM
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
            message.resize(last);
        }
}
    return message;
}
#endif

/**
sample error callback expecting a LLFILE* client object
*/
void error_callback(const char* msg, void*)
{
#if WANT_VERBOSE_OPJ_SPAM
    LL_WARNS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
#endif
}
/**
sample warning callback expecting a LLFILE* client object
*/
void warning_callback(const char* msg, void*)
{
#if WANT_VERBOSE_OPJ_SPAM
    LL_WARNS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
#endif
}
/**
sample debug callback expecting no client object
*/
void info_callback(const char* msg, void*)
{
#if WANT_VERBOSE_OPJ_SPAM
    LL_INFOS() << "LLImageJ2COJ: " << chomp(msg) << LL_ENDL;
#endif
}

// Divide a by 2 to the power of b and round upwards
int ceildivpow2(int a, int b)
{
    return (a + (1 << b) - 1) >> b;
}

class JPEG2KBase
{
public:
    JPEG2KBase() = default;

    U8*        buffer = nullptr;
    OPJ_SIZE_T size = 0;
    OPJ_OFF_T  offset = 0;
};

static OPJ_SIZE_T opj_read(void * buffer, OPJ_SIZE_T bytes, void* user_data)
{
    llassert(user_data);
    JPEG2KBase* jpeg_codec = static_cast<JPEG2KBase*>(user_data);
    OPJ_SIZE_T remainder = (jpeg_codec->size - jpeg_codec->offset);
    if (remainder <= 0)
    {
        jpeg_codec->offset = jpeg_codec->size;
        // Indicate end of stream (hacky?)
        return (OPJ_OFF_T)-1;
    }
    OPJ_SIZE_T to_read = llclamp(U32(bytes), U32(0), U32(remainder));
    memcpy(buffer, jpeg_codec->buffer + jpeg_codec->offset, to_read);
    jpeg_codec->offset += to_read;
    return to_read;
}

static OPJ_SIZE_T opj_write(void * buffer, OPJ_SIZE_T bytes, void* user_data)
{
    llassert(user_data);
    JPEG2KBase* jpeg_codec = static_cast<JPEG2KBase*>(user_data);
    OPJ_SIZE_T remainder = jpeg_codec->size - jpeg_codec->offset;
    if (remainder < bytes)
    {
        OPJ_SIZE_T new_size = jpeg_codec->size + (bytes - remainder);
        U8* new_buffer = (U8*)ll_aligned_malloc_16(new_size);
        memcpy(new_buffer, jpeg_codec->buffer, jpeg_codec->offset);
        U8* old_buffer = jpeg_codec->buffer;
        jpeg_codec->buffer = new_buffer;
        ll_aligned_free_16(old_buffer);
        jpeg_codec->size = new_size;
    }
    memcpy(jpeg_codec->buffer + jpeg_codec->offset, buffer, bytes);
    jpeg_codec->offset += bytes;
    return bytes;
}

static OPJ_OFF_T opj_skip(OPJ_OFF_T bytes, void* user_data)
{
    JPEG2KBase* jpeg_codec = static_cast<JPEG2KBase*>(user_data);
    jpeg_codec->offset += bytes;

    if (jpeg_codec->offset > (OPJ_OFF_T)jpeg_codec->size)
    {
        jpeg_codec->offset = jpeg_codec->size;
        // Indicate end of stream
        return (OPJ_OFF_T)-1;
    }

    if (jpeg_codec->offset < 0)
    {
        // Shouldn't be possible?
        jpeg_codec->offset = 0;
        return (OPJ_OFF_T)-1;
    }

    return bytes;
}

static OPJ_BOOL opj_seek(OPJ_OFF_T bytes, void * user_data)
{
    JPEG2KBase* jpeg_codec = static_cast<JPEG2KBase*>(user_data);
    jpeg_codec->offset = bytes;
    jpeg_codec->offset = llclamp(U32(jpeg_codec->offset), U32(0), U32(jpeg_codec->size));
    return OPJ_TRUE;
}

static void opj_free_user_data(void * user_data)
{
    JPEG2KBase* jpeg_codec = static_cast<JPEG2KBase*>(user_data);
    // Don't free, data is managed externally
    jpeg_codec->buffer = nullptr;
    jpeg_codec->size = 0;
    jpeg_codec->offset = 0;
}

static void opj_free_user_data_write(void * user_data)
{
    JPEG2KBase* jpeg_codec = static_cast<JPEG2KBase*>(user_data);
    // Free, data was allocated here
    ll_aligned_free_16(jpeg_codec->buffer);
    jpeg_codec->buffer = nullptr;
    jpeg_codec->size = 0;
    jpeg_codec->offset = 0;
}

class JPEG2KDecode : public JPEG2KBase
{
public:

    JPEG2KDecode(S8 discardLevel)
    {
        memset(&parameters, 0, sizeof(opj_dparameters_t));
        opj_set_default_decoder_parameters(&parameters);
        parameters.cp_reduce = discardLevel;
    }

    ~JPEG2KDecode()
    {
        if (decoder)
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
            opj_stream_destroy(stream);
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

        /* catch events using our callbacks and give a local context */
        opj_set_error_handler(decoder, error_callback, nullptr);
        opj_set_warning_handler(decoder, warning_callback, nullptr);
        opj_set_info_handler(decoder, info_callback, nullptr);

        if (!opj_setup_decoder(decoder, &parameters))
        {
            return false;
        }

        if (stream)
        {
            opj_stream_destroy(stream);
        }

        stream = opj_stream_create(dataSize, true);
        if (!stream)
        {
            return false;
        }

        opj_stream_set_user_data(stream, this, opj_free_user_data);
        opj_stream_set_user_data_length(stream, dataSize);
        opj_stream_set_read_function(stream, opj_read);
        opj_stream_set_write_function(stream, opj_write);
        opj_stream_set_skip_function(stream, opj_skip);
        opj_stream_set_seek_function(stream, opj_seek);

        buffer = data;
        size = dataSize;
        offset = 0;

        // enable decoding partially loaded images
        opj_decoder_set_strict_mode(decoder, OPJ_FALSE);

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
        U32 tilesW = codestream_info->tw;
        U32 tilesH = codestream_info->th;

        widthOut = S32(tilesW * tileDimX);
        heightOut = S32(tilesH * tileDimY);
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

    bool decode(U8* data, U32 dataSize, U32* channels, U8 discard_level)
    {
        parameters.flags &= ~OPJ_DPARAMETERS_DUMP_FLAG;

        decoder = opj_create_decompress(OPJ_CODEC_J2K);
        opj_setup_decoder(decoder, &parameters);

        opj_set_info_handler(decoder, info_callback, this);
        opj_set_warning_handler(decoder, warning_callback, this);
        opj_set_error_handler(decoder, error_callback, this);

        if (stream)
        {
            opj_stream_destroy(stream);
        }

        stream = opj_stream_create(dataSize, true);
        if (!stream)
        {
            return false;
        }

        opj_stream_set_user_data(stream, this, opj_free_user_data);
        opj_stream_set_user_data_length(stream, dataSize);
        opj_stream_set_read_function(stream, opj_read);
        opj_stream_set_write_function(stream, opj_write);
        opj_stream_set_skip_function(stream, opj_skip);
        opj_stream_set_seek_function(stream, opj_seek);

        buffer = data;
        size = dataSize;
        offset = 0;

        if (image)
        {
            opj_image_destroy(image);
            image = nullptr;
        }

        // needs to happen before opj_read_header and opj_decode...
        opj_set_decoded_resolution_factor(decoder, discard_level);

        // enable decoding partially loaded images
        opj_decoder_set_strict_mode(decoder, OPJ_FALSE);

        if (!opj_read_header(stream, decoder, &image))
        {
            return false;
        }

        // needs to happen before decode which may fail
        if (channels)
        {
            *channels = image->numcomps;
        }

        OPJ_BOOL decoded = opj_decode(decoder, stream, image);

        // count was zero.  The latter is just a sanity check before we
        // dereference the array.
        if (!decoded || !image || !image->numcomps)
        {
            opj_end_decompress(decoder, stream);
            return false;
        }

        opj_end_decompress(decoder, stream);

        return true;
    }

    opj_image_t* getImage() { return image; }

private:
    opj_dparameters_t         parameters;
    opj_image_t*              image = nullptr;
    opj_codec_t*              decoder = nullptr;
    opj_stream_t*             stream = nullptr;
    opj_codestream_info_v2_t* codestream_info = nullptr;
};

class JPEG2KEncode : public JPEG2KBase
{
public:
    const OPJ_UINT32 TILE_SIZE = 64 * 64 * 3;

    JPEG2KEncode(const char* comment_text_in, bool reversible)
    {
        memset(&parameters, 0, sizeof(opj_cparameters_t));

        opj_set_default_encoder_parameters(&parameters);
        parameters.cod_format = OPJ_CODEC_J2K;
        parameters.cp_disto_alloc = 1;

        if (reversible)
        {
            parameters.max_cs_size = 0; // do not limit size for reversible compression
            parameters.irreversible = 0; // should be the default, but, just in case
            parameters.tcp_numlayers = 1;
            /* documentation seems to be wrong, should be 0.0f for lossless, not 1.0f
               see https://github.com/uclouvain/openjpeg/blob/39e8c50a2f9bdcf36810ee3d41bcbf1cc78968ae/src/lib/openjp2/j2k.c#L7755
            */
            parameters.tcp_rates[0] = 0.0f;
        }
        else
        {
            parameters.irreversible = 1;
        }

        if (comment_text)
        {
            free(comment_text);
        }
        comment_text = comment_text_in ? strdup(comment_text_in) : nullptr;

        parameters.cp_comment = comment_text ? comment_text : (char*)"no comment";
        llassert(parameters.cp_comment);
    }

    ~JPEG2KEncode()
    {
        if (encoder)
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
            opj_stream_destroy(stream);
        }
        stream = nullptr;

        if (comment_text)
        {
            free(comment_text);
        }
        comment_text = nullptr;
    }

    bool encode(const LLImageRaw& rawImageIn, LLImageJ2C &compressedImageOut)
    {
        LLImageDataSharedLock lockIn(&rawImageIn);
        LLImageDataLock lockOut(&compressedImageOut);

        setImage(rawImageIn);

        encoder = opj_create_compress(OPJ_CODEC_J2K);

        /* catch events using our callbacks and give a local context */
        opj_set_error_handler(encoder, error_callback, nullptr);
        opj_set_warning_handler(encoder, warning_callback, nullptr);
        opj_set_info_handler(encoder, info_callback, nullptr);

        parameters.tcp_mct = (image->numcomps >= 3) ? 1 : 0;
        parameters.cod_format = OPJ_CODEC_J2K;
        parameters.prog_order = OPJ_RLCP;
        parameters.cp_disto_alloc = 1;

        // if not lossless compression, computes tcp_numlayers and max_cs_size depending on the image dimensions
        if( parameters.irreversible ) {

            // computes a number of layers
            U32 surface = rawImageIn.getWidth() * rawImageIn.getHeight();
            U32 nb_layers = 1;
            U32 s = 64*64;
            while (surface > s)
            {
                nb_layers++;
                s *= 4;
            }
            nb_layers = llclamp(nb_layers, 1, 6);

            parameters.tcp_numlayers = nb_layers;
            parameters.tcp_rates[nb_layers - 1] = (U32)(1.f / DEFAULT_COMPRESSION_RATE); // 1:8 by default

            // for each subsequent layer, computes its rate and adds surface * numcomps * 1/rate to the max_cs_size
            U32 max_cs_size = (U32)(surface * image->numcomps * DEFAULT_COMPRESSION_RATE);
            U32 multiplier;
            for (int i = nb_layers - 2; i >= 0; i--)
            {
                if( i == nb_layers - 2 )
                {
                    multiplier = 15;
                }
                else if( i == nb_layers - 3 )
                {
                    multiplier = 4;
                }
                else
                {
                    multiplier = 2;
                }
                parameters.tcp_rates[i] = parameters.tcp_rates[i + 1] * multiplier;
                max_cs_size += (U32)(surface * image->numcomps * (1 / parameters.tcp_rates[i]));
            }

            //ensure that we have at least a minimal size
            max_cs_size = llmax(max_cs_size, (U32)FIRST_PACKET_SIZE);

            parameters.max_cs_size = max_cs_size;
        }

        if (!opj_setup_encoder(encoder, &parameters, image))
        {
            return false;
        }

        U32 tile_count = (rawImageIn.getWidth() >> 6) * (rawImageIn.getHeight() >> 6);
        U32 data_size_guess = tile_count * TILE_SIZE;

        // will be freed in opj_free_user_data_write
        buffer = (U8*)ll_aligned_malloc_16(data_size_guess);
        size = data_size_guess;
        offset = 0;

        memset(buffer, 0, data_size_guess);

        if (stream)
        {
            opj_stream_destroy(stream);
        }

        stream = opj_stream_create(data_size_guess, false);
        if (!stream)
        {
            return false;
        }

        opj_stream_set_user_data(stream, this, opj_free_user_data_write);
        opj_stream_set_user_data_length(stream, data_size_guess);
        opj_stream_set_read_function(stream, opj_read);
        opj_stream_set_write_function(stream, opj_write);
        opj_stream_set_skip_function(stream, opj_skip);
        opj_stream_set_seek_function(stream, opj_seek);

        OPJ_BOOL started = opj_start_compress(encoder, image, stream);

        if (!started)
        {
            return false;
        }

        if (!opj_encode(encoder, stream))
        {
            return false;
        }

        OPJ_BOOL encoded = opj_end_compress(encoder, stream);

        // if we successfully encoded, then stream out the compressed data...
        if (encoded)
        {
            // "append" (set) the data we "streamed" (memcopied) for writing to the formatted image
            // with side-effect of setting the actually encoded size  to same
            compressedImageOut.allocateData((S32)offset);
            memcpy(compressedImageOut.getData(), buffer, offset);
            compressedImageOut.updateData(); // update width, height etc from header
        }
        return encoded;
    }

    void setImage(const LLImageRaw& raw)
    {
        opj_image_cmptparm_t cmptparm[MAX_ENCODED_DISCARD_LEVELS];
        memset(&cmptparm[0], 0, MAX_ENCODED_DISCARD_LEVELS * sizeof(opj_image_cmptparm_t));

        S32 numcomps = raw.getComponents();
        S32 width = raw.getWidth();
        S32 height = raw.getHeight();

        for (S32 c = 0; c < numcomps; c++)
        {
            cmptparm[c].prec = 8;
            cmptparm[c].bpp = 8;
            cmptparm[c].sgnd = 0;
            cmptparm[c].dx = parameters.subsampling_dx;
            cmptparm[c].dy = parameters.subsampling_dy;
            cmptparm[c].w = width;
            cmptparm[c].h = height;
        }

        image = opj_image_create(numcomps, &cmptparm[0], OPJ_CLRSPC_SRGB);

        image->x1 = width;
        image->y1 = height;

        const U8 *src_datap = raw.getData();

        S32 i = 0;
        for (S32 y = height - 1; y >= 0; y--)
        {
            for (S32 x = 0; x < width; x++)
            {
                const U8 *pixel = src_datap + (y*width + x) * numcomps;
                for (S32 c = 0; c < numcomps; c++)
                {
                    image->comps[c].data[i] = *pixel;
                    pixel++;
                }
                i++;
            }
        }

        // This likely works, but there seems to be an issue openjpeg side
        // check over after gixing that.

        // De-interleave to component plane data
        /*
        switch (numcomps)
        {
        case 0:
        default:
            break;

        case 1:
        {
            U32 rBitDepth = image->comps[0].bpp;
            U32 bytesPerPixel = rBitDepth >> 3;
            memcpy(image->comps[0].data, src, width * height * bytesPerPixel);
        }
        break;

        case 2:
        {
            U32 rBitDepth = image->comps[0].bpp;
            U32 gBitDepth = image->comps[1].bpp;
            U32 totalBitDepth = rBitDepth + gBitDepth;
            U32 bytesPerPixel = totalBitDepth >> 3;
            U32 stride = width * bytesPerPixel;
            U32 offset = 0;
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
            U32 rBitDepth = image->comps[0].bpp;
            U32 gBitDepth = image->comps[1].bpp;
            U32 bBitDepth = image->comps[2].bpp;
            U32 totalBitDepth = rBitDepth + gBitDepth + bBitDepth;
            U32 bytesPerPixel = totalBitDepth >> 3;
            U32 stride = width * bytesPerPixel;
            U32 offset = 0;
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

            U32 stride = width * bytesPerPixel;
            U32 offset = 0;
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
        }*/
    }

    opj_image_t* getImage() { return image; }

private:
    opj_cparameters_t   parameters;
    opj_image_t*        image = nullptr;
    opj_codec_t*        encoder = nullptr;
    opj_stream_t*       stream = nullptr;
    char*               comment_text = nullptr;
};


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
    return false;
}

bool LLImageJ2COJ::initEncode(LLImageJ2C &base, LLImageRaw &raw_image, int blocks_size, int precincts_size, int levels)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // No specific implementation for this method in the OpenJpeg case
    return false;
}

bool LLImageJ2COJ::decodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count)
{
    LLImageDataLock lockIn(&base);
    LLImageDataLock lockOut(&raw_image);

    JPEG2KDecode decoder(0);

    U32 image_channels = 0;
    S32 data_size = base.getDataSize();
    S32 max_bytes = (base.getMaxBytes() ? base.getMaxBytes() : data_size);
    bool decoded = decoder.decode(base.getData(), max_bytes, &image_channels, base.mDiscardLevel);

    // set correct channel count early so failed decodes don't miss it...
    S32 channels = (S32)image_channels - first_channel;
    channels = llmin(channels, max_channel_count);

    if (!decoded)
    {
        // reset the channel count if necessary
        if (raw_image.getComponents() != channels)
        {
            raw_image.resize(raw_image.getWidth(), raw_image.getHeight(), S8(channels));
        }

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
    U32 f = image->comps[0].factor;

    // do size the texture to the mem we'll acrually use...
    U32 width = image->comps[0].w;
    U32 height = image->comps[0].h;

    raw_image.resize(U16(width), U16(height), S8(channels));

    U8 *rawp = raw_image.getData();

    // first_channel is what channel to start copying from
    // dest is what channel to copy to.  first_channel comes from the
    // argument, dest always starts writing at channel zero.
    for (S32 comp = first_channel, dest = 0; comp < first_channel + channels; comp++, dest++)
    {
        llassert(image->comps[comp].data);
        if (image->comps[comp].data)
        {
            S32 offset = dest;
            for (S32 y = (height - 1); y >= 0; y--)
            {
                for (U32 x = 0; x < width; x++)
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

    base.setDiscardLevel(f);

    return true; // done
}


bool LLImageJ2COJ::encodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text, F32 encode_time, bool reversible)
{
    JPEG2KEncode encode(comment_text, reversible);
    bool encoded = encode.encode(raw_image, base);
    if (encoded)
    {
        LL_WARNS() << "Openjpeg encoding implementation isn't complete, returning false" << LL_ENDL;
    }
    return encoded;
    //return false;
}

bool LLImageJ2COJ::getMetadata(LLImageJ2C &base)
{
    LLImageDataLock lock(&base);

    JPEG2KDecode decode(0);

    S32 width = 0;
    S32 height = 0;
    S32 components = 0;
    S32 discard_level = 0;

    U32 dataSize = base.getDataSize();
    U8* data = base.getData();
    bool header_read = decode.readHeader(data, dataSize, width, height, components, discard_level);
    if (!header_read)
    {
        return false;
    }

    base.mDiscardLevel = discard_level;
    base.setSize(width, height, components);
    return true;
}
