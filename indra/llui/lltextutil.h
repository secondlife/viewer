/** 
 * @file lltextutil.h
 * @brief Misc text-related auxiliary methods
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLTEXTUTIL_H
#define LL_LLTEXTUTIL_H

#include "llstyle.h"

class LLTextBox;
class LLUrlMatch;
class LLTextBase;

namespace LLTextUtil
{

    /**
     * Set value for text box, highlighting substring hl_uc.
     * 
     * Used to highlight filter matches.
     * 
     * @param txtbox        Text box to set value for
     * @param normal_style  Style to use for non-highlighted text
     * @param text          Text to set
     * @param hl            Upper-cased string to highlight
     */
    void textboxSetHighlightedVal(
        LLTextBox *txtbox,
        const LLStyle::Params& normal_style,
        const std::string& text,
        const std::string& hl);

    void textboxSetGreyedVal(
            LLTextBox *txtbox,
            const LLStyle::Params& normal_style,
            const std::string& text,
            const std::string& greyed);

    /**
     * Adds icon before url if need.
     *
     * @param[in] match an object with results of matching
     * @param[in] text_base pointer to UI text object
     * @param[in] is_content_trusted true if context is trusted
     * @return reference to string with formatted phone number
     */
    bool processUrlMatch(LLUrlMatch* match, LLTextBase* text_base, bool is_content_trusted);

    class TextHelpers
    {

        //we need this special callback since we need to create LLAvataIconCtrls while parsing
        //avatar/group url but can't create LLAvataIconCtrl from LLUI
        public:
            static boost::function<bool(LLUrlMatch*,LLTextBase*)> iconCallbackCreationFunction;
    };

    
}

#endif // LL_LLTEXTUTIL_H
