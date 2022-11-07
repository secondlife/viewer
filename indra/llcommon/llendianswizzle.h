/** 
 * @file llendianswizzle.h
 * @brief Functions for in-place bit swizzling
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

#ifndef LL_LLENDIANSWIZZLE_H
#define LL_LLENDIANSWIZZLE_H

/* This function is intended to be used for in-place swizzling, particularly after fread() of
    binary values from a file.  Such as:
    
    numRead = fread(scale.mV, sizeof(float), 3, fp);
    llendianswizzle(scale.mV, sizeof(float), 3);
    
    It assumes that the values in the file are LITTLE endian, so it's a no-op on a little endian machine.
    
    It keys off of typesize to do the correct swizzle, so make sure that typesize is the size of the native type.
    
    64-bit types are not yet handled.
*/

#ifdef LL_LITTLE_ENDIAN
    // little endian is native for most things.
    inline void llendianswizzle(void *,int,int)
    {
        // Nothing to do
    }
#endif

#ifdef LL_BIG_ENDIAN
    // big endian requires a bit of work.
    inline void llendianswizzle(void *p,int typesize, int count)
    {
        int i;
        switch(typesize)
        {
            case 2:
            {
                U16 temp;
                for(i=count ;i!=0 ;i--)
                {
                    temp = ((U16*)p)[0];
                    ((U16*)p)[0] =  ((temp >> 8)  & 0x000000FF) | ((temp << 8)  & 0x0000FF00);
                    p = (void*)(((U16*)p) + 1);
                }
            }
            break;
            
            case 4:
            {
                U32 temp;
                for(i=count; i!=0; i--)
                {
                    temp = ((U32*)p)[0];
                    ((U32*)p)[0] =  
                            ((temp >> 24) & 0x000000FF) | 
                            ((temp >> 8)  & 0x0000FF00) | 
                            ((temp << 8)  & 0x00FF0000) |
                            ((temp << 24) & 0xFF000000);
                    p = (void*)(((U32*)p) + 1);
                }
            }
            break;
        }
        
    }
#endif

// Use this when working with a single integral value you want swizzled

#define llendianswizzleone(x) llendianswizzle(&(x), sizeof(x), 1)

#endif // LL_LLENDIANSWIZZLE_H
