/** 
 * @file lltoolpipette.h
 * @brief LLToolPipette class header file
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

// A tool to pick texture entry infro from objects in world (color/texture)
// This tool assumes it is transient in the codebase and must be used
// accordingly. We should probably restructure the way tools are
// managed so that this is handled automatically.

#ifndef LL_LLTOOLPIPETTE_H
#define LL_LLTOOLPIPETTE_H

#include "lltool.h"
#include "lltextureentry.h"
#include <boost/function.hpp>
#include <boost/signals2.hpp>

class LLViewerObject;
class LLPickInfo;

class LLToolPipette
:   public LLTool, public LLSingleton<LLToolPipette>
{
    LLSINGLETON(LLToolPipette);
    virtual ~LLToolPipette();

public:
    virtual BOOL    handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL    handleMouseUp(S32 x, S32 y, MASK mask);
    virtual BOOL    handleHover(S32 x, S32 y, MASK mask);
    virtual BOOL    handleToolTip(S32 x, S32 y, MASK mask);

    // Note: Don't return connection; use boost::bind + boost::signals2::trackable to disconnect slots
    typedef boost::signals2::signal<void (const LLTextureEntry& te)> signal_t;
    void setToolSelectCallback(const signal_t::slot_type& cb) { mSignal.connect(cb); }
    void setResult(BOOL success, const std::string& msg);
    
    void setTextureEntry(const LLTextureEntry* entry);
    static void pickCallback(const LLPickInfo& pick_info);

protected:
    LLTextureEntry  mTextureEntry;
    signal_t        mSignal;
    BOOL            mSuccess;
    std::string     mTooltipMsg;
};

#endif //LL_LLTOOLPIPETTE_H
