/** 
 * @file llfile.cpp
 * @author Michael Schlachter
 * @date 2006-03-23
 * @brief Implementation of cross-platform POSIX file buffer and c++
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

#include "linden_common.h"
#include "llfile.h"
#include "llstring.h"
#include "llerror.h"

using namespace std;

// static
int	LLFile::mkdir(const	char* dirname, int perms)
{
#if LL_WINDOWS	
	// permissions are ignored on Windows
	std::string utf8dirname = dirname;
	llutf16string utf16dirname = utf8str_to_utf16str(utf8dirname);
	return _wmkdir(utf16dirname.c_str());
#else
	return ::mkdir(dirname, (mode_t)perms);
#endif
}

// static
int	LLFile::rmdir(const	char* dirname)
{
#if LL_WINDOWS	
	// permissions are ignored on Windows
	std::string utf8dirname = dirname;
	llutf16string utf16dirname = utf8str_to_utf16str(utf8dirname);
	return _wrmdir(utf16dirname.c_str());
#else
	return ::rmdir(dirname);
#endif
}

// static
LLFILE*	LLFile::fopen(const	char* filename, const char* mode)	/* Flawfinder: ignore */
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8mode = mode;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16mode = utf8str_to_utf16str(utf8mode);
	return _wfopen(utf16filename.c_str(),utf16mode.c_str());
#else
	return ::fopen(filename,mode);	/* Flawfinder: ignore */
#endif
}

LLFILE*	LLFile::_fsopen(const char* filename, const char* mode, int sharingFlag)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8mode = mode;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16mode = utf8str_to_utf16str(utf8mode);
	return _wfsopen(utf16filename.c_str(),utf16mode.c_str(),sharingFlag);
#else
	llassert(0);//No corresponding function on non-windows
	return NULL;
#endif
}

int	LLFile::remove(const char* filename)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	return _wremove(utf16filename.c_str());
#else
	return ::remove(filename);
#endif
}

int	LLFile::rename(const char* filename, const char* newname)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8newname = newname;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16newname = utf8str_to_utf16str(utf8newname);
	return _wrename(utf16filename.c_str(),utf16newname.c_str());
#else
	return ::rename(filename,newname);
#endif
}

int	LLFile::stat(const char* filename, llstat* filestatus)
{
#if LL_WINDOWS
	std::string utf8filename = filename;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	return _wstat(utf16filename.c_str(),filestatus);
#else
	return ::stat(filename,filestatus);
#endif
}


/***************** Modified file stream created to overcome the incorrect behaviour of posix fopen in windows *******************/

#if USE_LLFILESTREAMS

LLFILE *	LLFile::_Fiopen(const char *filename, std::ios::openmode mode,int)	// protection currently unused
{	// open a file
	static const char *mods[] =
	{	// fopen mode strings corresponding to valid[i]
	"r", "w", "w", "a", "rb", "wb", "wb", "ab",
	"r+", "w+", "a+", "r+b", "w+b", "a+b",
	0};
	static const int valid[] =
	{	// valid combinations of open flags
		ios_base::in,
		ios_base::out,
		ios_base::out | ios_base::trunc,
		ios_base::out | ios_base::app,
		ios_base::in | ios_base::binary,
		ios_base::out | ios_base::binary,
		ios_base::out | ios_base::trunc | ios_base::binary,
		ios_base::out | ios_base::app | ios_base::binary,
		ios_base::in | ios_base::out,
		ios_base::in | ios_base::out | ios_base::trunc,
		ios_base::in | ios_base::out | ios_base::app,
		ios_base::in | ios_base::out | ios_base::binary,
		ios_base::in | ios_base::out | ios_base::trunc
			| ios_base::binary,
		ios_base::in | ios_base::out | ios_base::app
			| ios_base::binary,
	0};

	LLFILE *fp = 0;
	int n;
	ios_base::openmode atendflag = mode & ios_base::ate;
	ios_base::openmode norepflag = mode & ios_base::_Noreplace;

	if (mode & ios_base::_Nocreate)
		mode |= ios_base::in;	// file must exist
	mode &= ~(ios_base::ate | ios_base::_Nocreate | ios_base::_Noreplace);
	for (n = 0; valid[n] != 0 && valid[n] != mode; ++n)
		;	// look for a valid mode

	if (valid[n] == 0)
		return (0);	// no valid mode
	else if (norepflag && mode & (ios_base::out || ios_base::app)
		&& (fp = LLFile::fopen(filename, "r")) != 0)	/* Flawfinder: ignore */
		{	// file must not exist, close and fail
		fclose(fp);
		return (0);
		}
	else if (fp != 0 && fclose(fp) != 0)
		return (0);	// can't close after test open
// should open with protection here, if other than default
	else if ((fp = LLFile::fopen(filename, mods[n])) == 0)	/* Flawfinder: ignore */
		return (0);	// open failed

	if (!atendflag || fseek(fp, 0, SEEK_END) == 0)
		return (fp);	// no need to seek to end, or seek succeeded

	fclose(fp);	// can't position at end
	return (0);
}

/************** input file stream ********************************/

void llifstream::close()
{	// close the C stream
	if (_Filebuffer && _Filebuffer->close() == 0)
	{
		_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
	}
}

void llifstream::open(const char* _Filename,	/* Flawfinder: ignore */
	ios_base::openmode _Mode,
	int _Prot)
{	// open a C stream with specified mode
	
	LLFILE* filep = LLFile::_Fiopen(_Filename,_Mode | ios_base::in, _Prot);
	if(filep == NULL)
	{
		_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
		return;
	}
	llassert(_Filebuffer == NULL);
	_Filebuffer = new _Myfb(filep);
	_ShouldClose = true;
	_Myios::init(_Filebuffer);
}

bool llifstream::is_open() const
{	// test if C stream has been opened
	if(_Filebuffer)
		return (_Filebuffer->is_open());
	return false;
}
llifstream::~llifstream()
{	
	if (_ShouldClose)
	{
		close();
	}
	delete _Filebuffer;
}

llifstream::llifstream(const char *_Filename,
	ios_base::openmode _Mode,
	int _Prot)
	: std::basic_istream< char , std::char_traits< char > >(NULL,true),_Filebuffer(NULL),_ShouldClose(false)

{	// construct with named file and specified mode
	open(_Filename, _Mode | ios_base::in, _Prot);	/* Flawfinder: ignore */
}


/************** output file stream ********************************/

bool llofstream::is_open() const
{	// test if C stream has been opened
	if(_Filebuffer)
		return (_Filebuffer->is_open());
	return false;
}

void llofstream::open(const char* _Filename,	/* Flawfinder: ignore */
	ios_base::openmode _Mode,
	int _Prot)	
{	// open a C stream with specified mode

	LLFILE* filep = LLFile::_Fiopen(_Filename,_Mode | ios_base::out, _Prot);
	if(filep == NULL)
	{
		_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
		return;
	}
	llassert(_Filebuffer==NULL);
	_Filebuffer = new _Myfb(filep);
	_ShouldClose = true;
	_Myios::init(_Filebuffer);
}

void llofstream::close()
{	// close the C stream
	if(is_open())
	{
		if (_Filebuffer->close() == 0)
			_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
	}
}

llofstream::llofstream(const char *_Filename,
	std::ios_base::openmode _Mode,
	int _Prot) 
		: std::basic_ostream<char,std::char_traits < char > >(NULL,true),_Filebuffer(NULL),_ShouldClose(false)
{	// construct with named file and specified mode
	open(_Filename, _Mode , _Prot);	/* Flawfinder: ignore */
}

llofstream::~llofstream()
{	
	// destroy the object
	if (_ShouldClose)
	{
		close();
	}
	delete _Filebuffer;
}

#endif // #if USE_LLFILESTREAMS

/************** helper functions ********************************/

std::streamsize llifstream_size(llifstream& ifstr)
{
	if(!ifstr.is_open()) return 0;
	std::streampos pos_old = ifstr.tellg();
	ifstr.seekg(0, ios_base::beg);
	std::streampos pos_beg = ifstr.tellg();
	ifstr.seekg(0, ios_base::end);
	std::streampos pos_end = ifstr.tellg();
	ifstr.seekg(pos_old, ios_base::beg);
	return pos_end - pos_beg;
}

std::streamsize llofstream_size(llofstream& ofstr)
{
	if(!ofstr.is_open()) return 0;
	std::streampos pos_old = ofstr.tellp();
	ofstr.seekp(0, ios_base::beg);
	std::streampos pos_beg = ofstr.tellp();
	ofstr.seekp(0, ios_base::end);
	std::streampos pos_end = ofstr.tellp();
	ofstr.seekp(pos_old, ios_base::beg);
	return pos_end - pos_beg;
}


