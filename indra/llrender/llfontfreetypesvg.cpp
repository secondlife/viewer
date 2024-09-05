/**
 * @file llfontfreetypesvg.cpp
 * @brief Freetype font library SVG glyph rendering
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

#include "linden_common.h"

#include "llfontfreetypesvg.h"

#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable : 4702)
#endif

#define NANOSVG_IMPLEMENTATION
#include <nanosvg/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvgrast.h>

#if LL_WINDOWS
#pragma warning (pop)
#endif

struct LLSvgRenderData
{
    FT_UInt    GlyphIndex = 0;
    FT_Error   Error = FT_Err_Ok; // FreeType currently (@2.12.1) ignores the error value returned by the preset glyph slot callback so we return it at render time
    // (See https://github.com/freetype/freetype/blob/5faa1df8b93ebecf0f8fd5fe8fda7b9082eddced/src/base/ftobjs.c#L1170)
    NSVGimage* pNSvgImage = nullptr;
    float      Scale = 0.f;
};

// static
FT_Error LLFontFreeTypeSvgRenderer::OnInit(FT_Pointer* state)
{
    // The SVG driver hook state is shared across all callback invocations; since our state is lightweight
    // we store it in the glyph instead.
    *state = nullptr;

    return FT_Err_Ok;
}

// static
void LLFontFreeTypeSvgRenderer::OnFree(FT_Pointer* state)
{
}

// static
void LLFontFreeTypeSvgRenderer::OnDataFinalizer(void* objectp)
{
    FT_GlyphSlot glyph_slot = static_cast<FT_GlyphSlot>(objectp);

    LLSvgRenderData* pData = static_cast<LLSvgRenderData*>(glyph_slot->generic.data);
    glyph_slot->generic.data = nullptr;
    glyph_slot->generic.finalizer = nullptr;
    delete(pData);
}

//static
FT_Error LLFontFreeTypeSvgRenderer::OnPresetGlypthSlot(FT_GlyphSlot glyph_slot, FT_Bool cache, FT_Pointer*)
{
    FT_SVG_Document document = static_cast<FT_SVG_Document>(glyph_slot->other);

    llassert(!glyph_slot->generic.data || !cache || glyph_slot->glyph_index == ((LLSvgRenderData*)glyph_slot->generic.data)->GlyphIndex);
    if (!glyph_slot->generic.data)
    {
        glyph_slot->generic.data = new LLSvgRenderData();
        glyph_slot->generic.finalizer = LLFontFreeTypeSvgRenderer::OnDataFinalizer;
    }
    LLSvgRenderData* datap = static_cast<LLSvgRenderData*>(glyph_slot->generic.data);
    if (!cache)
    {
        datap->GlyphIndex = glyph_slot->glyph_index;
        datap->Error = FT_Err_Ok;
    }

    // NOTE: nsvgParse modifies the input string so we need a temporary copy
    llassert(!datap->pNSvgImage || cache);
    if (!datap->pNSvgImage)
    {
        char* document_buffer = new char[document->svg_document_length + 1];
        memcpy(document_buffer, document->svg_document, document->svg_document_length);
        document_buffer[document->svg_document_length] = '\0';

        datap->pNSvgImage = nsvgParse(document_buffer, "px", 0.);

        delete[] document_buffer;
    }

    if (!datap->pNSvgImage)
    {
        datap->Error = FT_Err_Invalid_SVG_Document;
        return FT_Err_Invalid_SVG_Document;
    }

    // We don't (currently) support transformations so test for an identity rotation matrix + zero translation
    if (document->transform.xx != 1 << 16 || document->transform.yx != 0 ||
        document->transform.xy != 0 || document->transform.yy != 1 << 16 ||
        document->delta.x > 0 || document->delta.y > 0)
    {
        datap->Error = FT_Err_Unimplemented_Feature;
        return FT_Err_Unimplemented_Feature;
    }

    float svg_width = datap->pNSvgImage->width;
    float svg_height = datap->pNSvgImage->height;
    if (svg_width == 0.f || svg_height == 0.f)
    {
        svg_width = document->units_per_EM;
        svg_height = document->units_per_EM;
    }

    float svg_x_scale = (float)document->metrics.x_ppem / floorf(svg_width);
    float svg_y_scale = (float)document->metrics.y_ppem / floorf(svg_height);
    float svg_scale = llmin(svg_x_scale, svg_y_scale);
    datap->Scale = svg_scale;

    glyph_slot->bitmap.width = (unsigned int)(floorf(svg_width) * svg_scale);
    glyph_slot->bitmap.rows = (unsigned int)(floorf(svg_height) * svg_scale);
    glyph_slot->bitmap_left = (document->metrics.x_ppem - glyph_slot->bitmap.width) / 2;
    glyph_slot->bitmap_top = (FT_Int)(glyph_slot->face->size->metrics.ascender / 64.f);
    glyph_slot->bitmap.pitch = glyph_slot->bitmap.width * 4;
    glyph_slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

    /* Copied as-is from fcft (MIT license) */

    // Compute all the bearings and set them correctly. The outline is scaled already, we just need to use the bounding box.
    float horiBearingX = 0.f;
    float horiBearingY = -(float)glyph_slot->bitmap_top;

    // XXX parentheses correct?
    float vertBearingX = glyph_slot->metrics.horiBearingX / 64.0f - glyph_slot->metrics.horiAdvance / 64.0f / 2;
    float vertBearingY = (glyph_slot->metrics.vertAdvance / 64.0f - glyph_slot->metrics.height / 64.0f) / 2;

    // Do conversion in two steps to avoid 'bad function cast' warning
    glyph_slot->metrics.width = glyph_slot->bitmap.width * 64;
    glyph_slot->metrics.height = glyph_slot->bitmap.rows * 64;
    glyph_slot->metrics.horiBearingX = (FT_Pos)(horiBearingX * 64);
    glyph_slot->metrics.horiBearingY = (FT_Pos)(horiBearingY * 64);
    glyph_slot->metrics.vertBearingX = (FT_Pos)(vertBearingX * 64);
    glyph_slot->metrics.vertBearingY = (FT_Pos)(vertBearingY * 64);
    if (glyph_slot->metrics.vertAdvance == 0)
    {
        glyph_slot->metrics.vertAdvance = (FT_Pos)(glyph_slot->bitmap.rows * 1.2f * 64);
    }

    return FT_Err_Ok;
}

// static
FT_Error LLFontFreeTypeSvgRenderer::OnRender(FT_GlyphSlot glyph_slot, FT_Pointer*)
{
    LLSvgRenderData* datap = static_cast<LLSvgRenderData*>(glyph_slot->generic.data);
    llassert(FT_Err_Ok == datap->Error);
    if (FT_Err_Ok != datap->Error)
    {
        return datap->Error;
    }

    // Render to glyph bitmap
    NSVGrasterizer* nsvgRasterizer = nsvgCreateRasterizer();
    nsvgRasterize(nsvgRasterizer, datap->pNSvgImage, 0, 0, datap->Scale, glyph_slot->bitmap.buffer, glyph_slot->bitmap.width, glyph_slot->bitmap.rows, glyph_slot->bitmap.pitch);
    nsvgDeleteRasterizer(nsvgRasterizer);
    nsvgDelete(datap->pNSvgImage);
    datap->pNSvgImage = nullptr;

    // Convert from RGBA to BGRA
    U32* pixel_buffer = (U32*)glyph_slot->bitmap.buffer; U8* byte_buffer = glyph_slot->bitmap.buffer;
    for (size_t y = 0, h = glyph_slot->bitmap.rows; y < h; y++)
    {
        for (size_t x = 0, w = glyph_slot->bitmap.pitch / 4; x < w; x++)
        {
            size_t pixel_idx = y * w + x;
            size_t byte_idx = pixel_idx * 4;
            U8 alpha = byte_buffer[byte_idx + 3];
            // Store as ARGB (*TODO - do we still have to care about endianness?)
            pixel_buffer[y * w + x] = alpha << 24 | (byte_buffer[byte_idx] * alpha / 0xFF) << 16 | (byte_buffer[byte_idx + 1] * alpha / 0xFF) << 8 | (byte_buffer[byte_idx + 2] * alpha / 0xFF);
        }
    }

    glyph_slot->format = FT_GLYPH_FORMAT_BITMAP;
    glyph_slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
    return FT_Err_Ok;
}
