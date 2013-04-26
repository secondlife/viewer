/** 
 * @file io.cpp
 * @author Phoenix
 * @date 2005-10-02
 * @brief Tests for io classes and helpers
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "linden_common.h"
#include "lltut.h"

#include <iterator>

#include "apr_pools.h"

#include "llbuffer.h"
#include "llbufferstream.h"
#include "lliosocket.h"
#include "llioutil.h"
#include "llmemorystream.h"
#include "llpipeutil.h"
#include "llpumpio.h"
#include "llsd.h"
#include "llsdrpcclient.h"
#include "llsdrpcserver.h"
#include "llsdserialize.h"
#include "lluuid.h"
#include "llinstantmessage.h"

namespace tut
{
	struct heap_buffer_data
	{
		heap_buffer_data() : mBuffer(NULL) {}
		~heap_buffer_data() { if(mBuffer) delete mBuffer; }
		LLHeapBuffer* mBuffer;
	};
	typedef test_group<heap_buffer_data> heap_buffer_test;
	typedef heap_buffer_test::object heap_buffer_object;
	tut::heap_buffer_test thb("heap_buffer");

	template<> template<>
	void heap_buffer_object::test<1>()
	{
		const S32 BUF_SIZE = 100;
		mBuffer = new LLHeapBuffer(BUF_SIZE);
		ensure_equals("empty buffer capacity", mBuffer->capacity(), BUF_SIZE);
		const S32 SEGMENT_SIZE = 50;
		LLSegment segment;
		mBuffer->createSegment(0, SEGMENT_SIZE, segment);
		ensure_equals("used buffer capacity", mBuffer->capacity(), BUF_SIZE);
	}

	template<> template<>
	void heap_buffer_object::test<2>()
	{
		const S32 BUF_SIZE = 10;
		mBuffer = new LLHeapBuffer(BUF_SIZE);
		LLSegment segment;
		mBuffer->createSegment(0, BUF_SIZE, segment);
		ensure("segment is in buffer", mBuffer->containsSegment(segment));
		ensure_equals("buffer consumed", mBuffer->bytesLeft(), 0);
		bool  created;
		created = mBuffer->createSegment(0, 0, segment);
		ensure("Create zero size segment fails", !created);
		created = mBuffer->createSegment(0, BUF_SIZE, segment);
		ensure("Create segment fails", !created);
	}

	template<> template<>
	void heap_buffer_object::test<3>()
	{
		const S32 BUF_SIZE = 10;
		mBuffer = new LLHeapBuffer(BUF_SIZE);
		LLSegment segment;
		mBuffer->createSegment(0, BUF_SIZE, segment);
		ensure("segment is in buffer", mBuffer->containsSegment(segment));
		ensure_equals("buffer consumed", mBuffer->bytesLeft(), 0);
		bool reclaimed = mBuffer->reclaimSegment(segment);
		ensure("buffer reclaimed.", reclaimed);
		ensure_equals("buffer available", mBuffer->bytesLeft(), BUF_SIZE);
		bool  created;
		created = mBuffer->createSegment(0, 0, segment);
		ensure("Create zero size segment fails", !created);
		created = mBuffer->createSegment(0, BUF_SIZE, segment);
		ensure("Create another segment succeeds", created);
	}

	template<> template<>
	void heap_buffer_object::test<4>()
	{
		const S32 BUF_SIZE = 10;
		const S32 SEGMENT_SIZE = 4;
		mBuffer = new LLHeapBuffer(BUF_SIZE);
		LLSegment seg1;
		mBuffer->createSegment(0, SEGMENT_SIZE, seg1);
		ensure("segment is in buffer", mBuffer->containsSegment(seg1));
		LLSegment seg2;
		mBuffer->createSegment(0, SEGMENT_SIZE, seg2);
		ensure("segment is in buffer", mBuffer->containsSegment(seg2));
		LLSegment seg3;
		mBuffer->createSegment(0, SEGMENT_SIZE, seg3);
		ensure("segment is in buffer", mBuffer->containsSegment(seg3));
		ensure_equals("segment is truncated", seg3.size(), 2);
		LLSegment seg4;
		bool created;
		created = mBuffer->createSegment(0, SEGMENT_SIZE, seg4);
		ensure("Create segment fails", !created);
		bool reclaimed;
		reclaimed = mBuffer->reclaimSegment(seg1);
		ensure("buffer reclaim succeed.", reclaimed);
		ensure_equals("no buffer available", mBuffer->bytesLeft(), 0);
		reclaimed = mBuffer->reclaimSegment(seg2);
		ensure("buffer reclaim succeed.", reclaimed);
		ensure_equals("buffer reclaimed", mBuffer->bytesLeft(), 0);
		reclaimed = mBuffer->reclaimSegment(seg3);
		ensure("buffer reclaim succeed.", reclaimed);
		ensure_equals("buffer reclaimed", mBuffer->bytesLeft(), BUF_SIZE);
		created = mBuffer->createSegment(0, SEGMENT_SIZE, seg1);
		ensure("segment is in buffer", mBuffer->containsSegment(seg1));
		ensure("Create segment succeds", created);
	}
}

namespace tut
{
	struct buffer_data
	{
		LLBufferArray mBuffer;
	};
	typedef test_group<buffer_data> buffer_test;
	typedef buffer_test::object buffer_object;
	tut::buffer_test tba("buffer_array");

	template<> template<>
	void buffer_object::test<1>()
	{
		const char HELLO_WORLD[] = "hello world";
		const S32 str_len = strlen(HELLO_WORLD);
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)HELLO_WORLD, str_len);
		S32 count = mBuffer.countAfter(ch.in(), NULL);
		ensure_equals("total append size", count, str_len);
		LLBufferArray::segment_iterator_t it = mBuffer.beginSegment();
		U8* first = (*it).data();
		count = mBuffer.countAfter(ch.in(), first);
		ensure_equals("offset append size", count, str_len - 1);
	}

	template<> template<>
	void buffer_object::test<2>()
	{
		const char HELLO_WORLD[] = "hello world";
		const S32 str_len = strlen(HELLO_WORLD);		/* Flawfinder: ignore */
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)HELLO_WORLD, str_len);
		mBuffer.append(ch.in(), (U8*)HELLO_WORLD, str_len);
		S32 count = mBuffer.countAfter(ch.in(), NULL);
		ensure_equals("total append size", count, 2 * str_len);
		LLBufferArray::segment_iterator_t it = mBuffer.beginSegment();
		U8* first = (*it).data();
		count = mBuffer.countAfter(ch.in(), first);
		ensure_equals("offset append size", count, (2 * str_len) - 1);
	}

	template<> template<>
	void buffer_object::test<3>()
	{
		const char ONE[] = "one";
		const char TWO[] = "two";
		std::string expected(ONE);
		expected.append(TWO);
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)ONE, 3);
		mBuffer.append(ch.in(), (U8*)TWO, 3);
		char buffer[255];	/* Flawfinder: ignore */
		S32 len = 6;
		mBuffer.readAfter(ch.in(), NULL, (U8*)buffer, len);
		ensure_equals(len, 6);
		buffer[len] = '\0';
		std::string actual(buffer);
		ensure_equals("read", actual, expected);
	}

	template<> template<>
	void buffer_object::test<4>()
	{
		const char ONE[] = "one";
		const char TWO[] = "two";
		std::string expected(ONE);
		expected.append(TWO);
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)TWO, 3);
		mBuffer.prepend(ch.in(), (U8*)ONE, 3);
		char buffer[255];	/* Flawfinder: ignore */
		S32 len = 6;
		mBuffer.readAfter(ch.in(), NULL, (U8*)buffer, len);
		ensure_equals(len, 6);
		buffer[len] = '\0';
		std::string actual(buffer);
		ensure_equals("read", actual, expected);
	}

	template<> template<>
	void buffer_object::test<5>()
	{
		const char ONE[] = "one";
		const char TWO[] = "two";
		std::string expected("netwo");
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)TWO, 3);
		mBuffer.prepend(ch.in(), (U8*)ONE, 3);
		char buffer[255];	/* Flawfinder: ignore */
		S32 len = 5;
		LLBufferArray::segment_iterator_t it = mBuffer.beginSegment();
		U8* addr = (*it).data();
		mBuffer.readAfter(ch.in(), addr, (U8*)buffer, len);
		ensure_equals(len, 5);
		buffer[len] = '\0';
		std::string actual(buffer);
		ensure_equals("read", actual, expected);
	}

	template<> template<>
	void buffer_object::test<6>()
	{
		std::string request("The early bird catches the worm.");
		std::string response("If you're a worm, sleep late.");
		std::ostringstream expected;
		expected << "ContentLength: " << response.length() << "\r\n\r\n"
				 << response;
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)request.c_str(), request.length());
		mBuffer.append(ch.out(), (U8*)response.c_str(), response.length());
		S32 count = mBuffer.countAfter(ch.out(), NULL);
		std::ostringstream header;
		header << "ContentLength: " << count << "\r\n\r\n";
		std::string head(header.str());
		mBuffer.prepend(ch.out(), (U8*)head.c_str(), head.length());
		char buffer[1024];	/* Flawfinder: ignore */
		S32 len = response.size() + head.length();
		ensure_equals("same length", len, (S32)expected.str().length());
		mBuffer.readAfter(ch.out(), NULL, (U8*)buffer, len);
		buffer[len] = '\0';
		std::string actual(buffer);
		ensure_equals("threaded writes", actual, expected.str());
	}

	template<> template<>
	void buffer_object::test<7>()
	{
		const S32 LINE_COUNT = 3;
		std::string lines[LINE_COUNT] =
			{
				std::string("GET /index.htm HTTP/1.0\r\n"),
				std::string("User-Agent: Wget/1.9.1\r\n"),
				std::string("Host: localhost:8008\r\n")
			};
		std::string text;
		S32 i;
		for(i = 0; i < LINE_COUNT; ++i)
		{
			text.append(lines[i]);
		}
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)text.c_str(), text.length());
		const S32 BUFFER_LEN = 1024;
		char buf[BUFFER_LEN];
		S32 len;
		U8* last = NULL;
		std::string last_line;
		for(i = 0; i < LINE_COUNT; ++i)
		{
			len = BUFFER_LEN;
			last = mBuffer.readAfter(ch.in(), last, (U8*)buf, len);
			char* newline = strchr((char*)buf, '\n');
			S32 offset = -((len - 1) - (newline - buf));
			++newline;
			*newline = '\0';
			last_line.assign(buf);
			std::ostringstream message;
			message << "line reads in line["	 << i << "]";
			ensure_equals(message.str().c_str(), last_line, lines[i]);
			last = mBuffer.seek(ch.in(), last, offset);
		}
	}

	template<> template<>
	void buffer_object::test<8>()
	{
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)"1", 1);
		LLBufferArray buffer;
		buffer.append(ch.in(), (U8*)"2", 1);
		mBuffer.takeContents(buffer);
		mBuffer.append(ch.in(), (U8*)"3", 1);
		S32 count = mBuffer.countAfter(ch.in(), NULL);
		ensure_equals("buffer size", count, 3);
		U8* temp = new U8[count];
		mBuffer.readAfter(ch.in(), NULL, temp, count);
		ensure("buffer content", (0 == memcmp(temp, (void*)"123", 3)));
		delete[] temp;
	}

	template<> template<>
	void buffer_object::test<9>()
	{
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.in(), (U8*)"1", 1);
		S32 capacity = mBuffer.capacity();
		ensure("has capacity", capacity > 0);
		U8* temp = new U8[capacity - 1];
		mBuffer.append(ch.in(), temp, capacity - 1);
		capacity = mBuffer.capacity();
		ensure("has capacity when full", capacity > 0);
		S32 used = mBuffer.countAfter(ch.in(), NULL);
		ensure_equals("used equals capacity", used, capacity);

		LLBufferArray::segment_iterator_t iter = mBuffer.beginSegment();
		while(iter != mBuffer.endSegment())
		{
			mBuffer.eraseSegment(iter++);
		}

		used = mBuffer.countAfter(ch.in(), NULL);
		ensure_equals("used is zero", used, 0);
		S32 capacity2 = mBuffer.capacity();
		ensure_equals("capacity the same after erase", capacity2, capacity);
		mBuffer.append(ch.in(), temp, capacity - 1);
		capacity2 = mBuffer.capacity();
		ensure_equals("capacity the same after append", capacity2, capacity);

		delete[] temp;
	}

#if 0
	template<> template<>
	void buffer_object::test<9>()
	{
		char buffer[1024];	/* Flawfinder: ignore */
		S32 size = sprintf(buffer,
						"%d|%d|%s|%s|%s|%s|%s|%x|%x|%x|%x|%x|%s|%s|%d|%d|%x",
						7,
						7,
						"Hang Glider INFO",
						"18e84d1e-04a4-4c0d-8cb6-6c73477f0a9a",
						"0e346d8b-4433-4d66-a6b0-fd37083abc4c",
						"0e346d8b-4433-4d66-a6b0-fd37083abc4c",
						"00000000-0000-0000-0000-000000000000",
						0x7fffffff,
						0x7fffffff,
						0,
						0,
						0x7fffffff,
						"69e0d357-2e7c-8990-a2bc-7f61c868e5a3",
						"2004-06-04 16:09:17 note card",
						0,
						10,
						0) + 1;

		//const char* expected = "7|7|Hang Glider INFO|18e84d1e-04a4-4c0d-8cb6-6c73477f0a9a|0e346d8b-4433-4d66-a6b0-fd37083abc4c|0e346d8b-4433-4d66-a6b0-fd37083abc4c|00000000-0000-0000-0000-000000000000|7fffffff|7fffffff|0|0|7fffffff|69e0d357-2e7c-8990-a2bc-7f61c868e5a3|2004-06-04 16:09:17 note card|0|10|0\0";
		
		LLSD* bin_bucket = LLIMInfo::buildSDfrombuffer((U8*)buffer,size);

		char post_buffer[1024];
		U32 post_size;
		LLIMInfo::getBinaryBucket(bin_bucket,(U8*)post_buffer,post_size);
		ensure_equals("Buffer sizes",size,(S32)post_size);
		ensure("Buffer content",!strcmp(buffer,post_buffer));
	}
#endif

	/*
	template<> template<>
	void buffer_object::test<>()
	{
	}
	*/
}

namespace tut
{
	struct buffer_and_stream_data
	{
		LLBufferArray mBuffer;
	};
	typedef test_group<buffer_and_stream_data> bas_test;
	typedef bas_test::object bas_object;
	tut::bas_test tbs("buffer_stream");

	template<> template<>
	void bas_object::test<1>()
	{
		const char HELLO_WORLD[] = "hello world";
		const S32 str_len = strlen(HELLO_WORLD);	/* Flawfinder: ignore */
		LLChannelDescriptors ch = mBuffer.nextChannel();
		LLBufferStream str(ch, &mBuffer);
		mBuffer.append(ch.in(), (U8*)HELLO_WORLD, str_len);
		std::string hello;
		std::string world;
		str >> hello >> world;
		ensure_equals("first word", hello, std::string("hello"));
		ensure_equals("second word", world, std::string("world"));
	}

	template<> template<>
	void bas_object::test<2>()
	{
		std::string part1("Eat my shor");
		std::string part2("ts ho");
		std::string part3("mer");
		std::string ignore("ignore me");
		LLChannelDescriptors ch = mBuffer.nextChannel();
		LLBufferStream str(ch, &mBuffer);
		mBuffer.append(ch.in(), (U8*)part1.c_str(), part1.length());
		mBuffer.append(ch.in(), (U8*)part2.c_str(), part2.length());
		mBuffer.append(ch.out(), (U8*)ignore.c_str(), ignore.length());
		mBuffer.append(ch.in(), (U8*)part3.c_str(), part3.length());
		std::string eat;
		std::string my;
		std::string shorts;
		std::string homer;
		str >> eat >> my >> shorts >> homer;
		ensure_equals("word1", eat, std::string("Eat"));
		ensure_equals("word2", my, std::string("my"));
		ensure_equals("word3", shorts, std::string("shorts"));
		ensure_equals("word4", homer, std::string("homer"));
	}

	template<> template<>
	void bas_object::test<3>()
	{
		std::string part1("junk in ");
		std::string part2("the trunk");
		const S32 CHANNEL = 0;
		mBuffer.append(CHANNEL, (U8*)part1.c_str(), part1.length());
		mBuffer.append(CHANNEL, (U8*)part2.c_str(), part2.length());
		U8* last = 0;
		const S32 BUF_LEN = 128;
		char buf[BUF_LEN];
		S32 len = 11;
		last = mBuffer.readAfter(CHANNEL, last, (U8*)buf, len);
		buf[len] = '\0';
		std::string actual(buf);
		ensure_equals("first read", actual, std::string("junk in the"));
		last = mBuffer.seek(CHANNEL, last, -6);
		len = 12;
		last = mBuffer.readAfter(CHANNEL, last, (U8*)buf, len);
		buf[len] = '\0';
		actual.assign(buf);
		ensure_equals("seek and read", actual, std::string("in the trunk"));
	}

	template<> template<>
	void bas_object::test<4>()
	{
		std::string phrase("zippity do da!");
		const S32 CHANNEL = 0;
		mBuffer.append(CHANNEL, (U8*)phrase.c_str(), phrase.length());
		const S32 BUF_LEN = 128;
		char buf[BUF_LEN];
		S32 len = 7;
		U8* last = mBuffer.readAfter(CHANNEL, NULL, (U8*)buf, len);
		mBuffer.splitAfter(last);
		LLBufferArray::segment_iterator_t it = mBuffer.beginSegment();
		LLBufferArray::segment_iterator_t end = mBuffer.endSegment();
		std::string first((char*)((*it).data()), (*it).size());
		ensure_equals("first part", first, std::string("zippity"));
		++it;
		std::string second((char*)((*it).data()), (*it).size());
		ensure_equals("second part",	second, std::string(" do da!"));
		++it;
		ensure("iterators equal",	 (it == end));
	}

	template<> template<>
	void bas_object::test<5>()
	{
		LLChannelDescriptors ch = mBuffer.nextChannel();
		LLBufferStream str(ch, &mBuffer);
		std::string h1("hello");
		std::string h2(", how are you doing?");
		std::string expected(h1);
		expected.append(h2);
		str << h1 << h2;
		str.flush();
		const S32 BUF_LEN = 128;
		char buf[BUF_LEN];
		S32 actual_len = BUF_LEN;
		S32 expected_len = h1.size() + h2.size();
		(void) mBuffer.readAfter(ch.out(), NULL, (U8*)buf, actual_len);
		ensure_equals("streamed size", actual_len, expected_len);
		buf[actual_len] = '\0';
		std::string actual(buf);
		ensure_equals("streamed to buf", actual, expected);
	}

	template<> template<>
	void bas_object::test<6>()
	{
		LLChannelDescriptors ch = mBuffer.nextChannel();
		LLBufferStream bstr(ch, &mBuffer);
		std::ostringstream ostr;
		std::vector<LLUUID> ids;
		LLUUID id;
		for(int i = 0; i < 5; ++i)
		{
			id.generate();
			ids.push_back(id);
		}
		bstr << "SELECT concat(u.username, ' ', l.name) "
			 << "FROM user u, user_last_name l "
			 << "WHERE u.last_name_id = l.last_name_id"
			 << " AND u.agent_id IN ('";
		ostr << "SELECT concat(u.username, ' ', l.name) "
			 << "FROM user u, user_last_name l "
			 << "WHERE u.last_name_id = l.last_name_id"
			 << " AND u.agent_id IN ('";
		std::copy(
			ids.begin(),
			ids.end(),
			std::ostream_iterator<LLUUID>(bstr, "','"));
		std::copy(
			ids.begin(),
			ids.end(),
			std::ostream_iterator<LLUUID>(ostr, "','"));
		bstr.seekp(-2, std::ios::cur);
		ostr.seekp(-2, std::ios::cur);
		bstr << ") ";
		ostr << ") ";
		bstr.flush();
		const S32 BUF_LEN = 512;
		char buf[BUF_LEN];		/* Flawfinder: ignore */
		S32 actual_len = BUF_LEN;
		(void) mBuffer.readAfter(ch.out(), NULL, (U8*)buf, actual_len);
		buf[actual_len] = '\0';
		std::string actual(buf);
		std::string expected(ostr.str());
		ensure_equals("size of string in seek",actual.size(),expected.size());
		ensure_equals("seek in ostream", actual, expected);
	}

	template<> template<>
	void bas_object::test<7>()
	{
		LLChannelDescriptors ch = mBuffer.nextChannel();
		LLBufferStream bstr(ch, &mBuffer);
		bstr << "1";
		bstr.flush();
		S32 count = mBuffer.countAfter(ch.out(), NULL);
		ensure_equals("buffer size 1", count, 1);
		LLBufferArray buffer;
		buffer.append(ch.out(), (U8*)"2", 1);
		mBuffer.takeContents(buffer);
		count = mBuffer.countAfter(ch.out(), NULL);
		ensure_equals("buffer size 2", count, 2);
		bstr << "3";
		bstr.flush();
		count = mBuffer.countAfter(ch.out(), NULL);
		ensure_equals("buffer size 3", count, 3);
		U8* temp = new U8[count];
		mBuffer.readAfter(ch.out(), NULL, temp, count);
		ensure("buffer content", (0 == memcmp(temp, (void*)"123", 3)));
		delete[] temp;
	}

	template<> template<>
	void bas_object::test<8>()
	{
		LLChannelDescriptors ch = mBuffer.nextChannel();
		LLBufferStream ostr(ch, &mBuffer);
		typedef std::vector<U8> buf_t;
		typedef std::vector<buf_t> actual_t;
		actual_t actual;
		buf_t source;
		bool need_comma = false;
		ostr << "[";
		S32 total_size = 1;
		for(S32 i = 2000; i < 2003; ++i)
		{
			if(need_comma)
			{
				ostr << ",";
				++total_size;
			}
			need_comma = true;
			srand(69 + i);	/* Flawfinder: ignore */
			S32 size = rand() % 1000 + 1000;
			std::generate_n(
				std::back_insert_iterator<buf_t>(source),
				size,
				rand);
			actual.push_back(source);
			ostr << "b(" << size << ")\"";
			total_size += 8;
			ostr.write((const char*)(&source[0]), size);
			total_size += size;
			source.clear();
			ostr << "\"";
			++total_size;
		}
		ostr << "]";
		++total_size;
		ostr.flush();

		// now that we have a bunch of data on a stream, parse it all.
		ch = mBuffer.nextChannel();
		S32 count = mBuffer.countAfter(ch.in(), NULL);
		ensure_equals("size of buffer", count, total_size);
		LLBufferStream istr(ch, &mBuffer);
		LLSD data;
		count = LLSDSerialize::fromNotation(data, istr, total_size);
		ensure("sd parsed", data.isDefined());

		for(S32 j = 0; j < 3; ++j)
		{
			std::ostringstream name;
			LLSD child(data[j]);
			name << "found buffer " << j;
			ensure(name.str(), child.isDefined());
			source = child.asBinary();
			name.str("");
			name << "buffer " << j << " size";
			ensure_equals(name.str().c_str(), source.size(), actual[j].size());
			name.str("");
			name << "buffer " << j << " contents";
			ensure(
				name.str(),
				(0 == memcmp(&source[0], &actual[j][0], source.size())));
		}
	}

	template<> template<>
	void bas_object::test<9>()
	{
		LLChannelDescriptors ch = mBuffer.nextChannel();
		LLBufferStream ostr(ch, &mBuffer);
		typedef std::vector<U8> buf_t;
		buf_t source;
		bool need_comma = false;
		ostr << "{";
		S32 total_size = 1;
		for(S32 i = 1000; i < 3000; ++i)
		{
			if(need_comma)
			{
				ostr << ",";
				++total_size;
			}
			need_comma = true;
			ostr << "'" << i << "':";
			total_size += 7;
			srand(69 + i);		/* Flawfinder: ignore */
			S32 size = rand() % 1000 + 1000;
			std::generate_n(
				std::back_insert_iterator<buf_t>(source),
				size,
				rand);
			ostr << "b(" << size << ")\"";
			total_size += 8;
			ostr.write((const char*)(&source[0]), size);
			total_size += size;
			source.clear();
			ostr << "\"";
			++total_size;
		}
		ostr << "}";
		++total_size;
		ostr.flush();

		// now that we have a bunch of data on a stream, parse it all.
		ch = mBuffer.nextChannel();
		S32 count = mBuffer.countAfter(ch.in(), NULL);
		ensure_equals("size of buffer", count, total_size);
		LLBufferStream istr(ch, &mBuffer);
		LLSD data;
		count = LLSDSerialize::fromNotation(data, istr, total_size);
		ensure("sd parsed", data.isDefined());
	}

	template<> template<>
	void bas_object::test<10>()
	{
//#if LL_WINDOWS && _MSC_VER >= 1400
//        skip_fail("Fails on VS2005 due to broken LLSDSerialize::fromNotation() parser.");
//#endif
		const char LOGIN_STREAM[] = "{'method':'login', 'parameter': [ {"
										"'uri': 'sl-am:kellys.region.siva.lindenlab.com/location?start=url&px=128&py=128&pz=128&lx=0&ly=0&lz=0'}, "
										"{'version': i1}, {'texture_data': [ '61d724fb-ad79-f637-2186-5cf457560daa', '6e38b9be-b7cc-e77a-8aec-029a42b0b416', "
										"'a9073524-e89b-2924-ca6e-a81944109a1a', '658f18b5-5f1e-e593-f5d5-36c3abc7249a', '0cc799f4-8c99-6b91-bd75-b179b12429e2', "
										"'59fd9b64-8300-a425-aad8-2ffcbe9a49d2', '59fd9b64-8300-a425-aad8-2ffcbe9a49d2', '5748decc-f629-461c-9a36-a35a221fe21f', "
										"'b8fc9be2-26a6-6b47-690b-0e902e983484', 'a13ca0fe-3802-dc97-e79a-70d12171c724', 'dd9643cf-fd5d-0376-ed4a-b1cc646a97d5', "
										"'4ad13ae9-a112-af09-210a-cf9353a7a9e7', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', "
										"'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', "
										"'5748decc-f629-461c-9a36-a35a221fe21f', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97'],"
										"'session_id': '324cfa9f-fe5d-4d1c-a317-35f20a86a4d1','position': [ i128, i128, i128],'last_name': 'Linden','group_title': '-> !BLING! <-','group_name': 'test!','agent_access': 'M',"
										"'attachment_data': [ {'asset_id': 'aaede2b1-9955-09d4-5c93-2b557c778cf3','attachment_point': i6,'item_id': 'f3694abc-5122-db33-73d9-e0f4288dc2bf'}],"
										"'buddy_ids': [ '101358d5-469d-4b24-9b85-4dc3c05e635d', '1b00fec7-6265-4875-acac-80d9cfe9295c', '203ad6df-b522-491d-ba48-4e24eb57aeff', "
										"'22d4dcdb-aebb-47fa-b925-a871cc75ee48','27da3df5-1339-4463-80aa-40504ee3b3e5', '299d1720-b61f-4268-8c29-9614aa2d44c2', "
										"'2b048a24-2737-4994-9fa5-becc8e466253', '2cd5dc14-a853-49a4-be3c-a5a7178e37bc', '3de548e1-57be-cfea-2b78-83ae3ad95998', "
										"'3dee98e4-a6a3-4543-91c3-bbd528447ba7', '3e2d81a3-6263-6ffe-ad5c-8ce04bee07e9', '40e70b98-fed7-47f3-9700-1bce93f9350b', "
										"'50a9b68e-b5aa-4d35-9137-3cfebda0a15c', '54295571-9357-43ff-ae74-a83b5138160f', '6191e2d7-5f96-4856-bdab-af0f79f47ae4', "
										"'63e577d8-cd34-4235-a0a3-de0500133364', '79cfb666-4fd0-4af7-95df-fb7d96b4e24d', '8121c2f3-4a88-4c33-9899-8fc1273f47ee', "
										"'909da964-ef23-4f2a-ba13-f2a8cfd454b6','a2e76fcd-9360-4f6d-a924-000000000001', 'aaa6d664-527e-4d83-9cbb-7ef79ccc7cc8', "
										"'b79bfb6c-23be-49eb-b35b-30ff2f501b37', 'ba0d9c79-148c-4a79-8e3c-0665eebe2427', 'bc9bda98-57cd-498f-b993-4ff1ac9dec93', "
										"'c62d16f6-81cb-419d-9cac-e46dc394084d', 'd48f8fa7-2512-4fe5-80c8-c0a923412e07', 'd77e3e24-7e6c-4c3f-96d0-a1746337f8fb', "
										"'da615c63-a84b-4592-a3d6-a90dd3e92e6e', 'df47190a-7eb7-4aff-985f-2d1d3ad6c6e9', 'e3380196-72cd-499c-a2ba-caa180bd5fe4', "
										"'e937863f-f134-4207-803b-d6e686651d6c', 'efcdf98b-5269-45ef-ac7a-0671f09ea9d9'],"
										"'circuit_code': i124,'group_id': '8615c885-9cf0-bf0a-6e40-0c11462aa652','limited_to_estate': i1,'look_at': [ i0, i0, i0],"
										"'agent_id': '0e346d8b-4433-4d66-a6b0-fd37083abc4c','first_name': 'Kelly','start': 'url'}]}";
		LLChannelDescriptors ch = mBuffer.nextChannel();
		mBuffer.append(ch.out(), (U8*)LOGIN_STREAM, strlen(LOGIN_STREAM));		/* Flawfinder: ignore */
		ch = mBuffer.nextChannel();
		LLBufferStream istr(ch, &mBuffer);
		LLSD data;
		S32 count = LLSDSerialize::fromNotation(
			data,
			istr,
			mBuffer.count(ch.in()));
		ensure("parsed something", (count > 0));
		ensure("sd parsed", data.isDefined());
		ensure_equals("sd type", data.type(), LLSD::TypeMap);
		ensure("has method", data.has("method"));
		ensure("has parameter", data.has("parameter"));
		LLSD parameter = data["parameter"];
		ensure_equals("parameter is array", parameter.type(), LLSD::TypeArray);
		LLSD agent_params = parameter[2];
		std::string s_value;
		s_value = agent_params["last_name"].asString();
		ensure_equals("last name", s_value, std::string("Linden"));
		s_value = agent_params["first_name"].asString();
		ensure_equals("first name", s_value, std::string("Kelly"));
		s_value = agent_params["agent_access"].asString();
		ensure_equals("agent access", s_value, std::string("M"));
		s_value = agent_params["group_name"].asString();
		ensure_equals("group name", s_value, std::string("test!"));
		s_value = agent_params["group_title"].asString();
		ensure_equals("group title", s_value, std::string("-> !BLING! <-"));

		LLUUID agent_id("0e346d8b-4433-4d66-a6b0-fd37083abc4c");
		LLUUID id = agent_params["agent_id"];
		ensure_equals("agent id", id, agent_id);
		LLUUID session_id("324cfa9f-fe5d-4d1c-a317-35f20a86a4d1");
		id = agent_params["session_id"];
		ensure_equals("session id", id, session_id);
		LLUUID group_id ("8615c885-9cf0-bf0a-6e40-0c11462aa652");
		id = agent_params["group_id"];
		ensure_equals("group id", id, group_id);

		S32 i_val = agent_params["limited_to_estate"];
		ensure_equals("limited to estate", i_val, 1);
		i_val = agent_params["circuit_code"];
		ensure_equals("circuit code", i_val, 124);
	}


	template<> template<>
	void bas_object::test<11>()
	{
		std::string val = "{!'foo'@:#'bar'}";
		std::istringstream istr;
		istr.str(val);
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, istr, val.size());
		ensure_equals("parser error return value", count, -1);
		ensure("data undefined", sd.isUndefined());
	}

	template<> template<>
	void bas_object::test<12>()
	{
//#if LL_WINDOWS && _MSC_VER >= 1400
//        skip_fail("Fails on VS2005 due to broken LLSDSerialize::fromNotation() parser.");
//#endif
		std::string val = "{!'foo':[i1,'hi',{@'bar'#:[$i2%,^'baz'&]*}+]=}";
		std::istringstream istr;
		istr.str(val);
		LLSD sd;
		S32 count = LLSDSerialize::fromNotation(sd, istr, val.size());
		ensure_equals("parser error return value", count, -1);
		ensure("data undefined", sd.isUndefined());
	}

/*
	template<> template<>
	void bas_object::test<13>()
	{
	}
	template<> template<>
	void bas_object::test<14>()
	{
	}
	template<> template<>
	void bas_object::test<15>()
	{
	}
*/
}


namespace tut
{
	class PumpAndChainTestData
	{
	protected:
		apr_pool_t* mPool;
		LLPumpIO* mPump;
		LLPumpIO::chain_t mChain;
		
	public:
		PumpAndChainTestData()
		{
			apr_pool_create(&mPool, NULL);
			mPump = new LLPumpIO(mPool);
		}
		
		~PumpAndChainTestData()
		{
			mChain.clear();
			delete mPump;
			apr_pool_destroy(mPool);
		}
	};
	typedef test_group<PumpAndChainTestData>	PumpAndChainTestGroup;
	typedef PumpAndChainTestGroup::object		PumpAndChainTestObject;
	PumpAndChainTestGroup pumpAndChainTestGroup("pump_and_chain");

	template<> template<>
	void PumpAndChainTestObject::test<1>()
	{
		LLPipeStringExtractor* extractor = new LLPipeStringExtractor();

		mChain.push_back(LLIOPipe::ptr_t(new LLIOFlush));
		mChain.push_back(LLIOPipe::ptr_t(extractor));

		LLTimer timer;
		timer.setTimerExpirySec(100.0f);

		mPump->addChain(mChain, DEFAULT_CHAIN_EXPIRY_SECS);
		while(!extractor->done() && !timer.hasExpired())
		{
			mPump->pump();
			mPump->callback();
		}
		
		ensure("reading string finished", extractor->done());
		ensure_equals("string was empty", extractor->string(), "");
	}
}

/*
namespace tut
{
	struct double_construct
	{
	public:
		double_construct()
		{
			llinfos << "constructed" << llendl;
		}
		~double_construct()
		{
			llinfos << "destroyed" << llendl;
		}
	};
	typedef test_group<double_construct> double_construct_test_group;
	typedef double_construct_test_group::object dc_test_object;
	double_construct_test_group dctest("double construct");
	template<> template<>
	void dc_test_object::test<1>()
	{
		ensure("test 1", true);
	}
}
*/

namespace tut
{
	/**
	 * @brief we want to test the pipes & pumps under bad conditions.
	 */
	struct pipe_and_pump_fitness
	{
	public:
		enum
		{
			SERVER_LISTEN_PORT = 13050
		};
		
		pipe_and_pump_fitness()
		{
			LLFrameTimer::updateFrameTime();
			apr_pool_create(&mPool, NULL);
			mPump = new LLPumpIO(mPool);
			mSocket = LLSocket::create(
				mPool,
				LLSocket::STREAM_TCP,
				SERVER_LISTEN_PORT);
		}

		~pipe_and_pump_fitness()
		{
			mSocket.reset();
			delete mPump;
			apr_pool_destroy(mPool);
		}

	protected:
		apr_pool_t* mPool;
		LLPumpIO* mPump;
		LLSocket::ptr_t mSocket;
	};
	typedef test_group<pipe_and_pump_fitness> fitness_test_group;
	typedef fitness_test_group::object fitness_test_object;
	fitness_test_group fitness("pipe and pump fitness");

	template<> template<>
	void fitness_test_object::test<1>()
	{
		lldebugs << "fitness_test_object::test<1>()" << llendl;

		// Set up the server
		//lldebugs << "fitness_test_object::test<1> - setting up server."
		//	 << llendl;
		LLPumpIO::chain_t chain;
		typedef LLCloneIOFactory<LLPipeStringInjector> emitter_t;
		emitter_t* emitter = new emitter_t(
			new LLPipeStringInjector("suckers never play me"));
		boost::shared_ptr<LLChainIOFactory> factory(emitter);
		LLIOServerSocket* server = new LLIOServerSocket(
			mPool,
			mSocket,
			factory);
		server->setResponseTimeout(SHORT_CHAIN_EXPIRY_SECS);
		chain.push_back(LLIOPipe::ptr_t(server));
		mPump->addChain(chain, NEVER_CHAIN_EXPIRY_SECS);

		// We need to tickle the pump a little to set up the listen()
		//lldebugs << "fitness_test_object::test<1> - initializing server."
		//	 << llendl;
		pump_loop(mPump, 0.1f);

		// Set up the client
		//lldebugs << "fitness_test_object::test<1> - connecting client."
		//	 << llendl;
		LLSocket::ptr_t client = LLSocket::create(mPool, LLSocket::STREAM_TCP);
		LLHost server_host("127.0.0.1", SERVER_LISTEN_PORT);
		bool connected = client->blockingConnect(server_host);
		ensure("Connected to server", connected);
		lldebugs << "connected" << llendl;

		// We have connected, since the socket reader does not block,
		// the first call to read data will return EAGAIN, so we need
		// to write something.
		chain.clear();
		chain.push_back(LLIOPipe::ptr_t(new LLPipeStringInjector("hi")));
		chain.push_back(LLIOPipe::ptr_t(new LLIOSocketWriter(client)));
		chain.push_back(LLIOPipe::ptr_t(new LLIONull));
		mPump->addChain(chain, 1.0f);

		// Now, the server should immediately send the data, but we'll
		// never read it. pump for a bit
		F32 elapsed = pump_loop(mPump, 2.0f);
		ensure("Did not take too long", (elapsed < 3.0f));
	}

	template<> template<>
	void fitness_test_object::test<2>()
	{
		lldebugs << "fitness_test_object::test<2>()" << llendl;

		// Set up the server
		LLPumpIO::chain_t chain;
		typedef LLCloneIOFactory<LLIOFuzz> emitter_t;
		emitter_t* emitter = new emitter_t(new LLIOFuzz(1000000));
		boost::shared_ptr<LLChainIOFactory> factory(emitter);
		LLIOServerSocket* server = new LLIOServerSocket(
			mPool,
			mSocket,
			factory);
		server->setResponseTimeout(SHORT_CHAIN_EXPIRY_SECS);
		chain.push_back(LLIOPipe::ptr_t(server));
		mPump->addChain(chain, NEVER_CHAIN_EXPIRY_SECS);

		// We need to tickle the pump a little to set up the listen()
		pump_loop(mPump, 0.1f);

		// Set up the client
		LLSocket::ptr_t client = LLSocket::create(mPool, LLSocket::STREAM_TCP);
		LLHost server_host("127.0.0.1", SERVER_LISTEN_PORT);
		bool connected = client->blockingConnect(server_host);
		ensure("Connected to server", connected);
		lldebugs << "connected" << llendl;

		// We have connected, since the socket reader does not block,
		// the first call to read data will return EAGAIN, so we need
		// to write something.
		chain.clear();
		chain.push_back(LLIOPipe::ptr_t(new LLPipeStringInjector("hi")));
		chain.push_back(LLIOPipe::ptr_t(new LLIOSocketWriter(client)));
		chain.push_back(LLIOPipe::ptr_t(new LLIONull));
		mPump->addChain(chain, SHORT_CHAIN_EXPIRY_SECS / 2.0f);

		// Now, the server should immediately send the data, but we'll
		// never read it. pump for a bit
		F32 elapsed = pump_loop(mPump, SHORT_CHAIN_EXPIRY_SECS * 2.0f);
		ensure("Did not take too long", (elapsed < 3.0f));
	}

	template<> template<>
	void fitness_test_object::test<3>()
	{
		lldebugs << "fitness_test_object::test<3>()" << llendl;

		// Set up the server
		LLPumpIO::chain_t chain;
		typedef LLCloneIOFactory<LLIOFuzz> emitter_t;
		emitter_t* emitter = new emitter_t(new LLIOFuzz(1000000));
		boost::shared_ptr<LLChainIOFactory> factory(emitter);
		LLIOServerSocket* server = new LLIOServerSocket(
			mPool,
			mSocket,
			factory);
		server->setResponseTimeout(SHORT_CHAIN_EXPIRY_SECS);
		chain.push_back(LLIOPipe::ptr_t(server));
		mPump->addChain(chain, NEVER_CHAIN_EXPIRY_SECS);

		// We need to tickle the pump a little to set up the listen()
		pump_loop(mPump, 0.1f);

		// Set up the client
		LLSocket::ptr_t client = LLSocket::create(mPool, LLSocket::STREAM_TCP);
		LLHost server_host("127.0.0.1", SERVER_LISTEN_PORT);
		bool connected = client->blockingConnect(server_host);
		ensure("Connected to server", connected);
		lldebugs << "connected" << llendl;

		// We have connected, since the socket reader does not block,
		// the first call to read data will return EAGAIN, so we need
		// to write something.
		chain.clear();
		chain.push_back(LLIOPipe::ptr_t(new LLPipeStringInjector("hi")));
		chain.push_back(LLIOPipe::ptr_t(new LLIOSocketWriter(client)));
		chain.push_back(LLIOPipe::ptr_t(new LLIONull));
		mPump->addChain(chain, SHORT_CHAIN_EXPIRY_SECS * 2.0f);

		// Now, the server should immediately send the data, but we'll
		// never read it. pump for a bit
		F32 elapsed = pump_loop(mPump, SHORT_CHAIN_EXPIRY_SECS * 2.0f + 1.0f);
		ensure("Did not take too long", (elapsed < 4.0f));
	}

	template<> template<>
	void fitness_test_object::test<4>()
	{
		lldebugs << "fitness_test_object::test<4>()" << llendl;

		// Set up the server
		LLPumpIO::chain_t chain;
		typedef LLCloneIOFactory<LLIOFuzz> emitter_t;
		emitter_t* emitter = new emitter_t(new LLIOFuzz(1000000));
		boost::shared_ptr<LLChainIOFactory> factory(emitter);
		LLIOServerSocket* server = new LLIOServerSocket(
			mPool,
			mSocket,
			factory);
		server->setResponseTimeout(SHORT_CHAIN_EXPIRY_SECS + 1.80f);
		chain.push_back(LLIOPipe::ptr_t(server));
		mPump->addChain(chain, NEVER_CHAIN_EXPIRY_SECS);

		// We need to tickle the pump a little to set up the listen()
		pump_loop(mPump, 0.1f);

		// Set up the client
		LLSocket::ptr_t client = LLSocket::create(mPool, LLSocket::STREAM_TCP);
		LLHost server_host("127.0.0.1", SERVER_LISTEN_PORT);
		bool connected = client->blockingConnect(server_host);
		ensure("Connected to server", connected);
		lldebugs << "connected" << llendl;

		// We have connected, since the socket reader does not block,
		// the first call to read data will return EAGAIN, so we need
		// to write something.
		chain.clear();
		chain.push_back(LLIOPipe::ptr_t(new LLPipeStringInjector("hi")));
		chain.push_back(LLIOPipe::ptr_t(new LLIOSocketWriter(client)));
		chain.push_back(LLIOPipe::ptr_t(new LLIONull));
		mPump->addChain(chain, NEVER_CHAIN_EXPIRY_SECS);

		// Now, the server should immediately send the data, but we'll
		// never read it. pump for a bit
		F32 elapsed = pump_loop(mPump, SHORT_CHAIN_EXPIRY_SECS + 3.0f);
		ensure("Did not take too long", (elapsed < DEFAULT_CHAIN_EXPIRY_SECS));
	}

	template<> template<>
	void fitness_test_object::test<5>()
	{
		// Set up the server
		LLPumpIO::chain_t chain;
		typedef LLCloneIOFactory<LLIOSleeper> sleeper_t;
		sleeper_t* sleeper = new sleeper_t(new LLIOSleeper);
		boost::shared_ptr<LLChainIOFactory> factory(sleeper);
		LLIOServerSocket* server = new LLIOServerSocket(
			mPool,
			mSocket,
			factory);
		server->setResponseTimeout(1.0);
		chain.push_back(LLIOPipe::ptr_t(server));
		mPump->addChain(chain, NEVER_CHAIN_EXPIRY_SECS);
		// We need to tickle the pump a little to set up the listen()
		pump_loop(mPump, 0.1f);
		U32 count = mPump->runningChains();
		ensure_equals("server chain onboard", count, 1);
		lldebugs << "** Server is up." << llendl;

		// Set up the client
		LLSocket::ptr_t client = LLSocket::create(mPool, LLSocket::STREAM_TCP);
		LLHost server_host("127.0.0.1", SERVER_LISTEN_PORT);
		bool connected = client->blockingConnect(server_host);
		ensure("Connected to server", connected);
		lldebugs << "connected" << llendl;
		pump_loop(mPump,0.1f);
		count = mPump->runningChains();
		ensure_equals("server chain onboard", count, 2);
		lldebugs << "** Client is connected." << llendl;

		// We have connected, since the socket reader does not block,
		// the first call to read data will return EAGAIN, so we need
		// to write something.
		chain.clear();
		chain.push_back(LLIOPipe::ptr_t(new LLPipeStringInjector("hi")));
		chain.push_back(LLIOPipe::ptr_t(new LLIOSocketWriter(client)));
		chain.push_back(LLIOPipe::ptr_t(new LLIONull));
		mPump->addChain(chain, 0.2f);
		chain.clear();

		// pump for a bit and make sure all 3 chains are running
		pump_loop(mPump,0.1f);
		count = mPump->runningChains();
		ensure_equals("client chain onboard", count, 3);
		lldebugs << "** request should have been sent." << llendl;

		// pump for long enough the the client socket closes, and the
		// server socket should not be closed yet.
		pump_loop(mPump,0.2f);
		count = mPump->runningChains();
		ensure_equals("client chain timed out ", count, 2);
		lldebugs << "** client chain should be closed." << llendl;

		// At this point, the socket should be closed by the timeout
		pump_loop(mPump,1.0f);
		count = mPump->runningChains();
		ensure_equals("accepted socked close", count, 1);
		lldebugs << "** Sleeper should have timed out.." << llendl;
	}
}

namespace tut
{
	struct rpc_server_data
	{
		class LLSimpleRPCResponse : public LLSDRPCResponse
		{
		public:
			LLSimpleRPCResponse(LLSD* response) :
				mResponsePtr(response)
			{
			}
			~LLSimpleRPCResponse() {}
			virtual bool response(LLPumpIO* pump)
			{
				*mResponsePtr = mReturnValue;
				return true;
			}
			virtual bool fault(LLPumpIO* pump)
			{
				*mResponsePtr = mReturnValue;
				return false;
			}
			virtual bool error(LLPumpIO* pump)
			{
				ensure("LLSimpleRPCResponse::error()", false);
				return false;
			}
		public:
			LLSD* mResponsePtr;
		};

		class LLSimpleRPCClient : public LLSDRPCClient
		{
		public:
			LLSimpleRPCClient(LLSD* response) :
				mResponsePtr(response)
			{
			}
			~LLSimpleRPCClient() {}
			void echo(const LLSD& parameter)
			{
				LLSimpleRPCResponse* resp;
				resp = new LLSimpleRPCResponse(mResponsePtr);
				static const std::string URI_NONE;
				static const std::string METHOD_ECHO("echo");
				call(URI_NONE, METHOD_ECHO, parameter, resp, EPBQ_CALLBACK);
			}
		public:
			LLSD* mResponsePtr;
		};

		class LLSimpleRPCServer : public LLSDRPCServer
		{
		public:
			LLSimpleRPCServer()
			{
				mMethods["echo"] = new mem_fn_t(
					this,
					&LLSimpleRPCServer::rpc_Echo);
			}
			~LLSimpleRPCServer() {}
		protected:
			typedef LLSDRPCMethodCall<LLSimpleRPCServer> mem_fn_t;
			ESDRPCSStatus rpc_Echo(
				const LLSD& parameter,
				const LLChannelDescriptors& channels,
				LLBufferArray* data)
			{
				buildResponse(channels, data, parameter);
				return ESDRPCS_DONE;
			}
		};

		apr_pool_t* mPool;
		LLPumpIO* mPump;
		LLPumpIO::chain_t mChain;
		LLSimpleRPCClient* mClient;
		LLSD mResponse;

		rpc_server_data() :
			mPool(NULL),
			mPump(NULL),
			mClient(NULL)
		{
			apr_pool_create(&mPool, NULL);
			mPump = new LLPumpIO(mPool);
			mClient = new LLSimpleRPCClient(&mResponse);
			mChain.push_back(LLIOPipe::ptr_t(mClient));
			mChain.push_back(LLIOPipe::ptr_t(new LLFilterSD2XMLRPCRequest));
			mChain.push_back(LLIOPipe::ptr_t(new LLFilterXMLRPCRequest2LLSD));
			mChain.push_back(LLIOPipe::ptr_t(new LLSimpleRPCServer));
			mChain.push_back(LLIOPipe::ptr_t(new LLFilterSD2XMLRPCResponse));
			mChain.push_back(LLIOPipe::ptr_t(new LLFilterXMLRPCResponse2LLSD));
			mChain.push_back(LLIOPipe::ptr_t(mClient));
		}
		~rpc_server_data()
		{
			mChain.clear();
			delete mPump;
			mPump = NULL;
			apr_pool_destroy(mPool);
			mPool = NULL;
		}
		void pump_loop(const LLSD& request)
		{
			LLTimer timer;
			timer.setTimerExpirySec(1.0f);
			mClient->echo(request);
			mPump->addChain(mChain, DEFAULT_CHAIN_EXPIRY_SECS);
			while(mResponse.isUndefined() && !timer.hasExpired())
			{
				mPump->pump();
				mPump->callback();
			}
		}
	};
	typedef test_group<rpc_server_data> rpc_server_test;
	typedef rpc_server_test::object rpc_server_object;
	tut::rpc_server_test rpc("rpc_server");

	template<> template<>
	void rpc_server_object::test<1>()
	{
		LLSD request;
		request = 1;
		pump_loop(request);
		//llinfos << "request: " << *request << llendl;
		//llinfos << "response: " << *mResponse << llendl;
		ensure_equals("integer request response", mResponse.asInteger(), 1);
	}

	template<> template<>
	void rpc_server_object::test<2>()
	{
//#if LL_WINDOWS && _MSC_VER >= 1400
//        skip_fail("Fails on VS2005 due to broken LLSDSerialize::fromNotation() parser.");
//#endif
		std::string uri("sl-am:66.150.244.180:12035/location?start=region&px=70.9247&py=254.378&pz=38.7304&lx=-0.043753&ly=-0.999042&lz=0");
		std::stringstream stream;
		stream << "{'task_id':ucc706f2d-0b68-68f8-11a4-f1043ff35ca0}\n{\n\tname\tObject|\n\tpermissions 0\n}";
		std::vector<U8> expected_binary;
		expected_binary.resize(stream.str().size());
		memcpy(&expected_binary[0], stream.str().c_str(), stream.str().size());		/* Flawfinder: ignore */
		stream.str("");
		stream << "[{'uri':'" << uri << "'}, {'version':i1}, "
				  << "{'agent_id':'3c115e51-04f4-523c-9fa6-98aff1034730', 'session_id':'2c585cec-038c-40b0-b42e-a25ebab4d132', 'circuit_code':i1075, 'start':'region', 'limited_to_estate':i1 'first_name':'Phoenix', 'last_name':'Linden', 'group_title':'', 'group_id':u00000000-0000-0000-0000-000000000000, 'position':[r70.9247,r254.378,r38.7304], 'look_at':[r-0.043753,r-0.999042,r0], 'granters':[ua2e76fcd-9360-4f6d-a924-000000000003], 'texture_data':['5e481e8a-58a6-fc34-6e61-c7a36095c07f', 'c39675f5-ca90-a304-bb31-42cdb803a132', '5c989edf-88d1-b2ac-b00b-5ed4bab8e368', '6522e74d-1660-4e7f-b601-6f48c1659a77', '7ca39b4c-bd19-4699-aff7-f93fd03d3e7b', '41c58177-5eb6-5aeb-029d-bc4093f3c130', '97b75473-8b93-9b25-2a11-035b9ae93195', '1c2d8d9b-90eb-89d4-dea8-c1ed83990614', '69ec543f-e27b-c07c-9094-a8be6300f274', 'c9f8b80f-c629-4633-04ee-c566ce9fea4b', '989cddba-7ab6-01ed-67aa-74accd2a2a65', '45e319b2-6a8c-fa5c-895b-1a7149b88aef', '5748decc-f629-461c-9a36-a35a221fe21f', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', '685fbe10-ab40-f065-0aec-726cc6dfd7a1', '406f98fd-9c89-1d52-5f39-e67d508c5ee5', '685fbe10-ab40-f065-0aec-726cc6dfd7a1', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97'], "
				  << "'attachment_data':["
				  << "{'attachment_point':i2, 'item_id':'d6852c11-a74e-309a-0462-50533f1ef9b3', 'asset_id':'c69b29b1-8944-58ae-a7c5-2ca7b23e22fb'},"
				  << "{'attachment_point':i10, 'item_id':'ff852c22-a74e-309a-0462-50533f1ef900', 'asset_data':b(" << expected_binary.size() << ")\"";
		stream.write((const char*)&expected_binary[0], expected_binary.size());
		stream << "\"}"
				  << "]"
				  << "}]";

		LLSD request;
		S32 count = LLSDSerialize::fromNotation(
			request,
			stream,
			stream.str().size());
		ensure("parsed something", (count > 0));

		pump_loop(request);
		ensure_equals("return type", mResponse.type(), LLSD::TypeArray);
		ensure_equals("return size", mResponse.size(), 3);

		ensure_equals(
			"uri parameter type",
			mResponse[0].type(),
			LLSD::TypeMap);
		ensure_equals(
			"uri type",
			mResponse[0]["uri"].type(),
			LLSD::TypeString);
		ensure_equals("uri value", mResponse[0]["uri"].asString(), uri);

		ensure_equals(
			"version parameter type",
			mResponse[1].type(),
			LLSD::TypeMap);
		ensure_equals(
			"version type",
			mResponse[1]["version"].type(),
			LLSD::TypeInteger);
		ensure_equals(
			"version value",
			mResponse[1]["version"].asInteger(),
			1);

		ensure_equals("agent params type", mResponse[2].type(), LLSD::TypeMap);
		LLSD attachment_data = mResponse[2]["attachment_data"];
		ensure("attachment data exists", attachment_data.isDefined());
		ensure_equals(
			"attachment type",
			attachment_data.type(),
			LLSD::TypeArray);
		ensure_equals(
			"attachment type 0",
			attachment_data[0].type(),
			LLSD::TypeMap);
		ensure_equals(
			"attachment type 1",
			attachment_data[1].type(),
			LLSD::TypeMap);
		ensure_equals("attachment size 1", attachment_data[1].size(), 3);
		ensure_equals(
			"asset data type",
			attachment_data[1]["asset_data"].type(),
			LLSD::TypeBinary);
		std::vector<U8> actual_binary;
		actual_binary = attachment_data[1]["asset_data"].asBinary();
		ensure_equals(
			"binary data size",
			actual_binary.size(),
			expected_binary.size());
		ensure(
			"binary data",
			(0 == memcmp(
				&actual_binary[0],
				&expected_binary[0],
				expected_binary.size())));
	}

	template<> template<>
	void rpc_server_object::test<3>()
	{
//#if LL_WINDOWS && _MSC_VER >= 1400
//        skip_fail("Fails on VS2005 due to broken LLSDSerialize::fromNotation() parser.");
//#endif
		std::string uri("sl-am:66.150.244.180:12035/location?start=region&px=70.9247&py=254.378&pz=38.7304&lx=-0.043753&ly=-0.999042&lz=0");

		LLBufferArray buffer;
		LLChannelDescriptors buffer_channels = buffer.nextChannel();
		LLBufferStream stream(buffer_channels, &buffer);
		stream << "[{'uri':'" << uri << "'}, {'version':i1}, "
				  << "{'agent_id':'3c115e51-04f4-523c-9fa6-98aff1034730', 'session_id':'2c585cec-038c-40b0-b42e-a25ebab4d132', 'circuit_code':i1075, 'start':'region', 'limited_to_estate':i1 'first_name':'Phoenix', 'last_name':'Linden', 'group_title':'', 'group_id':u00000000-0000-0000-0000-000000000000, 'position':[r70.9247,r254.378,r38.7304], 'look_at':[r-0.043753,r-0.999042,r0], 'granters':[ua2e76fcd-9360-4f6d-a924-000000000003], 'texture_data':['5e481e8a-58a6-fc34-6e61-c7a36095c07f', 'c39675f5-ca90-a304-bb31-42cdb803a132', '5c989edf-88d1-b2ac-b00b-5ed4bab8e368', '6522e74d-1660-4e7f-b601-6f48c1659a77', '7ca39b4c-bd19-4699-aff7-f93fd03d3e7b', '41c58177-5eb6-5aeb-029d-bc4093f3c130', '97b75473-8b93-9b25-2a11-035b9ae93195', '1c2d8d9b-90eb-89d4-dea8-c1ed83990614', '69ec543f-e27b-c07c-9094-a8be6300f274', 'c9f8b80f-c629-4633-04ee-c566ce9fea4b', '989cddba-7ab6-01ed-67aa-74accd2a2a65', '45e319b2-6a8c-fa5c-895b-1a7149b88aef', '5748decc-f629-461c-9a36-a35a221fe21f', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', '685fbe10-ab40-f065-0aec-726cc6dfd7a1', '406f98fd-9c89-1d52-5f39-e67d508c5ee5', '685fbe10-ab40-f065-0aec-726cc6dfd7a1', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97', 'c228d1cf-4b5d-4ba8-84f4-899a0796aa97'], "
				  << "'attachment_data':["
				  << "{'attachment_point':i2, 'item_id':'d6852c11-a74e-309a-0462-50533f1ef9b3', 'asset_id':'c69b29b1-8944-58ae-a7c5-2ca7b23e22fb'},";

		std::stringstream tmp_str;
		tmp_str << "{'task_id':ucc706f2d-0b68-68f8-11a4-f1043ff35ca0}\n{\n\tname\tObject|\n\tpermissions 0\n}";
		std::vector<U8> expected_binary;
		expected_binary.resize(tmp_str.str().size());
		memcpy(		/* Flawfinder: ignore */
			&expected_binary[0],
			tmp_str.str().c_str(),
			tmp_str.str().size());

		LLBufferArray attachment_buffer;
		LLChannelDescriptors attach_channels = attachment_buffer.nextChannel();
		LLBufferStream attach_stream(attach_channels, &attachment_buffer);
		attach_stream.write((const char*)&expected_binary[0], expected_binary.size());
		attach_stream.flush();
		S32 len = attachment_buffer.countAfter(attach_channels.out(), NULL);
		stream << "{'attachment_point':i10, 'item_id':'ff852c22-a74e-309a-0462-50533f1ef900', 'asset_data':b(" << len << ")\"";
		stream.flush();
		buffer.takeContents(attachment_buffer);
		stream << "\"}]}]";
		stream.flush();

		LLChannelDescriptors read_channel = buffer.nextChannel();
		LLBufferStream read_stream(read_channel, &buffer);
		LLSD request;
		S32 count = LLSDSerialize::fromNotation(
			request,
			read_stream,
			buffer.count(read_channel.in()));
		ensure("parsed something", (count > 0));
		ensure("deserialized", request.isDefined());

		// do the rpc round trip
		pump_loop(request);

		ensure_equals("return type", mResponse.type(), LLSD::TypeArray);
		ensure_equals("return size", mResponse.size(), 3);

		LLSD child = mResponse[0];
		ensure("uri map exists", child.isDefined());
		ensure_equals("uri parameter type", child.type(), LLSD::TypeMap);
		ensure("uri string exists", child.has("uri"));
		ensure_equals("uri type", child["uri"].type(), LLSD::TypeString);
		ensure_equals("uri value", child["uri"].asString(), uri);

		child = mResponse[1];
		ensure("version map exists",	child.isDefined());
		ensure_equals("version param type", child.type(), LLSD::TypeMap);
		ensure_equals(
			"version type",
			child["version"].type(),
			LLSD::TypeInteger);
		ensure_equals("version value", child["version"].asInteger(), 1);

		child = mResponse[2];
		ensure("agent params map exists", child.isDefined());
		ensure_equals("agent params type", child.type(), LLSD::TypeMap);
		child = child["attachment_data"];
		ensure("attachment data exists", child.isDefined());
		ensure_equals("attachment type", child.type(), LLSD::TypeArray);
		LLSD attachment = child[0];
		ensure_equals("attachment type 0", attachment.type(), LLSD::TypeMap);
		attachment = child[1];
		ensure_equals("attachment type 1", attachment.type(), LLSD::TypeMap);
		ensure_equals("attachment size 1", attachment.size(), 3);
		ensure_equals(
			"asset data type",
			attachment["asset_data"].type(),
			LLSD::TypeBinary);
		std::vector<U8> actual_binary = attachment["asset_data"].asBinary();
		ensure_equals(
			"binary data size",
			actual_binary.size(),
			expected_binary.size());
		ensure(
			"binary data",
			(0 == memcmp(
				&actual_binary[0],
				&expected_binary[0],
				expected_binary.size())));
	}

	template<> template<>
	void rpc_server_object::test<4>()
	{
		std::string message("parcel '' is naughty.");
		std::stringstream str;
		str << "{'message':'" << LLSDNotationFormatter::escapeString(message)
			<< "'}";
		LLSD request;
		S32 count = LLSDSerialize::fromNotation(
			request,
			str,
			str.str().size());
		ensure_equals("parse count", count, 2);
		ensure_equals("request type", request.type(), LLSD::TypeMap);
		pump_loop(request);
		ensure("valid response", mResponse.isDefined());
		ensure_equals("response type", mResponse.type(), LLSD::TypeMap);
		std::string actual = mResponse["message"].asString();
		ensure_equals("message contents", actual, message);
	}

	template<> template<>
	void rpc_server_object::test<5>()
	{
		// test some of the problem cases with llsdrpc over xmlrpc -
		// for example:
		// * arrays are auto-converted to parameter lists, thus, this
		// becomes one parameter.
		// * undef goes over the wire as false (this might not be a good idea)
		// * uuids are converted to string.
		std::string val = "[{'failures':!,'successfuls':[u3c115e51-04f4-523c-9fa6-98aff1034730]}]";
		std::istringstream istr;
		istr.str(val);
		LLSD sd;
		LLSDSerialize::fromNotation(sd, istr, val.size());
		pump_loop(sd);
		ensure("valid response", mResponse.isDefined());
		ensure_equals("parsed type", mResponse.type(), LLSD::TypeMap);
		ensure_equals("parsed size", mResponse.size(), 2);
		LLSD failures = mResponse["failures"];
		ensure_equals("no failures.", failures.asBoolean(), false);
		LLSD success = mResponse["successfuls"];
		ensure_equals("success type", success.type(), LLSD::TypeArray);
		ensure_equals("success size", success.size(), 1);
		ensure_equals(
			"success instance type",
			success[0].type(),
			LLSD::TypeString);
	}

/*
	template<> template<>
	void rpc_server_object::test<5>()
	{
		std::string expected("\xf3");//\xffsomething");
		LLSD* request = LLSD::createString(expected);
		pump_loop(request);
		std::string actual;
		mResponse->getString(actual);
		if(actual != expected)
		{
			//llwarns << "iteration " << i << llendl;
			std::ostringstream e_str;
			std::string::iterator iter = expected.begin();
			std::string::iterator end = expected.end();
			for(; iter != end; ++iter)
			{
				e_str << (S32)((U8)(*iter)) << " ";
			}
			e_str << std::endl;
			llsd_serialize_string(e_str, expected);
			llwarns << "expected size: " << expected.size() << llendl;
			llwarns << "expected:	  " << e_str.str() << llendl;

			std::ostringstream a_str;
			iter = actual.begin();
			end = actual.end();
			for(; iter != end; ++iter)
			{
				a_str << (S32)((U8)(*iter)) << " ";
			}
			a_str << std::endl;
			llsd_serialize_string(a_str, actual);
			llwarns << "actual size:	  " << actual.size() << llendl;
			llwarns << "actual:		" << a_str.str() << llendl;
		}
		ensure_equals("binary string request response", actual, expected);
		delete request;
	}

	template<> template<>
	void rpc_server_object::test<5>()
	{
	}
*/
}


/*
'asset_data':b(12100)"{'task_id':ucc706f2d-0b68-68f8-11a4-f1043ff35ca0}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreator_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444921\n\ttotal_crc\t323\n\ttype\t2\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.368634403\t0.00781063363\t-0.569040775\n\toldpos\t150.117996\t25.8658009\t8.19664001\n\trotation\t-0.06293071806430816650390625\t-0.6995697021484375\t-0.7002241611480712890625\t0.1277817934751510620117188\n\tchildpos\t-0.00499999989\t-0.0359999985\t0.307999998\n\tchildrot\t-0.515492737293243408203125\t-0.46601200103759765625\t0.529055416584014892578125\t0.4870323240756988525390625\n\tscale\t0.074629\t0.289956\t0.01\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t16\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\tscale_x\t1\n\t\t\tscale_y\t1\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t1\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t6\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t-1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061088050622956\n\treztime\t1094866329019785\n\tparceltime\t1133568981980596\n\ttax_rate\t1.00084\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tchild\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n{'task_id':u61fa7364-e151-0597-774c-523312dae31b}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreator_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444922\n\ttotal_crc\t324\n\ttype\t2\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.367110789\t0.00780026987\t-0.566269755\n\toldpos\t150.115005\t25.8479004\t8.18669987\n\trotation\t0.47332942485809326171875\t-0.380102097988128662109375\t-0.5734078884124755859375\t0.550168216228485107421875\n\tchildpos\t-0.00499999989\t-0.0370000005\t0.305000007\n\tchildrot\t-0.736649334430694580078125\t-0.03042060509324073791503906\t-0.02784589119255542755126953\t0.67501628398895263671875\n\tscale\t0.074629\t0.289956\t0.01\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t16\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\tscale_x\t1\n\t\t\tscale_y\t1\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t1\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t6\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tf54a0c32-3cd1-d49a-5b4f-7b792bebc204\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\tddde1ffc-678b-3cda-1748-513086bdf01b\n\t\tcolors\t0.937255 0.796078 0.494118 1\n\t\tscales\t1\n\t\tscalet\t-1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t0\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061087839248891\n\treztime\t1094866329020800\n\tparceltime\t1133568981981983\n\ttax_rate\t1.00084\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tchild\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n{'task_id':ub8d68643-7dd8-57af-0d24-8790032aed0c}\n{\n\tname\tObject|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreator_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444923\n\ttotal_crc\t235\n\ttype\t2\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.120029509\t-0.00284469454\t-0.0302077383\n\toldpos\t150.710999\t25.8584995\t8.19172001\n\trotation\t0.145459949970245361328125\t-0.1646589934825897216796875\t0.659558117389678955078125\t-0.718826770782470703125\n\tchildpos\t0\t-0.182999998\t-0.26699999\n\tchildrot\t0.991444766521453857421875\t3.271923924330621957778931e-05\t-0.0002416197530692443251609802\t0.1305266767740249633789062\n\tscale\t0.0382982\t0.205957\t0.368276\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t32\n\t\t\tbegin\t0.3\n\t\t\tend\t0.65\n\t\t\tscale_x\t1\n\t\t\tscale_y\t0.05\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t0\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t3\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061087534454174\n\treztime\t1094866329021741\n\tparceltime\t1133568981982889\n\ttax_rate\t1.00326\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tchild\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n{'task_id':ue4b19200-9d33-962f-c8c5-6f25be3a3fd0}\n{\n\tname\tApotheosis_Immolaine_tail|\n\tpermissions 0\n\t{\n\t\tbase_mask\t7fffffff\n\t\towner_mask\t7fffffff\n\t\tgroup_mask\t00000000\n\t\teveryone_mask\t00000000\n\t\tnext_owner_mask\t7fffffff\n\t\tcreator_id\t13fd9595-a47b-4d64-a5fb-6da645f038e0\n\t\towner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tlast_owner_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\t\tgroup_id\t00000000-0000-0000-0000-000000000000\n\t}\n\tlocal_id\t217444924\n\ttotal_crc\t675\n\ttype\t1\n\ttask_valid\t2\n\ttravel_access\t13\n\tdisplayopts\t2\n\tdisplaytype\tv\n\tpos\t-0.34780401\t-0.00968400016\t-0.260098994\n\toldpos\t0\t0\t0\n\trotation\t0.73164522647857666015625\t-0.67541944980621337890625\t-0.07733880728483200073242188\t0.05022468417882919311523438\n\tvelocity\t0\t0\t0\n\tangvel\t0\t0\t0\n\tscale\t0.0382982\t0.32228\t0.383834\n\tsit_offset\t0\t0\t0\n\tcamera_eye_offset\t0\t0\t0\n\tcamera_at_offset\t0\t0\t0\n\tsit_quat\t0\t0\t0\t1\n\tsit_hint\t0\n\tstate\t160\n\tmaterial\t3\n\tsoundid\t00000000-0000-0000-0000-000000000000\n\tsoundgain\t0\n\tsoundradius\t0\n\tsoundflags\t0\n\ttextcolor\t0 0 0 1\n\tselected\t0\n\tselector\t00000000-0000-0000-0000-000000000000\n\tusephysics\t0\n\trotate_x\t1\n\trotate_y\t1\n\trotate_z\t1\n\tphantom\t0\n\tremote_script_access_pin\t0\n\tvolume_detect\t0\n\tblock_grabs\t0\n\tdie_at_edge\t0\n\treturn_at_edge\t0\n\ttemporary\t0\n\tsandbox\t0\n\tsandboxhome\t0\t0\t0\n\tshape 0\n\t{\n\t\tpath 0\n\t\t{\n\t\t\tcurve\t32\n\t\t\tbegin\t0.3\n\t\t\tend\t0.65\n\t\t\tscale_x\t1\n\t\t\tscale_y\t0.05\n\t\t\tshear_x\t0\n\t\t\tshear_y\t0\n\t\t\ttwist\t0\n\t\t\ttwist_begin\t0\n\t\t\tradius_offset\t0\n\t\t\ttaper_x\t0\n\t\t\ttaper_y\t0\n\t\t\trevolutions\t1\n\t\t\tskew\t0\n\t\t}\n\t\tprofile 0\n\t\t{\n\t\t\tcurve\t0\n\t\t\tbegin\t0\n\t\t\tend\t1\n\t\t\thollow\t0\n\t\t}\n\t}\n\tfaces\t3\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\t{\n\t\timageid\te7150bed-3e3e-c698-eb15-d17b178148af\n\t\tcolors\t0.843137 0.156863 0.156863 1\n\t\tscales\t15\n\t\tscalet\t1\n\t\toffsets\t0\n\t\toffsett\t0\n\t\timagerot\t-1.57084\n\t\tbump\t0\n\t\tfullbright\t0\n\t\tmedia_flags\t0\n\t}\n\tps_next_crc\t1\n\tgpw_bias\t1\n\tip\t0\n\tcomplete\tTRUE\n\tdelay\t50000\n\tnextstart\t0\n\tbirthtime\t1061087463950186\n\treztime\t1094866329022555\n\tparceltime\t1133568981984359\n\tdescription\t(No Description)|\n\ttax_rate\t1.01736\n\tnamevalue\tAttachPt U32 RW S 10\n\tnamevalue\tAttachmentOrientation VEC3 RW DS -3.110088, -0.182018, 1.493795\n\tnamevalue\tAttachmentOffset VEC3 RW DS -0.347804, -0.009684, -0.260099\n\tnamevalue\tAttachItemID STRING RW SV 20f36c3a-b44b-9bc7-87f3-018bfdfc8cda\n\tscratchpad\t0\n\t{\n\t\n\t}\n\tsale_info\t0\n\t{\n\t\tsale_type\tnot\n\t\tsale_price\t10\n\t}\n\torig_asset_id\t8747acbc-d391-1e59-69f1-41d06830e6c0\n\torig_item_id\t20f36c3a-b44b-9bc7-87f3-018bfdfc8cda\n\tfrom_task_id\t3c115e51-04f4-523c-9fa6-98aff1034730\n\tcorrect_family_id\t00000000-0000-0000-0000-000000000000\n\thas_rezzed\t0\n\tpre_link_base_mask\t7fffffff\n\tlinked \tlinked\n\tdefault_pay_price\t-2\t1\t5\t10\t20\n}\n"
*/
