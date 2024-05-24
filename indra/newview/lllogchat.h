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
#include "llthread.h"

class LLChat;

class LLActionThread : public LLThread
{
public:
    LLActionThread(const std::string& name);
    ~LLActionThread();

    void waitFinished();
    bool isFinished() { return mFinished; }
protected:
    void setFinished();
private:
    bool mFinished;
    LLMutex mMutex;
    LLCondition mRunCondition;
};

class LLLoadHistoryThread : public LLActionThread
{
private:
    const std::string& mFileName;
    std::list<LLSD>* mMessages;
    LLSD mLoadParams;
    bool mNewLoad;
public:
    LLLoadHistoryThread(const std::string& file_name, std::list<LLSD>* messages, const LLSD& load_params);
    ~LLLoadHistoryThread();
    //void setHistoryParams(const std::string& file_name, const LLSD& load_params);
    virtual void loadHistory(const std::string& file_name, std::list<LLSD>* messages, const LLSD& load_params);
    virtual void run();

    typedef boost::signals2::signal<void (std::list<LLSD>* messages,const std::string& file_name)> load_end_signal_t;
    load_end_signal_t * mLoadEndSignal;
    boost::signals2::connection setLoadEndSignal(const load_end_signal_t::slot_type& cb);
    void removeLoadEndSignal(const load_end_signal_t::slot_type& cb);
};

class LLDeleteHistoryThread : public LLActionThread
{
private:
    std::list<LLSD>* mMessages;
    LLLoadHistoryThread* mLoadThread;
public:
    LLDeleteHistoryThread(std::list<LLSD>* messages, LLLoadHistoryThread* loadThread);
    ~LLDeleteHistoryThread();

    virtual void run();
    static void deleteHistory();
};

class LLLogChat : public LLSingleton<LLLogChat>
{
    LLSINGLETON(LLLogChat);
    ~LLLogChat();
public:
    // status values for callback function
    enum ELogLineType {
        LOG_EMPTY,
        LOG_LINE,
        LOG_LLSD,
        LOG_END
    };

    static std::string timestamp2LogString(U32 timestamp, bool withdate);
    static std::string makeLogFileName(std::string(filename));
    static void renameLogFile(const std::string& old_filename, const std::string& new_filename);
    /**
    *Add functions to get old and non date stamped file names when needed
    */
    static std::string oldLogFileName(std::string(filename));
    static void saveHistory(const std::string& filename,
                const std::string& from,
                const LLUUID& from_id,
                const std::string& line);
    static bool transcriptFilesExist();
    static void findTranscriptFiles(std::string pattern, std::vector<std::string>& list_of_transcriptions);
    static void getListOfTranscriptFiles(std::vector<std::string>& list);
    static void getListOfTranscriptBackupFiles(std::vector<std::string>& list_of_transcriptions);

    static void loadChatHistory(const std::string& file_name, std::list<LLSD>& messages, const LLSD& load_params = LLSD(), bool is_group = false);

    typedef boost::signals2::signal<void ()> save_history_signal_t;
    boost::signals2::connection setSaveHistorySignal(const save_history_signal_t::slot_type& cb);

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
    static bool isAdHocTranscriptExist(std::string file_name);
    static bool isTranscriptFileFound(std::string fullname);

    bool historyThreadsFinished(LLUUID session_id);
    LLLoadHistoryThread* getLoadHistoryThread(LLUUID session_id);
    LLDeleteHistoryThread* getDeleteHistoryThread(LLUUID session_id);
    bool addLoadHistoryThread(LLUUID& session_id, LLLoadHistoryThread* lthread);
    bool addDeleteHistoryThread(LLUUID& session_id, LLDeleteHistoryThread* dthread);
    void cleanupHistoryThreads();

private:
    static std::string cleanFileName(std::string filename);

    LLMutex* historyThreadsMutex();
    void triggerHistorySignal();

    save_history_signal_t * mSaveHistorySignal;
    std::map<LLUUID,LLLoadHistoryThread *> mLoadHistoryThreads;
    std::map<LLUUID,LLDeleteHistoryThread *> mDeleteHistoryThreads;
    LLMutex* mHistoryThreadsMutex;
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

extern const std::string GROUP_CHAT_SUFFIX;

// LLSD map lookup constants
extern const std::string LL_IM_TIME; //("time");
extern const std::string LL_IM_DATE_TIME; //("datetime");
extern const std::string LL_IM_TEXT; //("message");
extern const std::string LL_IM_FROM; //("from");
extern const std::string LL_IM_FROM_ID; //("from_id");
extern const std::string LL_TRANSCRIPT_FILE_EXTENSION; //("txt");

#endif
