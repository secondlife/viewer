/**
 * @file lltextparser.cpp
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

#include "linden_common.h"

#include "lltextparser.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "llerror.h"
#include "lluuid.h"
#include "llstring.h"
#include "message.h"
#include "llmath.h"
#include "v4color.h"
#include "lldir.h"
#include "lluicolor.h"

//
// Member Functions
//

LLTextParser::LLTextParser()
:   mLoaded(false)
{}


S32 LLTextParser::findPattern(const std::string &text, LLSD highlight)
{
    if (!highlight.has("pattern")) return -1;

    std::string pattern=std::string(highlight["pattern"]);
    std::string ltext=text;

    if (!(bool)highlight["case_sensitive"])
    {
        ltext   = utf8str_tolower(text);
        pattern= utf8str_tolower(pattern);
    }

    size_t found=std::string::npos;

    switch ((S32)highlight["condition"])
    {
        case CONTAINS:
            found = ltext.find(pattern);
            break;
        case MATCHES:
            found = (! ltext.compare(pattern) ? 0 : std::string::npos);
            break;
        case STARTS_WITH:
            found = (! ltext.find(pattern) ? 0 : std::string::npos);
            break;
        case ENDS_WITH:
            auto pos = ltext.rfind(pattern);
            if (pos != std::string::npos && pos >= 0 && (ltext.length() - pattern.length() == pos)) found = pos;
            break;
    }
    return static_cast<S32>(found);
}

LLTextParser::parser_out_vec_t LLTextParser::parsePartialLineHighlights(const std::string &text, const LLUIColor& color, EHighlightPosition part, S32 index)
{
    loadKeywords();

    //evil recursive string atomizer.
    parser_out_vec_t ret_vec, start_vec, middle_vec, end_vec;

    for (S32 i=index, size = (S32)mHighlights.size();i< size;i++)
    {
        S32 condition = mHighlights[i]["condition"];
        if ((S32)mHighlights[i]["highlight"]==PART && condition!=MATCHES)
        {
            if ( (condition==STARTS_WITH && part==START) ||
                 (condition==ENDS_WITH   && part==END)   ||
                  condition==CONTAINS    || part==WHOLE )
            {
                S32 start = findPattern(text,mHighlights[i]);
                if (start >= 0 )
                {
                    auto end =  std::string(mHighlights[i]["pattern"]).length();
                    auto len = text.length();
                    EHighlightPosition newpart;
                    if (start==0)
                    {
                        if (start_vec.empty())
                        {
                            start_vec.push_back(std::make_pair(text.substr(0, end), LLColor4(mHighlights[i]["color"])));
                        }
                        else
                        {
                            start_vec[0] = std::make_pair(text.substr(0, end), LLColor4(mHighlights[i]["color"]));
                        }

                        if (end < len)
                        {
                            if (part==END   || part==WHOLE) newpart=END; else newpart=MIDDLE;
                            end_vec = parsePartialLineHighlights(text.substr( end ),color,newpart,i);
                        }
                    }
                    else
                    {
                        if (part==START || part==WHOLE) newpart=START; else newpart=MIDDLE;

                        start_vec = parsePartialLineHighlights(text.substr(0,start),color,newpart,i+1);

                        if (end < len)
                        {
                            if (middle_vec.empty())
                            {
                                middle_vec.push_back(std::make_pair(text.substr(start, end), LLColor4(mHighlights[i]["color"])));
                            }
                            else
                            {
                                middle_vec[0] = std::make_pair(text.substr(start, end), LLColor4(mHighlights[i]["color"]));
                            }

                            if (part==END   || part==WHOLE) newpart=END; else newpart=MIDDLE;

                            end_vec = parsePartialLineHighlights(text.substr( (start+end) ),color,newpart,i);
                        }
                        else
                        {
                            if (end_vec.empty())
                            {
                                end_vec.push_back(std::make_pair(text.substr(start, end), LLColor4(mHighlights[i]["color"])));
                            }
                            else
                            {
                                end_vec[0] = std::make_pair(text.substr(start, end), LLColor4(mHighlights[i]["color"]));
                            }
                        }
                    }

                    ret_vec.reserve(start_vec.size() + middle_vec.size() + end_vec.size());
                    ret_vec.insert(ret_vec.end(), start_vec.begin(), start_vec.end());
                    ret_vec.insert(ret_vec.end(), middle_vec.begin(), middle_vec.end());
                    ret_vec.insert(ret_vec.end(), end_vec.begin(), end_vec.end());

                    return ret_vec;
                }
            }
        }
    }

    //No patterns found.  Just send back what was passed in.
    ret_vec.push_back(std::make_pair(text, color));
    return ret_vec;
}

bool LLTextParser::parseFullLineHighlights(const std::string &text, LLColor4 *color)
{
    loadKeywords();

    for (S32 i=0;i<mHighlights.size();i++)
    {
        if ((S32)mHighlights[i]["highlight"]==ALL || (S32)mHighlights[i]["condition"]==MATCHES)
        {
            if (findPattern(text,mHighlights[i]) >= 0 )
            {
                LLSD color_llsd = mHighlights[i]["color"];
                color->setValue(color_llsd);
                return true;
            }
        }
    }
    return false;   //No matches found.
}

std::string LLTextParser::getFileName()
{
    std::string path=gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "");

    if (!path.empty())
    {
        path = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "highlights.xml");
    }
    return path;
}

void LLTextParser::loadKeywords()
{
    if (mLoaded)
    {// keywords already loaded
        return;
    }
    std::string filename=getFileName();
    if (!filename.empty())
    {
        llifstream file;
        file.open(filename.c_str());
        if (file.is_open())
        {
            LLSDSerialize::fromXML(mHighlights, file);
        }
        file.close();
        mLoaded = true;
    }
}

bool LLTextParser::saveToDisk(LLSD highlights)
{
    mHighlights=highlights;
    std::string filename=getFileName();
    if (filename.empty())
    {
        LL_WARNS() << "LLTextParser::saveToDisk() no valid user directory." << LL_ENDL;
        return false;
    }
    llofstream file;
    file.open(filename.c_str());
    LLSDSerialize::toPrettyXML(mHighlights, file);
    file.close();
    return true;
}
