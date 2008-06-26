/** 
 * @file llcrc.h
 * @brief LLCRC class header file.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLCRC_H
#define LL_LLCRC_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class llcrc
//
// Simple 32 bit crc. To use, instantiate an LLCRC instance and feed
// it the bytes you want to check. It will update the internal crc as
// you go, and you can qery it at the end. As a horribly inefficient
// example (don't try this at work kids):
//
//  LLCRC crc;
//  FILE* fp = LLFile::fopen(filename,"rb");
//  while(!feof(fp)) {
//    crc.update(fgetc(fp));
//  }
//  fclose(fp);
//  llinfos << "File crc: " << crc.getCRC() << llendl;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLCRC
{
protected:
	U32 mCurrent;
	
public:
	LLCRC();

	U32 getCRC() const;
	void update(U8 next_byte);
	void update(const U8* buffer, size_t buffer_size);
	void update(const std::string& filename);

#ifdef _DEBUG
	// This function runs tests to make sure the crc is
	// working. Returns TRUE if it is.
	static BOOL testHarness();
#endif
};


#endif // LL_LLCRC_H
