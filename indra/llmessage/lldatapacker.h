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
	virtual void		reset();
	virtual void		dumpBufferToLog();

	virtual BOOL		hasNext() const = 0;

	virtual BOOL		packString(const std::string& value, const char *name) = 0;
	virtual BOOL		unpackString(std::string& value, const char *name) = 0;

	virtual BOOL		packBinaryData(const U8 *value, S32 size, const char *name) = 0;
	virtual BOOL		unpackBinaryData(U8 *value, S32 &size, const char *name) = 0;

	// Constant size binary data packing
	virtual BOOL		packBinaryDataFixed(const U8 *value, S32 size, const char *name) = 0;
	virtual BOOL		unpackBinaryDataFixed(U8 *value, S32 size, const char *name) = 0;

	virtual BOOL		packU8(const U8 value, const char *name) = 0;
	virtual BOOL		unpackU8(U8 &value, const char *name) = 0;

	virtual BOOL		packU16(const U16 value, const char *name) = 0;
	virtual BOOL		unpackU16(U16 &value, const char *name) = 0;

	virtual BOOL		packU32(const U32 value, const char *name) = 0;
	virtual BOOL		unpackU32(U32 &value, const char *name) = 0;

	virtual BOOL		packS32(const S32 value, const char *name) = 0;
	virtual BOOL		unpackS32(S32 &value, const char *name) = 0;

	virtual BOOL		packF32(const F32 value, const char *name) = 0;
	virtual BOOL		unpackF32(F32 &value, const char *name) = 0;

	// Packs a float into an integer, using the given size
	// and picks the right U* data type to pack into.
	BOOL				packFixed(const F32 value, const char *name,
								const BOOL is_signed, const U32 int_bits, const U32 frac_bits);
	BOOL				unpackFixed(F32 &value, const char *name,
								const BOOL is_signed, const U32 int_bits, const U32 frac_bits);

	virtual BOOL		packColor4(const LLColor4 &value, const char *name) = 0;
	virtual BOOL		unpackColor4(LLColor4 &value, const char *name) = 0;

	virtual BOOL		packColor4U(const LLColor4U &value, const char *name) = 0;
	virtual BOOL		unpackColor4U(LLColor4U &value, const char *name) = 0;

	virtual BOOL		packVector2(const LLVector2 &value, const char *name) = 0;
	virtual BOOL		unpackVector2(LLVector2 &value, const char *name) = 0;

	virtual BOOL		packVector3(const LLVector3 &value, const char *name) = 0;
	virtual BOOL		unpackVector3(LLVector3 &value, const char *name) = 0;

	virtual BOOL		packVector4(const LLVector4 &value, const char *name) = 0;
	virtual BOOL		unpackVector4(LLVector4 &value, const char *name) = 0;

	virtual BOOL		packUUID(const LLUUID &value, const char *name) = 0;
	virtual BOOL		unpackUUID(LLUUID &value, const char *name) = 0;
			U32			getPassFlags() const	{ return mPassFlags; }
			void		setPassFlags(U32 flags)	{ mPassFlags = flags; }
protected:
	LLDataPacker();
protected:
	U32	mPassFlags;
	BOOL mWriteEnabled; // disable this to do things like determine filesize without actually copying data
};

class LLDataPackerBinaryBuffer : public LLDataPacker
{
public:
	LLDataPackerBinaryBuffer(U8 *bufferp, S32 size)
	:	LLDataPacker(),
		mBufferp(bufferp),
		mCurBufferp(bufferp),
		mBufferSize(size)
	{
		mWriteEnabled = TRUE;
	}

	LLDataPackerBinaryBuffer()
	:	LLDataPacker(),
		mBufferp(NULL),
		mCurBufferp(NULL),
		mBufferSize(0)
	{
	}

	/*virtual*/ BOOL		packString(const std::string& value, const char *name);
	/*virtual*/ BOOL		unpackString(std::string& value, const char *name);

	/*virtual*/ BOOL		packBinaryData(const U8 *value, S32 size, const char *name);
	/*virtual*/ BOOL		unpackBinaryData(U8 *value, S32 &size, const char *name);

	// Constant size binary data packing
	/*virtual*/ BOOL		packBinaryDataFixed(const U8 *value, S32 size, const char *name);
	/*virtual*/ BOOL		unpackBinaryDataFixed(U8 *value, S32 size, const char *name);

	/*virtual*/ BOOL		packU8(const U8 value, const char *name);
	/*virtual*/ BOOL		unpackU8(U8 &value, const char *name);

	/*virtual*/ BOOL		packU16(const U16 value, const char *name);
	/*virtual*/ BOOL		unpackU16(U16 &value, const char *name);

	/*virtual*/ BOOL		packU32(const U32 value, const char *name);
	/*virtual*/ BOOL		unpackU32(U32 &value, const char *name);

	/*virtual*/ BOOL		packS32(const S32 value, const char *name);
	/*virtual*/ BOOL		unpackS32(S32 &value, const char *name);

	/*virtual*/ BOOL		packF32(const F32 value, const char *name);
	/*virtual*/ BOOL		unpackF32(F32 &value, const char *name);

	/*virtual*/ BOOL		packColor4(const LLColor4 &value, const char *name);
	/*virtual*/ BOOL		unpackColor4(LLColor4 &value, const char *name);

	/*virtual*/ BOOL		packColor4U(const LLColor4U &value, const char *name);
	/*virtual*/ BOOL		unpackColor4U(LLColor4U &value, const char *name);

	/*virtual*/ BOOL		packVector2(const LLVector2 &value, const char *name);
	/*virtual*/ BOOL		unpackVector2(LLVector2 &value, const char *name);

	/*virtual*/ BOOL		packVector3(const LLVector3 &value, const char *name);
	/*virtual*/ BOOL		unpackVector3(LLVector3 &value, const char *name);

	/*virtual*/ BOOL		packVector4(const LLVector4 &value, const char *name);
	/*virtual*/ BOOL		unpackVector4(LLVector4 &value, const char *name);

	/*virtual*/ BOOL		packUUID(const LLUUID &value, const char *name);
	/*virtual*/ BOOL		unpackUUID(LLUUID &value, const char *name);

				S32			getCurrentSize() const	{ return (S32)(mCurBufferp - mBufferp); }
				S32			getBufferSize() const	{ return mBufferSize; }
				const U8*   getBuffer() const   { return mBufferp; }    
				void		reset()				{ mCurBufferp = mBufferp; mWriteEnabled = (mCurBufferp != NULL); }
				void		freeBuffer()		{ delete [] mBufferp; mBufferp = mCurBufferp = NULL; mBufferSize = 0; mWriteEnabled = FALSE; }
				void		assignBuffer(U8 *bufferp, S32 size)
				{
					if(mBufferp && mBufferp != bufferp)
					{
						freeBuffer() ;
					}
					mBufferp = bufferp;
					mCurBufferp = bufferp;
					mBufferSize = size;
					mWriteEnabled = TRUE;
				}
				const LLDataPackerBinaryBuffer&	operator=(const LLDataPackerBinaryBuffer &a);

	/*virtual*/ BOOL		hasNext() const			{ return getCurrentSize() < getBufferSize(); }

	/*virtual*/ void dumpBufferToLog();
protected:
	inline BOOL verifyLength(const S32 data_size, const char *name);

	U8 *mBufferp;
	U8 *mCurBufferp;
	S32 mBufferSize;
};

inline BOOL LLDataPackerBinaryBuffer::verifyLength(const S32 data_size, const char *name)
{
	if (mWriteEnabled && (mCurBufferp - mBufferp) > mBufferSize - data_size)
	{
		llwarns << "Buffer overflow in BinaryBuffer length verify, field name " << name << "!" << llendl;
		llwarns << "Current pos: " << (int)(mCurBufferp - mBufferp) << " Buffer size: " << mBufferSize << " Data size: " << data_size << llendl;
		return FALSE;
	}

	return TRUE;
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
		mIncludeNames = FALSE;
		mWriteEnabled = TRUE;
	}

	LLDataPackerAsciiBuffer()
	{
		mBufferp = NULL;
		mCurBufferp = NULL;
		mBufferSize = 0;
		mPassFlags = 0;
		mIncludeNames = FALSE;
		mWriteEnabled = FALSE;
	}

	/*virtual*/ BOOL		packString(const std::string& value, const char *name);
	/*virtual*/ BOOL		unpackString(std::string& value, const char *name);

	/*virtual*/ BOOL		packBinaryData(const U8 *value, S32 size, const char *name);
	/*virtual*/ BOOL		unpackBinaryData(U8 *value, S32 &size, const char *name);

	// Constant size binary data packing
	/*virtual*/ BOOL		packBinaryDataFixed(const U8 *value, S32 size, const char *name);
	/*virtual*/ BOOL		unpackBinaryDataFixed(U8 *value, S32 size, const char *name);

	/*virtual*/ BOOL		packU8(const U8 value, const char *name);
	/*virtual*/ BOOL		unpackU8(U8 &value, const char *name);

	/*virtual*/ BOOL		packU16(const U16 value, const char *name);
	/*virtual*/ BOOL		unpackU16(U16 &value, const char *name);

	/*virtual*/ BOOL		packU32(const U32 value, const char *name);
	/*virtual*/ BOOL		unpackU32(U32 &value, const char *name);

	/*virtual*/ BOOL		packS32(const S32 value, const char *name);
	/*virtual*/ BOOL		unpackS32(S32 &value, const char *name);

	/*virtual*/ BOOL		packF32(const F32 value, const char *name);
	/*virtual*/ BOOL		unpackF32(F32 &value, const char *name);

	/*virtual*/ BOOL		packColor4(const LLColor4 &value, const char *name);
	/*virtual*/ BOOL		unpackColor4(LLColor4 &value, const char *name);

	/*virtual*/ BOOL		packColor4U(const LLColor4U &value, const char *name);
	/*virtual*/ BOOL		unpackColor4U(LLColor4U &value, const char *name);

	/*virtual*/ BOOL		packVector2(const LLVector2 &value, const char *name);
	/*virtual*/ BOOL		unpackVector2(LLVector2 &value, const char *name);

	/*virtual*/ BOOL		packVector3(const LLVector3 &value, const char *name);
	/*virtual*/ BOOL		unpackVector3(LLVector3 &value, const char *name);

	/*virtual*/ BOOL		packVector4(const LLVector4 &value, const char *name);
	/*virtual*/ BOOL		unpackVector4(LLVector4 &value, const char *name);

	/*virtual*/ BOOL		packUUID(const LLUUID &value, const char *name);
	/*virtual*/ BOOL		unpackUUID(LLUUID &value, const char *name);

	void		setIncludeNames(BOOL b)	{ mIncludeNames = b; }

	// Include the trailing NULL so it's always a valid string
	S32			getCurrentSize() const	{ return (S32)(mCurBufferp - mBufferp) + 1; }

	S32			getBufferSize() const	{ return mBufferSize; }
	/*virtual*/ void		reset()					{ mCurBufferp = mBufferp; mWriteEnabled = (mCurBufferp != NULL); }

	/*virtual*/ BOOL		hasNext() const			{ return getCurrentSize() < getBufferSize(); }

	inline void	freeBuffer();
	inline void	assignBuffer(char* bufferp, S32 size);
	void		dump();

protected:
	void writeIndentedName(const char *name);
	BOOL getValueStr(const char *name, char *out_value, const S32 value_len);
	
protected:
	inline BOOL verifyLength(const S32 data_size, const char *name);

	char *mBufferp;
	char *mCurBufferp;
	S32 mBufferSize;
	BOOL mIncludeNames;	// useful for debugging, print the name of each field
};

inline void	LLDataPackerAsciiBuffer::freeBuffer()
{
	delete [] mBufferp; 
	mBufferp = mCurBufferp = NULL; 
	mBufferSize = 0;
	mWriteEnabled = FALSE;
}

inline void	LLDataPackerAsciiBuffer::assignBuffer(char* bufferp, S32 size)
{
	mBufferp = bufferp;
	mCurBufferp = bufferp;
	mBufferSize = size;
	mWriteEnabled = TRUE;
}

inline BOOL LLDataPackerAsciiBuffer::verifyLength(const S32 data_size, const char *name)
{
	if (mWriteEnabled && (mCurBufferp - mBufferp) > mBufferSize - data_size)
	{
		llwarns << "Buffer overflow in AsciiBuffer length verify, field name " << name << "!" << llendl;
		llwarns << "Current pos: " << (int)(mCurBufferp - mBufferp) << " Buffer size: " << mBufferSize << " Data size: " << data_size << llendl;
		return FALSE;
	}

	return TRUE;
}

class LLDataPackerAsciiFile : public LLDataPacker
{
public:
	LLDataPackerAsciiFile(LLFILE *fp, const S32 indent = 2)
	: 	LLDataPacker(),
		mIndent(indent),
		mFP(fp),
		mOutputStream(NULL),
		mInputStream(NULL)
	{
	}

	LLDataPackerAsciiFile(std::ostream& output_stream, const S32 indent = 2)
	: 	LLDataPacker(),
		mIndent(indent),
		mFP(NULL),
		mOutputStream(&output_stream),
		mInputStream(NULL)
	{
		mWriteEnabled = TRUE;
	}

	LLDataPackerAsciiFile(std::istream& input_stream, const S32 indent = 2)
	: 	LLDataPacker(),
		mIndent(indent),
		mFP(NULL),
		mOutputStream(NULL),
		mInputStream(&input_stream)
	{
	}

	/*virtual*/ BOOL		packString(const std::string& value, const char *name);
	/*virtual*/ BOOL		unpackString(std::string& value, const char *name);

	/*virtual*/ BOOL		packBinaryData(const U8 *value, S32 size, const char *name);
	/*virtual*/ BOOL		unpackBinaryData(U8 *value, S32 &size, const char *name);

	/*virtual*/ BOOL		packBinaryDataFixed(const U8 *value, S32 size, const char *name);
	/*virtual*/ BOOL		unpackBinaryDataFixed(U8 *value, S32 size, const char *name);

	/*virtual*/ BOOL		packU8(const U8 value, const char *name);
	/*virtual*/ BOOL		unpackU8(U8 &value, const char *name);

	/*virtual*/ BOOL		packU16(const U16 value, const char *name);
	/*virtual*/ BOOL		unpackU16(U16 &value, const char *name);

	/*virtual*/ BOOL		packU32(const U32 value, const char *name);
	/*virtual*/ BOOL		unpackU32(U32 &value, const char *name);

	/*virtual*/ BOOL		packS32(const S32 value, const char *name);
	/*virtual*/ BOOL		unpackS32(S32 &value, const char *name);

	/*virtual*/ BOOL		packF32(const F32 value, const char *name);
	/*virtual*/ BOOL		unpackF32(F32 &value, const char *name);

	/*virtual*/ BOOL		packColor4(const LLColor4 &value, const char *name);
	/*virtual*/ BOOL		unpackColor4(LLColor4 &value, const char *name);

	/*virtual*/ BOOL		packColor4U(const LLColor4U &value, const char *name);
	/*virtual*/ BOOL		unpackColor4U(LLColor4U &value, const char *name);

	/*virtual*/ BOOL		packVector2(const LLVector2 &value, const char *name);
	/*virtual*/ BOOL		unpackVector2(LLVector2 &value, const char *name);

	/*virtual*/ BOOL		packVector3(const LLVector3 &value, const char *name);
	/*virtual*/ BOOL		unpackVector3(LLVector3 &value, const char *name);

	/*virtual*/ BOOL		packVector4(const LLVector4 &value, const char *name);
	/*virtual*/ BOOL		unpackVector4(LLVector4 &value, const char *name);

	/*virtual*/ BOOL		packUUID(const LLUUID &value, const char *name);
	/*virtual*/ BOOL		unpackUUID(LLUUID &value, const char *name);
protected:
	void writeIndentedName(const char *name);
	BOOL getValueStr(const char *name, char *out_value, const S32 value_len);
	
	/*virtual*/ BOOL		hasNext() const			{ return true; }

protected:
	S32 mIndent;
	LLFILE *mFP;
	std::ostream* mOutputStream;
	std::istream* mInputStream;
};

#endif // LL_LLDATAPACKER

