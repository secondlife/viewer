/** 
 * @file llurl.cpp
 * @brief Text url class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include <string.h>
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
	
	char url_copy[MAX_STRING];

	strcpy (url_copy,url);

	char *parse;
	char *leftover_url = url_copy;
	S32 span = 0;

	// copy and lop off tag
	if ((parse = strchr(url_copy,'#')))
	{
		strcpy(mTag,parse+1);
		*parse = '\0';
	}

	// copy out URI if it exists, update leftover_url 
	if ((parse = strchr(url_copy,':')))
	{
		*parse = '\0';
		strcpy(mURI,leftover_url);
		leftover_url = parse + 1;
	}

	// copy out authority if it exists, update leftover_url
	if ((leftover_url[0] == '/') && (leftover_url[1] == '/'))
	{
		leftover_url += 2; // skip the "//"

		span = strcspn(leftover_url, "/"); 
		strncat(mAuthority,leftover_url,span);
		leftover_url += span;
	}
	
	if ((parse = strrchr(leftover_url,'.')))
	{
		// copy and lop off extension
		strcpy(mExtension,parse+1);
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
	strcpy(mFilename,parse);
	*parse = '\0';

	// what's left should be the path
	strcpy(mPath,leftover_url);

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
	char        fqurl[LL_MAX_PATH];

	fqurl[0] = '\0';

	if (mURI[0])
	{
		strcat(fqurl,mURI);
		strcat(fqurl,":");
		if (mAuthority[0])
		{
			strcat(fqurl,"//");
		}
	}

	if (mAuthority[0])
	{
		strcat(fqurl,mAuthority);
	}

	strcat(fqurl,mPath);

	strcat(fqurl,mFilename);
	
	if (mExtension[0])
	{
		strcat(fqurl,".");
		strcat(fqurl,mExtension);
	}

	if (mTag[0])
	{
		strcat(fqurl,"#");
		strcat(fqurl,mTag);
	}

	strcpy(LLURL::sReturnString,fqurl);

	return(LLURL::sReturnString);
}


const char* LLURL::updateRelativePath(const LLURL &url)
{
	char new_path[LL_MAX_PATH];
	char tmp_path[LL_MAX_PATH];

	char *parse;

	if (mPath[0] != '/')
	{
		//start with existing path
		strcpy (new_path,url.mPath);
		strcpy (tmp_path,mPath);

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
					strcat(new_path,"../");
				}

			}
			else 
			{
				strcat(new_path,parse);
				strcat(new_path,"/");
			}
			parse = strtok(NULL,"/");
		}
		strcpy(mPath,new_path);
	}
	return mPath;
}

const char * LLURL::getFullPath()
{
	strcpy(LLURL::sReturnString,mPath);
	strcat(LLURL::sReturnString,mFilename);
	strcat(LLURL::sReturnString,".");
	strcat(LLURL::sReturnString,mExtension);
	return(sReturnString);
}


char LLURL::sReturnString[LL_MAX_PATH] = "";
