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

class LLUUID;
class LLVector3d;
class LLColor4;

class LLTextParser : public LLSingleton<LLTextParser>
{
    LLSINGLETON(LLTextParser);

public:
    typedef enum e_condition_type { CONTAINS, MATCHES, STARTS_WITH, ENDS_WITH } EConditionType;
    typedef enum e_highlight_type { PART, ALL } EHighlightType;
    typedef enum e_highlight_position { WHOLE, START, MIDDLE, END } EHighlightPosition;
    typedef enum e_dialog_action { ACTION_NONE, ACTION_CLOSE, ACTION_ADD, ACTION_COPY, ACTION_UPDATE } EDialogAction;

    LLSD parsePartialLineHighlights(const std::string &text,const LLColor4 &color, EHighlightPosition part=WHOLE, S32 index=0);
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
