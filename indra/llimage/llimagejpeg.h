/** 
 * @file llimagejpeg.h
 * @brief This class compresses and decompresses JPEG files
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

#ifndef LL_LLIMAGEJPEG_H
#define LL_LLIMAGEJPEG_H

#include <csetjmp>

#include "llimage.h"

#include "llwin32headerslean.h"
extern "C" {
#ifdef LL_USESYSTEMLIBS
# include <jpeglib.h>
# include <jerror.h>
#else
# include "jpeglib/jpeglib.h"
# include "jpeglib/jerror.h"
#endif
}

class LLImageJPEG : public LLImageFormatted
{
protected:
    virtual ~LLImageJPEG();
    
public:
    LLImageJPEG(S32 quality = 75);

    /*virtual*/ std::string getExtension() { return std::string("jpg"); }
    /*virtual*/ bool updateData();
    /*virtual*/ bool decode(LLImageRaw* raw_image, F32 decode_time);
    /*virtual*/ bool encode(const LLImageRaw* raw_image, F32 encode_time);

    void            setEncodeQuality( S32 q )   { mEncodeQuality = q; } // on a scale from 1 to 100
    S32             getEncodeQuality()          { return mEncodeQuality; }

    // Callbacks registered with jpeglib
    static void     encodeInitDestination ( j_compress_ptr cinfo );
    static boolean  encodeEmptyOutputBuffer(j_compress_ptr cinfo);
    static void     encodeTermDestination(j_compress_ptr cinfo);

    static void     decodeInitSource(j_decompress_ptr cinfo);
    static boolean  decodeFillInputBuffer(j_decompress_ptr cinfo);
    static void     decodeSkipInputData(j_decompress_ptr cinfo, long num_bytes);
    static void     decodeTermSource(j_decompress_ptr cinfo);


    static void     errorExit(j_common_ptr cinfo);
    static void     errorEmitMessage(j_common_ptr cinfo, int msg_level);
    static void     errorOutputMessage(j_common_ptr cinfo);

    static bool     decompress(LLImageJPEG* imagep);

protected:
    U8*             mOutputBuffer;      // temp buffer used during encoding
    S32             mOutputBufferSize;  // bytes in mOuputBuffer

    S32             mEncodeQuality;     // on a scale from 1 to 100
private:
    static jmp_buf  sSetjmpBuffer;      // To allow the library to abort.
};

#endif  // LL_LLIMAGEJPEG_H
