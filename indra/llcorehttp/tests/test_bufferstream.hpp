/** 
 * @file test_bufferstream.hpp
 * @brief unit tests for the LLCore::BufferArrayStreamBuf/BufferArrayStream classes
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#ifndef TEST_LLCORE_BUFFER_STREAM_H_
#define TEST_LLCORE_BUFFER_STREAM_H_

#include "bufferstream.h"

#include <iostream>

#include "test_allocator.h"
#include "llsd.h"
#include "llsdserialize.h"


using namespace LLCore;


namespace tut
{

struct BufferStreamTestData
{
	// the test objects inherit from this so the member functions and variables
	// can be referenced directly inside of the test functions.
	size_t mMemTotal;
};

typedef test_group<BufferStreamTestData> BufferStreamTestGroupType;
typedef BufferStreamTestGroupType::object BufferStreamTestObjectType;
BufferStreamTestGroupType BufferStreamTestGroup("BufferStream Tests");
typedef BufferArrayStreamBuf::traits_type tst_traits_t;


template <> template <>
void BufferStreamTestObjectType::test<1>()
{
	set_test_name("BufferArrayStreamBuf construction with NULL BufferArray");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArrayStreamBuf * bsb = new BufferArrayStreamBuf(NULL);
	ensure("Memory being used", mMemTotal < GetMemTotal());

	// Not much will work with a NULL
	ensure("underflow() on NULL fails", tst_traits_t::eof() == bsb->underflow());
	ensure("uflow() on NULL fails", tst_traits_t::eof() == bsb->uflow());
	ensure("pbackfail() on NULL fails", tst_traits_t::eof() == bsb->pbackfail('c'));
	ensure("showmanyc() on NULL fails", bsb->showmanyc() == -1);
	ensure("overflow() on NULL fails", tst_traits_t::eof() == bsb->overflow('c'));
	ensure("xsputn() on NULL fails", bsb->xsputn("blah", 4) == 0);
	ensure("seekoff() on NULL fails", bsb->seekoff(0, std::ios_base::beg, std::ios_base::in) == std::streampos(-1));
	
	// release the implicit reference, causing the object to be released
	delete bsb;
	bsb = NULL;

	// make sure we didn't leak any memory
	ensure("Allocated memory returned", mMemTotal == GetMemTotal());
}


template <> template <>
void BufferStreamTestObjectType::test<2>()
{
	set_test_name("BufferArrayStream construction with NULL BufferArray");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted object with an implicit reference
	BufferArrayStream * bas = new BufferArrayStream(NULL);
	ensure("Memory being used", mMemTotal < GetMemTotal());

	// Not much will work with a NULL here
	ensure("eof() is false on NULL", ! bas->eof());
	ensure("fail() is false on NULL", ! bas->fail());
	ensure("good() on NULL", bas->good());
		   
	// release the implicit reference, causing the object to be released
	delete bas;
	bas = NULL;

	// make sure we didn't leak any memory
	ensure("Allocated memory returned", mMemTotal == GetMemTotal());
}


template <> template <>
void BufferStreamTestObjectType::test<3>()
{
	set_test_name("BufferArrayStreamBuf construction with empty BufferArray");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted BufferArray with implicit reference
	BufferArray * ba = new BufferArray;
	BufferArrayStreamBuf * bsb = new BufferArrayStreamBuf(ba);
	ensure("Memory being used", mMemTotal < GetMemTotal());

	// I can release my ref on the BA
	ba->release();
	ba = NULL;
	
	// release the implicit reference, causing the object to be released
	delete bsb;
	bsb = NULL;

	// make sure we didn't leak any memory
	ensure("Allocated memory returned", mMemTotal == GetMemTotal());
}


template <> template <>
void BufferStreamTestObjectType::test<4>()
{
	set_test_name("BufferArrayStream construction with empty BufferArray");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted BufferArray with implicit reference
	BufferArray * ba = new BufferArray;

	{
		// create a new ref counted object with an implicit reference
		BufferArrayStream bas(ba);
		ensure("Memory being used", mMemTotal < GetMemTotal());
	}

	// release the implicit reference, causing the object to be released
	ba->release();
	ba = NULL;
	
	// make sure we didn't leak any memory
	ensure("Allocated memory returned", mMemTotal == GetMemTotal());
}


template <> template <>
void BufferStreamTestObjectType::test<5>()
{
	set_test_name("BufferArrayStreamBuf construction with real BufferArray");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted BufferArray with implicit reference
	BufferArray * ba = new BufferArray;
	const char * content("This is a string.  A fragment.");
	const size_t c_len(strlen(content));
	ba->append(content, c_len);

	// Creat an adapter for the BufferArray
	BufferArrayStreamBuf * bsb = new BufferArrayStreamBuf(ba);
	ensure("Memory being used", mMemTotal < GetMemTotal());

	// I can release my ref on the BA
	ba->release();
	ba = NULL;
	
	// Various static state
	ensure("underflow() returns 'T'", bsb->underflow() == 'T');
	ensure("underflow() returns 'T' again", bsb->underflow() == 'T');
	ensure("uflow() returns 'T'", bsb->uflow() == 'T');
	ensure("uflow() returns 'h'", bsb->uflow() == 'h');
	ensure("pbackfail('i') fails", tst_traits_t::eof() == bsb->pbackfail('i'));
	ensure("pbackfail('T') fails", tst_traits_t::eof() == bsb->pbackfail('T'));
	ensure("pbackfail('h') succeeds", bsb->pbackfail('h') == 'h');
	ensure("showmanyc() is everything but the 'T'", bsb->showmanyc() == (c_len - 1));
	ensure("overflow() appends", bsb->overflow('c') == 'c');
	ensure("showmanyc() reflects append", bsb->showmanyc() == (c_len - 1 + 1));
	ensure("xsputn() appends some more", bsb->xsputn("bla!", 4) == 4);
	ensure("showmanyc() reflects 2nd append", bsb->showmanyc() == (c_len - 1 + 5));
	ensure("seekoff() succeeds", bsb->seekoff(0, std::ios_base::beg, std::ios_base::in) == std::streampos(0));
	ensure("seekoff() succeeds 2", bsb->seekoff(4, std::ios_base::cur, std::ios_base::in) == std::streampos(4));
	ensure("showmanyc() picks up seekoff", bsb->showmanyc() == (c_len + 5 - 4));
	ensure("seekoff() succeeds 3", bsb->seekoff(0, std::ios_base::end, std::ios_base::in) == std::streampos(c_len + 4));
	ensure("pbackfail('!') succeeds", tst_traits_t::eof() == bsb->pbackfail('!'));
	
	// release the implicit reference, causing the object to be released
	delete bsb;
	bsb = NULL;

	// make sure we didn't leak any memory
	ensure("Allocated memory returned", mMemTotal == GetMemTotal());
}


template <> template <>
void BufferStreamTestObjectType::test<6>()
{
	set_test_name("BufferArrayStream construction with real BufferArray");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted BufferArray with implicit reference
	BufferArray * ba = new BufferArray;
	//const char * content("This is a string.  A fragment.");
	//const size_t c_len(strlen(content));
	//ba->append(content, strlen(content));

	{
		// Creat an adapter for the BufferArray
		BufferArrayStream bas(ba);
		ensure("Memory being used", mMemTotal < GetMemTotal());

		// Basic operations
		bas << "Hello" << 27 << ".";
		ensure("BA length 8", ba->size() == 8);

		std::string str;
		bas >> str;
		ensure("reads correctly", str == "Hello27.");
	}

	// release the implicit reference, causing the object to be released
	ba->release();
	ba = NULL;

	// make sure we didn't leak any memory
	// ensure("Allocated memory returned", mMemTotal == GetMemTotal());
	// static U64 mem = GetMemTotal();
}


template <> template <>
void BufferStreamTestObjectType::test<7>()
{
	set_test_name("BufferArrayStream with LLSD serialization");

	// record the total amount of dynamically allocated memory
	mMemTotal = GetMemTotal();

	// create a new ref counted BufferArray with implicit reference
	BufferArray * ba = new BufferArray;

	{
		// Creat an adapter for the BufferArray
		BufferArrayStream bas(ba);
		ensure("Memory being used", mMemTotal < GetMemTotal());

		// LLSD
		LLSD llsd = LLSD::emptyMap();

		llsd["int"] = LLSD::Integer(3);
		llsd["float"] = LLSD::Real(923289.28992);
		llsd["string"] = LLSD::String("aksjdl;ajsdgfjgfal;sdgjakl;sdfjkl;ajsdfkl;ajsdfkl;jaskl;dfj");

		LLSD llsd_map = LLSD::emptyMap();
		llsd_map["int"] = LLSD::Integer(-2889);
		llsd_map["float"] = LLSD::Real(2.37829e32);
		llsd_map["string"] = LLSD::String("OHIGODHSPDGHOSDHGOPSHDGP");

		llsd["map"] = llsd_map;
		
		// Serialize it
		LLSDSerialize::toXML(llsd, bas);

		std::string str;
		bas >> str;
		// std::cout << "SERIALIZED LLSD:  " << str << std::endl;
		ensure("Extracted string has reasonable length", str.size() > 60);
	}

	// release the implicit reference, causing the object to be released
	ba->release();
	ba = NULL;

	// make sure we didn't leak any memory
	// ensure("Allocated memory returned", mMemTotal == GetMemTotal());
}


}  // end namespace tut


#endif  // TEST_LLCORE_BUFFER_STREAM_H_
