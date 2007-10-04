/** 
 * @file VertexCache.h
 * @brief VertexCache class definition
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

  
#ifndef VERTEX_CACHE_H

#define VERTEX_CACHE_H

class VertexCache
{
	
public:
	
	VertexCache(int size)
	{
		numEntries = size;
		
		entries = new int[numEntries];
		
		for(int i = 0; i < numEntries; i++)
			entries[i] = -1;
	}
		
	VertexCache() { VertexCache(16); }
	~VertexCache() { delete[] entries; entries = 0; }
	
	bool InCache(int entry)
	{
		bool returnVal = false;
		for(int i = 0; i < numEntries; i++)
		{
			if(entries[i] == entry)
			{
				returnVal = true;
				break;
			}
		}
		
		return returnVal;
	}
	
	int AddEntry(int entry)
	{
		int removed;
		
		removed = entries[numEntries - 1];
		
		//push everything right one
		for(int i = numEntries - 2; i >= 0; i--)
		{
			entries[i + 1] = entries[i];
		}
		
		entries[0] = entry;
		
		return removed;
	}

	void Clear()
	{
		memset(entries, -1, sizeof(int) * numEntries);
	}
	
	void Copy(VertexCache* inVcache) 
	{
		for(int i = 0; i < numEntries; i++)
		{
			inVcache->Set(i, entries[i]);
		}
	}

	int At(int index) { return entries[index]; }
	void Set(int index, int value) { entries[index] = value; }

private:

  int *entries;
  int numEntries;

};

#endif
