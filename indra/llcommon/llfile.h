/** 
 * @file llfile.h
 * @author Michael Schlachter
 * @date 2006-03-23
 * @brief Declaration of cross-platform POSIX file buffer and c++
 * stream classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLFILE_H
#define LL_LLFILE_H

/**
 * This class provides a cross platform interface to the filesystem.
 * Attempts to mostly mirror the POSIX style IO functions.
 */

typedef FILE	LLFILE;

#ifdef LL_WINDOWS
#define	USE_LLFILESTREAMS	1
#else
#define	USE_LLFILESTREAMS	0
#endif


#if LL_WINDOWS
// windows version of stat function and stat data structure are called _stat
typedef struct _stat	llstat;
#else
#include <sys/stat.h>
typedef struct stat		llstat;
#endif

class	LLFile
{
public:
	// All these functions take UTF8 path/filenames.
	static	LLFILE*	fopen(const char* filename,const char* accessmode);	/* Flawfinder: ignore */
	static	LLFILE*	_fsopen(const char* filename,const char* accessmode,int	sharingFlag);

	// perms is a permissions mask like 0777 or 0700.  In most cases it will
	// be overridden by the user's umask.  It is ignored on Windows.
	static	int		mkdir(const char* filename, int perms = 0700);

	static	int		rmdir(const char* filename);
	static	int		remove(const char* filename);
	static	int		rename(const char* filename,const char*	newname);
	static	int		stat(const char*	filename,llstat*	file_status);
	static	LLFILE *	_Fiopen(const char *filename, std::ios::openmode mode,int);	// protection currently unused
};


#if USE_LLFILESTREAMS

class	llifstream	:	public	std::basic_istream < char , std::char_traits < char > >
{
	// input stream associated with a C stream
public:
	typedef std::basic_ifstream<char,std::char_traits < char > > _Myt;
	typedef std::basic_filebuf<char,std::char_traits< char > > _Myfb;
	typedef std::basic_ios<char,std::char_traits< char > > _Myios;

	llifstream()
		: std::basic_istream<char,std::char_traits< char > >(NULL,true),_Filebuffer(NULL),_ShouldClose(false)
	{	// construct unopened
	}

	explicit llifstream(const char *_Filename,
		ios_base::openmode _Mode = ios_base::in,
		int _Prot = (int)ios_base::_Openprot);

	explicit llifstream(_Filet *_File)
		: std::basic_istream<char,std::char_traits< char > >(NULL,true),
			_Filebuffer(new _Myfb(_File)),
			_ShouldClose(false)
	{	// construct with specified C stream
	}
	virtual ~llifstream();

	_Myfb *rdbuf() const
	{	// return pointer to file buffer
		return _Filebuffer;
	}
	bool is_open() const;
	void open(const char* _Filename,	/* Flawfinder: ignore */
		ios_base::openmode _Mode = ios_base::in,
		int _Prot = (int)ios_base::_Openprot);	
	void close();

private:
	_Myfb* _Filebuffer;	// the file buffer
	bool _ShouldClose;
};


class	llofstream	:	public	std::basic_ostream< char , std::char_traits < char > >
{
public:
	typedef std::basic_ostream< char , std::char_traits < char > > _Myt;
	typedef std::basic_filebuf< char , std::char_traits < char > > _Myfb;
	typedef std::basic_ios<char,std::char_traits < char > > _Myios;

	llofstream()
		: std::basic_ostream<char,std::char_traits < char > >(NULL,true),_Filebuffer(NULL)
	{	// construct unopened
	}

	explicit llofstream(const char *_Filename,
		std::ios_base::openmode _Mode = ios_base::out,
		int _Prot = (int)std::ios_base::_Openprot);
	

	explicit llofstream(_Filet *_File)
		: std::basic_ostream<char,std::char_traits < char > >(NULL,true),
			_Filebuffer(new _Myfb(_File))//_File)
	{	// construct with specified C stream
	}

	virtual ~llofstream();

	_Myfb *rdbuf() const
	{	// return pointer to file buffer
		return _Filebuffer;
	}

	bool is_open() const;

	void open(const char *_Filename,ios_base::openmode _Mode = ios_base::out,int _Prot = (int)ios_base::_Openprot);	/* Flawfinder: ignore */

	void close();

private:
	_Myfb *_Filebuffer;	// the file buffer
};



#else
//Use standard file streams on non windows platforms
#define	llifstream	std::ifstream
#define	llofstream	std::ofstream

#endif

/**
 * @breif filesize helpers.
 *
 * The file size helpers are not considered particularly efficient,
 * and should only be used for config files and the like -- not in a
 * loop.
 */
std::streamsize llifstream_size(llifstream& fstr);
std::streamsize llofstream_size(llofstream& fstr);

#endif // not LL_LLFILE_H
