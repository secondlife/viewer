/**
 * @file llimagej2c.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llapr.h"
#include "lldir.h"
#include "llimagej2c.h"
#include "lltimer.h"
#include "llmath.h"
#include "llmemory.h"
#include "llsd.h"

// Declare the prototype for this factory function here. It is implemented in
// other files which define a LLImageJ2CImpl subclass, but only ONE static
// library which has the implementation for this function should ever be
// linked.
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl();

//static
std::string LLImageJ2C::getEngineInfo()
{
    // All known LLImageJ2CImpl implementation subclasses are cheap to
    // construct.
    std::unique_ptr<LLImageJ2CImpl> impl(fallbackCreateLLImageJ2CImpl());
    return impl->getEngineInfo();
}

LLImageJ2C::LLImageJ2C() :  LLImageFormatted(IMG_CODEC_J2C),
                            mMaxBytes(0),
                            mRawDiscardLevel(-1),
                            mRate(DEFAULT_COMPRESSION_RATE),
                            mReversible(false),
                            mAreaUsedForDataSizeCalcs(0)
{
    mImpl.reset(fallbackCreateLLImageJ2CImpl());

    // Clear data size table
    for( S32 i = 0; i <= MAX_DISCARD_LEVEL; i++)
    {   // Array size is MAX_DISCARD_LEVEL+1
        mDataSizes[i] = 0;
    }
}

// virtual
LLImageJ2C::~LLImageJ2C() {}

// virtual
void LLImageJ2C::resetLastError()
{
    mLastError.clear();
}

//virtual
void LLImageJ2C::setLastError(const std::string& message, const std::string& filename)
{
    mLastError = message;
    if (!filename.empty())
        mLastError += std::string(" FILE: ") + filename;
}

// virtual
S8  LLImageJ2C::getRawDiscardLevel()
{
    return mRawDiscardLevel;
}

bool LLImageJ2C::updateData()
{
    bool res = true;
    resetLastError();

    LLImageDataLock lock(this);

    // Check to make sure that this instance has been initialized with data
    if (!getData() || (getDataSize() < 16))
    {
        setLastError("LLImageJ2C uninitialized");
        res = false;
    }
    else
    {
        res = mImpl->getMetadata(*this);
    }

    if (res)
    {
        // SJB: override discard based on mMaxBytes elsewhere
        S32 max_bytes = getDataSize(); // mMaxBytes ? mMaxBytes : getDataSize();
        S32 discard = calcDiscardLevelBytes(max_bytes);
        setDiscardLevel(discard);
    }

    if (!mLastError.empty())
    {
        LLImage::setLastError(mLastError);
    }
    return res;
}

bool LLImageJ2C::initDecode(LLImageRaw &raw_image, int discard_level, int* region)
{
    setDiscardLevel(discard_level != -1 ? discard_level : 0);
    return mImpl->initDecode(*this,raw_image,discard_level,region);
}

bool LLImageJ2C::initEncode(LLImageRaw &raw_image, int blocks_size, int precincts_size, int levels)
{
    return mImpl->initEncode(*this,raw_image,blocks_size,precincts_size,levels);
}

bool LLImageJ2C::decode(LLImageRaw *raw_imagep, F32 decode_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    return decodeChannels(raw_imagep, decode_time, 0, 4);
}


// Returns true to mean done, whether successful or not.
bool LLImageJ2C::decodeChannels(LLImageRaw *raw_imagep, F32 decode_time, S32 first_channel, S32 max_channel_count )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLTimer elapsed;

    resetLastError();

    bool res;
    {
        LLImageDataLock lock(this);

        mDecoding = true;
        // Check to make sure that this instance has been initialized with data
        if (!getData() || (getDataSize() < 16))
        {
            setLastError("LLImageJ2C uninitialized");
            res = true; // done
        }
        else
        {
            // Update the raw discard level
            updateRawDiscardLevel();
            res = mImpl->decodeImpl(*this, *raw_imagep, decode_time, first_channel, max_channel_count);
        }
    }

    if (res)
    {
        if (!mDecoding)
        {
            // Failed
            raw_imagep->deleteData();
            res = false;
        }
        else
        {
            mDecoding = false;
        }
    }
    else
    {
        if (mDecoding)
        {
            LL_WARNS() << "decodeImpl failed but mDecoding is true" << LL_ENDL;
            mDecoding = false;
        }
    }

    if (!mLastError.empty())
    {
        LLImage::setLastError(mLastError);
    }

    return res;
}


bool LLImageJ2C::encode(const LLImageRaw *raw_imagep, F32 encode_time)
{
    return encode(raw_imagep, NULL, encode_time);
}


bool LLImageJ2C::encode(const LLImageRaw *raw_imagep, const char* comment_text, F32 encode_time)
{
    LLTimer elapsed;
    resetLastError();
    bool res = mImpl->encodeImpl(*this, *raw_imagep, comment_text, encode_time, mReversible);
    if (!mLastError.empty())
    {
        LLImage::setLastError(mLastError);
    }

    return res;
}

//static
S32 LLImageJ2C::calcHeaderSizeJ2C()
{
    return FIRST_PACKET_SIZE; // Hack. just needs to be >= actual header size...
}

//static
S32 LLImageJ2C::calcDataSizeJ2C(S32 w, S32 h, S32 comp, S32 discard_level, F32 rate)
{
    // Note: This provides an estimation for the first to last quality layer of a given discard level
    // This is however an efficient approximation, as the true discard level boundary would be
    // in general too big for fast fetching.
    // For details about the equation used here, see https://wiki.lindenlab.com/wiki/THX1138_KDU_Improvements#Byte_Range_Study

    // Estimate the number of layers. This is consistent with what's done for j2c encoding in LLImageJ2CKDU::encodeImpl().
    constexpr S32 precision = 8; // assumed bitrate per component channel, might change in future for HDR support
    constexpr S32 max_components = 4; // assumed the file has four components; three color and alpha
    // Use MAX_IMAGE_SIZE_DEFAULT (currently 2048) if either dimension is unknown (zero)
    S32 width  = (w > 0) ? w : 2048;
    S32 height = (h > 0) ? h : 2048;
    S32 max_dimension = llmax(width, height); // Find largest dimension
    S32 block_area = MAX_BLOCK_SIZE * MAX_BLOCK_SIZE; // Calculated initial block area from established max block size (currently 64)
    S32 max_layers = (S32)llmax(llround(log2f((float)max_dimension) - log2f((float)MAX_BLOCK_SIZE)), 4); // Find number of powers of two between extents and block size to a minimum of 4
    block_area *= llmax(max_layers, 1); // Adjust initial block area by max number of layers
    S32 totalbytes = (S32) (MIN_LAYER_SIZE * max_components * precision); // Start estimation with a minimum reasonable size
    S32 block_layers = 0;
    while (block_layers <= max_layers) // Walk the layers
    {
        if (block_layers <= (5 - discard_level))  // Walk backwards from discard 5 to required discard layer.
            totalbytes += (S32) (block_area * max_components * precision * rate); // Add each block layer reduced by assumed compression rate
        block_layers++; // Move to next layer
        block_area *= 4; // Increase block area by power of four
    }

    totalbytes /= 8; // to bytes
    totalbytes += calcHeaderSizeJ2C();  // header

    return totalbytes;
}

S32 LLImageJ2C::calcHeaderSize()
{
    return calcHeaderSizeJ2C();
}

// calcDataSize() returns how many bytes to read to load discard_level (including header)
S32 LLImageJ2C::calcDataSize(S32 discard_level)
{
    discard_level = llclamp(discard_level, 0, MAX_DISCARD_LEVEL);
    if ( mAreaUsedForDataSizeCalcs != (getHeight() * getWidth())
        || (mDataSizes[0] == 0))
    {
        mAreaUsedForDataSizeCalcs = getHeight() * getWidth();

        S32 level = MAX_DISCARD_LEVEL;  // Start at the highest discard
        while ( level >= 0 )
        {
            mDataSizes[level] = calcDataSizeJ2C(getWidth(), getHeight(), getComponents(), level, mRate);
            level--;
        }
    }
    return mDataSizes[discard_level];
}

S32 LLImageJ2C::calcDiscardLevelBytes(S32 bytes)
{
    llassert(bytes >= 0);
    S32 discard_level = 0;
    if (bytes == 0)
    {
        return MAX_DISCARD_LEVEL;
    }
    while (1)
    {
        S32 bytes_needed = calcDataSize(discard_level);
        // Use TextureReverseByteRange percent (see settings.xml) of the optimal size to qualify as correct rendering for the given discard level
        if (bytes >= (bytes_needed*LLImage::getReverseByteRangePercent()/100))
        {
            break;
        }
        discard_level++;
        if (discard_level >= MAX_DISCARD_LEVEL)
        {
            break;
        }
    }
    return discard_level;
}

void LLImageJ2C::setMaxBytes(S32 max_bytes)
{
    mMaxBytes = max_bytes;
}

void LLImageJ2C::setReversible(const bool reversible)
{
    mReversible = reversible;
}


bool LLImageJ2C::loadAndValidate(const std::string &filename)
{
    bool res = true;

    resetLastError();

    S32 file_size = 0;
    LLAPRFile infile ;
    infile.open(filename, LL_APR_RB, NULL, &file_size);
    apr_file_t* apr_file = infile.getFileHandle() ;
    if (!apr_file)
    {
        setLastError("Unable to open file for reading", filename);
        res = false;
    }
    else if (file_size == 0)
    {
        setLastError("File is empty",filename);
        res = false;
    }
    else
    {
        U8 *data = (U8*)ll_aligned_malloc_16(file_size);
        if (!data)
        {
            infile.close();
            setLastError("Out of memory", filename);
            res = false;
        }
        else
        {
            apr_size_t bytes_read = file_size;
            apr_status_t s = apr_file_read(apr_file, data, &bytes_read); // modifies bytes_read
            infile.close();

            if (s != APR_SUCCESS || (S32)bytes_read != file_size)
            {
                ll_aligned_free_16(data);
                setLastError("Unable to read entire file");
                res = false;
            }
            else
            {
                res = validate(data, file_size);
            }
        }
    }

    if (!mLastError.empty())
    {
        LLImage::setLastError(mLastError);
    }

    return res;
}


bool LLImageJ2C::validate(U8 *data, U32 file_size)
{
    resetLastError();

    LLImageDataLock lock(this);

    setData(data, file_size);

    bool res = updateData();
    if ( res )
    {
        // Check to make sure that this instance has been initialized with data
        if (!getData() || (0 == getDataSize()))
        {
            setLastError("LLImageJ2C uninitialized");
            res = false;
        }
        else
        {
            res = mImpl->getMetadata(*this);
        }
    }

    if (!mLastError.empty())
    {
        LLImage::setLastError(mLastError);
    }
    return res;
}

void LLImageJ2C::decodeFailed()
{
    mDecoding = false;
}

void LLImageJ2C::updateRawDiscardLevel()
{
    mRawDiscardLevel = mMaxBytes ? calcDiscardLevelBytes(mMaxBytes) : mDiscardLevel;
}

LLImageJ2CImpl::~LLImageJ2CImpl()
{
}
