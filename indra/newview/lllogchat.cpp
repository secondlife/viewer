/** 
 * @file lllogchat.cpp
 * @brief LLLogChat class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lllogchat.h"

const S32 LOG_RECALL_SIZE = 2048;

//static
LLString LLLogChat::makeLogFileName(LLString filename)
{
	
	filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_ACCOUNT_CHAT_LOGS,filename.c_str());
	filename += ".txt";
	return filename;
}

//static
void LLLogChat::saveHistory(LLString filename, LLString line)
{
	FILE *fp = LLFile::fopen(LLLogChat::makeLogFileName(filename).c_str(), "a"); 
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
	FILE *fptr = LLFile::fopen(makeLogFileName(filename).c_str(), "r");
	if (!fptr)
	{
		return;			//No previous conversation with this name.
	}
	else
	{
		char buffer[LOG_RECALL_SIZE];
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
			len = strlen(buffer) - 1;
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
