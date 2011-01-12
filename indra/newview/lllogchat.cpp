/** 
 * @file lllogchat.cpp
 * @brief LLLogChat class implementation
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
 */

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llagentui.h"
#include "lllogchat.h"
#include "lltrans.h"
#include "llviewercontrol.h"

#include "lldiriterator.h"
#include "llinstantmessage.h"
#include "llsingleton.h" // for LLSingleton

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <boost/regex/v4/match_results.hpp>

#if LL_MSVC
// disable warning about boost::lexical_cast unreachable code
// when it fails to parse the string
#pragma warning (disable:4702)
#endif

#include <boost/date_time/gregorian/gregorian.hpp>
#if LL_MSVC
#pragma warning(pop)   // Restore all warnings to the previous state
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>

const S32 LOG_RECALL_SIZE = 2048;

const std::string IM_TIME("time");
const std::string IM_TEXT("message");
const std::string IM_FROM("from");
const std::string IM_FROM_ID("from_id");

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
 *
 * Note: "You" was used as an avatar names in viewers of previous versions
 */
const static boost::regex TIMESTAMP_AND_STUFF("^(\\[\\d{4}/\\d{1,2}/\\d{1,2}\\s+\\d{1,2}:\\d{2}\\]\\s+|\\[\\d{1,2}:\\d{2}\\]\\s+)?(.*)$");

/**
 *  Regular expression suitable to match names like
 *  "You", "Second Life", "Igor ProductEngine", "Object", "Mega House"
 */
const static boost::regex NAME_AND_TEXT("([^:]+[:]{1})?(\\s*)(.*)");

/**
 * These are recognizers for matching the names of ad-hoc conferences when generating the log file name
 * On invited side, an ad-hoc is named like "<first name> <last name> Conference 2010/11/19 03:43 f0f4"
 * On initiating side, an ad-hoc is named like Ad-hoc Conference hash<hash>"
 * If the naming system for ad-hoc conferences are change in LLIMModel::LLIMSession::buildHistoryFileName()
 * then these definition need to be adjusted as well.
 */
const static boost::regex INBOUND_CONFERENCE("^[a-zA-Z]{1,31} [a-zA-Z]{1,31} Conference [0-9]{4}/[0-9]{2}/[0-9]{2} [0-9]{2}:[0-9]{2} [0-9a-f]{4}");
const static boost::regex OUTBOUND_CONFERENCE("^Ad-hoc Conference hash[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}");

//is used to parse complex object names like "Xstreet SL Terminal v2.2.5 st"
const static std::string NAME_TEXT_DIVIDER(": ");

// is used for timestamps adjusting
const static char* DATE_FORMAT("%Y/%m/%d %H:%M");
const static char* TIME_FORMAT("%H:%M");

const static int IDX_TIMESTAMP = 1;
const static int IDX_STUFF = 2;
const static int IDX_NAME = 1;
const static int IDX_TEXT = 3;

using namespace boost::posix_time;
using namespace boost::gregorian;

class LLLogChatTimeScanner: public LLSingleton<LLLogChatTimeScanner>
{
public:
	LLLogChatTimeScanner()
	{
		// Note, date/time facets will be destroyed by string streams
		mDateStream.imbue(std::locale(mDateStream.getloc(), new date_input_facet(DATE_FORMAT)));
		mTimeStream.imbue(std::locale(mTimeStream.getloc(), new time_facet(TIME_FORMAT)));
		mTimeStream.imbue(std::locale(mTimeStream.getloc(), new time_input_facet(DATE_FORMAT)));
	}

	date getTodayPacificDate()
	{
		typedef	boost::date_time::local_adjustor<ptime, -8, no_dst> pst;
		typedef boost::date_time::local_adjustor<ptime, -7, no_dst> pdt;
		time_t t_time = time(NULL);
		ptime p_time = LLStringOps::getPacificDaylightTime()
			? pdt::utc_to_local(from_time_t(t_time))
			: pst::utc_to_local(from_time_t(t_time));
		struct tm s_tm = to_tm(p_time);
		return date_from_tm(s_tm);
	}

	void checkAndCutOffDate(std::string& time_str)
	{
		// Cuts off the "%Y/%m/%d" from string for todays timestamps.
		// Assume that passed string has at least "%H:%M" time format.
		date log_date(not_a_date_time);
		date today(getTodayPacificDate());

		// Parse the passed date
		mDateStream.str(LLStringUtil::null);
		mDateStream << time_str;
		mDateStream >> log_date;
		mDateStream.clear();

		days zero_days(0);
		days days_alive = today - log_date;

		if ( days_alive == zero_days )
		{
			// Yep, today's so strip "%Y/%m/%d" info
			ptime stripped_time(not_a_date_time);

			mTimeStream.str(LLStringUtil::null);
			mTimeStream << time_str;
			mTimeStream >> stripped_time;
			mTimeStream.clear();

			time_str.clear();

			mTimeStream.str(LLStringUtil::null);
			mTimeStream << stripped_time;
			mTimeStream >> time_str;
			mTimeStream.clear();
		}

		LL_DEBUGS("LLChatLogParser")
			<< " log_date: "
			<< log_date
			<< " today: "
			<< today
			<< " days alive: "
			<< days_alive
			<< " new time: "
			<< time_str
			<< LL_ENDL;
	}


private:
	std::stringstream mDateStream;
	std::stringstream mTimeStream;
};

//static
std::string LLLogChat::makeLogFileName(std::string filename)
{
	/**
	* Testing for in bound and out bound ad-hoc file names
	* if it is then skip date stamping.
	**/
	//LL_INFOS("") << "Befor:" << filename << LL_ENDL;/* uncomment if you want to verify step, delete on commit */
    boost::match_results<std::string::const_iterator> matches;
	bool inboundConf = boost::regex_match(filename, matches, INBOUND_CONFERENCE);
	bool outboundConf = boost::regex_match(filename, matches, OUTBOUND_CONFERENCE);
	if (!(inboundConf || outboundConf))
	{
		if( gSavedPerAccountSettings.getBOOL("LogFileNamewithDate") )
		{
			time_t now;
			time(&now);
			char dbuffer[20];		/* Flawfinder: ignore */
			if (filename == "chat")
			{
				strftime(dbuffer, 20, "-%Y-%m-%d", localtime(&now));
			}
			else
			{
				strftime(dbuffer, 20, "-%Y-%m", localtime(&now));
			}
			filename += dbuffer;
		}
	}
	//LL_INFOS("") << "After:" << filename << LL_ENDL;/* uncomment if you want to verify step, delete on commit */
	filename = cleanFileName(filename);
	filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS,filename);
	filename += ".txt";
	//LL_INFOS("") << "Full:" << filename << LL_ENDL;/* uncomment if you want to verify step, delete on commit */
	return filename;
}

std::string LLLogChat::cleanFileName(std::string filename)
{
	std::string invalidChars = "\"\'\\/?*:.<>|";
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
	std::string tmp_filename = filename;
	LLStringUtil::trim(tmp_filename);
	if (tmp_filename.empty())
	{
		std::string warn = "Chat history filename [" + filename + "] is empty!";
		llwarning(warn, 666);
		llassert(tmp_filename.size());
		return;
	}
	
	llofstream file (LLLogChat::makeLogFileName(filename), std::ios_base::app);
	if (!file.is_open())
	{
		llwarns << "Couldn't open chat history log! - " + filename << llendl;
		return;
	}

	LLSD item;

	if (gSavedPerAccountSettings.getBOOL("LogTimestamp"))
		 item["time"] = LLLogChat::timestamp(gSavedPerAccountSettings.getBOOL("LogTimestampDate"));

	item["from_id"]	= from_id;
	item["message"]	= line;

	//adding "Second Life:" for all system messages to make chat log history parsing more reliable
	if (from.empty() && from_id.isNull())
	{
		item["from"] = SYSTEM_FROM; 
	}
	else
	{
		item["from"] = from;
	}

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

// static
void LLLogChat::loadAllHistory(const std::string& file_name, std::list<LLSD>& messages)
{
	if (file_name.empty())
	{
		llwarns << "Session name is Empty!" << llendl;
		return ;
	}
	//LL_INFOS("") << "Loading:" << file_name << LL_ENDL;/* uncomment if you want to verify step, delete on commit */
	//LL_INFOS("") << "Current:" << makeLogFileName(file_name) << LL_ENDL;/* uncomment if you want to verify step, delete on commit */
	LLFILE* fptr = LLFile::fopen(makeLogFileName(file_name), "r");/*Flawfinder: ignore*/
	if (!fptr)
    {
		fptr = LLFile::fopen(oldLogFileName(file_name), "r");/*Flawfinder: ignore*/
        if (!fptr)
        {
			if (!fptr) return;      //No previous conversation with this name.
        }
	}
 
    //LL_INFOS("") << "Reading:" << file_name << LL_ENDL;
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
		LLLogChatTimeScanner::instance().checkAndCutOffDate(timestamp);
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

	//possibly a case of complex object names consisting of 3+ words
	if (!has_name)
	{
		U32 divider_pos = stuff.find(NAME_TEXT_DIVIDER);
		if (divider_pos != std::string::npos && divider_pos < (stuff.length() - NAME_TEXT_DIVIDER.length()))
		{
			im[IM_FROM] = stuff.substr(0, divider_pos);
			im[IM_TEXT] = stuff.substr(divider_pos + NAME_TEXT_DIVIDER.length());
			return true;
		}
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
std::string LLLogChat::oldLogFileName(std::string filename)
{
    std::string scanResult;
	std::string directory = gDirUtilp->getPerAccountChatLogsDir();/* get Users log directory */
	directory += gDirUtilp->getDirDelimiter();/* add final OS dependent delimiter */
	filename=cleanFileName(filename);/* lest make shure the file name has no invalad charecters befor making the pattern */
	std::string pattern = (filename+(( filename == "chat" ) ? "-???\?-?\?-??.txt" : "-???\?-??.txt"));/* create search pattern*/
	//LL_INFOS("") << "Checking:" << directory << " for " << pattern << LL_ENDL;/* uncomment if you want to verify step, delete on commit */
	std::vector<std::string> allfiles;

	LLDirIterator iter(directory, pattern);
	while (iter.next(scanResult))
    {
		//LL_INFOS("") << "Found   :" << scanResult << LL_ENDL;
        allfiles.push_back(scanResult);
    }

    if (allfiles.size() == 0)  // if no result from date search, return generic filename
    {
        scanResult = directory + filename + ".txt";
    }
    else 
    {
        std::sort(allfiles.begin(), allfiles.end());
        scanResult = directory + allfiles.back();
        // thisfile is now the most recent version of the file.
    }
	//LL_INFOS("") << "Reading:" << scanResult << LL_ENDL;/* uncomment if you want to verify step, delete on commit */
    return scanResult;
}
