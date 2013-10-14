/**
 * @file llbuffer_tut.cpp
 * @author Adroit
 * @date 2007-03
 * @brief llbuffer test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include <tut/tut.hpp>
#include "linden_common.h"
#include "lltut.h"
#include "llbuffer.h"
#include "llerror.h"


namespace tut
{
	struct buffer
	{
	};

	typedef test_group<buffer> buffer_t;
	typedef buffer_t::object buffer_object_t;
	tut::buffer_t tut_buffer("buffer");

	template<> template<>
	void buffer_object_t::test<1>()
	{
		LLChannelDescriptors channelDescriptors;
		ensure("in() and out() functions Failed", (0 == channelDescriptors.in() && 1 == channelDescriptors.out()));
	
		S32 val = 50;
		LLChannelDescriptors channelDescriptors1(val);
		ensure("LLChannelDescriptors in() and out() functions Failed", (50 == channelDescriptors1.in() && 51 == channelDescriptors1.out()));
	}	
	
	template<> template<>
	void buffer_object_t::test<2>()
	{
		LLSegment segment;
		ensure("LLSegment get functions failed", (0 == segment.getChannel() && NULL == segment.data() && 0 == segment.size()));
		segment.setChannel(50);
		ensure_equals("LLSegment setChannel() function failed", segment.getChannel(), 50);
		ensure("LLSegment isOnChannel() function failed", (TRUE == segment.isOnChannel(50)));
	}	

	template<> template<>
	void buffer_object_t::test<3>()
	{
		S32 channel = 30;
		const char str[] = "SecondLife";
		S32 len = sizeof(str);
		LLSegment segment(channel, (U8*)str, len);
		ensure("LLSegment get functions failed", (30 == segment.getChannel() && len == segment.size() && (U8*)str == segment.data()));
		ensure_memory_matches("LLSegment::data() failed",  segment.data(), segment.size(), (U8*)str, len);
		ensure("LLSegment isOnChannel() function failed", (TRUE == segment.isOnChannel(channel)));
	}	 
	
	template<> template<>
	void buffer_object_t::test<4>()
	{
		S32 channel = 50;
		S32 bigSize = 16384*2;
		char str[] = "SecondLife";
		S32 smallSize = sizeof(str);

		LLSegment segment;
		LLHeapBuffer buf; // use default size of DEFAULT_HEAP_BUFFER_SIZE = 16384

		S32 requestSize;

		requestSize = 16384-1;
		ensure("1. LLHeapBuffer createSegment failed", (TRUE == buf.createSegment(channel, requestSize, segment)) && segment.size() == requestSize);
		// second request for remainign 1 byte

		requestSize = 1;
		ensure("2. LLHeapBuffer createSegment failed", (TRUE == buf.createSegment(channel, requestSize, segment)) && segment.size() == requestSize);

		// it should fail now.
		requestSize = 1;
		ensure("3. LLHeapBuffer createSegment failed", (FALSE == buf.createSegment(channel, requestSize, segment)));

		LLHeapBuffer buf1(bigSize);

		// requst for more than default size but less than total sizeit should fail now.
		requestSize = 16384 + 1;
		ensure("4. LLHeapBuffer createSegment failed", (TRUE == buf1.createSegment(channel, requestSize, segment)) && segment.size() == requestSize);

		LLHeapBuffer buf2((U8*) str, smallSize);
		requestSize = smallSize;
		ensure("5. LLHeapBuffer createSegment failed", (TRUE == buf2.createSegment(channel, requestSize, segment)) && segment.size() == requestSize && memcmp(segment.data(), (U8*) str, requestSize) == 0);
		requestSize = smallSize+1;
		ensure("6. LLHeapBuffer createSegment failed", (FALSE == buf2.createSegment(channel, requestSize, segment)));
	}	

	//makeChannelConsumer()
	template<> template<>
	void buffer_object_t::test<5>()
	{
		LLChannelDescriptors inchannelDescriptors(20);
		LLChannelDescriptors outchannelDescriptors = LLBufferArray::makeChannelConsumer(inchannelDescriptors);	
		ensure("LLBufferArray::makeChannelConsumer() function Failed", (21 == outchannelDescriptors.in()));
	}

	template<> template<>
	void buffer_object_t::test<6>()
	{
		LLBufferArray bufferArray;
		const char array[] = "SecondLife";
		S32 len = strlen(array);
		LLChannelDescriptors channelDescriptors = bufferArray.nextChannel();
		bufferArray.append(channelDescriptors.in(), (U8*)array, len);
		S32 count = bufferArray.countAfter(channelDescriptors.in(), NULL);
		ensure_equals("Appended size is:", count, len);
	}

	//append() and prepend()
	template<> template<>
	void buffer_object_t::test<7>()
	{
		LLBufferArray bufferArray;
		const char array[] = "SecondLife";
		S32 len = strlen(array);
		const char array1[] = "LindenLabs";
		S32 len1 = strlen(array1);
		
		std::string str(array1);
		str.append(array);
		
		LLChannelDescriptors channelDescriptors = bufferArray.nextChannel();
		bufferArray.append(channelDescriptors.in(), (U8*)array, len);
		bufferArray.prepend(channelDescriptors.in(), (U8*)array1, len1);
		char buf[100];
		S32 len2 = 20;
		bufferArray.readAfter(channelDescriptors.in(), NULL, (U8*)buf, len2);
		ensure_equals("readAfter length failed", len2, 20);
		
		buf[len2] = '\0';
		ensure_equals("readAfter/prepend/append failed", buf, str);
	}

	//append()
	template<> template<>
	void buffer_object_t::test<8>()
	{
		LLBufferArray bufferArray;
		const char array[] = "SecondLife";
		S32 len = strlen(array);
		const char array1[] = "LindenLabs";
		S32 len1 = strlen(array1);
		
		std::string str(array);
		str.append(array1);
		
		LLChannelDescriptors channelDescriptors = bufferArray.nextChannel();
		bufferArray.append(channelDescriptors.in(), (U8*)array, len);		
		bufferArray.append(channelDescriptors.in(), (U8*)array1, len1);
		char buf[100];
		S32 len2 = 20;
		bufferArray.readAfter(channelDescriptors.in(), NULL, (U8*)buf, len2);
		ensure_equals("readAfter length failed", len2, 20);
		
		buf[len2] = '\0';
		ensure_equals("readAfter/append/append failed", buf, str);
	}

	template<> template<>
	void buffer_object_t::test<9>()
	{
		LLBufferArray bufferArray;
		const char array[] = "SecondLife";
		S32 len = strlen(array) + 1;
		std::string str(array);
		LLChannelDescriptors channelDescriptors = bufferArray.nextChannel();
		bufferArray.append(channelDescriptors.in(), (U8*)array, len);
		LLBufferArray bufferArray1;
		ensure("Contents are not copied and the source buffer is not empty", (1 == bufferArray1.takeContents(bufferArray)));
		
		char buf[100];
		S32 len2 = len;
		bufferArray1.readAfter(channelDescriptors.in(), NULL, (U8*)buf, len2);	
		ensure_equals("takeContents failed to copy", buf, str);
	}

	//seek()
	template<> template<>
	void buffer_object_t::test<10>()
	{
		const char array[] = "SecondLife is a Virtual World";
		S32 len = strlen(array);
		LLBufferArray bufferArray;
		bufferArray.append(0, (U8*)array, len);
		
		char buf[255];
		S32 len1 = 16;
		U8* last = bufferArray.readAfter(0, 0, (U8*)buf, len1);
		buf[len1] = '\0';
		last = bufferArray.seek(0, last, -2);

		len1 = 15;
		last = bufferArray.readAfter(0, last, (U8*)buf, len1);
		buf[len1] = '\0';
		std::string str(buf);
		ensure_equals("Seek does'nt worked", str, std::string("a Virtual World"));
	}

	template<> template<>
	void buffer_object_t::test<11>()
	{
		const char array[] = "SecondLife is a Virtual World";
		S32 len = strlen(array);
		LLBufferArray bufferArray;
		bufferArray.append(0, (U8*)array, len);
		
		char buf[255];
		S32 len1 = 10;
		U8* last = bufferArray.readAfter(0, 0, (U8*)buf, len1);
		bufferArray.splitAfter(last);
		LLBufferArray::segment_iterator_t iterator = bufferArray.beginSegment();
		++iterator;
		std::string str(((char*)(*iterator).data()), (*iterator).size());
		ensure_equals("Strings are not equal;splitAfter() operation failed", str, std::string(" is a Virtual World"));
	}

	//makeSegment()->eraseSegment()
	template<> template<>
	void buffer_object_t::test<12>()
	{
		LLBufferArray bufferArray;
		LLChannelDescriptors channelDescriptors;
		LLBufferArray::segment_iterator_t it;
		S32 length = 1000;
		it = bufferArray.makeSegment(channelDescriptors.out(), length);
		ensure("makeSegment() function failed", (it != bufferArray.endSegment()));
		ensure("eraseSegment() function failed", bufferArray.eraseSegment(it));
		ensure("eraseSegment() begin/end should now be same", bufferArray.beginSegment() == bufferArray.endSegment());
	}

	// constructSegmentAfter()
	template<> template<>
	void buffer_object_t::test<13>()
	{
		LLBufferArray bufferArray;
		LLBufferArray::segment_iterator_t it;
		LLSegment segment;
		LLBufferArray::segment_iterator_t end = bufferArray.endSegment();
		it = bufferArray.constructSegmentAfter(NULL, segment);
		ensure("constructSegmentAfter() function failed", (it == end));
	}
}
