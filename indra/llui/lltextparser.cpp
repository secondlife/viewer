/** 
 * @file lltextparser.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

// Routines used for parsing text for TextParsers and html

LLTextParser* LLTextParser::sInstance = NULL;

//
// Member Functions
//

LLTextParser::~LLTextParser()
{
	sInstance=NULL;
}

// static
LLTextParser* LLTextParser::getInstance()
{
	if (!sInstance)
	{
		sInstance = new LLTextParser();
		sInstance->loadFromDisk();
	}
	return sInstance;
}

// Moved triggerAlerts() to llfloaterchat.cpp to break llui/llaudio library dependency.

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

LLSD LLTextParser::loadFromDisk()
{
	std::string filename=getFileName();
	if (filename.empty())
	{
		llwarns << "LLTextParser::loadFromDisk() no valid user directory." << llendl; 
	}
	else
	{
		llifstream file;
		file.open(filename.c_str());
		if (file.is_open())
		{
			LLSDSerialize::fromXML(mHighlights, file);
		}
		file.close();
	}

	return mHighlights;
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
