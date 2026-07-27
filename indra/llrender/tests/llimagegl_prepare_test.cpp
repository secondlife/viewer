/**
 * @file llimagegl_prepare_test.cpp
 * @brief Tests for CPU-side texture upload preparation.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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

#include "linden_common.h"

#include "../test/lltut.h"

#include "llimage.h"
#include "llimagegl.h"

namespace tut
{
    struct llimagegl_prepare_data
    {
        static LLPointer<LLImageRaw> make_image(U16 width, U16 height, S8 components)
        {
            LLPointer<LLImageRaw> image = new LLImageRaw(width, height, components);
            memset(image->getData(), 0, image->getDataSize());
            return image;
        }
    };

    typedef test_group<llimagegl_prepare_data> llimagegl_prepare_test;
    typedef llimagegl_prepare_test::object llimagegl_prepare_object;
    llimagegl_prepare_test llimagegl_prepare_test_factory("LLImageGL upload preparation");

    template<> template<>
    void llimagegl_prepare_object::test<1>()
    {
        LLPointer<LLImageRaw> image = make_image(3, 5, 4);
        U8* data = image->getData();
        for (S32 y = 0; y < image->getHeight(); y += 2)
        {
            data[(y * image->getWidth()) * image->getComponents() + 3] = 255;
        }

        LLImageGL::TextureUploadPreparation result = LLImageGL::prepareForUpload(image);

        ensure("RGBA preparation has usable results", result.hasResults());
        ensure("alpha analysis is prepared", result.mAlphaAnalyzed);
        ensure("binary alpha is classified as a mask", result.mIsMask);
        ensure("RGBA pick mask is prepared", result.mPickMaskPrepared);
        ensure_equals("odd width uses ceiling half-width", result.mPickMaskWidth, U16(2));
        ensure_equals("odd height uses ceiling half-height", result.mPickMaskHeight, U16(3));
        ensure_equals("alternating pick bits", result.mPickMask[0], U8(0x15));
    }

    template<> template<>
    void llimagegl_prepare_object::test<2>()
    {
        LLPointer<LLImageRaw> image = make_image(15, 2, 4);
        U8* data = image->getData();
        for (S32 x = 0; x < image->getWidth(); x += 4)
        {
            data[x * image->getComponents() + 3] = 255;
        }

        LLImageGL::TextureUploadPreparation result = LLImageGL::prepareForUpload(image);

        ensure("RGBA pick mask is prepared", result.mPickMaskPrepared);
        ensure_equals("pick width matches samples written", result.mPickMaskWidth, U16(8));
        ensure_equals("pick height matches samples written", result.mPickMaskHeight, U16(1));
        ensure_equals("exactly eight bits need one byte", result.mPickMask.size(), size_t(1));
        ensure_equals("all eight sample positions map correctly", result.mPickMask[0], U8(0x55));
    }

    template<> template<>
    void llimagegl_prepare_object::test<3>()
    {
        LLPointer<LLImageRaw> image = make_image(3, 3, 2);
        U8* data = image->getData();
        for (S32 pixel = 0; pixel < image->getWidth() * image->getHeight(); ++pixel)
        {
            data[pixel * image->getComponents() + 1] = 255;
        }

        LLImageGL::TextureUploadPreparation result = LLImageGL::prepareForUpload(image);

        ensure("luminance-alpha analysis is prepared", result.mAlphaAnalyzed);
        ensure("opaque alpha is classified as a mask", result.mIsMask);
        ensure("unsupported pick-mask layout falls back to setImage", !result.mPickMaskPrepared);
        ensure("no unsupported pick-mask data is produced", result.mPickMask.empty());
    }

    template<> template<>
    void llimagegl_prepare_object::test<4>()
    {
        LLPointer<LLImageRaw> image = make_image(2, 2, 3);

        LLImageGL::TextureUploadPreparation result = LLImageGL::prepareForUpload(image);

        ensure("RGB preparation has no usable results", !result.hasResults());
        ensure("RGB has no alpha preparation", !result.mAlphaAnalyzed);
        ensure("RGB has no pick-mask preparation", !result.mPickMaskPrepared);
        ensure("RGB has no prepared pick-mask data", result.mPickMask.empty());
    }
}
