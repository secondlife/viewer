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
#include "llfloaterconversationpreview.h"
#include "llagent.h"
#include "llagentui.h"
#include "llavatarnamecache.h"
#include "lllogchat.h"
#include "llregex.h"
#include "lltrans.h"
#include "llviewercontrol.h"

#include "lldiriterator.h"
#include "llfloaterimsessiontab.h"
#include "llinstantmessage.h"
#include "llsingleton.h" // for LLSingleton

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex/v4/match_results.hpp>
#include <boost/foreach.hpp>

#if LL_MSVC
#pragma warning(push)  
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

const std::string LL_IM_TIME("time");
const std::string LL_IM_DATE_TIME("datetime");
const std::string LL_IM_TEXT("message");
const std::string LL_IM_FROM("from");
const std::string LL_IM_FROM_ID("from_id");
const std::string LL_TRANSCRIPT_FILE_EXTENSION("txt");

const std::string GROUP_CHAT_SUFFIX(" (group)");

const static char IM_SYMBOL_SEPARATOR(':');
const static std::string IM_SEPARATOR(std::string() + IM_SYMBOL_SEPARATOR + " ");
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
const static boost::regex TIMESTAMP("^(\\[\\d{4}/\\d{1,2}/\\d{1,2}\\s+\\d{1,2}:\\d{2}\\]|\\[\\d{1,2}:\\d{2}\\]).*");

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

void append_to_last_message(std::list<LLSD>& messages, const std::string& line)
{
	if (!messages.size()) return;

	std::string im_text = messages.back()[LL_IM_TEXT].asString();
	im_text.append(line);
	messages.back()[LL_IM_TEXT] = im_text;
}

const char* remove_utf8_bom(const char* buf)
{
    const char* start = buf;
	if (start[0] == (char)0xEF && start[1] == (char)0xBB && start[2] == (char)0xBF)
	{   // If string starts with the magic bytes, return pointer after it.
        start += 3;
	}
	return start;
}

class LLLogChatTimeScanner: public LLSingleton<LLLogChatTimeScanner>
{
	LLSINGLETON(LLLogChatTimeScanner);

public:
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

inline
LLLogChatTimeScanner::LLLogChatTimeScanner()
{
	// Note, date/time facets will be destroyed by string streams
	mDateStream.imbue(std::locale(mDateStream.getloc(), new date_input_facet(DATE_FORMAT)));
	mTimeStream.imbue(std::locale(mTimeStream.getloc(), new time_facet(TIME_FORMAT)));
	mTimeStream.imbue(std::locale(mTimeStream.getloc(), new time_input_facet(DATE_FORMAT)));
}

LLLogChat::LLLogChat()
: mSaveHistorySignal(NULL) // only needed in preferences
{
    mHistoryThreadsMutex = new LLMutex();
}

LLLogChat::~LLLogChat()
{
    delete mHistoryThreadsMutex;
    mHistoryThreadsMutex = NULL;

    if (mSaveHistorySignal)
    {
        mSaveHistorySignal->disconnect_all_slots();
        delete mSaveHistorySignal;
        mSaveHistorySignal = NULL;
    }
}


//static
std::string LLLogChat::makeLogFileName(std::string filename)
{
	/**
	 * Testing for in bound and out bound ad-hoc file names
	 * if it is then skip date stamping.
	 **/

	boost::match_results<std::string::const_iterator> matches;
	bool inboundConf = ll_regex_match(filename, matches, INBOUND_CONFERENCE);
	bool outboundConf = ll_regex_match(filename, matches, OUTBOUND_CONFERENCE);
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

	filename = cleanFileName(filename);
	filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS, filename);
	if (!filename.empty())
	{
		filename += '.' + LL_TRANSCRIPT_FILE_EXTENSION;
	}

	return filename;
}

//static
void LLLogChat::renameLogFile(const std::string& old_filename, const std::string& new_filename)
{
    std::string new_name = cleanFileName(new_filename);
    std::string old_name = cleanFileName(old_filename);
    new_name = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS, new_name);
    old_name = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS, old_name);

    if (new_name.empty() || old_name.empty())
    {
        return;
    }

    new_name += '.' + LL_TRANSCRIPT_FILE_EXTENSION;
    old_name += '.' + LL_TRANSCRIPT_FILE_EXTENSION;

    if (!LLFile::isfile(new_name) && LLFile::isfile(old_name))
    {
        LLFile::rename(old_name, new_name);
    }
}

std::string LLLogChat::cleanFileName(std::string filename)
{
	std::string invalidChars = "\"\'\\/?*:.<>|[]{}~"; // Cannot match glob or illegal filename chars
	std::string::size_type position = filename.find_first_of(invalidChars);
	while (position != filename.npos)
	{
		filename[position] = '_';
		position = filename.find_first_of(invalidChars, position);
	}
	return filename;
}

std::string LLLogChat::timestamp2LogString(U32 timestamp, bool withdate)
{
	std::string timeStr;
	if (withdate)
	{
		timeStr = "[" + LLTrans::getString ("TimeYear") + "]/["
				  + LLTrans::getString ("TimeMonth") + "]/["
				  + LLTrans::getString ("TimeDay") + "] ["
				  + LLTrans::getString ("TimeHour") + "]:["
				  + LLTrans::getString ("TimeMin") + "]";
	}
	else
	{
		timeStr = "[" + LLTrans::getString("TimeHour") + "]:["
				  + LLTrans::getString ("TimeMin")+"]";
	}

	LLSD substitution;
    if (timestamp == 0)
    {
        substitution["datetime"] = (S32)time_corrected();
    }
    else
    {   // timestamp is correct utc already
        substitution["datetime"] = (S32)timestamp;
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
		LL_WARNS() << warn << LL_ENDL;
		llassert(tmp_filename.size());
		return;
	}

	llofstream file(LLLogChat::makeLogFileName(filename).c_str(), std::ios_base::app);
	if (!file.is_open())
	{
		LL_WARNS() << "Couldn't open chat history log! - " + filename << LL_ENDL;
		return;
	}

	LLSD item;

	if (gSavedPerAccountSettings.getBOOL("LogTimestamp"))
		 item["time"] = LLLogChat::timestamp2LogString(0, gSavedPerAccountSettings.getBOOL("LogTimestampDate"));

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

	LLLogChat::getInstance()->triggerHistorySignal();
}

// static
void LLLogChat::loadChatHistory(const std::string& file_name, std::list<LLSD>& messages, const LLSD& load_params, bool is_group)
{
	if (file_name.empty())
	{
		LL_WARNS("LLLogChat::loadChatHistory") << "Local history file name is empty!" << LL_ENDL;
		return ;
	}

	bool load_all_history = load_params.has("load_all_history") ? load_params["load_all_history"].asBoolean() : false;

    // Stat the file to find it and get the last history entry time
    llstat stat_data;

    std::string log_file_name = LLLogChat::makeLogFileName(file_name);
    LL_DEBUGS("ChatHistory") << "First attempt to stat chat history file " << log_file_name << LL_ENDL;

    S32 no_stat = LLFile::stat(log_file_name, &stat_data);

    if (no_stat)
    {
        if (is_group)
        {
            std::string old_name(file_name);
            old_name.erase(old_name.size() - GROUP_CHAT_SUFFIX.size());     // trim off " (group)"
            log_file_name = LLLogChat::makeLogFileName(old_name);
            LL_DEBUGS("ChatHistory") << "Attempting to stat adjusted chat history file " << log_file_name << LL_ENDL;
            no_stat = LLFile::stat(log_file_name, &stat_data);
            if (!no_stat)
            {   // Found it without "(group)", copy to new naming style.  We already have the mod time in stat_data
                log_file_name = LLLogChat::makeLogFileName(file_name);
                LL_DEBUGS("ChatHistory") << "Attempt to stat copied history file " << log_file_name << LL_ENDL;
                LLFile::copy(LLLogChat::makeLogFileName(old_name), log_file_name);
            }
        }
        if (no_stat)
        {
            log_file_name = LLLogChat::oldLogFileName(file_name);
            LL_DEBUGS("ChatHistory") << "Attempt to stat old history file name " << log_file_name << LL_ENDL;
            no_stat = LLFile::stat(log_file_name, &stat_data);
            if (no_stat)
            {
                LL_DEBUGS("ChatHistory") << "No previous conversation log file found for " << file_name << LL_ENDL;
                return;						//No previous conversation with this name.
            }
        }
    }

    // If we got here, we managed to stat the file.
    // Open the file to read
    LLFILE* fptr = LLFile::fopen(log_file_name, "r");       /*Flawfinder: ignore*/
	if (!fptr)
	{   // Ok, this is strange but not really tragic in the big picture of things
        LL_WARNS("ChatHistory") << "Unable to read file " << log_file_name << " after stat was successful" << LL_ENDL;
        return;
	}

    S32 save_num_messages = messages.size();

	char buffer[LOG_RECALL_SIZE];		/*Flawfinder: ignore*/
	char *bptr;
	S32 len;
	bool firstline = TRUE;

	if (load_all_history || fseek(fptr, (LOG_RECALL_SIZE - 1) * -1  , SEEK_END))
	{	//We need to load the whole historyFile or it's smaller than recall size, so get it all.
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
        // backfill any end of line characters with nulls
		for (bptr = (buffer + len); (*bptr == '\n' || *bptr == '\r') && bptr>buffer; bptr--)	*bptr='\0';

		if (firstline)
		{
			firstline = FALSE;
			continue;
		}

		std::string line(remove_utf8_bom(buffer));

		//updated 1.23 plain text log format requires a space added before subsequent lines in a multilined message
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
			if (!LLChatLogParser::parse(line, item, load_params))
			{
				item[LL_IM_TEXT] = line;
			}
			messages.push_back(item);
		}
	}
	fclose(fptr);

    LL_DEBUGS("ChatHistory") << "Read " << (messages.size() - save_num_messages)
        << " messages of chat history from " << log_file_name
        << " file mod time " << (F64)stat_data.st_mtime << LL_ENDL;
}

bool LLLogChat::historyThreadsFinished(LLUUID session_id)
{
	LLMutexLock lock(historyThreadsMutex());
	bool finished = true;
	std::map<LLUUID,LLLoadHistoryThread *>::iterator it = mLoadHistoryThreads.find(session_id);
	if (it != mLoadHistoryThreads.end())
	{
		finished = it->second->isFinished();
	}
	if (!finished)
	{
		return false;
	}
	std::map<LLUUID,LLDeleteHistoryThread *>::iterator dit = mDeleteHistoryThreads.find(session_id);
	if (dit != mDeleteHistoryThreads.end())
	{
		finished = finished && dit->second->isFinished();
	}
	return finished;
}

LLLoadHistoryThread* LLLogChat::getLoadHistoryThread(LLUUID session_id)
{
	LLMutexLock lock(historyThreadsMutex());
	std::map<LLUUID,LLLoadHistoryThread *>::iterator it = mLoadHistoryThreads.find(session_id);
	if (it != mLoadHistoryThreads.end())
	{
		return it->second;
	}
	return NULL;
}

LLDeleteHistoryThread* LLLogChat::getDeleteHistoryThread(LLUUID session_id)
{
	LLMutexLock lock(historyThreadsMutex());
	std::map<LLUUID,LLDeleteHistoryThread *>::iterator it = mDeleteHistoryThreads.find(session_id);
	if (it != mDeleteHistoryThreads.end())
	{
		return it->second;
	}
	return NULL;
}

bool LLLogChat::addLoadHistoryThread(LLUUID& session_id, LLLoadHistoryThread* lthread)
{
	LLMutexLock lock(historyThreadsMutex());
	std::map<LLUUID,LLLoadHistoryThread *>::const_iterator it = mLoadHistoryThreads.find(session_id);
	if (it != mLoadHistoryThreads.end())
	{
		return false;
	}
	mLoadHistoryThreads[session_id] = lthread;
	return true;
}

bool LLLogChat::addDeleteHistoryThread(LLUUID& session_id, LLDeleteHistoryThread* dthread)
{
	LLMutexLock lock(historyThreadsMutex());
	std::map<LLUUID,LLDeleteHistoryThread *>::const_iterator it = mDeleteHistoryThreads.find(session_id);
	if (it != mDeleteHistoryThreads.end())
	{
		return false;
	}
	mDeleteHistoryThreads[session_id] = dthread;
	return true;
}

void LLLogChat::cleanupHistoryThreads()
{
	LLMutexLock lock(historyThreadsMutex());
	std::vector<LLUUID> uuids;
	std::map<LLUUID,LLLoadHistoryThread *>::iterator lit = mLoadHistoryThreads.begin();
	for (; lit != mLoadHistoryThreads.end(); lit++)
	{
		if (lit->second->isFinished() && mDeleteHistoryThreads[lit->first]->isFinished())
		{
			delete lit->second;
			delete mDeleteHistoryThreads[lit->first];
			uuids.push_back(lit->first);
		}
	}
	std::vector<LLUUID>::iterator uuid_it = uuids.begin();
	for ( ;uuid_it != uuids.end(); uuid_it++)
	{
		mLoadHistoryThreads.erase(*uuid_it);
		mDeleteHistoryThreads.erase(*uuid_it);
	}
}

LLMutex* LLLogChat::historyThreadsMutex()
{
	return mHistoryThreadsMutex;
}

void LLLogChat::triggerHistorySignal()
{
    if (NULL != mSaveHistorySignal)
    {
        (*mSaveHistorySignal)();
    }
}

// static
std::string LLLogChat::oldLogFileName(std::string filename)
{
	// get Users log directory
	std::string directory = gDirUtilp->getPerAccountChatLogsDir();

	// add final OS dependent delimiter
	directory += gDirUtilp->getDirDelimiter();

	// lest make sure the file name has no invalid characters before making the pattern
	filename = cleanFileName(filename);

	// create search pattern
	std::string pattern = filename + ( filename == "chat" ? "-???\?-?\?-??.txt" : "-???\?-??.txt");

	std::vector<std::string> allfiles;
	LLDirIterator iter(directory, pattern);
	std::string scanResult;

	while (iter.next(scanResult))
	{
		allfiles.push_back(scanResult);
	}

	if (allfiles.size() == 0)  // if no result from date search, return generic filename
	{
		scanResult = directory + filename + '.' + LL_TRANSCRIPT_FILE_EXTENSION;
	}
	else 
	{
		sort(allfiles.begin(), allfiles.end());
		scanResult = directory + allfiles.back();
		// this file is now the most recent version of the file.
	}

	return scanResult;
}

// static
void LLLogChat::findTranscriptFiles(std::string pattern, std::vector<std::string>& list_of_transcriptions)
{
	// get Users log directory
	std::string dirname = gDirUtilp->getPerAccountChatLogsDir();

	// add final OS dependent delimiter
	dirname += gDirUtilp->getDirDelimiter();

	LLDirIterator iter(dirname, pattern);
	std::string filename;
	while (iter.next(filename))
	{
		std::string fullname = gDirUtilp->add(dirname, filename);
		if (isTranscriptFileFound(fullname))
		{
			list_of_transcriptions.push_back(fullname);
		}		
	}
}

// static
void LLLogChat::getListOfTranscriptFiles(std::vector<std::string>& list_of_transcriptions)
{
	// create search pattern
	std::string pattern = "*." + LL_TRANSCRIPT_FILE_EXTENSION;
	findTranscriptFiles(pattern, list_of_transcriptions);
}

// static
void LLLogChat::getListOfTranscriptBackupFiles(std::vector<std::string>& list_of_transcriptions)
{
	// create search pattern
	std::string pattern = "*." + LL_TRANSCRIPT_FILE_EXTENSION + ".backup*";
	findTranscriptFiles(pattern, list_of_transcriptions);
}

boost::signals2::connection LLLogChat::setSaveHistorySignal(const save_history_signal_t::slot_type& cb)
{
	if (NULL == mSaveHistorySignal)
	{
		mSaveHistorySignal = new save_history_signal_t();
	}

	return mSaveHistorySignal->connect(cb);
}

//static
bool LLLogChat::moveTranscripts(const std::string originDirectory, 
								const std::string targetDirectory, 
								std::vector<std::string>& listOfFilesToMove,
								std::vector<std::string>& listOfFilesMoved)
{
	std::string newFullPath;
	bool movedAllTranscripts = true;
	std::string backupFileName;
	unsigned backupFileCount;

	BOOST_FOREACH(const std::string& fullpath, listOfFilesToMove)
	{
		backupFileCount = 0;
		newFullPath = targetDirectory + fullpath.substr(originDirectory.length(), std::string::npos);

		//The target directory contains that file already, so lets store it
		if(LLFile::isfile(newFullPath))
		{
			backupFileName = newFullPath + ".backup";

			//If needed store backup file as .backup1 etc.
			while(LLFile::isfile(backupFileName))
			{
				++backupFileCount;
				backupFileName = newFullPath + ".backup" + boost::lexical_cast<std::string>(backupFileCount);
			}

			//Rename the file to its backup name so it is not overwritten
			LLFile::rename(newFullPath, backupFileName);
		}

		S32 retry_count = 0;
		while (retry_count < 5)
		{
			//success is zero
			if (LLFile::rename(fullpath, newFullPath) != 0)
			{
				retry_count++;
				S32 result = errno;
				LL_WARNS("LLLogChat::moveTranscripts") << "Problem renaming " << fullpath << " - errorcode: "
					<< result << " attempt " << retry_count << LL_ENDL;

				ms_sleep(100);
			}
			else
			{
				listOfFilesMoved.push_back(newFullPath);

				if (retry_count)
				{
					LL_WARNS("LLLogChat::moveTranscripts") << "Successfully renamed " << fullpath << LL_ENDL;
				}
				break;
			}			
		}
	}

	if(listOfFilesMoved.size() != listOfFilesToMove.size())
	{
		movedAllTranscripts = false;
	}		

	return movedAllTranscripts;
}

//static
bool LLLogChat::moveTranscripts(const std::string currentDirectory, 
	const std::string newDirectory, 
	std::vector<std::string>& listOfFilesToMove)
{
	std::vector<std::string> listOfFilesMoved;
	return moveTranscripts(currentDirectory, newDirectory, listOfFilesToMove, listOfFilesMoved);
}

//static
void LLLogChat::deleteTranscripts()
{
	std::vector<std::string> list_of_transcriptions;
	getListOfTranscriptFiles(list_of_transcriptions);
	getListOfTranscriptBackupFiles(list_of_transcriptions);

	BOOST_FOREACH(const std::string& fullpath, list_of_transcriptions)
	{
		S32 retry_count = 0;
		while (retry_count < 5)
		{
			if (0 != LLFile::remove(fullpath))
			{
				retry_count++;
				S32 result = errno;
				LL_WARNS("LLLogChat::deleteTranscripts") << "Problem removing " << fullpath << " - errorcode: "
					<< result << " attempt " << retry_count << LL_ENDL;

				if(retry_count >= 5)
				{
					LL_WARNS("LLLogChat::deleteTranscripts") << "Failed to remove " << fullpath << LL_ENDL;
					return;
				}

				ms_sleep(100);
			}
			else
			{
				if (retry_count)
				{
					LL_WARNS("LLLogChat::deleteTranscripts") << "Successfully removed " << fullpath << LL_ENDL;
				}
				break;
			}			
		}
	}

	LLFloaterIMSessionTab::processChatHistoryStyleUpdate(true);
}

// static
bool LLLogChat::isTranscriptExist(const LLUUID& avatar_id, bool is_group)
{
	LLAvatarName avatar_name;
	LLAvatarNameCache::get(avatar_id, &avatar_name);
	std::string avatar_user_name = avatar_name.getAccountName();
	if(!is_group)
	{
		std::replace(avatar_user_name.begin(), avatar_user_name.end(), '.', '_');
		return isTranscriptFileFound(makeLogFileName(avatar_user_name));
	}
	else
	{
		std::string file_name;
		gCacheName->getGroupName(avatar_id, file_name);
		file_name = makeLogFileName(file_name + GROUP_CHAT_SUFFIX);
		return isTranscriptFileFound(file_name);
	}
	return false;
}

bool LLLogChat::isNearbyTranscriptExist()
{
	return isTranscriptFileFound(makeLogFileName("chat"));;
}

bool LLLogChat::isAdHocTranscriptExist(std::string file_name)
{
	return isTranscriptFileFound(makeLogFileName(file_name));;
}

// static
bool LLLogChat::isTranscriptFileFound(std::string fullname)
{
	bool result = false;
	LLFILE * filep = LLFile::fopen(fullname, "rb");
	if (NULL != filep)
	{
		if (makeLogFileName("chat") == fullname)
		{
			LLFile::close(filep);
			return true;
		}
		char buffer[LOG_RECALL_SIZE];

		fseek(filep, 0, SEEK_END);			// seek to end of file
		S32 bytes_to_read = ftell(filep);	// get current file pointer
		fseek(filep, 0, SEEK_SET);			// seek back to beginning of file

		// limit the number characters to read from file
		if (bytes_to_read >= LOG_RECALL_SIZE)
		{
			bytes_to_read = LOG_RECALL_SIZE - 1;
		}

		if (bytes_to_read > 0 && NULL != fgets(buffer, bytes_to_read, filep))
		{
			//matching a timestamp
			boost::match_results<std::string::const_iterator> matches;
            std::string line(remove_utf8_bom(buffer));
			if (ll_regex_match(line, matches, TIMESTAMP))
			{
				result = true;
			}
		}
		LLFile::close(filep);
	}
	return result;
}

//*TODO mark object's names in a special way so that they will be distinguishable form avatar name
//which are more strict by its nature (only firstname and secondname)
//Example, an object's name can be written like "Object <actual_object's_name>"
void LLChatLogFormatter::format(const LLSD& im, std::ostream& ostr) const
{
	if (!im.isMap())
	{
		LL_WARNS() << "invalid LLSD type of an instant message" << LL_ENDL;
		return;
	}

	if (im[LL_IM_TIME].isDefined())
	{
		std::string timestamp = im[LL_IM_TIME].asString();
		boost::trim(timestamp);
		ostr << '[' << timestamp << ']' << TWO_SPACES;
	}
	
	//*TODO mark object's names in a special way so that they will be distinguishable from avatar name 
	//which are more strict by its nature (only firstname and secondname)
	//Example, an object's name can be written like "Object <actual_object's_name>"
	if (im[LL_IM_FROM].isDefined())
	{
		std::string from = im[LL_IM_FROM].asString();
		boost::trim(from);

		std::size_t found = from.find(IM_SYMBOL_SEPARATOR);
		std::size_t len = from.size();
		std::size_t start = 0;
		while (found != std::string::npos)
		{
			std::size_t sub_len = found - start;
			if (sub_len > 0)
			{
				ostr << from.substr(start, sub_len);
			}
			LLURI::encodeCharacter(ostr, IM_SYMBOL_SEPARATOR);
			start = found + 1;
			found = from.find(IM_SYMBOL_SEPARATOR, start);
		}
		if (start < len)
		{
			std::string str_end = from.substr(start, len - start);
			ostr << str_end;
		}
		if (len > 0)
		{
			ostr << IM_SEPARATOR;
		}
	}

	if (im[LL_IM_TEXT].isDefined())
	{
		std::string im_text = im[LL_IM_TEXT].asString();

		//multilined text will be saved with prepended spaces
		boost::replace_all(im_text, NEW_LINE, NEW_LINE_SPACE_PREFIX);
		ostr << im_text;
	}
}

bool LLChatLogParser::parse(std::string& raw, LLSD& im, const LLSD& parse_params)
{
	if (!raw.length()) return false;
	
	bool cut_off_todays_date = parse_params.has("cut_off_todays_date")  ? parse_params["cut_off_todays_date"].asBoolean()  : true;
	im = LLSD::emptyMap();

	//matching a timestamp
	boost::match_results<std::string::const_iterator> matches;
	if (!ll_regex_match(raw, matches, TIMESTAMP_AND_STUFF)) return false;
	
	bool has_timestamp = matches[IDX_TIMESTAMP].matched;
	if (has_timestamp)
	{
		//timestamp was successfully parsed
		std::string timestamp = matches[IDX_TIMESTAMP];
		boost::trim(timestamp);
		timestamp.erase(0, 1);
		timestamp.erase(timestamp.length()-1, 1);

        im[LL_IM_DATE_TIME] = timestamp;    // Retain full date-time for merging chat histories

        if (cut_off_todays_date)
		{
			LLLogChatTimeScanner::instance().checkAndCutOffDate(timestamp);
		}

		im[LL_IM_TIME] = timestamp;
	}
	else
	{   //timestamp is optional
        im[LL_IM_DATE_TIME] = "";
        im[LL_IM_TIME] = "";
	}

	bool has_stuff = matches[IDX_STUFF].matched;
	if (!has_stuff)
	{
		return false;  //*TODO should return false or not?
	}

	//matching a name and a text
	std::string stuff = matches[IDX_STUFF];
	boost::match_results<std::string::const_iterator> name_and_text;
	if (!ll_regex_match(stuff, name_and_text, NAME_AND_TEXT)) return false;

	bool has_name = name_and_text[IDX_NAME].matched;
	std::string name = LLURI::unescape(name_and_text[IDX_NAME]);

	//we don't need a name/text separator
	if (has_name && name.length() && name[name.length()-1] == ':')
	{
		name.erase(name.length()-1, 1);
	}

	if (!has_name || name == SYSTEM_FROM)
	{
		//name is optional too
		im[LL_IM_FROM] = SYSTEM_FROM;
		im[LL_IM_FROM_ID] = LLUUID::null;
	}

	//possibly a case of complex object names consisting of 3+ words
	if (!has_name)
	{
		std::string::size_type divider_pos = stuff.find(NAME_TEXT_DIVIDER);
		if (divider_pos != std::string::npos && divider_pos < (stuff.length() - NAME_TEXT_DIVIDER.length()))
		{
			im[LL_IM_FROM] = LLURI::unescape(stuff.substr(0, divider_pos));
			im[LL_IM_TEXT] = stuff.substr(divider_pos + NAME_TEXT_DIVIDER.length());
			return true;
		}
	}

	if (!has_name)
	{
		//text is mandatory
		im[LL_IM_TEXT] = stuff;
		return true; //parse as a message from Second Life
	}
	
	bool has_text = name_and_text[IDX_TEXT].matched;
	if (!has_text) return false;

	//for parsing logs created in very old versions of a viewer
	if (name == "You")
	{
		std::string agent_name;
		LLAgentUI::buildFullname(agent_name);
		im[LL_IM_FROM] = agent_name;
		im[LL_IM_FROM_ID] = gAgentID;
	}
	else
	{
		im[LL_IM_FROM] = name;
	}
	
	im[LL_IM_TEXT] = name_and_text[IDX_TEXT];
	return true;  //parsed name and message text, maybe have a timestamp too
}

LLDeleteHistoryThread::LLDeleteHistoryThread(std::list<LLSD>* messages, LLLoadHistoryThread* loadThread)
	: LLActionThread("delete chat history"),
	mMessages(messages),
	mLoadThread(loadThread)
{
}
LLDeleteHistoryThread::~LLDeleteHistoryThread()
{
}
void LLDeleteHistoryThread::run()
{
	if (mLoadThread != NULL)
	{
		mLoadThread->waitFinished();
	}
	if (NULL != mMessages)
	{
		delete mMessages;
	}
	mMessages = NULL;
	setFinished();
}

LLActionThread::LLActionThread(const std::string& name)
	: LLThread(name),
	mMutex(),
	mRunCondition(),
	mFinished(false)
{
}

LLActionThread::~LLActionThread()
{
}

void LLActionThread::waitFinished()
{
	mMutex.lock();
	if (!mFinished)
	{
		mMutex.unlock();
		mRunCondition.wait();
	}
	else
	{
		mMutex.unlock();	
	}
}

void LLActionThread::setFinished()
{
	mMutex.lock();
	mFinished = true;
	mMutex.unlock();
	mRunCondition.signal();
}

LLLoadHistoryThread::LLLoadHistoryThread(const std::string& file_name, std::list<LLSD>* messages, const LLSD& load_params)
	: LLActionThread("load chat history"),
	mMessages(messages),
	mFileName(file_name),
	mLoadParams(load_params),
	mNewLoad(true),
	mLoadEndSignal(NULL)
{
}

LLLoadHistoryThread::~LLLoadHistoryThread()
{
}

void LLLoadHistoryThread::run()
{
	if(mNewLoad)
	{
		loadHistory(mFileName, mMessages, mLoadParams);
		int count = mMessages->size();
		LL_INFOS() << "mMessages->size(): " << count << LL_ENDL;
		setFinished();
	}
}

void LLLoadHistoryThread::loadHistory(const std::string& file_name, std::list<LLSD>* messages, const LLSD& load_params)
{
	if (file_name.empty())
	{
		LL_WARNS("LLLogChat::loadHistory") << "Session name is Empty!" << LL_ENDL;
		return ;
	}

	bool load_all_history = load_params.has("load_all_history") ? load_params["load_all_history"].asBoolean() : false;
	LLFILE* fptr = LLFile::fopen(LLLogChat::makeLogFileName(file_name), "r");/*Flawfinder: ignore*/

	if (!fptr)
	{
		bool is_group = load_params.has("is_group") ? load_params["is_group"].asBoolean() : false;
		if (is_group)
		{
			std::string old_name(file_name);
			old_name.erase(old_name.size() - GROUP_CHAT_SUFFIX.size());
			fptr = LLFile::fopen(LLLogChat::makeLogFileName(old_name), "r");
			if (fptr)
			{
				fclose(fptr);
				LLFile::copy(LLLogChat::makeLogFileName(old_name), LLLogChat::makeLogFileName(file_name));
			}
			fptr = LLFile::fopen(LLLogChat::makeLogFileName(file_name), "r");
		}
		if (!fptr)
		{
			fptr = LLFile::fopen(LLLogChat::oldLogFileName(file_name), "r");/*Flawfinder: ignore*/
			if (!fptr)
			{
				mNewLoad = false;
				(*mLoadEndSignal)(messages, file_name);
				return;						//No previous conversation with this name.
			}
		}
	}

	char buffer[LOG_RECALL_SIZE];		/*Flawfinder: ignore*/

	char *bptr;
	S32 len;
	bool firstline = TRUE;

	if (load_all_history || fseek(fptr, (LOG_RECALL_SIZE - 1) * -1  , SEEK_END))
	{	//We need to load the whole historyFile or it's smaller than recall size, so get it all.
		firstline = FALSE;
		if (fseek(fptr, 0, SEEK_SET))
		{
			fclose(fptr);
			mNewLoad = false;
			(*mLoadEndSignal)(messages, file_name);
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
		std::string line(remove_utf8_bom(buffer));

		//updated 1.23 plaint text log format requires a space added before subsequent lines in a multilined message
		if (' ' == line[0])
		{
			line.erase(0, MULTI_LINE_PREFIX.length());
			append_to_last_message(*messages, '\n' + line);
		}
		else if (0 == len && ('\n' == line[0] || '\r' == line[0]))
		{
			//to support old format's multilined messages with new lines used to divide paragraphs
			append_to_last_message(*messages, line);
		}
		else
		{
			LLSD item;
			if (!LLChatLogParser::parse(line, item, load_params))
			{
				item[LL_IM_TEXT] = line;
			}
			messages->push_back(item);
		}
	}

	fclose(fptr);
	mNewLoad = false;
	(*mLoadEndSignal)(messages, file_name);
}
	
boost::signals2::connection LLLoadHistoryThread::setLoadEndSignal(const load_end_signal_t::slot_type& cb)
{
	if (NULL == mLoadEndSignal)
	{
		mLoadEndSignal = new load_end_signal_t();
	}

	return mLoadEndSignal->connect(cb);
}

void LLLoadHistoryThread::removeLoadEndSignal(const load_end_signal_t::slot_type& cb)
{
	if (NULL != mLoadEndSignal)
	{
		mLoadEndSignal->disconnect_all_slots();
		delete mLoadEndSignal;
	}
	mLoadEndSignal = NULL;
}
