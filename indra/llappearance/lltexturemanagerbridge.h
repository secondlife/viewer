/** 
 * @file lltexturemanagerbridge.h
 * @brief Bridge to an application-specific texture manager.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_TEXTUREMANAGERBRIDGE_H
#define LL_TEXTUREMANAGERBRIDGE_H

#include "llavatarappearancedefines.h"
#include "llpointer.h"
#include "llgltexture.h"

// Abstract bridge interface
class LLTextureManagerBridge
{
public:
	virtual LLPointer<LLGLTexture> getLocalTexture(BOOL usemipmaps = TRUE, BOOL generate_gl_tex = TRUE) = 0;
	virtual LLPointer<LLGLTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex = TRUE) = 0;
	virtual LLGLTexture* getFetchedTexture(const LLUUID &image_id) = 0;
};

extern LLTextureManagerBridge* gTextureManagerBridgep;

#endif // LL_TEXTUREMANAGERBRIDGE_H

