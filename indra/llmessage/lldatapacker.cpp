/**
 * @file lldatapacker.cpp
 * @brief Data packer implementation.
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

#include "lldatapacker.h"
#include "llerror.h"

#include "message.h"

#include "v4color.h"
#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "lluuid.h"

// *NOTE: there are functions below which use sscanf and rely on this
// particular value of DP_BUFSIZE. Search for '511' (DP_BUFSIZE - 1)
// to find them if you change this number.
const S32 DP_BUFSIZE = 512;

static char DUMMY_BUFFER[128]; /*Flawfinder: ignore*/

LLDataPacker::LLDataPacker() : mPassFlags(0), mWriteEnabled(false)
{
}

//virtual
void LLDataPacker::reset()
{
    LL_ERRS() << "Using unimplemented datapacker reset!" << LL_ENDL;
}

//virtual
void LLDataPacker::dumpBufferToLog()
{
    LL_ERRS() << "dumpBufferToLog not implemented for this type!" << LL_ENDL;
}

bool LLDataPacker::packFixed(const F32 value, const char *name,
                             const bool is_signed, const U32 int_bits, const U32 frac_bits)
{
    bool success = true;
    S32 unsigned_bits = int_bits + frac_bits;
    S32 total_bits = unsigned_bits;

    if (is_signed)
    {
        total_bits++;
    }

    S32 min_val;
    U32 max_val;
    if (is_signed)
    {
        min_val = 1 << int_bits;
        min_val *= -1;
    }
    else
    {
        min_val = 0;
    }
    max_val = 1 << int_bits;

    // Clamp to be within range
    F32 fixed_val = llclamp(value, (F32)min_val, (F32)max_val);
    if (is_signed)
    {
        fixed_val += max_val;
    }
    fixed_val *= 1 << frac_bits;

    if (total_bits <= 8)
    {
        packU8((U8)fixed_val, name);
    }
    else if (total_bits <= 16)
    {
        packU16((U16)fixed_val, name);
    }
    else if (total_bits <= 31)
    {
        packU32((U32)fixed_val, name);
    }
    else
    {
        LL_ERRS() << "Using fixed-point packing of " << total_bits << " bits, why?!" << LL_ENDL;
    }
    return success;
}

bool LLDataPacker::unpackFixed(F32 &value, const char *name,
                               const bool is_signed, const U32 int_bits, const U32 frac_bits)
{
    bool success = true;
    //LL_INFOS() << "unpackFixed:" << name << " int:" << int_bits << " frac:" << frac_bits << LL_ENDL;
    S32 unsigned_bits = int_bits + frac_bits;
    S32 total_bits = unsigned_bits;

    if (is_signed)
    {
        total_bits++;
    }

    U32 max_val;
    max_val = 1 << int_bits;

    F32 fixed_val;
    if (total_bits <= 8)
    {
        U8 fixed_8;
        success = unpackU8(fixed_8, name);
        fixed_val = (F32)fixed_8;
    }
    else if (total_bits <= 16)
    {
        U16 fixed_16;
        success = unpackU16(fixed_16, name);
        fixed_val = (F32)fixed_16;
    }
    else if (total_bits <= 31)
    {
        U32 fixed_32;
        success = unpackU32(fixed_32, name);
        fixed_val = (F32)fixed_32;
    }
    else
    {
        fixed_val = 0;
        LL_ERRS() << "Bad bit count: " << total_bits << LL_ENDL;
    }

    //LL_INFOS() << "Fixed_val:" << fixed_val << LL_ENDL;

    fixed_val /= (F32)(1 << frac_bits);
    if (is_signed)
    {
        fixed_val -= max_val;
    }
    value = fixed_val;
    //LL_INFOS() << "Value: " << value << LL_ENDL;
    return success;
}

bool LLDataPacker::unpackU16s(U16 *values, S32 count, const char *name)
{
    for (S32 idx = 0; idx < count; ++idx)
    {
        if (!unpackU16(values[idx], name))
        {
            LL_WARNS("DATAPACKER") << "Buffer overflow reading Unsigned 16s \"" << name << "\" at index " << idx << "!" << LL_ENDL;
            return false;
        }
    }
    return true;
}

bool LLDataPacker::unpackS16s(S16 *values, S32 count, const char *name)
{
    for (S32 idx = 0; idx < count; ++idx)
    {
        if (!unpackS16(values[idx], name))
        {
            LL_WARNS("DATAPACKER") << "Buffer overflow reading Signed 16s \"" << name << "\" at index " << idx << "!" << LL_ENDL;
            return false;
        }
    }
    return true;
}

bool LLDataPacker::unpackF32s(F32 *values, S32 count, const char *name)
{
    for (S32 idx = 0; idx < count; ++idx)
    {
        if (!unpackF32(values[idx], name))
        {
            LL_WARNS("DATAPACKER") << "Buffer overflow reading Float 32s \"" << name << "\" at index " << idx << "!" << LL_ENDL;
            return false;
        }
    }
    return true;
}

bool LLDataPacker::unpackColor4Us(LLColor4U *values, S32 count, const char *name)
{
    for (S32 idx = 0; idx < count; ++idx)
    {
        if (!unpackColor4U(values[idx], name))
        {
            LL_WARNS("DATAPACKER") << "Buffer overflow reading Float 32s \"" << name << "\" at index " << idx << "!" << LL_ENDL;
            return false;
        }
    }
    return true;
}

bool LLDataPacker::unpackUUIDs(LLUUID *values, S32 count, const char *name)
{
    for (S32 idx = 0; idx < count; ++idx)
    {
        if (!unpackUUID(values[idx], name))
        {
            LL_WARNS("DATAPACKER") << "Buffer overflow reading UUIDs \"" << name << "\" at index " << idx << "!" << LL_ENDL;
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------
// LLDataPackerBinaryBuffer implementation
//---------------------------------------------------------------------------

bool LLDataPackerBinaryBuffer::packString(const std::string& value, const char *name)
{
    S32 length = static_cast<S32>(value.length()) + 1;

    if (!verifyLength(length, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value.c_str(), MVT_VARIABLE, length);
    }
    mCurBufferp += length;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackString(std::string& value, const char *name)
{
    S32 length = (S32)strlen((char *)mCurBufferp) + 1; /*Flawfinder: ignore*/

    if (!verifyLength(length, name))
    {
        return false;
    }

    value = std::string((char*)mCurBufferp); // We already assume NULL termination calling strlen()

    mCurBufferp += length;
    return true;
}

bool LLDataPackerBinaryBuffer::packBinaryData(const U8 *value, S32 size, const char *name)
{
    if (!verifyLength(size + 4, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, &size, MVT_S32, 4);
    }
    mCurBufferp += 4;
    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value, MVT_VARIABLE, size);
    }
    mCurBufferp += size;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackBinaryData(U8 *value, S32 &size, const char *name)
{
    if (!verifyLength(4, name))
    {
        LL_WARNS() << "LLDataPackerBinaryBuffer::unpackBinaryData would unpack invalid data, aborting!" << LL_ENDL;
        return false;
    }

    htolememcpy(&size, mCurBufferp, MVT_S32, 4);

    if (size < 0)
    {
        LL_WARNS() << "LLDataPackerBinaryBuffer::unpackBinaryData unpacked invalid size, aborting!" << LL_ENDL;
        return false;
    }

    mCurBufferp += 4;

    if (!verifyLength(size, name))
    {
        LL_WARNS() << "LLDataPackerBinaryBuffer::unpackBinaryData would unpack invalid data, aborting!" << LL_ENDL;
        return false;
    }

    htolememcpy(value, mCurBufferp, MVT_VARIABLE, size);
    mCurBufferp += size;

    return true;
}


bool LLDataPackerBinaryBuffer::packBinaryDataFixed(const U8 *value, S32 size, const char *name)
{
    if (!verifyLength(size, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value, MVT_VARIABLE, size);
    }
    mCurBufferp += size;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackBinaryDataFixed(U8 *value, S32 size, const char *name)
{
    if (!verifyLength(size, name))
    {
        return false;
    }
    htolememcpy(value, mCurBufferp, MVT_VARIABLE, size);
    mCurBufferp += size;
    return true;
}


bool LLDataPackerBinaryBuffer::packU8(const U8 value, const char *name)
{
    if (!verifyLength(sizeof(U8), name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        *mCurBufferp = value;
    }
    mCurBufferp++;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackU8(U8 &value, const char *name)
{
    if (!verifyLength(sizeof(U8), name))
    {
        return false;
    }

    value = *mCurBufferp;
    mCurBufferp++;
    return true;
}


bool LLDataPackerBinaryBuffer::packU16(const U16 value, const char *name)
{
    if (!verifyLength(sizeof(U16), name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, &value, MVT_U16, 2);
    }
    mCurBufferp += 2;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackU16(U16 &value, const char *name)
{
    if (!verifyLength(sizeof(U16), name))
    {
        return false;
    }

    htolememcpy(&value, mCurBufferp, MVT_U16, 2);
    mCurBufferp += 2;
    return true;
}

bool LLDataPackerBinaryBuffer::packS16(const S16 value, const char *name)
{
    bool success = verifyLength(sizeof(S16), name);

    if (mWriteEnabled && success)
    {
        htolememcpy(mCurBufferp, &value, MVT_S16, 2);
    }
    mCurBufferp += 2;
    return success;
}

bool LLDataPackerBinaryBuffer::unpackS16(S16 &value, const char *name)
{
    bool success = verifyLength(sizeof(S16), name);

    if (success)
    {
        htolememcpy(&value, mCurBufferp, MVT_S16, 2);
    }
    mCurBufferp += 2;
    return success;
}

bool LLDataPackerBinaryBuffer::packU32(const U32 value, const char *name)
{
    if (!verifyLength(sizeof(U32), name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, &value, MVT_U32, 4);
    }
    mCurBufferp += 4;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackU32(U32 &value, const char *name)
{
    if (!verifyLength(sizeof(U32), name))
    {
        return false;
    }

    htolememcpy(&value, mCurBufferp, MVT_U32, 4);
    mCurBufferp += 4;
    return true;
}


bool LLDataPackerBinaryBuffer::packS32(const S32 value, const char *name)
{
    if (!verifyLength(sizeof(S32), name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, &value, MVT_S32, 4);
    }
    mCurBufferp += 4;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackS32(S32 &value, const char *name)
{
    if(!verifyLength(sizeof(S32), name))
    {
        return false;
    }

    htolememcpy(&value, mCurBufferp, MVT_S32, 4);
    mCurBufferp += 4;
    return true;
}


bool LLDataPackerBinaryBuffer::packF32(const F32 value, const char *name)
{
    if (!verifyLength(sizeof(F32), name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, &value, MVT_F32, 4);
    }
    mCurBufferp += 4;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackF32(F32 &value, const char *name)
{
    if (!verifyLength(sizeof(F32), name))
    {
        return false;
    }

    htolememcpy(&value, mCurBufferp, MVT_F32, 4);
    mCurBufferp += 4;
    return true;
}


bool LLDataPackerBinaryBuffer::packColor4(const LLColor4 &value, const char *name)
{
    if (!verifyLength(16, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value.mV, MVT_LLVector4, 16);
    }
    mCurBufferp += 16;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackColor4(LLColor4 &value, const char *name)
{
    if (!verifyLength(16, name))
    {
        return false;
    }

    htolememcpy(value.mV, mCurBufferp, MVT_LLVector4, 16);
    mCurBufferp += 16;
    return true;
}


bool LLDataPackerBinaryBuffer::packColor4U(const LLColor4U &value, const char *name)
{
    if (!verifyLength(4, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value.mV, MVT_VARIABLE, 4);
    }
    mCurBufferp += 4;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackColor4U(LLColor4U &value, const char *name)
{
    if (!verifyLength(4, name))
    {
        return false;
    }

    htolememcpy(value.mV, mCurBufferp, MVT_VARIABLE, 4);
    mCurBufferp += 4;
    return true;
}



bool LLDataPackerBinaryBuffer::packVector2(const LLVector2 &value, const char *name)
{
    if (!verifyLength(8, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, &value.mV[0], MVT_F32, 4);
        htolememcpy(mCurBufferp+4, &value.mV[1], MVT_F32, 4);
    }
    mCurBufferp += 8;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackVector2(LLVector2 &value, const char *name)
{
    if (!verifyLength(8, name))
    {
        return false;
    }

    htolememcpy(&value.mV[0], mCurBufferp, MVT_F32, 4);
    htolememcpy(&value.mV[1], mCurBufferp+4, MVT_F32, 4);
    mCurBufferp += 8;
    return true;
}


bool LLDataPackerBinaryBuffer::packVector3(const LLVector3 &value, const char *name)
{
    if (!verifyLength(12, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value.mV, MVT_LLVector3, 12);
    }
    mCurBufferp += 12;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackVector3(LLVector3 &value, const char *name)
{
    if (!verifyLength(12, name))
    {
        return false;
    }

    htolememcpy(value.mV, mCurBufferp, MVT_LLVector3, 12);
    mCurBufferp += 12;
    return true;
}

bool LLDataPackerBinaryBuffer::packVector4(const LLVector4 &value, const char *name)
{
    if (!verifyLength(16, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value.mV, MVT_LLVector4, 16);
    }
    mCurBufferp += 16;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackVector4(LLVector4 &value, const char *name)
{
    if (!verifyLength(16, name))
    {
        return false;
    }

    htolememcpy(value.mV, mCurBufferp, MVT_LLVector4, 16);
    mCurBufferp += 16;
    return true;
}

bool LLDataPackerBinaryBuffer::packUUID(const LLUUID &value, const char *name)
{
    if (!verifyLength(16, name))
    {
        return false;
    }

    if (mWriteEnabled)
    {
        htolememcpy(mCurBufferp, value.mData, MVT_LLUUID, 16);
    }
    mCurBufferp += 16;
    return true;
}


bool LLDataPackerBinaryBuffer::unpackUUID(LLUUID &value, const char *name)
{
    if (!verifyLength(16, name))
    {
        return false;
    }

    htolememcpy(value.mData, mCurBufferp, MVT_LLUUID, 16);
    mCurBufferp += 16;
    return true;
}

const LLDataPackerBinaryBuffer& LLDataPackerBinaryBuffer::operator=(const LLDataPackerBinaryBuffer &a)
{
    if (a.getBufferSize() > getBufferSize())
    {
        // We've got problems, ack!
        LL_ERRS() << "Trying to do an assignment with not enough room in the target." << LL_ENDL;
    }
    memcpy(mBufferp, a.mBufferp, a.getBufferSize());    /*Flawfinder: ignore*/
    return *this;
}

void LLDataPackerBinaryBuffer::dumpBufferToLog()
{
    LL_WARNS() << "Binary Buffer Dump, size: " << mBufferSize << LL_ENDL;
    char line_buffer[256]; /*Flawfinder: ignore*/
    S32 i;
    S32 cur_line_pos = 0;

    S32 cur_line = 0;
    for (i = 0; i < mBufferSize; i++)
    {
        snprintf(line_buffer + cur_line_pos*3, sizeof(line_buffer) - cur_line_pos*3, "%02x ", mBufferp[i]);     /* Flawfinder: ignore */
        cur_line_pos++;
        if (cur_line_pos >= 16)
        {
            cur_line_pos = 0;
            LL_WARNS() << "Offset:" << std::hex << cur_line*16 << std::dec << " Data:" << line_buffer << LL_ENDL;
            cur_line++;
        }
    }
    if (cur_line_pos)
    {
        LL_WARNS() << "Offset:" << std::hex << cur_line*16 << std::dec << " Data:" << line_buffer << LL_ENDL;
    }
}

//---------------------------------------------------------------------------
// LLDataPackerAsciiBuffer implementation
//---------------------------------------------------------------------------
bool LLDataPackerAsciiBuffer::packString(const std::string& value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
        numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%s\n", value.c_str());       /* Flawfinder: ignore */
    }
    else
    {
        numCopied = static_cast<S32>(value.length()) + 1; /*Flawfinder: ignore*/
    }

    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        // *NOTE: I believe we need to mark a failure bit at this point.
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packString: string truncated: " << value << LL_ENDL;
    }
    mCurBufferp += numCopied;
    return success;
}

bool LLDataPackerAsciiBuffer::unpackString(std::string& value, const char *name)
{
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore*/
    if (!getValueStr(name, valuestr, DP_BUFSIZE))  // NULL terminated
    {
        return false;
    }
    value = valuestr;
    return true;
}


bool LLDataPackerAsciiBuffer::packBinaryData(const U8 *value, S32 size, const char *name)
{
    bool success = true;
    writeIndentedName(name);

    int numCopied = 0;
    if (mWriteEnabled)
    {
        numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%010d ", size);  /* Flawfinder: ignore */

        // snprintf returns number of bytes that would have been
        // written had the output not being truncated. In that case,
        // it will retuen >= passed in size value.  so a check needs
        // to be added to detect truncation, and if there is any, only
        // account for the actual number of bytes written..and not
        // what could have been written.
        if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
        {
            numCopied = getBufferSize()-getCurrentSize();
            LL_WARNS() << "LLDataPackerAsciiBuffer::packBinaryData: number truncated: " << size << LL_ENDL;
        }
        mCurBufferp += numCopied;


        S32 i;
        bool bBufferFull = false;
        for (i = 0; i < size && !bBufferFull; i++)
        {
            numCopied = snprintf(mCurBufferp, getBufferSize()-getCurrentSize(), "%02x ", value[i]); /* Flawfinder: ignore */
            if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
            {
                numCopied = getBufferSize()-getCurrentSize();
                LL_WARNS() << "LLDataPackerAsciiBuffer::packBinaryData: data truncated: " << LL_ENDL;
                bBufferFull = true;
            }
            mCurBufferp += numCopied;
        }

        if (!bBufferFull)
        {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(), "\n");   /* Flawfinder: ignore */
            if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
                {
                numCopied = getBufferSize()-getCurrentSize();
                LL_WARNS() << "LLDataPackerAsciiBuffer::packBinaryData: newline truncated: " << LL_ENDL;
                }
                mCurBufferp += numCopied;
        }
    }
    else
    {
        // why +10 ?? XXXCHECK
        numCopied = 10 + 1; // size plus newline
        numCopied += size;
        if (numCopied > getBufferSize()-getCurrentSize())
        {
            numCopied = getBufferSize()-getCurrentSize();
        }
        mCurBufferp += numCopied;
    }

    return success;
}


bool LLDataPackerAsciiBuffer::unpackBinaryData(U8 *value, S32 &size, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];      /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    char *cur_pos = &valuestr[0];
    sscanf(valuestr,"%010d", &size);
    cur_pos += 11;

    S32 i;
    for (i = 0; i < size; i++)
    {
        S32 val;
        sscanf(cur_pos,"%02x", &val);
        value[i] = val;
        cur_pos += 3;
    }
    return success;
}


bool LLDataPackerAsciiBuffer::packBinaryDataFixed(const U8 *value, S32 size, const char *name)
{
    bool success = true;
    writeIndentedName(name);

    if (mWriteEnabled)
    {
        S32 i;
        int numCopied = 0;
        bool bBufferFull = false;
        for (i = 0; i < size && !bBufferFull; i++)
        {
            numCopied = snprintf(mCurBufferp, getBufferSize()-getCurrentSize(), "%02x ", value[i]); /* Flawfinder: ignore */
            if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
            {
                numCopied = getBufferSize()-getCurrentSize();
                LL_WARNS() << "LLDataPackerAsciiBuffer::packBinaryDataFixed: data truncated: " << LL_ENDL;
                bBufferFull = true;
            }
            mCurBufferp += numCopied;

        }
        if (!bBufferFull)
        {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(), "\n");   /* Flawfinder: ignore */
            if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
            {
                numCopied = getBufferSize()-getCurrentSize();
                LL_WARNS() << "LLDataPackerAsciiBuffer::packBinaryDataFixed: newline truncated: " << LL_ENDL;
            }

            mCurBufferp += numCopied;
        }
    }
    else
    {
        int numCopied = 2 * size + 1; //hex bytes plus newline
        if (numCopied > getBufferSize()-getCurrentSize())
        {
            numCopied = getBufferSize()-getCurrentSize();
        }
        mCurBufferp += numCopied;
    }
    return success;
}


bool LLDataPackerAsciiBuffer::unpackBinaryDataFixed(U8 *value, S32 size, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];      /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    char *cur_pos = &valuestr[0];

    S32 i;
    for (i = 0; i < size; i++)
    {
        S32 val;
        sscanf(cur_pos,"%02x", &val);
        value[i] = val;
        cur_pos += 3;
    }
    return success;
}



bool LLDataPackerAsciiBuffer::packU8(const U8 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d\n", value);   /* Flawfinder: ignore */
    }
    else
    {
        // just do the write to a temp buffer to get the length
        numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%d\n", value);    /* Flawfinder: ignore */
    }

    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packU8: val truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;

    return success;
}


bool LLDataPackerAsciiBuffer::unpackU8(U8 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];      /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 in_val;
    sscanf(valuestr,"%d", &in_val);
    value = in_val;
    return success;
}

bool LLDataPackerAsciiBuffer::packU16(const U16 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d\n", value);   /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%d\n", value);    /* Flawfinder: ignore */
    }

    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packU16: val truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;

    return success;
}


bool LLDataPackerAsciiBuffer::unpackU16(U16 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];      /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 in_val;
    sscanf(valuestr,"%d", &in_val);
    value = in_val;
    return success;
}

bool LLDataPackerAsciiBuffer::packS16(const S16 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
        numCopied = snprintf(mCurBufferp, getBufferSize() - getCurrentSize(), "%d\n", value); /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%d\n", value); /* Flawfinder: ignore */
    }

    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if(numCopied < 0 || numCopied > getBufferSize() - getCurrentSize())
    {
        numCopied = getBufferSize() - getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packS16: val truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;

    return success;
}


bool LLDataPackerAsciiBuffer::unpackS16(S16 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 in_val;
    sscanf(valuestr, "%d", &in_val);
    value = in_val;
    return success;
}

bool LLDataPackerAsciiBuffer::packU32(const U32 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%u\n", value);   /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%u\n", value);    /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packU32: val truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackU32(U32 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];      /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%u", &value);
    return success;
}


bool LLDataPackerAsciiBuffer::packS32(const S32 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d\n", value);   /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%d\n", value);        /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packS32: val truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackS32(S32 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];      /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%d", &value);
    return success;
}


bool LLDataPackerAsciiBuffer::packF32(const F32 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f\n", value);       /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%f\n", value);        /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packF32: val truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackF32(F32 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];      /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f", &value);
    return success;
}


bool LLDataPackerAsciiBuffer::packColor4(const LLColor4 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]); /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);    /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packColor4: truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackColor4(LLColor4 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];  /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
    return success;
}

bool LLDataPackerAsciiBuffer::packColor4U(const LLColor4U &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
        numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d %d %d %d\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]); /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%d %d %d %d\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);    /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packColor4U: truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackColor4U(LLColor4U &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];   /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 r, g, b, a;

    sscanf(valuestr,"%d %d %d %d", &r, &g, &b, &a);
    value.mV[0] = r;
    value.mV[1] = g;
    value.mV[2] = b;
    value.mV[3] = a;
    return success;
}


bool LLDataPackerAsciiBuffer::packVector2(const LLVector2 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f\n", value.mV[0], value.mV[1]); /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f\n", value.mV[0], value.mV[1]);        /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packVector2: truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackVector2(LLVector2 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];   /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f", &value.mV[0], &value.mV[1]);
    return success;
}


bool LLDataPackerAsciiBuffer::packVector3(const LLVector3 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f %f\n", value.mV[0], value.mV[1], value.mV[2]); /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f %f\n", value.mV[0], value.mV[1], value.mV[2]);    /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packVector3: truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackVector3(LLVector3 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];  /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f %f", &value.mV[0], &value.mV[1], &value.mV[2]);
    return success;
}

bool LLDataPackerAsciiBuffer::packVector4(const LLVector4 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    int numCopied = 0;
    if (mWriteEnabled)
    {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]); /* Flawfinder: ignore */
    }
    else
    {
        numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);    /* Flawfinder: ignore */
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packVector4: truncated: " << LL_ENDL;
    }

    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackVector4(LLVector4 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];  /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
    return success;
}


bool LLDataPackerAsciiBuffer::packUUID(const LLUUID &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);

    int numCopied = 0;
    if (mWriteEnabled)
    {
        std::string tmp_str;
        value.toString(tmp_str);
        numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%s\n", tmp_str.c_str()); /* Flawfinder: ignore */
    }
    else
    {
        numCopied = 64 + 1; // UUID + newline
    }
    // snprintf returns number of bytes that would have been written
    // had the output not being truncated. In that case, it will
    // return either -1 or value >= passed in size value . So a check needs to be added
    // to detect truncation, and if there is any, only account for the
    // actual number of bytes written..and not what could have been
    // written.
    if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
    {
        numCopied = getBufferSize()-getCurrentSize();
        LL_WARNS() << "LLDataPackerAsciiBuffer::packUUID: truncated: " << LL_ENDL;
        success = false;
    }
    mCurBufferp += numCopied;
    return success;
}


bool LLDataPackerAsciiBuffer::unpackUUID(LLUUID &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];  /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    char tmp_str[64];   /* Flawfinder: ignore */
    sscanf(valuestr, "%63s", tmp_str);  /* Flawfinder: ignore */
    value.set(tmp_str);

    return success;
}

void LLDataPackerAsciiBuffer::dump()
{
    LL_INFOS() << "Buffer: " << mBufferp << LL_ENDL;
}

void LLDataPackerAsciiBuffer::writeIndentedName(const char *name)
{
    if (mIncludeNames)
    {
        int numCopied = 0;
        if (mWriteEnabled)
        {
            numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%s\t", name);    /* Flawfinder: ignore */
        }
        else
        {
            numCopied = (S32)strlen(name) + 1;  /* Flawfinder: ignore */ //name + tab
        }

        // snprintf returns number of bytes that would have been written
        // had the output not being truncated. In that case, it will
        // return either -1 or value >= passed in size value . So a check needs to be added
        // to detect truncation, and if there is any, only account for the
        // actual number of bytes written..and not what could have been
        // written.
        if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
        {
            numCopied = getBufferSize()-getCurrentSize();
            LL_WARNS() << "LLDataPackerAsciiBuffer::writeIndentedName: truncated: " << LL_ENDL;
        }

        mCurBufferp += numCopied;
    }
}

bool LLDataPackerAsciiBuffer::getValueStr(const char *name, char *out_value, S32 value_len)
{
    bool success = true;
    char buffer[DP_BUFSIZE];    /* Flawfinder: ignore */
    char keyword[DP_BUFSIZE];   /* Flawfinder: ignore */
    char value[DP_BUFSIZE]; /* Flawfinder: ignore */

    buffer[0] = '\0';
    keyword[0] = '\0';
    value[0] = '\0';

    if (mIncludeNames)
    {
        // Read both the name and the value, and validate the name.
        sscanf(mCurBufferp, "%511[^\n]", buffer);
        // Skip the \n
        mCurBufferp += (S32)strlen(buffer) + 1; /* Flawfinder: ignore */

        sscanf(buffer, "%511s %511[^\n]", keyword, value);  /* Flawfinder: ignore */

        if (strcmp(keyword, name))
        {
            LL_WARNS() << "Data packer expecting keyword of type " << name << ", got " << keyword << " instead!" << LL_ENDL;
            return false;
        }
    }
    else
    {
        // Just the value exists
        sscanf(mCurBufferp, "%511[^\n]", value);
        // Skip the \n
        mCurBufferp += (S32)strlen(value) + 1;  /* Flawfinder: ignore */
    }

    S32 in_value_len = (S32)strlen(value)+1;    /* Flawfinder: ignore */
    S32 min_len = llmin(in_value_len, value_len);
    memcpy(out_value, value, min_len);  /* Flawfinder: ignore */
    out_value[min_len-1] = 0;

    return success;
}

// helper function used by LLDataPackerAsciiFile
// to convert F32 into a string. This is to avoid
// << operator writing F32 value into a stream
// since it does not seem to preserve the float value
std::string convertF32ToString(F32 val)
{
    std::string str;
    char  buf[20];
    snprintf(buf, 20, "%f", val);
    str = buf;
    return str;
}

//---------------------------------------------------------------------------
// LLDataPackerAsciiFile implementation
//---------------------------------------------------------------------------
bool LLDataPackerAsciiFile::packString(const std::string& value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%s\n", value.c_str());
    }
    else if (mOutputStream)
    {
        *mOutputStream << value << "\n";
    }
    return success;
}

bool LLDataPackerAsciiFile::unpackString(std::string& value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE];  /* Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }
    value = valuestr;
    return success;
}


bool LLDataPackerAsciiFile::packBinaryData(const U8 *value, S32 size, const char *name)
{
    bool success = true;
    writeIndentedName(name);

    if (mFP)
    {
        fprintf(mFP, "%010d ", size);

        S32 i;
        for (i = 0; i < size; i++)
        {
            fprintf(mFP, "%02x ", value[i]);
        }
        fprintf(mFP, "\n");
    }
    else if (mOutputStream)
    {
        char buffer[32];    /* Flawfinder: ignore */
        snprintf(buffer,sizeof(buffer), "%010d ", size);    /* Flawfinder: ignore */
        *mOutputStream << buffer;

        S32 i;
        for (i = 0; i < size; i++)
        {
            snprintf(buffer, sizeof(buffer), "%02x ", value[i]);    /* Flawfinder: ignore */
            *mOutputStream << buffer;
        }
        *mOutputStream << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackBinaryData(U8 *value, S32 &size, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore*/
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    char *cur_pos = &valuestr[0];
    sscanf(valuestr,"%010d", &size);
    cur_pos += 11;

    S32 i;
    for (i = 0; i < size; i++)
    {
        S32 val;
        sscanf(cur_pos,"%02x", &val);
        value[i] = val;
        cur_pos += 3;
    }
    return success;
}


bool LLDataPackerAsciiFile::packBinaryDataFixed(const U8 *value, S32 size, const char *name)
{
    bool success = true;
    writeIndentedName(name);

    if (mFP)
    {
        S32 i;
        for (i = 0; i < size; i++)
        {
            fprintf(mFP, "%02x ", value[i]);
        }
        fprintf(mFP, "\n");
    }
    else if (mOutputStream)
    {
        char buffer[32]; /*Flawfinder: ignore*/
        S32 i;
        for (i = 0; i < size; i++)
        {
            snprintf(buffer, sizeof(buffer), "%02x ", value[i]);    /* Flawfinder: ignore */
            *mOutputStream << buffer;
        }
        *mOutputStream << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackBinaryDataFixed(U8 *value, S32 size, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore*/
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    char *cur_pos = &valuestr[0];

    S32 i;
    for (i = 0; i < size; i++)
    {
        S32 val;
        sscanf(cur_pos,"%02x", &val);
        value[i] = val;
        cur_pos += 3;
    }
    return success;
}



bool LLDataPackerAsciiFile::packU8(const U8 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%d\n", value);
    }
    else if (mOutputStream)
    {
        // We have to cast this to an integer because streams serialize
        // bytes as bytes - not as text.
        *mOutputStream << (S32)value << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackU8(U8 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 in_val;
    sscanf(valuestr,"%d", &in_val);
    value = in_val;
    return success;
}

bool LLDataPackerAsciiFile::packU16(const U16 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%d\n", value);
    }
    else if (mOutputStream)
    {
        *mOutputStream <<"" << value << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackU16(U16 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 in_val;
    sscanf(valuestr,"%d", &in_val);
    value = in_val;
    return success;
}

bool LLDataPackerAsciiFile::packS16(const S16 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP, "%d\n", value);
    }
    else if (mOutputStream)
    {
        *mOutputStream << "" << value << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackS16(S16 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 in_val;
    sscanf(valuestr, "%d", &in_val);
    value = in_val;
    return success;
}

bool LLDataPackerAsciiFile::packU32(const U32 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%u\n", value);
    }
    else if (mOutputStream)
    {
        *mOutputStream <<"" << value << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackU32(U32 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%u", &value);
    return success;
}


bool LLDataPackerAsciiFile::packS32(const S32 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%d\n", value);
    }
    else if (mOutputStream)
    {
        *mOutputStream <<"" << value << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackS32(S32 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%d", &value);
    return success;
}


bool LLDataPackerAsciiFile::packF32(const F32 value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%f\n", value);
    }
    else if (mOutputStream)
    {
        *mOutputStream <<"" << convertF32ToString(value) << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackF32(F32 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f", &value);
    return success;
}


bool LLDataPackerAsciiFile::packColor4(const LLColor4 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);
    }
    else if (mOutputStream)
    {
        *mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << " " << convertF32ToString(value.mV[2]) << " " << convertF32ToString(value.mV[3]) << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackColor4(LLColor4 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
    return success;
}

bool LLDataPackerAsciiFile::packColor4U(const LLColor4U &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%d %d %d %d\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);
    }
    else if (mOutputStream)
    {
        *mOutputStream << (S32)(value.mV[0]) << " " << (S32)(value.mV[1]) << " " << (S32)(value.mV[2]) << " " << (S32)(value.mV[3]) << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackColor4U(LLColor4U &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    S32 r, g, b, a;

    sscanf(valuestr,"%d %d %d %d", &r, &g, &b, &a);
    value.mV[0] = r;
    value.mV[1] = g;
    value.mV[2] = b;
    value.mV[3] = a;
    return success;
}


bool LLDataPackerAsciiFile::packVector2(const LLVector2 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%f %f\n", value.mV[0], value.mV[1]);
    }
    else if (mOutputStream)
    {
        *mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackVector2(LLVector2 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f", &value.mV[0], &value.mV[1]);
    return success;
}


bool LLDataPackerAsciiFile::packVector3(const LLVector3 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%f %f %f\n", value.mV[0], value.mV[1], value.mV[2]);
    }
    else if (mOutputStream)
    {
        *mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << " " << convertF32ToString(value.mV[2]) << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackVector3(LLVector3 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f %f", &value.mV[0], &value.mV[1], &value.mV[2]);
    return success;
}

bool LLDataPackerAsciiFile::packVector4(const LLVector4 &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    if (mFP)
    {
        fprintf(mFP,"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);
    }
    else if (mOutputStream)
    {
        *mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << " " << convertF32ToString(value.mV[2]) << " " << convertF32ToString(value.mV[3]) << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackVector4(LLVector4 &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
    return success;
}


bool LLDataPackerAsciiFile::packUUID(const LLUUID &value, const char *name)
{
    bool success = true;
    writeIndentedName(name);
    std::string tmp_str;
    value.toString(tmp_str);
    if (mFP)
    {
        fprintf(mFP,"%s\n", tmp_str.c_str());
    }
    else if (mOutputStream)
    {
        *mOutputStream <<"" << tmp_str << "\n";
    }
    return success;
}


bool LLDataPackerAsciiFile::unpackUUID(LLUUID &value, const char *name)
{
    bool success = true;
    char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
    if (!getValueStr(name, valuestr, DP_BUFSIZE))
    {
        return false;
    }

    char tmp_str[64]; /*Flawfinder: ignore */
    sscanf(valuestr,"%63s",tmp_str);    /* Flawfinder: ignore */
    value.set(tmp_str);

    return success;
}


void LLDataPackerAsciiFile::writeIndentedName(const char *name)
{
    std::string indent_buf;
    indent_buf.reserve(mIndent+1);

    S32 i;
    for(i = 0; i < mIndent; i++)
    {
        indent_buf[i] = '\t';
    }
    indent_buf[i] = 0;
    if (mFP)
    {
        fprintf(mFP,"%s%s\t",indent_buf.c_str(), name);
    }
    else if (mOutputStream)
    {
        *mOutputStream << indent_buf << name << "\t";
    }
}

bool LLDataPackerAsciiFile::getValueStr(const char *name, char *out_value, S32 value_len)
{
    bool success = false;
    char buffer[DP_BUFSIZE]; /*Flawfinder: ignore*/
    char keyword[DP_BUFSIZE]; /*Flawfinder: ignore*/
    char value[DP_BUFSIZE]; /*Flawfinder: ignore*/

    buffer[0] = '\0';
    keyword[0] = '\0';
    value[0] = '\0';

    if (mFP)
    {
        fpos_t last_pos;
        if (0 != fgetpos(mFP, &last_pos)) // 0==success for fgetpos
        {
            LL_WARNS() << "Data packer failed to fgetpos" << LL_ENDL;
            return false;
        }

        if (fgets(buffer, DP_BUFSIZE, mFP) == NULL)
        {
            buffer[0] = '\0';
        }

        sscanf(buffer, "%511s %511[^\n]", keyword, value);  /* Flawfinder: ignore */

        if (!keyword[0])
        {
            LL_WARNS() << "Data packer could not get the keyword!" << LL_ENDL;
            fsetpos(mFP, &last_pos);
            return false;
        }
        if (strcmp(keyword, name))
        {
            LL_WARNS() << "Data packer expecting keyword of type " << name << ", got " << keyword << " instead!" << LL_ENDL;
            fsetpos(mFP, &last_pos);
            return false;
        }

        S32 in_value_len = (S32)strlen(value)+1; /*Flawfinder: ignore*/
        S32 min_len = llmin(in_value_len, value_len);
        memcpy(out_value, value, min_len); /*Flawfinder: ignore*/
        out_value[min_len-1] = 0;
        success = true;
    }
    else if (mInputStream)
    {
        mInputStream->getline(buffer, DP_BUFSIZE);

        sscanf(buffer, "%511s %511[^\n]", keyword, value);  /* Flawfinder: ignore */
        if (!keyword[0])
        {
            LL_WARNS() << "Data packer could not get the keyword!" << LL_ENDL;
            return false;
        }
        if (strcmp(keyword, name))
        {
            LL_WARNS() << "Data packer expecting keyword of type " << name << ", got " << keyword << " instead!" << LL_ENDL;
            return false;
        }

        S32 in_value_len = (S32)strlen(value)+1; /*Flawfinder: ignore*/
        S32 min_len = llmin(in_value_len, value_len);
        memcpy(out_value, value, min_len); /*Flawfinder: ignore*/
        out_value[min_len-1] = 0;
        success = true;
    }

    return success;
}
