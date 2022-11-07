/** 
 * @file llviewerassettype.h
 * @brief Declaration of LLViewerViewerAssetType.
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

#ifndef LL_LLVIEWERASSETTYPE_H
#define LL_LLVIEWERASSETTYPE_H

#include <string>
#include "llassettype.h"
#include "llui.h" //EDragAndDropType

// This class is similar to llassettype, but contains methods
// only used by the viewer.
class LLViewerAssetType : public LLAssetType
{
public:
    // Generate a good default description. You may want to add a verb
    // or agent name after this depending on your application.
    static void                 generateDescriptionFor(LLViewerAssetType::EType asset_type,
                                                       std::string& description);
    static EDragAndDropType     lookupDragAndDropType(EType asset_type);
protected:
    LLViewerAssetType() {}
    ~LLViewerAssetType() {}
};

#endif // LL_LLVIEWERASSETTYPE_H
