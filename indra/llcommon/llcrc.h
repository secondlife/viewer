/** 
 * @file llcrc.h
 * @brief LLCRC class header file.
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
//  LL_INFOS() << "File crc: " << crc.getCRC() << LL_ENDL;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LL_COMMON_API LLCRC
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
	static bool testHarness();
#endif
};


#endif // LL_LLCRC_H
