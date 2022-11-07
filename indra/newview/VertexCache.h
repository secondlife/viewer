/** 
 * @file VertexCache.h
 * @brief VertexCache class definition
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
