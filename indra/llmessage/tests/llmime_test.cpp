/** 
 * @file llmime_test.cpp
 * @author Phoenix
 * @date 2006-12-24
 * @brief BRIEF_DESC of llmime_test.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llsdserialize.h"

#include "../llmime.h"

#include "../test/lltut.h"

namespace tut
{
	struct mime_index
	{
	};
	typedef test_group<mime_index> mime_index_t;
	typedef mime_index_t::object mime_index_object_t;
	tut::mime_index_t tut_mime_index("mime_index");

	template<> template<>
	void mime_index_object_t::test<1>()
	{
		LLMimeIndex mime;
		ensure("no headers", mime.headers().isUndefined());
		ensure_equals("invalid offset", mime.offset(), -1);
		ensure_equals("invalid content length", mime.contentLength(), -1);
		ensure("no content type", mime.contentType().empty());
		ensure("not multipart", !mime.isMultipart());
		ensure_equals("no attachments", mime.subPartCount(), 0);
	}

	template<> template<>
	void mime_index_object_t::test<2>()
	{
		const S32 CONTENT_LENGTH = 6000;
		const S32 CONTENT_OFFSET = 100;
		const std::string CONTENT_TYPE = std::string("image/j2c");
		LLSD headers;
		headers["Content-Length"] = CONTENT_LENGTH;
		headers["Content-Type"] = CONTENT_TYPE;
		LLMimeIndex mime(headers, CONTENT_OFFSET);
		ensure("headers are map", mime.headers().isMap());
		ensure_equals("offset", mime.offset(), CONTENT_OFFSET);
		ensure_equals("content length", mime.contentLength(), CONTENT_LENGTH);
		ensure_equals("type is image/j2c", mime.contentType(), CONTENT_TYPE);
		ensure("not multipart", !mime.isMultipart());
		ensure_equals("no attachments", mime.subPartCount(), 0);
	}

	template<> template<>
	void mime_index_object_t::test<3>()
	{
		const S32 MULTI_CONTENT_LENGTH = 8000;
		const S32 MULTI_CONTENT_OFFSET = 100;
		const std::string MULTI_CONTENT_TYPE = std::string("multipart/mixed");
		LLSD headers;
		headers["Content-Length"] = MULTI_CONTENT_LENGTH;
		headers["Content-Type"] = MULTI_CONTENT_TYPE;
		LLMimeIndex mime(headers, MULTI_CONTENT_OFFSET);
		llinfos << "headers: " << LLSDOStreamer<LLSDNotationFormatter>(headers)
			<< llendl;


		const S32 META_CONTENT_LENGTH = 700;
		const S32 META_CONTENT_OFFSET = 69;
		const std::string META_CONTENT_TYPE = std::string(
			"text/llsd+xml");
		headers = LLSD::emptyMap();
		headers["Content-Length"] = META_CONTENT_LENGTH;
		headers["Content-Type"] = META_CONTENT_TYPE;
		LLMimeIndex meta(headers, META_CONTENT_OFFSET);
		mime.attachSubPart(meta);

		const S32 IMAGE_CONTENT_LENGTH = 6000;
		const S32 IMAGE_CONTENT_OFFSET = 200;
		const std::string IMAGE_CONTENT_TYPE = std::string("image/j2c");
		headers = LLSD::emptyMap();
		headers["Content-Length"] = IMAGE_CONTENT_LENGTH;
		headers["Content-Type"] = IMAGE_CONTENT_TYPE;
		LLMimeIndex image(headers, IMAGE_CONTENT_OFFSET);
		mime.attachSubPart(image);

		// make sure we have a valid multi-part
		ensure("is multipart", mime.isMultipart());
		ensure_equals("multi offset", mime.offset(), MULTI_CONTENT_OFFSET);
		ensure_equals(
			"multi content length",
			mime.contentLength(),
			MULTI_CONTENT_LENGTH);
		ensure_equals("two attachments", mime.subPartCount(), 2);

		// make sure ranged gets do the right thing with out of bounds
		// sub-parts.
		LLMimeIndex invalid_child(mime.subPart(-1));
		ensure("no headers", invalid_child.headers().isUndefined());
		ensure_equals("invalid offset", invalid_child.offset(), -1);
		ensure_equals(
			"invalid content length", invalid_child.contentLength(), -1);
		ensure("no content type", invalid_child.contentType().empty());
		ensure("not multipart", !invalid_child.isMultipart());
		ensure_equals("no attachments", invalid_child.subPartCount(), 0);

		invalid_child = mime.subPart(2);
		ensure("no headers", invalid_child.headers().isUndefined());
		ensure_equals("invalid offset", invalid_child.offset(), -1);
		ensure_equals(
			"invalid content length", invalid_child.contentLength(), -1);
		ensure("no content type", invalid_child.contentType().empty());
		ensure("not multipart", !invalid_child.isMultipart());
		ensure_equals("no attachments", invalid_child.subPartCount(), 0);
	}

	template<> template<>
	void mime_index_object_t::test<4>()
	{
		const S32 MULTI_CONTENT_LENGTH = 8000;
		const S32 MULTI_CONTENT_OFFSET = 100;
		const std::string MULTI_CONTENT_TYPE = std::string("multipart/mixed");
		LLSD headers;
		headers["Content-Length"] = MULTI_CONTENT_LENGTH;
		headers["Content-Type"] = MULTI_CONTENT_TYPE;
		LLMimeIndex mime(headers, MULTI_CONTENT_OFFSET);

		const S32 META_CONTENT_LENGTH = 700;
		const S32 META_CONTENT_OFFSET = 69;
		const std::string META_CONTENT_TYPE = std::string(
			"application/llsd+xml");
		headers = LLSD::emptyMap();
		headers["Content-Length"] = META_CONTENT_LENGTH;
		headers["Content-Type"] = META_CONTENT_TYPE;
		LLMimeIndex meta(headers, META_CONTENT_OFFSET);
		mime.attachSubPart(meta);

		const S32 IMAGE_CONTENT_LENGTH = 6000;
		const S32 IMAGE_CONTENT_OFFSET = 200;
		const std::string IMAGE_CONTENT_TYPE = std::string("image/j2c");
		headers = LLSD::emptyMap();
		headers["Content-Length"] = IMAGE_CONTENT_LENGTH;
		headers["Content-Type"] = IMAGE_CONTENT_TYPE;
		LLMimeIndex image(headers, IMAGE_CONTENT_OFFSET);
		mime.attachSubPart(image);

		// check what we have
		ensure("is multipart", mime.isMultipart());
		ensure_equals("multi offset", mime.offset(), MULTI_CONTENT_OFFSET);
		ensure_equals(
			"multi content length",
			mime.contentLength(),
			MULTI_CONTENT_LENGTH);
		ensure_equals("two attachments", mime.subPartCount(), 2);

		LLMimeIndex actual_meta = mime.subPart(0);
		ensure_equals(
			"meta type", actual_meta.contentType(), META_CONTENT_TYPE);
		ensure_equals(
			"meta offset", actual_meta.offset(), META_CONTENT_OFFSET);
		ensure_equals(
			"meta content length",
			actual_meta.contentLength(),
			META_CONTENT_LENGTH);

		LLMimeIndex actual_image = mime.subPart(1);
		ensure_equals(
			"image type", actual_image.contentType(), IMAGE_CONTENT_TYPE);
		ensure_equals(
			"image offset", actual_image.offset(), IMAGE_CONTENT_OFFSET);
		ensure_equals(
			"image content length",
			actual_image.contentLength(),
			IMAGE_CONTENT_LENGTH);
	}

/*
	template<> template<>
	void mime_index_object_t::test<5>()
	{
	}
	template<> template<>
	void mime_index_object_t::test<6>()
	{
	}
	template<> template<>
	void mime_index_object_t::test<7>()
	{
	}
	template<> template<>
	void mime_index_object_t::test<8>()
	{
	}
	template<> template<>
	void mime_index_object_t::test<>()
	{
	}
*/
}


namespace tut
{
	struct mime_parse
	{
	};
	typedef test_group<mime_parse> mime_parse_t;
	typedef mime_parse_t::object mime_parse_object_t;
	tut::mime_parse_t tut_mime_parse("mime_parse");

	template<> template<>
	void mime_parse_object_t::test<1>()
	{
		// parse one mime object
		const std::string SERIALIZED_MIME("Content-Length: 200\r\nContent-Type: text/plain\r\n\r\naaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccc\r\n");
		std::stringstream istr;
		istr.str(SERIALIZED_MIME);
		LLMimeIndex mime;
		LLMimeParser parser;
		bool ok = parser.parseIndex(istr, mime);
		ensure("Parse successful.", ok);
		ensure_equals("content type", mime.contentType(), "text/plain");
		ensure_equals("content length", mime.contentLength(), 200);
		ensure_equals("offset", mime.offset(), 49);
 	}

	template<> template<>
	void mime_parse_object_t::test<2>()
	{
		// make sure we only parse one.
		const std::string SERIALIZED_MIME("Content-Length: 200\r\nContent-Type: text/plain\r\n\r\naaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccc\r\n\r\nContent-Length: 200\r\nContent-Type: text/plain\r\n\r\naaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccc\r\n\r\n");
		std::stringstream istr;
		istr.str(SERIALIZED_MIME);
		LLMimeIndex mime;
		LLMimeParser parser;
		bool ok = parser.parseIndex(istr, mime);
		ensure("Parse successful.", ok);
		ensure("not multipart.", !mime.isMultipart());
		ensure_equals("content type", mime.contentType(), "text/plain");
		ensure_equals("content length", mime.contentLength(), 200);
		ensure_equals("offset", mime.offset(), 49);
	}

	template<> template<>
	void mime_parse_object_t::test<3>()
	{
		// test multi-part and lack of content length for some of it.
		/*
Content-Type: multipart/mixed; boundary="segment"rnContent-Length: 148rnrn--segmentrnContent-Type: text/plainrnrnsome datarnrn--segmentrnContent-Type: text/xml; charset=UTF-8rnContent-Length: 22rnrn<llsd><undef /></llsd>rnrn
		 */
		const std::string SERIALIZED_MIME("Content-Type: multipart/mixed; boundary=\"segment\"\r\nContent-Length: 150\r\n\r\n--segment\r\nContent-Type: text/plain\r\n\r\nsome data\r\n\r\n--segment\r\nContent-Type: text/xml; charset=UTF-8\r\nContent-Length: 22\r\n\r\n<llsd><undef /></llsd>\r\n\r\n");
		std::stringstream istr;
		istr.str(SERIALIZED_MIME);
		LLMimeIndex mime;
		LLMimeParser parser;
		bool ok = parser.parseIndex(istr, mime);
		ensure("Parse successful.", ok);
		ensure("is multipart.", mime.isMultipart());
		ensure_equals("sub-part count", mime.subPartCount(), 2);
		ensure_equals("content length", mime.contentLength(), 150);
		ensure_equals("data offset for multipart", mime.offset(), 74);

		LLMimeIndex mime_plain(mime.subPart(0));
		ensure_equals(
			"first part type",
			mime_plain.contentType(),
			"text/plain");
		ensure_equals(
			"first part content length not known.",
			mime_plain.contentLength(),
			-1);
		ensure_equals("first part offset", mime_plain.offset(), 113);

		LLMimeIndex mime_xml(mime.subPart(1));
		ensure_equals(
			"second part type",
			mime_xml.contentType(),
			"text/xml; charset=UTF-8");
		ensure_equals(
			"second part content length",
			mime_xml.contentLength(),
			22);
		ensure_equals("second part offset", mime_xml.offset(), 198);
	}

	template<> template<>
	void mime_parse_object_t::test<4>()
	{
		// test multi-part, unquoted separator, and premature eof conditions
		/*
Content-Type: multipart/mixed; boundary=segmentrnContent-Length: 220rnrn--segmentrnContent-Type: text/plainrnContent-Length: 55rnrnhow are you today?rnI do not know. I guess I am:n'fine'rnrn--segmentrnContent-Type: text/xml; charset=UTF-8rnContent-Length: 22rnrn<llsd><undef /></llsd>rnrn		 */
		const std::string SERIALIZED_MIME("Content-Type: multipart/mixed; boundary=segment\r\nContent-Length: 220\r\n\r\n--segment\r\nContent-Type: text/plain\r\nContent-Length: 55\r\n\r\nhow are you today?\r\nI do not know. I guess I am:\n'fine'\r\n\r\n--segment\r\nContent-Type: text/xml; charset=UTF-8\r\nContent-Length: 22\r\n\r\n<llsd><undef /></llsd>\r\n\r\n");
		std::stringstream istr;
		istr.str(SERIALIZED_MIME);
		LLMimeIndex mime;
		LLMimeParser parser;
		bool ok = parser.parseIndex(istr, mime);
		ensure("Parse successful.", ok);
		ensure("is multipart.", mime.isMultipart());
		ensure_equals("sub-part count", mime.subPartCount(), 2);
		ensure_equals("content length", mime.contentLength(), 220);
		ensure_equals("data offset for multipart", mime.offset(), 72);

		LLMimeIndex mime_plain(mime.subPart(0));
		ensure_equals(
			"first part type",
			mime_plain.contentType(),
			"text/plain");
		ensure_equals(
			"first part content length",
			mime_plain.contentLength(),
			55);
		ensure_equals("first part offset", mime_plain.offset(), 131);

		LLMimeIndex mime_xml(mime.subPart(1));
		ensure_equals(
			"second part type",
			mime_xml.contentType(),
			"text/xml; charset=UTF-8");
		ensure_equals(
			"second part content length",
			mime_xml.contentLength(),
			22);
		ensure_equals("second part offset", mime_xml.offset(), 262);
	}

	template<> template<>
	void mime_parse_object_t::test<5>()
	{
		// test multi-part with multiple params
		const std::string SERIALIZED_MIME("Content-Type: multipart/mixed; boundary=segment; comment=\"testing multiple params.\"\r\nContent-Length: 220\r\n\r\n--segment\r\nContent-Type: text/plain\r\nContent-Length: 55\r\n\r\nhow are you today?\r\nI do not know. I guess I am:\n'fine'\r\n\r\n--segment\r\nContent-Type: text/xml; charset=UTF-8\r\nContent-Length: 22\r\n\r\n<llsd><undef /></llsd>\r\n\r\n");
		std::stringstream istr;
		istr.str(SERIALIZED_MIME);
		LLMimeIndex mime;
		LLMimeParser parser;
		bool ok = parser.parseIndex(istr, mime);
		ensure("Parse successful.", ok);
		ensure("is multipart.", mime.isMultipart());
		ensure_equals("sub-part count", mime.subPartCount(), 2);
		ensure_equals("content length", mime.contentLength(), 220);

		LLMimeIndex mime_plain(mime.subPart(0));
		ensure_equals(
			"first part type",
			mime_plain.contentType(),
			"text/plain");
		ensure_equals(
			"first part content length",
			mime_plain.contentLength(),
			55);

		LLMimeIndex mime_xml(mime.subPart(1));
		ensure_equals(
			"second part type",
			mime_xml.contentType(),
			"text/xml; charset=UTF-8");
		ensure_equals(
			"second part content length",
			mime_xml.contentLength(),
			22);
	}

	template<> template<>
	void mime_parse_object_t::test<6>()
	{
		// test multi-part with no specified boundary and eof
/*
Content-Type: multipart/relatedrnContent-Length: 220rnrn--rnContent-Type: text/plainrnContent-Length: 55rnrnhow are you today?rnI do not know. I guess I am:n'fine'rnrn--rnContent-Type: text/xml; charset=UTF-8rnContent-Length: 22rnrn<llsd><undef /></llsd>rnrn
*/
		const std::string SERIALIZED_MIME("Content-Type: multipart/related\r\nContent-Length: 500\r\n\r\n--\r\nContent-Type: text/plain\r\nContent-Length: 55\r\n\r\nhow are you today?\r\nI do not know. I guess I am:\n'fine'\r\n\r\n--\r\nContent-Type: text/xml; charset=UTF-8\r\nContent-Length: 22\r\n\r\n<llsd><undef /></llsd>\r\n\r\n");
		std::stringstream istr;
		istr.str(SERIALIZED_MIME);
		LLMimeIndex mime;
		LLMimeParser parser;
		bool ok = parser.parseIndex(istr, mime);
		ensure("Parse successful.", ok);
		ensure("is multipart.", mime.isMultipart());
		ensure_equals("sub-part count", mime.subPartCount(), 2);
		ensure_equals("content length", mime.contentLength(), 500);
		ensure_equals("data offset for multipart", mime.offset(), 56);

		LLMimeIndex mime_plain(mime.subPart(0));
		ensure_equals(
			"first part type",
			mime_plain.contentType(),
			"text/plain");
		ensure_equals(
			"first part content length",
			mime_plain.contentLength(),
			55);
		ensure_equals("first part offset", mime_plain.offset(), 108);

		LLMimeIndex mime_xml(mime.subPart(1));
		ensure_equals(
			"second part type",
			mime_xml.contentType(),
			"text/xml; charset=UTF-8");
		ensure_equals(
			"second part content length",
			mime_xml.contentLength(),
			22);
		ensure_equals("second part offset", mime_xml.offset(), 232);
	}

/*
	template<> template<>
	void mime_parse_object_t::test<>()
	{
	}
	template<> template<>
	void mime_parse_object_t::test<>()
	{
	}
	template<> template<>
	void mime_parse_object_t::test<>()
	{
	}
	template<> template<>
	void mime_parse_object_t::test<>()
	{
	}
*/
}
