/** 
 * @file llendianswizzle.h
 * @brief Functions for in-place bit swizzling
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
					((U16*)p)[0] = 	((temp >> 8)  & 0x000000FF) | ((temp << 8)  & 0x0000FF00);
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
