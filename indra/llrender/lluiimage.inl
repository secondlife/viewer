/**
 * @file lluiimage.inl
 * @brief UI inline func implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

void LLUIImage::draw(S32 x, S32 y, const LLColor4& color) const
{
    draw(x, y, getWidth(), getHeight(), color);
}

void LLUIImage::draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, bool solid_color) const
{
    if (sEnableDisplayListsCollection)
    {
        // Get display list for this configuration
        buffer_data_list_t* display_list = findDisplayList(x, y, width, height, color, solid_color);

        if (display_list && !display_list->empty())
        {
            // Deliberately empty pending verts.
            // They aren't related to the image, so don't register them under draw zone
            gGL.flush();
            LL_PROFILE_ZONE_SCOPED;
            gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);

            //gGL.pushUIMatrix();

            if (solid_color)
            {
                gSolidColorProgram.bind();
            }

            gGL.color4fv(color.mV); // for the shader

            // Replay the cached display list
            for (LLVertexBufferData& buffer : *display_list)
            {
                buffer.draw();
            }

            if (solid_color)
            {
                gUIProgram.bind();
            }
            //gGL.popUIMatrix();
        }
        else
        {
            // Create, draw and capture display list.
            // Basically a wrapper around gl_draw_scaled_image_with_border
            // that records the output into a list.
            genDisplayList(x, y, width, height, color, solid_color);
        }
    }
    else
    {
        gl_draw_scaled_image_with_border(
            x, y,
            width, height,
            mImage,
            color,
            solid_color,
            mClipRegion,
            mScaleRegion,
            mScaleStyle == SCALE_INNER);
    }
}

void LLUIImage::draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
    draw(x, y, width, height, color, false);
}

void LLUIImage::drawSolid(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
    draw(x, y, width, height, color, true);
}

void LLUIImage::drawBorder(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, S32 border_width) const
{
    LLRect border_rect;
    border_rect.setOriginAndSize(x, y, width, height);
    border_rect.stretch(border_width, border_width);
    drawSolid(border_rect, color);
}

// returns dimensions of underlying textures, which might not be equal to ui image portion
S32 LLUIImage::getTextureWidth() const
{
    mCachedW = (mCachedW == -1) ? getWidth() : mCachedW;
    return mCachedW;
}

S32 LLUIImage::getTextureHeight() const
{
    mCachedH = (mCachedH == -1) ? getHeight() : mCachedH;
    return mCachedH;
}
