/**
 * @file llgltexture_test.cpp
 * @brief Tests for textures without a GL image.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#include "llgltexture.h"
#include "../test/lltut.h"

namespace tut
{
    struct gltexture_test {};
    typedef test_group<gltexture_test> gltexture_group_t;
    typedef gltexture_group_t::object gltexture_object_t;
    gltexture_group_t gltexture_instance("LLGLTexture");

    template<> template<>
    void gltexture_object_t::test<1>()
    {
        LLPointer<LLGLTexture> texture = new LLGLTexture();
        ensure("An uninitialized texture has no GL image", texture->getGLTexture() == nullptr);
        ensure("An uninitialized texture has no GPU storage", !texture->hasGLTexture());
    }
}
