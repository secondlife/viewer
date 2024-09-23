/**
 * @file lldatapacker.h
 * @brief Data packer declaration for tightly storing binary data.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLDATAPACKER_H
#define LL_LLDATAPACKER_H

class LLColor4;
class LLColor4U;
class LLVector2;
class LLVector3;
class LLVector4;
class LLUUID;

class LLDataPacker
{
public:
    virtual ~LLDataPacker() {}

    // Not required to override, but error to call?
    virtual void        reset();
    virtual void        dumpBufferToLog();

    virtual bool        hasNext() const = 0;

    virtual bool        packString(const std::string& value, const char *name) = 0;
    virtual bool        unpackString(std::string& value, const char *name) = 0;

    virtual bool        packBinaryData(const U8 *value, S32 size, const char *name) = 0;
    virtual bool        unpackBinaryData(U8 *value, S32 &size, const char *name) = 0;

    // Constant size binary data packing
    virtual bool        packBinaryDataFixed(const U8 *value, S32 size, const char *name) = 0;
    virtual bool        unpackBinaryDataFixed(U8 *value, S32 size, const char *name) = 0;

    virtual bool        packU8(const U8 value, const char *name) = 0;
    virtual bool        unpackU8(U8 &value, const char *name) = 0;

    virtual bool        packU16(const U16 value, const char *name) = 0;
    virtual bool        unpackU16(U16 &value, const char *name) = 0;
    bool                unpackU16s(U16 *value, S32 count, const char *name);

    virtual bool        packS16(const S16 value, const char *name) = 0;
    virtual bool        unpackS16(S16 &value, const char *name) = 0;
    bool                unpackS16s(S16 *value, S32 count, const char *name);

    virtual bool        packU32(const U32 value, const char *name) = 0;
    virtual bool        unpackU32(U32 &value, const char *name) = 0;

    virtual bool        packS32(const S32 value, const char *name) = 0;
    virtual bool        unpackS32(S32 &value, const char *name) = 0;

    virtual bool        packF32(const F32 value, const char *name) = 0;
    virtual bool        unpackF32(F32 &value, const char *name) = 0;
    bool                unpackF32s(F32 *values, S32 count, const char *name);

    // Packs a float into an integer, using the given size
    // and picks the right U* data type to pack into.
    bool                packFixed(const F32 value, const char *name,
                                const bool is_signed, const U32 int_bits, const U32 frac_bits);
    bool                unpackFixed(F32 &value, const char *name,
                                const bool is_signed, const U32 int_bits, const U32 frac_bits);

    virtual bool        packColor4(const LLColor4 &value, const char *name) = 0;
    virtual bool        unpackColor4(LLColor4 &value, const char *name) = 0;

    virtual bool        packColor4U(const LLColor4U &value, const char *name) = 0;
    virtual bool        unpackColor4U(LLColor4U &value, const char *name) = 0;
    bool                unpackColor4Us(LLColor4U *values, S32 count, const char *name);

    virtual bool        packVector2(const LLVector2 &value, const char *name) = 0;
    virtual bool        unpackVector2(LLVector2 &value, const char *name) = 0;

    virtual bool        packVector3(const LLVector3 &value, const char *name) = 0;
    virtual bool        unpackVector3(LLVector3 &value, const char *name) = 0;

    virtual bool        packVector4(const LLVector4 &value, const char *name) = 0;
    virtual bool        unpackVector4(LLVector4 &value, const char *name) = 0;

    virtual bool        packUUID(const LLUUID &value, const char *name) = 0;
    virtual bool        unpackUUID(LLUUID &value, const char *name) = 0;
    bool                unpackUUIDs(LLUUID *values, S32 count, const char *name);
            U32         getPassFlags() const    { return mPassFlags; }
            void        setPassFlags(U32 flags) { mPassFlags = flags; }
protected:
    LLDataPacker();
protected:
    U32 mPassFlags;
    bool mWriteEnabled; // disable this to do things like determine filesize without actually copying data
};

class LLDataPackerBinaryBuffer : public LLDataPacker
{
public:
    LLDataPackerBinaryBuffer(U8 *bufferp, S32 size)
    :   LLDataPacker(),
        mBufferp(bufferp),
        mCurBufferp(bufferp),
        mBufferSize(size)
    {
        mWriteEnabled = true;
    }

    LLDataPackerBinaryBuffer()
    :   LLDataPacker(),
        mBufferp(NULL),
        mCurBufferp(NULL),
        mBufferSize(0)
    {
    }

    /*virtual*/ bool        packString(const std::string& value, const char *name);
    /*virtual*/ bool        unpackString(std::string& value, const char *name);

    /*virtual*/ bool        packBinaryData(const U8 *value, S32 size, const char *name);
    /*virtual*/ bool        unpackBinaryData(U8 *value, S32 &size, const char *name);

    // Constant size binary data packing
    /*virtual*/ bool        packBinaryDataFixed(const U8 *value, S32 size, const char *name);
    /*virtual*/ bool        unpackBinaryDataFixed(U8 *value, S32 size, const char *name);

    /*virtual*/ bool        packU8(const U8 value, const char *name);
    /*virtual*/ bool        unpackU8(U8 &value, const char *name);

    /*virtual*/ bool        packU16(const U16 value, const char *name);
    /*virtual*/ bool        unpackU16(U16 &value, const char *name);

    /*virtual*/ bool        packS16(const S16 value, const char *name);
    /*virtual*/ bool        unpackS16(S16 &value, const char *name);

    /*virtual*/ bool        packU32(const U32 value, const char *name);
    /*virtual*/ bool        unpackU32(U32 &value, const char *name);

    /*virtual*/ bool        packS32(const S32 value, const char *name);
    /*virtual*/ bool        unpackS32(S32 &value, const char *name);

    /*virtual*/ bool        packF32(const F32 value, const char *name);
    /*virtual*/ bool        unpackF32(F32 &value, const char *name);

    /*virtual*/ bool        packColor4(const LLColor4 &value, const char *name);
    /*virtual*/ bool        unpackColor4(LLColor4 &value, const char *name);

    /*virtual*/ bool        packColor4U(const LLColor4U &value, const char *name);
    /*virtual*/ bool        unpackColor4U(LLColor4U &value, const char *name);

    /*virtual*/ bool        packVector2(const LLVector2 &value, const char *name);
    /*virtual*/ bool        unpackVector2(LLVector2 &value, const char *name);

    /*virtual*/ bool        packVector3(const LLVector3 &value, const char *name);
    /*virtual*/ bool        unpackVector3(LLVector3 &value, const char *name);

    /*virtual*/ bool        packVector4(const LLVector4 &value, const char *name);
    /*virtual*/ bool        unpackVector4(LLVector4 &value, const char *name);

    /*virtual*/ bool        packUUID(const LLUUID &value, const char *name);
    /*virtual*/ bool        unpackUUID(LLUUID &value, const char *name);

                S32         getCurrentSize() const  { return (S32)(mCurBufferp - mBufferp); }
                S32         getBufferSize() const   { return mBufferSize; }
                const U8*   getBuffer() const   { return mBufferp; }
                void        reset()             { mCurBufferp = mBufferp; mWriteEnabled = (mCurBufferp != NULL); }
                void        shift(S32 offset)   { reset(); mCurBufferp += offset;}
                void        freeBuffer()        { delete [] mBufferp; mBufferp = mCurBufferp = NULL; mBufferSize = 0; mWriteEnabled = false; }
                void        assignBuffer(U8 *bufferp, S32 size)
                {
                    if(mBufferp && mBufferp != bufferp)
                    {
                        freeBuffer() ;
                    }
                    mBufferp = bufferp;
                    mCurBufferp = bufferp;
                    mBufferSize = size;
                    mWriteEnabled = true;
                }
                const LLDataPackerBinaryBuffer& operator=(const LLDataPackerBinaryBuffer &a);

    /*virtual*/ bool        hasNext() const         { return getCurrentSize() < getBufferSize(); }

    /*virtual*/ void dumpBufferToLog();
protected:
    inline bool verifyLength(const S32 data_size, const char *name);

    U8 *mBufferp;
    U8 *mCurBufferp;
    S32 mBufferSize;
};

inline bool LLDataPackerBinaryBuffer::verifyLength(const S32 data_size, const char *name)
{
    if (mWriteEnabled && (mCurBufferp - mBufferp) > mBufferSize - data_size)
    {
        LL_WARNS() << "Buffer overflow in BinaryBuffer length verify, field name " << name << "!" << LL_ENDL;
        LL_WARNS() << "Current pos: " << (int)(mCurBufferp - mBufferp) << " Buffer size: " << mBufferSize << " Data size: " << data_size << LL_ENDL;
        return false;
    }

    return true;
}

class LLDataPackerAsciiBuffer : public LLDataPacker
{
public:
    LLDataPackerAsciiBuffer(char* bufferp, S32 size)
    {
        mBufferp = bufferp;
        mCurBufferp = bufferp;
        mBufferSize = size;
        mPassFlags = 0;
        mIncludeNames = false;
        mWriteEnabled = true;
    }

    LLDataPackerAsciiBuffer()
    {
        mBufferp = NULL;
        mCurBufferp = NULL;
        mBufferSize = 0;
        mPassFlags = 0;
        mIncludeNames = false;
        mWriteEnabled = false;
    }

    /*virtual*/ bool        packString(const std::string& value, const char *name);
    /*virtual*/ bool        unpackString(std::string& value, const char *name);

    /*virtual*/ bool        packBinaryData(const U8 *value, S32 size, const char *name);
    /*virtual*/ bool        unpackBinaryData(U8 *value, S32 &size, const char *name);

    // Constant size binary data packing
    /*virtual*/ bool        packBinaryDataFixed(const U8 *value, S32 size, const char *name);
    /*virtual*/ bool        unpackBinaryDataFixed(U8 *value, S32 size, const char *name);

    /*virtual*/ bool        packU8(const U8 value, const char *name);
    /*virtual*/ bool        unpackU8(U8 &value, const char *name);

    /*virtual*/ bool        packU16(const U16 value, const char *name);
    /*virtual*/ bool        unpackU16(U16 &value, const char *name);

    /*virtual*/ bool        packS16(const S16 value, const char *name);
    /*virtual*/ bool        unpackS16(S16 &value, const char *name);

    /*virtual*/ bool        packU32(const U32 value, const char *name);
    /*virtual*/ bool        unpackU32(U32 &value, const char *name);

    /*virtual*/ bool        packS32(const S32 value, const char *name);
    /*virtual*/ bool        unpackS32(S32 &value, const char *name);

    /*virtual*/ bool        packF32(const F32 value, const char *name);
    /*virtual*/ bool        unpackF32(F32 &value, const char *name);

    /*virtual*/ bool        packColor4(const LLColor4 &value, const char *name);
    /*virtual*/ bool        unpackColor4(LLColor4 &value, const char *name);

    /*virtual*/ bool        packColor4U(const LLColor4U &value, const char *name);
    /*virtual*/ bool        unpackColor4U(LLColor4U &value, const char *name);

    /*virtual*/ bool        packVector2(const LLVector2 &value, const char *name);
    /*virtual*/ bool        unpackVector2(LLVector2 &value, const char *name);

    /*virtual*/ bool        packVector3(const LLVector3 &value, const char *name);
    /*virtual*/ bool        unpackVector3(LLVector3 &value, const char *name);

    /*virtual*/ bool        packVector4(const LLVector4 &value, const char *name);
    /*virtual*/ bool        unpackVector4(LLVector4 &value, const char *name);

    /*virtual*/ bool        packUUID(const LLUUID &value, const char *name);
    /*virtual*/ bool        unpackUUID(LLUUID &value, const char *name);

    void        setIncludeNames(bool b) { mIncludeNames = b; }

    // Include the trailing NULL so it's always a valid string
    S32         getCurrentSize() const  { return (S32)(mCurBufferp - mBufferp) + 1; }

    S32         getBufferSize() const   { return mBufferSize; }
    /*virtual*/ void        reset()                 { mCurBufferp = mBufferp; mWriteEnabled = (mCurBufferp != NULL); }

    /*virtual*/ bool        hasNext() const         { return getCurrentSize() < getBufferSize(); }

    inline void freeBuffer();
    inline void assignBuffer(char* bufferp, S32 size);
    void        dump();

protected:
    void writeIndentedName(const char *name);
    bool getValueStr(const char *name, char *out_value, const S32 value_len);

protected:
    inline bool verifyLength(const S32 data_size, const char *name);

    char *mBufferp;
    char *mCurBufferp;
    S32 mBufferSize;
    bool mIncludeNames; // useful for debugging, print the name of each field
};

inline void LLDataPackerAsciiBuffer::freeBuffer()
{
    delete [] mBufferp;
    mBufferp = mCurBufferp = NULL;
    mBufferSize = 0;
    mWriteEnabled = false;
}

inline void LLDataPackerAsciiBuffer::assignBuffer(char* bufferp, S32 size)
{
    mBufferp = bufferp;
    mCurBufferp = bufferp;
    mBufferSize = size;
    mWriteEnabled = true;
}

inline bool LLDataPackerAsciiBuffer::verifyLength(const S32 data_size, const char *name)
{
    if (mWriteEnabled && (mCurBufferp - mBufferp) > mBufferSize - data_size)
    {
        LL_WARNS() << "Buffer overflow in AsciiBuffer length verify, field name " << name << "!" << LL_ENDL;
        LL_WARNS() << "Current pos: " << (int)(mCurBufferp - mBufferp) << " Buffer size: " << mBufferSize << " Data size: " << data_size << LL_ENDL;
        return false;
    }

    return true;
}

class LLDataPackerAsciiFile : public LLDataPacker
{
public:
    LLDataPackerAsciiFile(LLFILE *fp, const S32 indent = 2)
    :   LLDataPacker(),
        mIndent(indent),
        mFP(fp),
        mOutputStream(NULL),
        mInputStream(NULL)
    {
    }

    LLDataPackerAsciiFile(std::ostream& output_stream, const S32 indent = 2)
    :   LLDataPacker(),
        mIndent(indent),
        mFP(NULL),
        mOutputStream(&output_stream),
        mInputStream(NULL)
    {
        mWriteEnabled = true;
    }

    LLDataPackerAsciiFile(std::istream& input_stream, const S32 indent = 2)
    :   LLDataPacker(),
        mIndent(indent),
        mFP(NULL),
        mOutputStream(NULL),
        mInputStream(&input_stream)
    {
    }

    /*virtual*/ bool        packString(const std::string& value, const char *name);
    /*virtual*/ bool        unpackString(std::string& value, const char *name);

    /*virtual*/ bool        packBinaryData(const U8 *value, S32 size, const char *name);
    /*virtual*/ bool        unpackBinaryData(U8 *value, S32 &size, const char *name);

    /*virtual*/ bool        packBinaryDataFixed(const U8 *value, S32 size, const char *name);
    /*virtual*/ bool        unpackBinaryDataFixed(U8 *value, S32 size, const char *name);

    /*virtual*/ bool        packU8(const U8 value, const char *name);
    /*virtual*/ bool        unpackU8(U8 &value, const char *name);

    /*virtual*/ bool        packU16(const U16 value, const char *name);
    /*virtual*/ bool        unpackU16(U16 &value, const char *name);

    /*virtual*/ bool        packS16(const S16 value, const char *name);
    /*virtual*/ bool        unpackS16(S16 &value, const char *name);

    /*virtual*/ bool        packU32(const U32 value, const char *name);
    /*virtual*/ bool        unpackU32(U32 &value, const char *name);

    /*virtual*/ bool        packS32(const S32 value, const char *name);
    /*virtual*/ bool        unpackS32(S32 &value, const char *name);

    /*virtual*/ bool        packF32(const F32 value, const char *name);
    /*virtual*/ bool        unpackF32(F32 &value, const char *name);

    /*virtual*/ bool        packColor4(const LLColor4 &value, const char *name);
    /*virtual*/ bool        unpackColor4(LLColor4 &value, const char *name);

    /*virtual*/ bool        packColor4U(const LLColor4U &value, const char *name);
    /*virtual*/ bool        unpackColor4U(LLColor4U &value, const char *name);

    /*virtual*/ bool        packVector2(const LLVector2 &value, const char *name);
    /*virtual*/ bool        unpackVector2(LLVector2 &value, const char *name);

    /*virtual*/ bool        packVector3(const LLVector3 &value, const char *name);
    /*virtual*/ bool        unpackVector3(LLVector3 &value, const char *name);

    /*virtual*/ bool        packVector4(const LLVector4 &value, const char *name);
    /*virtual*/ bool        unpackVector4(LLVector4 &value, const char *name);

    /*virtual*/ bool        packUUID(const LLUUID &value, const char *name);
    /*virtual*/ bool        unpackUUID(LLUUID &value, const char *name);
protected:
    void writeIndentedName(const char *name);
    bool getValueStr(const char *name, char *out_value, const S32 value_len);

    /*virtual*/ bool        hasNext() const         { return true; }

protected:
    S32 mIndent;
    LLFILE *mFP;
    std::ostream* mOutputStream;
    std::istream* mInputStream;
};

#endif // LL_LLDATAPACKER

