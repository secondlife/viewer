/** 
 * @file llurl.cpp
 * @brief Text url class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include "llurl.h"
#include "llerror.h"

LLURL::LLURL()
{
	init("");
}

LLURL::LLURL(const LLURL &url)
{
	if (this != &url)
	{
		init(url.getFQURL());
	}
	else
	{
		init("");
	}
}

LLURL::LLURL(const char * url)
{
	init(url);
}

LLURL::~LLURL()
{
	free();
}

void LLURL::init(const char * url)
{
	mURI[0] = '\0';
	mAuthority[0] = '\0';
	mPath[0] = '\0';
	mFilename[0] = '\0';
	mExtension[0] = '\0';
	mTag[0] = '\0';
	
	char url_copy[MAX_STRING];		/* Flawfinder: ignore */

	strncpy (url_copy,url, MAX_STRING -1);		/* Flawfinder: ignore */
	url_copy[MAX_STRING -1] = '\0';

	char *parse;
	char *leftover_url = url_copy;
	S32 span = 0;

	// copy and lop off tag
	if ((parse = strchr(url_copy,'#')))
	{
		strncpy(mTag,parse+1, LL_MAX_PATH -1);		/* Flawfinder: ignore */
		mTag[LL_MAX_PATH -1] = '\0';
		*parse = '\0';
	}

	// copy out URI if it exists, update leftover_url 
	if ((parse = strchr(url_copy,':')))
	{
		*parse = '\0';
		strncpy(mURI,leftover_url, LL_MAX_PATH -1);		/* Flawfinder: ignore */
		mURI[LL_MAX_PATH -1] = '\0';
		leftover_url = parse + 1;
	}

	// copy out authority if it exists, update leftover_url
	if ((leftover_url[0] == '/') && (leftover_url[1] == '/'))
	{
		leftover_url += 2; // skip the "//"

		span = strcspn(leftover_url, "/"); 
		strncat(mAuthority,leftover_url,span);		/* Flawfinder: ignore */
		leftover_url += span;
	}
	
	if ((parse = strrchr(leftover_url,'.')))
	{
		// copy and lop off extension
		strncpy(mExtension,parse+1, LL_MAX_PATH -1);		/* Flawfinder: ignore */
		mExtension[LL_MAX_PATH -1] = '\0';
		*parse = '\0';
	}

	if ((parse = strrchr(leftover_url,'/')))
	{
		parse++;
	}
	else
	{
		parse = leftover_url;
	}

	// copy and lop off filename
	strncpy(mFilename,parse, LL_MAX_PATH -1);/* Flawfinder: ignore */
	mFilename[LL_MAX_PATH -1] = '\0';
	*parse = '\0';

	// what's left should be the path
	strncpy(mPath,leftover_url, LL_MAX_PATH -1);		/* Flawfinder: ignore */
	mPath[LL_MAX_PATH -1] = '\0';

//	llinfos << url << "  decomposed into: " << llendl;
//	llinfos << "  URI : <" << mURI << ">" << llendl;
//	llinfos << "  Auth: <" << mAuthority << ">" << llendl;
//	llinfos << "  Path: <" << mPath << ">" << llendl;
//	llinfos << "  File: <" << mFilename << ">" << llendl;
//	llinfos << "  Ext : <" << mExtension << ">" << llendl;
//	llinfos << "  Tag : <" << mTag << ">" << llendl;
}

void LLURL::free()
{
}

// Copy assignment
LLURL &LLURL::operator=(const LLURL &rhs)
{
	if (this != &rhs)
	{
		this->init(rhs.getFQURL());
	}
	
	return *this;
}

// Compare
bool LLURL::operator==(const LLURL &rhs) const
{
	if ((strcmp(mURI, rhs.mURI))
		|| (strcmp(mAuthority, rhs.mAuthority))
		|| (strcmp(mPath, rhs.mPath))
		|| (strcmp(mFilename, rhs.mFilename))
		|| (strcmp(mExtension, rhs.mExtension))
		|| (strcmp(mTag, rhs.mTag))
		)
	{
		return FALSE;
	}
	return TRUE;
}

bool LLURL::operator!=(const LLURL& rhs) const
{
	return !(*this == rhs);
}

const char * LLURL::getFQURL() const
{
	char        fqurl[LL_MAX_PATH];		/* Flawfinder: ignore */

	fqurl[0] = '\0';

	if (mURI[0])
	{
		strncat(fqurl,mURI, LL_MAX_PATH - strlen(fqurl) -1);		/* Flawfinder: ignore */
		strcat(fqurl,":");		/* Flawfinder: ignore */
		if (mAuthority[0])
		{
			strcat(fqurl,"//");		/* Flawfinder: ignore */
		}
	}

	if (mAuthority[0])
	{
		strncat(fqurl,mAuthority, LL_MAX_PATH - strlen(fqurl) -1);		/* Flawfinder: ignore */
	}

	strncat(fqurl,mPath, LL_MAX_PATH - strlen(fqurl) -1);		/* Flawfinder: ignore */

	strncat(fqurl,mFilename, LL_MAX_PATH - strlen(fqurl) -1);		/* Flawfinder: ignore */
	
	if (mExtension[0])
	{
		strcat(fqurl,".");		/* Flawfinder: ignore */
		strncat(fqurl,mExtension, LL_MAX_PATH - strlen(fqurl) -1);		/* Flawfinder: ignore */
	}

	if (mTag[0])
	{
		strcat(fqurl,"#");		/* Flawfinder: ignore */
		strncat(fqurl,mTag, LL_MAX_PATH - strlen(fqurl) -1);		/* Flawfinder: ignore */
	}

	strncpy(LLURL::sReturnString,fqurl, LL_MAX_PATH -1);		/* Flawfinder: ignore */
	LLURL::sReturnString[LL_MAX_PATH -1] = '\0';

	return(LLURL::sReturnString);
}


const char* LLURL::updateRelativePath(const LLURL &url)
{
	char new_path[LL_MAX_PATH];		/* Flawfinder: ignore */
	char tmp_path[LL_MAX_PATH];		/* Flawfinder: ignore */

	char *parse;

	if (mPath[0] != '/')
	{
		//start with existing path
		strncpy (new_path,url.mPath, LL_MAX_PATH -1);		/* Flawfinder: ignore */
		new_path[LL_MAX_PATH -1] = '\0';
		strncpy (tmp_path,mPath, LL_MAX_PATH -1);		/* Flawfinder: ignore */
		tmp_path[LL_MAX_PATH -1] = '\0';

		parse = strtok(tmp_path,"/");
		while (parse)
		{
			if (!strcmp(parse,"."))
			{
				// skip this, it's meaningless
			}
			else if (!strcmp(parse,".."))
			{
				if ((parse = strrchr(new_path, '/')))
				{
					*parse = '\0';
					if ((parse = strrchr(new_path, '/')))
					{
						*(parse+1) = '\0';
					}
					else
					{
						new_path[0] = '\0';
					}
				}
				else
				{
					strcat(new_path,"../");		/* Flawfinder: ignore */
				}

			}
			else 
			{
				strncat(new_path,parse, LL_MAX_PATH - strlen(new_path) -1 );		/* Flawfinder: ignore */
				strcat(new_path,"/");		/* Flawfinder: ignore */
			}
			parse = strtok(NULL,"/");
		}
		strncpy(mPath,new_path, LL_MAX_PATH -1);		/* Flawfinder: ignore */
		mPath[LL_MAX_PATH -1] = '\0';
	}
	return mPath;
}

const char * LLURL::getFullPath()
{
	strncpy(LLURL::sReturnString,mPath, LL_MAX_PATH -1);		/* Flawfinder: ignore */
	LLURL::sReturnString[LL_MAX_PATH -1] = '\0';
	strncat(LLURL::sReturnString,mFilename, LL_MAX_PATH - strlen(LLURL::sReturnString) -1);		/* Flawfinder: ignore */
	strcat(LLURL::sReturnString,".");		/* Flawfinder: ignore */
	strncat(LLURL::sReturnString,mExtension, LL_MAX_PATH - strlen(LLURL::sReturnString) -1);		/* Flawfinder: ignore */
	return(sReturnString);
}


char LLURL::sReturnString[LL_MAX_PATH] = "";
