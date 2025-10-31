/**
 * @file llprocesstexture.h
 * @brief Declaration of LLProcessTexture class.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLPROCESSTEXTURE_H
#define LL_LLPROCESSTEXTURE_H

#include "llbakingprocess.h"
#include "llbakingtexture.h"
#include "llpointer.h"
#include "lltexturemanagerbridge.h"
#include "llbakingwindow.h"

class LLBakingWindow;

class LLProcessTexture : public LLBakingProcess, public LLTextureManagerBridge
{
public:
    LLProcessTexture(LLAppAppearanceUtility* app);

    /////// LLBakingProcess interface. /////////
    void parseInput(std::istream& input) override;
    void process(std::ostream& output) override;
    void init() override;
    void cleanup() override;

    /////// LLTextureManagerBridge interface. ////////
    LLPointer<LLGLTexture> getLocalTexture(bool usemipmaps = true, bool generate_gl_tex = true) override;
    LLPointer<LLGLTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, bool usemipmaps, bool generate_gl_tex = true) override;
    LLGLTexture* getFetchedTexture(const LLUUID &image_id) override;

private:
    typedef std::map< LLUUID, LLPointer<LLBakingTexture> > texture_map_t;
    texture_map_t mTextureData;
    LLBakingWindow* mWindow;
    std::istream* mInputRaw;
    S32 mBakeSize;
};

#endif /* LL_LLPROCESSTEXTURE_H */

