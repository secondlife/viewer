/**
 * @file llimagedxt.h
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

#ifndef LL_LLIMAGEDXT_H
#define LL_LLIMAGEDXT_H

#include "llimage.h"
#include "llpointer.h"

// This class decodes and encodes LL DXT files (which may unclude uncompressed RGB or RGBA mipped data)

class LLImageDXT : public LLImageFormatted
{
public:
    enum EFileFormat
    {
        FORMAT_UNKNOWN = 0,
        FORMAT_I8 = 1,
        FORMAT_A8,
        FORMAT_RGB8,
        FORMAT_RGBA8,
        FORMAT_DXT1,
        FORMAT_DXT2,
        FORMAT_DXT3,
        FORMAT_DXT4,
        FORMAT_DXT5,
        FORMAT_DXR1,
        FORMAT_DXR2,
        FORMAT_DXR3,
        FORMAT_DXR4,
        FORMAT_DXR5,
        FORMAT_NOFILE = 0xff,
    };

    struct dxtfile_header_old_t
    {
        S32 format;
        S32 maxlevel;
        S32 maxwidth;
        S32 maxheight;
    };

    struct dxtfile_header_t
    {
        S32 fourcc;
        // begin DDSURFACEDESC2 struct
        S32 header_size; // size of the header
        S32 flags; // flags - unused
        S32 maxheight;
        S32 maxwidth;
        S32 image_size; // size of the compressed image
        S32 depth;
        S32 num_mips;
        S32 reserved[11];
        struct pixel_format
        {
            S32 struct_size; // size of this structure
            S32 flags;
            S32 fourcc;
            S32 bit_count;
            S32 r_mask;
            S32 g_mask;
            S32 b_mask;
            S32 a_mask;
        } pixel_fmt;
        S32 caps[4];
        S32 reserved2;
    };

protected:
    /*virtual*/ ~LLImageDXT();

private:
    bool encodeDXT(const LLImageRaw* raw_image, F32 decode_time, bool explicit_mips);

public:
    LLImageDXT();

    /*virtual*/ std::string getExtension() { return std::string("dxt"); }
    /*virtual*/ bool updateData();

    /*virtual*/ bool decode(LLImageRaw* raw_image, F32 decode_time);
    /*virtual*/ bool encode(const LLImageRaw* raw_image, F32 encode_time);

    /*virtual*/ S32 calcHeaderSize();
    /*virtual*/ S32 calcDataSize(S32 discard_level = 0);

    bool getMipData(LLPointer<LLImageRaw>& raw, S32 discard=-1);

    void setFormat();
    S32 getMipOffset(S32 discard);

    EFileFormat getFileFormat() { return mFileFormat; }
    bool isCompressed() { return (mFileFormat >= FORMAT_DXT1 && mFileFormat <= FORMAT_DXR5); }

    bool convertToDXR(); // convert from DXT to DXR

    static void checkMinWidthHeight(EFileFormat format, S32& width, S32& height);
    static S32 formatBits(EFileFormat format);
    static S32 formatBytes(EFileFormat format, S32 width, S32 height);
    static S32 formatOffset(EFileFormat format, S32 width, S32 height, S32 max_width, S32 max_height);
    static S32 formatComponents(EFileFormat format);

    static EFileFormat getFormat(S32 fourcc);
    static S32 getFourCC(EFileFormat format);

    static void calcDiscardWidthHeight(S32 discard_level, EFileFormat format, S32& width, S32& height);
    static S32 calcNumMips(S32 width, S32 height);

private:
    static void extractMip(const U8 *indata, U8* mipdata, int width, int height,
                           int mip_width, int mip_height, EFileFormat format);

private:
    EFileFormat mFileFormat;
    S32 mHeaderSize;
};

#endif
