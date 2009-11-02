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

#include "lllogchat.h"
#include "llappviewer.h"
#include "llfloaterchat.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"

const S32 LOG_RECALL_SIZE = 2048;

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
				  +LLTrans::getString ("TimeMin")+"] ";
	}
	else
	{
		timeStr = "[" + LLTrans::getString("TimeHour") + "]:["
			      + LLTrans::getString ("TimeMin")+"] "; 
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
	if (!gSavedPerAccountSettings.getBOOL("LogInstantMessages"))
		return;

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

	file << LLSDOStreamer <LLSDNotationFormatter>(item) << std::endl;

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

		// the parser's destructor is protected so we cannot create in the stack.
		LLPointer<LLSDParser> parser = new LLSDNotationParser();

		while ( fgets(buffer, LOG_RECALL_SIZE, fptr)  && !feof(fptr) ) 
		{
			len = strlen(buffer) - 1;		/*Flawfinder: ignore*/
			for ( bptr = (buffer + len); (*bptr == '\n' || *bptr == '\r') && bptr>buffer; bptr--)	*bptr='\0';
			
			if (!firstline)
			{
				LLSD item;
				std::string line(buffer);
				std::istringstream iss(line);
				if (parser->parse(iss, item, line.length()) == LLSDParser::PARSE_FAILURE)
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
