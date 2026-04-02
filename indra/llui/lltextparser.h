/**
 * @file llTextParser.h
 * @brief GUI for user-defined highlights
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
 *
 */

#ifndef LL_LLTEXTPARSER_H
#define LL_LLTEXTPARSER_H

#include "llsd.h"
#include "llsingleton.h"
#include "lluicolor.h"

class LLUUID;
class LLVector3d;
class LLColor4;

class LLTextParser : public LLSingleton<LLTextParser>
{
    LLSINGLETON(LLTextParser);

public:
    enum EConditionType { CONTAINS, MATCHES, STARTS_WITH, ENDS_WITH };
    enum EHighlightType { PART, ALL };
    enum EHighlightPosition { WHOLE, START, MIDDLE, END };
    enum EDialogAction { ACTION_NONE, ACTION_CLOSE, ACTION_ADD, ACTION_COPY, ACTION_UPDATE };

    using parser_out_vec_t = std::vector<std::pair<std::string, LLUIColor>>;

    parser_out_vec_t parsePartialLineHighlights(const std::string &text,const LLUIColor &color, EHighlightPosition part=WHOLE, S32 index=0);
    bool parseFullLineHighlights(const std::string &text, LLColor4 *color);

private:
    S32  findPattern(const std::string &text, LLSD highlight);
    std::string getFileName();
    void loadKeywords();
    bool saveToDisk(LLSD highlights);

public:
    LLSD    mHighlights;
    bool    mLoaded;
};

#endif
