/** 
 * @file lllogchat.h
 * @brief LLFloaterChat class definition
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

#ifndef LL_LLLOGCHAT_H
#define LL_LLLOGCHAT_H

class LLChat;

class LLLogChat
{
public:
	// status values for callback function
	enum ELogLineType {
		LOG_EMPTY,
		LOG_LINE,
		LOG_LLSD,
		LOG_END
	};
	static std::string timestamp(bool withdate = false);
	static std::string makeLogFileName(std::string(filename));
	/**
	*Add functions to get old and non date stamped file names when needed
	*/
	static std::string oldLogFileName(std::string(filename));
	static void saveHistory(const std::string& filename,
				const std::string& from,
				const LLUUID& from_id,
				const std::string& line);
	static void findTranscriptFiles(std::string pattern, std::vector<std::string>& list_of_transcriptions);
	static void getListOfTranscriptFiles(std::vector<std::string>& list);
	static void getListOfTranscriptBackupFiles(std::vector<std::string>& list_of_transcriptions);

	static void loadChatHistory(const std::string& file_name, std::list<LLSD>& messages, const LLSD& load_params = LLSD());

	typedef boost::signals2::signal<void ()> save_history_signal_t;
	static boost::signals2::connection setSaveHistorySignal(const save_history_signal_t::slot_type& cb);

	static bool moveTranscripts(const std::string currentDirectory, 
									const std::string newDirectory, 
									std::vector<std::string>& listOfFilesToMove,
									std::vector<std::string>& listOfFilesMoved);
	static bool moveTranscripts(const std::string currentDirectory, 
		const std::string newDirectory, 
		std::vector<std::string>& listOfFilesToMove);

	static void deleteTranscripts();
	static bool isTranscriptExist(const LLUUID& avatar_id, bool is_group=false);
	static bool isNearbyTranscriptExist();

private:
	static std::string cleanFileName(std::string filename);
	static save_history_signal_t * sSaveHistorySignal;
};

/**
 * Formatter for the plain text chat log files
 */
class LLChatLogFormatter
{
public:
	LLChatLogFormatter(const LLSD& im) : mIM(im) {}
	virtual ~LLChatLogFormatter() {};

	friend std::ostream& operator<<(std::ostream& str, const LLChatLogFormatter& formatter)
	{
		formatter.format(formatter.mIM, str);
		return str;
	}

protected:

	/**
	 * Format an instant message to a stream
	 * Timestamps and sender names are required
	 * New lines of multilined messages are prepended with a space
	 */
	void format(const LLSD& im, std::ostream& ostr) const;

	LLSD mIM;
};

/**
 * Parser for the plain text chat log files
 */
class LLChatLogParser
{
public:

	 /**
	 * Parse a line from the plain text chat log file
	 * General plain text log format is like: "[timestamp]  [name]: [message]"
	 * [timestamp] and [name] are optional
	 * Examples of plain text chat log lines:
	 * "[2009/11/20 2:53]  Igor ProductEngine: howdy"
	 * "Igor ProductEngine: howdy"
	 * "Dserduk ProductEngine is Online"
	 *
	 * @return false if failed to parse mandatory data - message text
	 */
	static bool parse(std::string& raw, LLSD& im, const LLSD& parse_params = LLSD());

protected:
	LLChatLogParser();
	virtual ~LLChatLogParser() {};
};

// LLSD map lookup constants
extern const std::string LL_IM_TIME; //("time");
extern const std::string LL_IM_TEXT; //("message");
extern const std::string LL_IM_FROM; //("from");
extern const std::string LL_IM_FROM_ID; //("from_id");
extern const std::string LL_TRANSCRIPT_FILE_EXTENSION; //("txt");

#endif
