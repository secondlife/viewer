/** 
 * @file llimagedimentionsinfo.h
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

protected:
	LLAPRFile mInfile ;
	std::string mSrcFilename;

	std::string mLastError;

	U8* mData;

	S32 mWidth;
	S32 mHeight;
};
#endif
