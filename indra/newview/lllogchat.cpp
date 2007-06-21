/** 
 * @file lllogchat.cpp
 * @brief LLLogChat class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lllogchat.h"
#include "viewer.h"
	
const S32 LOG_RECALL_SIZE = 2048;

//static
LLString LLLogChat::makeLogFileName(LLString filename)
{
	filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS,filename.c_str());
	filename += ".txt";
	return filename;
}

LLString LLLogChat::timestamp(bool withdate)
{
	U32 utc_time;
	utc_time = time_corrected();

	// There's only one internal tm buffer.
	struct tm* timep;

	// Convert to Pacific, based on server's opinion of whether
	// it's daylight savings time there.
	timep = utc_to_pacific_time(utc_time, gPacificDaylightTime);

	LLString text;
	if (withdate)
		text = llformat("[%d/%02d/%02d %d:%02d]  ", (timep->tm_year-100)+2000, timep->tm_mon+1, timep->tm_mday, timep->tm_hour, timep->tm_min);
	else
		text = llformat("[%d:%02d]  ", timep->tm_hour, timep->tm_min);

	return text;
}


//static
void LLLogChat::saveHistory(LLString filename, LLString line)
{
	if(!filename.size())
	{
		llinfos << "Filename is Empty!" << llendl;
		return;
	}

	FILE* fp = LLFile::fopen(LLLogChat::makeLogFileName(filename).c_str(), "a"); 		/*Flawfinder: ignore*/
	if (!fp)
	{
		llinfos << "Couldn't open chat history log!" << llendl;
	}
	else
	{
		fprintf(fp, "%s\n", line.c_str());
		
		fclose (fp);
	}
}

void LLLogChat::loadHistory(LLString filename , void (*callback)(LLString,void*), void* userdata)
{
	if(!filename.size())
	{
		llerrs << "Filename is Empty!" << llendl;
	}

	FILE* fptr = LLFile::fopen(makeLogFileName(filename).c_str(), "r");		/*Flawfinder: ignore*/
	if (!fptr)
	{
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
				callback(buffer,userdata);
			}
			else
			{
				firstline = FALSE;
			}
		}
		callback("-- End of Log ---",userdata);
		
		fclose(fptr);
	}
}
