/** 
 * @file llimagedimentionsinfo.h
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


#ifndef LL_LLIMAGEDIMENSIONSINFO_H
#define LL_LLIMAGEDIMENSIONSINFO_H

//-----------------------------------------------------------------------------
// LLImageDimensionsInfo
// helper class to get image dimensions WITHOUT loading image to memore
// usefull when image may be too large...
//-----------------------------------------------------------------------------
class LLImageDimensionsInfo
{
public:
	LLImageDimensionsInfo():
		mData(NULL)
		,mHeight(0)
		,mWidth(0)
	{}
	~LLImageDimensionsInfo()
	{
		clean();
	}

	bool load(const std::string& src_filename,U32 codec);
	S32 getWidth() const { return mWidth;}
	S32 getHeight() const { return mHeight;}

	const std::string& getLastError()
	{
		return mLastError;
	}
protected:

	void clean()
	{
		mInfile.close();
		delete[] mData;
		mData = NULL;
		mWidth = 0;
		mHeight = 0;
	}

	U8* getData()
	{
		return mData;
	}


	void setLastError(const std::string& message, const std::string& filename)
	{
		std::string error = message;
		if (!filename.empty())
			error += std::string(" FILE: ") + filename;
		mLastError = error;
	}


	bool getImageDimensionsBmp();
	bool getImageDimensionsTga();
	bool getImageDimensionsPng();
	bool getImageDimensionsJpeg();
	
	S32 read_s32()
	{
		char p[4];
		mInfile.read(&p[0],4);
		S32 temp =	(((S32)p[3])       & 0x000000FF) |
					(((S32)p[2] << 8 ) & 0x0000FF00) |
					(((S32)p[1] << 16) & 0x00FF0000) |
					(((S32)p[0] << 24) & 0xFF000000);

		return temp;
	}
	S32 read_reverse_s32()
	{
		char p[4];
		mInfile.read(&p[0],4);
		S32 temp =	(((S32)p[0])       & 0x000000FF) |
					(((S32)p[1] << 8 ) & 0x0000FF00) |
					(((S32)p[2] << 16) & 0x00FF0000) |
					(((S32)p[3] << 24) & 0xFF000000);

		return temp;
	}

	U8 read_byte()
	{
		U8 bt;
		mInfile.read(&bt,1);
		return bt;
	}

	U16 read_short()
	{
		return read_byte() << 8 | read_byte();
	}

	/// Check if the file is not shorter than min_len bytes.
	bool checkFileLength(S32 min_len);

protected:
	LLAPRFile mInfile ;
	std::string mSrcFilename;

	std::string mLastError;

	U8* mData;

	S32 mWidth;
	S32 mHeight;
};
#endif
