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

//
// Member Functions
//

LLTextParser::LLTextParser()
:	mLoaded(false)
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
			S32 pos = ltext.rfind(pattern); 
			if (pos >= 0 && (ltext.length()-pattern.length()==pos)) found = pos;
			break;
	}
	return found;
}

LLSD LLTextParser::parsePartialLineHighlights(const std::string &text, const LLColor4 &color, EHighlightPosition part, S32 index)
{
	loadKeywords();

	//evil recursive string atomizer.
	LLSD ret_llsd, start_llsd, middle_llsd, end_llsd;

	for (S32 i=index;i<mHighlights.size();i++)
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
					S32 end =  std::string(mHighlights[i]["pattern"]).length();
					S32 len = text.length();
					EHighlightPosition newpart;
					if (start==0)
					{
						start_llsd[0]["text"] =text.substr(0,end);
						start_llsd[0]["color"]=mHighlights[i]["color"];
						
						if (end < len)
						{
							if (part==END   || part==WHOLE) newpart=END; else newpart=MIDDLE;
							end_llsd=parsePartialLineHighlights(text.substr( end ),color,newpart,i);
						}
					}
					else
					{
						if (part==START || part==WHOLE) newpart=START; else newpart=MIDDLE;

						start_llsd=parsePartialLineHighlights(text.substr(0,start),color,newpart,i+1);
						
						if (end < len)
						{
							middle_llsd[0]["text"] =text.substr(start,end);
							middle_llsd[0]["color"]=mHighlights[i]["color"];
						
							if (part==END   || part==WHOLE) newpart=END; else newpart=MIDDLE;

							end_llsd=parsePartialLineHighlights(text.substr( (start+end) ),color,newpart,i);
						}
						else
						{
							end_llsd[0]["text"] =text.substr(start,end);
							end_llsd[0]["color"]=mHighlights[i]["color"];
						}
					}
						
					S32 retcount=0;
					
					//FIXME These loops should be wrapped into a subroutine.
					for (LLSD::array_iterator iter = start_llsd.beginArray();
						 iter != start_llsd.endArray();++iter)
					{
						LLSD highlight = *iter;
						ret_llsd[retcount++]=highlight;
					}
						   
					for (LLSD::array_iterator iter = middle_llsd.beginArray();
						 iter != middle_llsd.endArray();++iter)
					{
						LLSD highlight = *iter;
						ret_llsd[retcount++]=highlight;
					}
						   
					for (LLSD::array_iterator iter = end_llsd.beginArray();
						 iter != end_llsd.endArray();++iter)
					{
						LLSD highlight = *iter;
						ret_llsd[retcount++]=highlight;
					}
						   
					return ret_llsd;
				}
			}
		}
	}
	
	//No patterns found.  Just send back what was passed in.
	ret_llsd[0]["text"] =text;
	LLSD color_sd = color.getValue();
	ret_llsd[0]["color"]=color_sd;
	return ret_llsd;
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
				return TRUE;
			}
		}
	}
	return FALSE;	//No matches found.
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
		llwarns << "LLTextParser::saveToDisk() no valid user directory." << llendl; 
		return FALSE;
	}	
	llofstream file;
	file.open(filename.c_str());
	LLSDSerialize::toPrettyXML(mHighlights, file);
	file.close();
	return TRUE;
}
