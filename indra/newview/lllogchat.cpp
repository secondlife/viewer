/** 
 * @file lllogchat.cpp
 * @brief LLLogChat class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llagentui.h"
#include "lllogchat.h"
#include "lltrans.h"
#include "llviewercontrol.h"

#include "llinstantmessage.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <boost/regex/v4/match_results.hpp>

const S32 LOG_RECALL_SIZE = 2048;

const static std::string IM_TIME("time");
const static std::string IM_TEXT("message");
const static std::string IM_FROM("from");
const static std::string IM_FROM_ID("from_id");
const static std::string IM_SEPARATOR(": ");

const static std::string NEW_LINE("\n");
const static std::string NEW_LINE_SPACE_PREFIX("\n ");
const static std::string TWO_SPACES("  ");
const static std::string MULTI_LINE_PREFIX(" ");

/**
 *  Chat log lines - timestamp and name are optional but message text is mandatory.
 *
 *  Typical plain text chat log lines:
 *
 *  SuperCar: You aren't the owner
 *  [2:59]  SuperCar: You aren't the owner
 *  [2009/11/20 3:00]  SuperCar: You aren't the owner
 *  Katar Ivercourt is Offline
 *  [3:00]  Katar Ivercourt is Offline
 *  [2009/11/20 3:01]  Corba ProductEngine is Offline
 */
const static boost::regex TIMESTAMP_AND_STUFF("^(\\[\\d{4}/\\d{1,2}/\\d{1,2}\\s+\\d{1,2}:\\d{2}\\]\\s+|\\[\\d{1,2}:\\d{2}\\]\\s+)?(.*)$");

/**
 *  Regular expression suitable to match names like
 *  "You", "Second Life", "Igor ProductEngine", "Object", "Mega House"
 */
const static boost::regex NAME_AND_TEXT("(You:|Second Life:|[^\\s:]+\\s*[:]{1}|\\S+\\s+[^\\s:]+[:]{1})?(\\s*)(.*)");

const static int IDX_TIMESTAMP = 1;
const static int IDX_STUFF = 2;
const static int IDX_NAME = 1;
const static int IDX_TEXT = 3;

//static
std::string LLLogChat::makeLogFileName(std::string filename)
{
	filename = cleanFileName(filename);
	filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS,filename);
	filename += ".txt";
	return filename;
}

std::string LLLogChat::cleanFileName(std::string filename)
{
	std::string invalidChars = "\"\'\\/?*:<>|";
	std::string::size_type position = filename.find_first_of(invalidChars);
	while (position != filename.npos)
	{
		filename[position] = '_';
		position = filename.find_first_of(invalidChars, position);
	}
	return filename;
}

std::string LLLogChat::timestamp(bool withdate)
{
	time_t utc_time;
	utc_time = time_corrected();

	std::string timeStr;
	LLSD substitution;
	substitution["datetime"] = (S32) utc_time;

	if (withdate)
	{
		timeStr = "["+LLTrans::getString ("TimeYear")+"]/["
		          +LLTrans::getString ("TimeMonth")+"]/["
				  +LLTrans::getString ("TimeDay")+"] ["
				  +LLTrans::getString ("TimeHour")+"]:["
				  +LLTrans::getString ("TimeMin")+"]";
	}
	else
	{
		timeStr = "[" + LLTrans::getString("TimeHour") + "]:["
			      + LLTrans::getString ("TimeMin")+"]";
	}

	LLStringUtil::format (timeStr, substitution);
	return timeStr;
}


//static
void LLLogChat::saveHistory(const std::string& filename,
			    const std::string& from,
			    const LLUUID& from_id,
			    const std::string& line)
{
	if(!filename.size())
	{
		llinfos << "Filename is Empty!" << llendl;
		return;
	}

	llofstream file (LLLogChat::makeLogFileName(filename), std::ios_base::app);
	if (!file.is_open())
	{
		llinfos << "Couldn't open chat history log!" << llendl;
		return;
	}

	LLSD item;

	if (gSavedPerAccountSettings.getBOOL("LogTimestamp"))
		 item["time"] = LLLogChat::timestamp(gSavedPerAccountSettings.getBOOL("LogTimestampDate"));

	item["from"]	= from;
	item["from_id"]	= from_id;
	item["message"]	= line;

	file << LLChatLogFormatter(item) << std::endl;

	file.close();
}

void LLLogChat::loadHistory(const std::string& filename, void (*callback)(ELogLineType, const LLSD&, void*), void* userdata)
{
	if(!filename.size())
	{
		llwarns << "Filename is Empty!" << llendl;
		return ;
	}
        
	LLFILE* fptr = LLFile::fopen(makeLogFileName(filename), "r");		/*Flawfinder: ignore*/
	if (!fptr)
	{
		callback(LOG_EMPTY, LLSD(), userdata);
		return;			//No previous conversation with this name.
	}
	else
	{
		char buffer[LOG_RECALL_SIZE];		/*Flawfinder: ignore*/
		char *bptr;
		S32 len;
		bool firstline=TRUE;

		if ( fseek(fptr, (LOG_RECALL_SIZE - 1) * -1  , SEEK_END) )		
		{	//File is smaller than recall size.  Get it all.
			firstline = FALSE;
			if ( fseek(fptr, 0, SEEK_SET) )
			{
				fclose(fptr);
				return;
			}
		}

		while ( fgets(buffer, LOG_RECALL_SIZE, fptr)  && !feof(fptr) ) 
		{
			len = strlen(buffer) - 1;		/*Flawfinder: ignore*/
			for ( bptr = (buffer + len); (*bptr == '\n' || *bptr == '\r') && bptr>buffer; bptr--)	*bptr='\0';
			
			if (!firstline)
			{
				LLSD item;
				std::string line(buffer);
				std::istringstream iss(line);
				
				if (!LLChatLogParser::parse(line, item))
				{
					item["message"]	= line;
					callback(LOG_LINE, item, userdata);
				}
				else
				{
					callback(LOG_LLSD, item, userdata);
				}
			}
			else
			{
				firstline = FALSE;
			}
		}
		callback(LOG_END, LLSD(), userdata);
		
		fclose(fptr);
	}
}

void append_to_last_message(std::list<LLSD>& messages, const std::string& line)
{
	if (!messages.size()) return;

	std::string im_text = messages.back()[IM_TEXT].asString();
	im_text.append(line);
	messages.back()[IM_TEXT] = im_text;
}

void LLLogChat::loadAllHistory(const std::string& session_name, std::list<LLSD>& messages)
{
	if (session_name.empty())
	{
		llwarns << "Session name is Empty!" << llendl;
		return ;
	}

	LLFILE* fptr = LLFile::fopen(makeLogFileName(session_name), "r");		/*Flawfinder: ignore*/
	if (!fptr) return;	//No previous conversation with this name.

	char buffer[LOG_RECALL_SIZE];		/*Flawfinder: ignore*/
	char *bptr;
	S32 len;
	bool firstline = TRUE;

	if (fseek(fptr, (LOG_RECALL_SIZE - 1) * -1  , SEEK_END))
	{	//File is smaller than recall size.  Get it all.
		firstline = FALSE;
		if (fseek(fptr, 0, SEEK_SET))
		{
			fclose(fptr);
			return;
		}
	}

	while (fgets(buffer, LOG_RECALL_SIZE, fptr)  && !feof(fptr)) 
	{
		len = strlen(buffer) - 1;		/*Flawfinder: ignore*/
		for (bptr = (buffer + len); (*bptr == '\n' || *bptr == '\r') && bptr>buffer; bptr--)	*bptr='\0';
		
		if (firstline)
		{
			firstline = FALSE;
			continue;
		}

		std::string line(buffer);

		//updated 1.23 plaint text log format requires a space added before subsequent lines in a multilined message
		if (' ' == line[0])
		{
			line.erase(0, MULTI_LINE_PREFIX.length());
			append_to_last_message(messages, '\n' + line);
		}
		else if (0 == len && ('\n' == line[0] || '\r' == line[0]))
		{
			//to support old format's multilined messages with new lines used to divide paragraphs
			append_to_last_message(messages, line);
		}
		else
		{
			LLSD item;
			if (!LLChatLogParser::parse(line, item))
			{
				item[IM_TEXT] = line;
			}
			messages.push_back(item);
		}
	}
	fclose(fptr);
}

//*TODO mark object's names in a special way so that they will be distinguishable form avatar name 
//which are more strict by its nature (only firstname and secondname)
//Example, an object's name can be writen like "Object <actual_object's_name>"
void LLChatLogFormatter::format(const LLSD& im, std::ostream& ostr) const
{
	if (!im.isMap())
	{
		llwarning("invalid LLSD type of an instant message", 0);
		return;
	}

	if (im[IM_TIME].isDefined())
	{
		std::string timestamp = im[IM_TIME].asString();
		boost::trim(timestamp);
		ostr << '[' << timestamp << ']' << TWO_SPACES;
	}

	//*TODO mark object's names in a special way so that they will be distinguishable form avatar name 
	//which are more strict by its nature (only firstname and secondname)
	//Example, an object's name can be writen like "Object <actual_object's_name>"
	if (im[IM_FROM].isDefined())
	{
		std::string from = im[IM_FROM].asString();
		boost::trim(from);
		if (from.size())
		{
			ostr << from << IM_SEPARATOR;
		}
	}
	
	if (im[IM_TEXT].isDefined())
	{
		std::string im_text = im[IM_TEXT].asString();

		//multilined text will be saved with prepended spaces
		boost::replace_all(im_text, NEW_LINE, NEW_LINE_SPACE_PREFIX);
		ostr << im_text;
	}
}

bool LLChatLogParser::parse(std::string& raw, LLSD& im)
{
	if (!raw.length()) return false;
	
	im = LLSD::emptyMap();

	//matching a timestamp
	boost::match_results<std::string::const_iterator> matches;
	if (!boost::regex_match(raw, matches, TIMESTAMP_AND_STUFF)) return false;
	
	bool has_timestamp = matches[IDX_TIMESTAMP].matched;
	if (has_timestamp)
	{
		//timestamp was successfully parsed
		std::string timestamp = matches[IDX_TIMESTAMP];
		boost::trim(timestamp);
		timestamp.erase(0, 1);
		timestamp.erase(timestamp.length()-1, 1);
		im[IM_TIME] = timestamp;	
	}
	else
	{
		//timestamp is optional
		im[IM_TIME] = "";
	}

	bool has_stuff = matches[IDX_STUFF].matched;
	if (!has_stuff)
	{
		return false;  //*TODO should return false or not?
	}

	//matching a name and a text
	std::string stuff = matches[IDX_STUFF];
	boost::match_results<std::string::const_iterator> name_and_text;
	if (!boost::regex_match(stuff, name_and_text, NAME_AND_TEXT)) return false;
	
	bool has_name = name_and_text[IDX_NAME].matched;
	std::string name = name_and_text[IDX_NAME];

	//we don't need a name/text separator
	if (has_name && name.length() && name[name.length()-1] == ':')
	{
		name.erase(name.length()-1, 1);
	}

	if (!has_name || name == SYSTEM_FROM)
	{
		//name is optional too
		im[IM_FROM] = SYSTEM_FROM;
		im[IM_FROM_ID] = LLUUID::null;
	}

	if (!has_name)
	{
		//text is mandatory
		im[IM_TEXT] = stuff;
		return true; //parse as a message from Second Life
	}
	
	bool has_text = name_and_text[IDX_TEXT].matched;
	if (!has_text) return false;

	//for parsing logs created in very old versions of a viewer
	if (name == "You")
	{
		std::string agent_name;
		LLAgentUI::buildFullname(agent_name);
		im[IM_FROM] = agent_name;
		im[IM_FROM_ID] = gAgentID;
	}
	else
	{
		im[IM_FROM] = name;
	}
	

	im[IM_TEXT] = name_and_text[IDX_TEXT];
	return true;  //parsed name and message text, maybe have a timestamp too
}
