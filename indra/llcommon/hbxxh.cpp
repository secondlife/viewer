/**
 * @file hbxxh.cpp
 * @brief High performances vectorized hashing based on xxHash.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2023, Henri Beauchamp.
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

// This define ensures that xxHash will be compiled within this module, with
// vectorized (*) and inlined functions (with no exported API symbol); our
// xxhash "pre-built library" package actually only contains the xxhash.h
// header (no library needed at link time).
// (*) SSE2 is normally used for x86(_64) builds, unless you enabled AVX2
// in your build, in which case the latter would be used instead. For ARM64
// builds, this would also automatically enable NEON vectorization.
#define XXH_INLINE_ALL
#include "xxhash/xxhash.h"

#include "hbxxh.h"

// How many bytes to grab at a time when hashing files or streams
constexpr size_t BLOCK_LEN = 4096;

///////////////////////////////////////////////////////////////////////////////
// HBXXH64 class
///////////////////////////////////////////////////////////////////////////////

//static
U64 HBXXH64::digest(const void* buffer, size_t len)
{
    return XXH3_64bits(buffer, len);
}

//static
U64 HBXXH64::digest(const char* str)
{
    return XXH3_64bits((const void*)str, strlen(str));
}

//static
U64 HBXXH64::digest(const std::string& str)
{
    return XXH3_64bits((const void*)str.c_str(), str.size());
}

// Must be called by all constructors.
void HBXXH64::init()
{
    mDigest = 0;
    mState = (void*)XXH3_createState();
    if (!mState || XXH3_64bits_reset((XXH3_state_t*)mState) != XXH_OK)
    {
        LL_WARNS() << "Failed to initialize state !" << LL_ENDL;
    }
}

HBXXH64::~HBXXH64()
{
    if (mState)
    {
        XXH3_freeState((XXH3_state_t*)mState);
    }
}

void HBXXH64::update(const void* buffer, size_t len)
{
    if (mState)
    {
        XXH3_64bits_update((XXH3_state_t*)mState, buffer, len);
    }
    else
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
    }
}

void HBXXH64::update(const std::string& str)
{
    if (mState)
    {
        XXH3_64bits_update((XXH3_state_t*)mState, (const void*)str.c_str(),
                           str.length());
    }
    else
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
    }
}

void HBXXH64::update(std::istream& stream)
{
    if (!mState)
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
        return;
    }

    char buffer[BLOCK_LEN];
    size_t len;
    while (stream.good())
    {
        stream.read(buffer, BLOCK_LEN);
        len = stream.gcount();
        XXH3_64bits_update((XXH3_state_t*)mState, (const void*)buffer, len);
    }
}

void HBXXH64::update(FILE* file)
{
    if (!mState)
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
        return;
    }

    char buffer[BLOCK_LEN];
    size_t len;
    while ((len = fread((void*)buffer, 1, BLOCK_LEN, file)))
    {
        XXH3_64bits_update((XXH3_state_t*)mState, (const void*)buffer, len);
    }
    fclose(file);
}

void HBXXH64::finalize()
{
    if (!mState)
    {
        LL_WARNS() << "Already finalized !" << LL_ENDL;
        return;
    }
    mDigest = XXH3_64bits_digest((XXH3_state_t*)mState);
    XXH3_freeState((XXH3_state_t*)mState);
    mState = NULL;
}

U64 HBXXH64::digest() const
{
    return mState ? XXH3_64bits_digest((XXH3_state_t*)mState) : mDigest;
}

std::ostream& operator<<(std::ostream& stream, HBXXH64 context)
{
    stream << context.digest();
    return stream;
}

///////////////////////////////////////////////////////////////////////////////
// HBXXH128 class
///////////////////////////////////////////////////////////////////////////////

//static
LLUUID HBXXH128::digest(const void* buffer, size_t len)
{
    XXH128_hash_t hash = XXH3_128bits(buffer, len);
    LLUUID id;
    U64* data = (U64*)id.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
    return id;
}

//static
LLUUID HBXXH128::digest(const char* str)
{
    XXH128_hash_t hash = XXH3_128bits((const void*)str, strlen(str));
    LLUUID id;
    U64* data = (U64*)id.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
    return id;
}

//static
LLUUID HBXXH128::digest(const std::string& str)
{
    XXH128_hash_t hash = XXH3_128bits((const void*)str.c_str(), str.size());
    LLUUID id;
    U64* data = (U64*)id.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
    return id;
}

//static
void HBXXH128::digest(LLUUID& result, const void* buffer, size_t len)
{
    XXH128_hash_t hash = XXH3_128bits(buffer, len);
    U64* data = (U64*)result.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
}

//static
void HBXXH128::digest(LLUUID& result, const char* str)
{
    XXH128_hash_t hash = XXH3_128bits((const void*)str, strlen(str));
    U64* data = (U64*)result.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
}

//static
void HBXXH128::digest(LLUUID& result, const std::string& str)
{
    XXH128_hash_t hash = XXH3_128bits((const void*)str.c_str(), str.size());
    U64* data = (U64*)result.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
}

// Must be called by all constructors.
void HBXXH128::init()
{
    mState = (void*)XXH3_createState();
    if (!mState || XXH3_128bits_reset((XXH3_state_t*)mState) != XXH_OK)
    {
        LL_WARNS() << "Failed to initialize state !" << LL_ENDL;
    }
}

HBXXH128::~HBXXH128()
{
    if (mState)
    {
        XXH3_freeState((XXH3_state_t*)mState);
    }
}

void HBXXH128::update(const void* buffer, size_t len)
{
    if (mState)
    {
        XXH3_128bits_update((XXH3_state_t*)mState, buffer, len);
    }
    else
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
    }
}

void HBXXH128::update(const std::string& str)
{
    if (mState)
    {
        XXH3_128bits_update((XXH3_state_t*)mState, (const void*)str.c_str(),
                           str.length());
    }
    else
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
    }
}

void HBXXH128::update(std::istream& stream)
{
    if (!mState)
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
        return;
    }

    char buffer[BLOCK_LEN];
    size_t len;
    while (stream.good())
    {
        stream.read(buffer, BLOCK_LEN);
        len = stream.gcount();
        XXH3_128bits_update((XXH3_state_t*)mState, (const void*)buffer, len);
    }
}

void HBXXH128::update(FILE* file)
{
    if (!mState)
    {
        LL_WARNS() << "Cannot update a finalized digest !" << LL_ENDL;
        return;
    }

    char buffer[BLOCK_LEN];
    size_t len;
    while ((len = fread((void*)buffer, 1, BLOCK_LEN, file)))
    {
        XXH3_128bits_update((XXH3_state_t*)mState, (const void*)buffer, len);
    }
    fclose(file);
}

void HBXXH128::finalize()
{
    if (!mState)
    {
        LL_WARNS() << "Already finalized !" << LL_ENDL;
        return;
    }
    XXH128_hash_t hash = XXH3_128bits_digest((XXH3_state_t*)mState);
    U64* data = (U64*)mDigest.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
    XXH3_freeState((XXH3_state_t*)mState);
    mState = NULL;
}

const LLUUID& HBXXH128::digest() const
{
    if (mState)
    {
        XXH128_hash_t hash = XXH3_128bits_digest((XXH3_state_t*)mState);
        // We cheat the const-ness of the method here, but this is OK, since
        // mDigest is private and cannot be accessed indirectly by other
        // methods than digest() ones, that do check for mState to decide
        // wether mDigest's current value may be provided as is or not. This
        // cheat saves us a temporary LLLUID copy.
        U64* data = (U64*)mDigest.mData;
        // Note: we do not check endianness here and we just store in the same
        // order as XXH128_hash_t, that is low word "first".
        data[0] = hash.low64;
        data[1] = hash.high64;
    }
    return mDigest;
}

void HBXXH128::digest(LLUUID& result) const
{
    if (!mState)
    {
        result = mDigest;
        return;
    }
    XXH128_hash_t hash = XXH3_128bits_digest((XXH3_state_t*)mState);
    U64* data = (U64*)result.mData;
    // Note: we do not check endianness here and we just store in the same
    // order as XXH128_hash_t, that is low word "first".
    data[0] = hash.low64;
    data[1] = hash.high64;
}

std::ostream& operator<<(std::ostream& stream, HBXXH128 context)
{
    stream << context.digest();
    return stream;
}
