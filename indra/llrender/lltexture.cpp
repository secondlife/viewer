/** 
 * @file lltexture.cpp
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#include "lltexture.h"

//virtual 
LLTexture::~LLTexture()
{
}

S8   LLTexture::getType() const { llassert(false); return 0; }
void LLTexture::setKnownDrawSize(S32 width, S32 height) { llassert(false); }
bool LLTexture::bindDefaultImage(const S32 stage) { llassert(false); return false; }
bool LLTexture::bindDebugImage(const S32 stage) { llassert(false); return false; }
void LLTexture::forceImmediateUpdate() { llassert(false); }
void LLTexture::setActive() { llassert(false);  }
S32  LLTexture::getWidth(S32 discard_level) const { llassert(false); return 0; }
S32  LLTexture::getHeight(S32 discard_level) const { llassert(false); return 0; }
bool LLTexture::isActiveFetching() { llassert(false); return false; }
LLImageGL* LLTexture::getGLTexture() const { llassert(false); return nullptr; }
void LLTexture::updateBindStatsForTester() { }
