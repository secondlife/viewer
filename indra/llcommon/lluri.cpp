/** 
 * @file lluri.cpp
 * @author Phoenix
 * @date 2006-02-08
 * @brief Implementation of the LLURI class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lluri.h"
#include "llsd.h"

// uric = reserved | unreserved | escaped
// reserved = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ","
// unreserved = alphanum | mark
// mark = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
// escaped = "%" hex hex
static const char* ESCAPED_CHARACTERS[256] =
{
	"%00",		// 0
	"%01",		// 1
	"%02",		// 2
	"%03",		// 3
	"%04",		// 4
	"%05",		// 5
	"%06",		// 6
	"%07",		// 7
	"%08",		// 8
	"%09",		// 9
	"%0a",		// 10
	"%0b",		// 11
	"%0c",		// 12
	"%0d",		// 13
	"%0e",		// 14
	"%0f",		// 15
	"%10",		// 16
	"%11",		// 17
	"%12",		// 18
	"%13",		// 19
	"%14",		// 20
	"%15",		// 21
	"%16",		// 22
	"%17",		// 23
	"%18",		// 24
	"%19",		// 25
	"%1a",		// 26
	"%1b",		// 27
	"%1c",		// 28
	"%1d",		// 29
	"%1e",		// 30
	"%1f",		// 31
	"%20",		// 32
	"!",		// 33
	"%22",		// 34
	"%23",		// 35
	"$",		// 36
	"%25",		// 37
	"&",		// 38
	"'",		// 39
	"(",		// 40
	")",		// 41
	"*",		// 42
	"+",		// 43
	",",		// 44
	"-",		// 45
	".",		// 46
	"/",		// 47
	"0",		// 48
	"1",		// 49
	"2",		// 50
	"3",		// 51
	"4",		// 52
	"5",		// 53
	"6",		// 54
	"7",		// 55
	"8",		// 56
	"9",		// 57
	":",		// 58
	";",		// 59
	"%3c",		// 60
	"=",		// 61
	"%3e",		// 62
	"?",		// 63
	"@",		// 64
	"A",		// 65
	"B",		// 66
	"C",		// 67
	"D",		// 68
	"E",		// 69
	"F",		// 70
	"G",		// 71
	"H",		// 72
	"I",		// 73
	"J",		// 74
	"K",		// 75
	"L",		// 76
	"M",		// 77
	"N",		// 78
	"O",		// 79
	"P",		// 80
	"Q",		// 81
	"R",		// 82
	"S",		// 83
	"T",		// 84
	"U",		// 85
	"V",		// 86
	"W",		// 87
	"X",		// 88
	"Y",		// 89
	"Z",		// 90
	"%5b",		// 91
	"%5c",		// 92
	"%5d",		// 93
	"%5e",		// 94
	"_",		// 95
	"%60",		// 96
	"a",		// 97
	"b",		// 98
	"c",		// 99
	"d",		// 100
	"e",		// 101
	"f",		// 102
	"g",		// 103
	"h",		// 104
	"i",		// 105
	"j",		// 106
	"k",		// 107
	"l",		// 108
	"m",		// 109
	"n",		// 110
	"o",		// 111
	"p",		// 112
	"q",		// 113
	"r",		// 114
	"s",		// 115
	"t",		// 116
	"u",		// 117
	"v",		// 118
	"w",		// 119
	"x",		// 120
	"y",		// 121
	"z",		// 122
	"%7b",		// 123
	"%7c",		// 124
	"%7d",		// 125
	"~",		// 126
	"%7f",		// 127
	"%80",		// 128
	"%81",		// 129
	"%82",		// 130
	"%83",		// 131
	"%84",		// 132
	"%85",		// 133
	"%86",		// 134
	"%87",		// 135
	"%88",		// 136
	"%89",		// 137
	"%8a",		// 138
	"%8b",		// 139
	"%8c",		// 140
	"%8d",		// 141
	"%8e",		// 142
	"%8f",		// 143
	"%90",		// 144
	"%91",		// 145
	"%92",		// 146
	"%93",		// 147
	"%94",		// 148
	"%95",		// 149
	"%96",		// 150
	"%97",		// 151
	"%98",		// 152
	"%99",		// 153
	"%9a",		// 154
	"%9b",		// 155
	"%9c",		// 156
	"%9d",		// 157
	"%9e",		// 158
	"%9f",		// 159
	"%a0",		// 160
	"%a1",		// 161
	"%a2",		// 162
	"%a3",		// 163
	"%a4",		// 164
	"%a5",		// 165
	"%a6",		// 166
	"%a7",		// 167
	"%a8",		// 168
	"%a9",		// 169
	"%aa",		// 170
	"%ab",		// 171
	"%ac",		// 172
	"%ad",		// 173
	"%ae",		// 174
	"%af",		// 175
	"%b0",		// 176
	"%b1",		// 177
	"%b2",		// 178
	"%b3",		// 179
	"%b4",		// 180
	"%b5",		// 181
	"%b6",		// 182
	"%b7",		// 183
	"%b8",		// 184
	"%b9",		// 185
	"%ba",		// 186
	"%bb",		// 187
	"%bc",		// 188
	"%bd",		// 189
	"%be",		// 190
	"%bf",		// 191
	"%c0",		// 192
	"%c1",		// 193
	"%c2",		// 194
	"%c3",		// 195
	"%c4",		// 196
	"%c5",		// 197
	"%c6",		// 198
	"%c7",		// 199
	"%c8",		// 200
	"%c9",		// 201
	"%ca",		// 202
	"%cb",		// 203
	"%cc",		// 204
	"%cd",		// 205
	"%ce",		// 206
	"%cf",		// 207
	"%d0",		// 208
	"%d1",		// 209
	"%d2",		// 210
	"%d3",		// 211
	"%d4",		// 212
	"%d5",		// 213
	"%d6",		// 214
	"%d7",		// 215
	"%d8",		// 216
	"%d9",		// 217
	"%da",		// 218
	"%db",		// 219
	"%dc",		// 220
	"%dd",		// 221
	"%de",		// 222
	"%df",		// 223
	"%e0",		// 224
	"%e1",		// 225
	"%e2",		// 226
	"%e3",		// 227
	"%e4",		// 228
	"%e5",		// 229
	"%e6",		// 230
	"%e7",		// 231
	"%e8",		// 232
	"%e9",		// 233
	"%ea",		// 234
	"%eb",		// 235
	"%ec",		// 236
	"%ed",		// 237
	"%ee",		// 238
	"%ef",		// 239
	"%f0",		// 240
	"%f1",		// 241
	"%f2",		// 242
	"%f3",		// 243
	"%f4",		// 244
	"%f5",		// 245
	"%f6",		// 246
	"%f7",		// 247
	"%f8",		// 248
	"%f9",		// 249
	"%fa",		// 250
	"%fb",		// 251
	"%fc",		// 252
	"%fd",		// 253
	"%fe",		// 254
	"%ff"		// 255
};

LLURI::LLURI()
{
}

LLURI::LLURI(const std::string& escaped_str)
{
	std::string::size_type delim_pos, delim_pos2;
	delim_pos = escaped_str.find(':');
	std::string temp;
	if (delim_pos == std::string::npos)
	{
		mScheme = "";
		mEscapedOpaque = escaped_str;
	}
	else
	{
		mScheme = escaped_str.substr(0, delim_pos);
		mEscapedOpaque = escaped_str.substr(delim_pos+1);
	}

	if (mScheme == "http" || mScheme == "https" || mScheme == "ftp")
	{
		if (mEscapedOpaque.substr(0,2) != "//")
		{
			return;
		}
		
		delim_pos = mEscapedOpaque.find('/', 2);
		delim_pos2 = mEscapedOpaque.find('?', 2);
		// no path, no query
		if (delim_pos == std::string::npos &&
			delim_pos2 == std::string::npos)
		{
			mEscapedAuthority = mEscapedOpaque.substr(2);
			mEscapedPath = "";
		}
		// path exist, no query
		else if (delim_pos2 == std::string::npos)
		{
			mEscapedAuthority = mEscapedOpaque.substr(2,delim_pos-2);
			mEscapedPath = mEscapedOpaque.substr(delim_pos);
		}
		// no path, only query
		else if (delim_pos == std::string::npos ||
				 delim_pos2 < delim_pos)
		{
			mEscapedAuthority = mEscapedOpaque.substr(2,delim_pos2-2);
			// query part will be broken out later
			mEscapedPath = mEscapedOpaque.substr(delim_pos2);
		}
		// path and query
		else
		{
			mEscapedAuthority = mEscapedOpaque.substr(2,delim_pos-2);
			// query part will be broken out later
			mEscapedPath = mEscapedOpaque.substr(delim_pos);
		}
	}

	delim_pos = mEscapedPath.find('?');
	if (delim_pos != std::string::npos)
	{
		mEscapedQuery = mEscapedPath.substr(delim_pos+1);
		mEscapedPath = mEscapedPath.substr(0,delim_pos);
	}
}

LLURI::~LLURI()
{
}


LLURI LLURI::buildHTTP(const std::string& host_port,
					   const LLSD& path)
{
	LLURI result;
	result.mScheme = "HTTP";
	// TODO: deal with '/' '?' '#' in host_port
	result.mEscapedAuthority = "//" + escape(host_port);
	if (path.isArray())
	{
		// break out and escape each path component
		for (LLSD::array_const_iterator it = path.beginArray();
			 it != path.endArray();
			 ++it)
		{
			lldebugs << "PATH: inserting " << it->asString() << llendl;
			result.mEscapedPath += "/" + escape(it->asString());
		}
	}
	result.mEscapedOpaque = result.mEscapedAuthority +
		result.mEscapedPath;
	return result;
}

// static
LLURI LLURI::buildHTTP(const std::string& host_port,
					   const LLSD& path,
					   const LLSD& query)
{
	LLURI result = buildHTTP(host_port, path);
	// break out and escape each query component
	if (query.isMap())
	{
		for (LLSD::map_const_iterator it = query.beginMap();
			 it != query.endMap();
			 it++)
		{
			result.mEscapedQuery += escape(it->first) +
				(it->second.isUndefined() ? "" : "=" + it->second.asString()) +
				"&";
		}
		if (query.size() > 0)
		{
			result.mEscapedOpaque += "?" + result.mEscapedQuery;
		}
	}
	return result;
}

std::string LLURI::asString() const
{
	if (mScheme.empty())
	{
		return mEscapedOpaque;
	}
	else
	{
		return mScheme + ":" + mEscapedOpaque;
	}
}

std::string LLURI::scheme() const
{
	return mScheme;
}

std::string LLURI::opaque() const
{
	return unescape(mEscapedOpaque);
}

std::string LLURI::authority() const
{
	return unescape(mEscapedAuthority);
}


namespace {
	void findAuthorityParts(const std::string& authority,
							std::string& user,
							std::string& host,
							std::string& port)
	{
		std::string::size_type start_pos = authority.find('@');
		if (start_pos == std::string::npos)
		{
			user = "";
			start_pos = 0;
		}
		else
		{
			user = authority.substr(0, start_pos);
			start_pos += 1;
		}

		std::string::size_type end_pos = authority.find(':', start_pos);
		if (end_pos == std::string::npos)
		{
			host = authority.substr(start_pos);
			port = "";
		}
		else
		{
			host = authority.substr(start_pos, end_pos - start_pos);
			port = authority.substr(end_pos + 1);
		}
	}
}
	
std::string LLURI::hostName() const
{
	std::string user, host, port;
	findAuthorityParts(mEscapedAuthority, user, host, port);
	return unescape(host);
}

U16 LLURI::hostPort() const
{
	std::string user, host, port;
	findAuthorityParts(mEscapedAuthority, user, host, port);
	if (port.empty())
	{
		if (mScheme == "http")
			return 80;
		if (mScheme == "https")
			return 443;
		if (mScheme == "ftp")
			return 21;		
		return 0;
	}
	return atoi(port.c_str());
}	

std::string LLURI::path() const
{
	return unescape(mEscapedPath);
}

std::string LLURI::query() const
{
	return unescape(mEscapedQuery);
}

LLSD LLURI::queryMap() const
{
	return queryMap(mEscapedQuery);
}

// static
LLSD LLURI::queryMap(std::string escaped_query_string)
{
	lldebugs << "LLURI::queryMap query params: " << escaped_query_string << llendl;

	LLSD result = LLSD::emptyArray();
	while(!escaped_query_string.empty())
	{
		// get tuple first
		std::string tuple;
		std::string::size_type tuple_begin = escaped_query_string.find('&');
		if (tuple_begin != std::string::npos)
		{
			tuple = escaped_query_string.substr(0, tuple_begin);
			escaped_query_string = escaped_query_string.substr(tuple_begin+1);
		}
		else
		{
			tuple = escaped_query_string;
			escaped_query_string = "";
		}
		if (tuple.empty()) continue;

		// parse tuple
		std::string::size_type key_end = tuple.find('=');
		if (key_end != std::string::npos)
		{
			std::string key = unescape(tuple.substr(0,key_end));
			std::string value = unescape(tuple.substr(key_end+1));
			lldebugs << "inserting key " << key << " value " << value << llendl;
			result[key] = value;
		}
		else
		{
			lldebugs << "inserting key " << unescape(tuple) << " value true" << llendl;
		    result[unescape(tuple)] = true;
		}
	}
	return result;
}

// static
std::string LLURI::escape(const std::string& str)
{
	std::ostringstream ostr;
	std::string::const_iterator it = str.begin();
	std::string::const_iterator end = str.end();
	S32 c;
	for(; it != end; ++it)
	{
		c = (S32)(*it);
		ostr << ESCAPED_CHARACTERS[c];
	}
	return ostr.str();
}

// static
std::string LLURI::unescape(const std::string& str)
{
	std::ostringstream ostr;
	std::string::const_iterator it = str.begin();
	std::string::const_iterator end = str.end();
	for(; it != end; ++it)
	{
		if((*it) == '%')
		{
			++it;
			if(it == end) break;
			U8 c = hex_as_nybble(*it++);
			c = c << 4;
			if (it == end) break;
			c |= hex_as_nybble(*it);
			ostr.put((char)c);
		}
		else
		{
			ostr.put(*it);
		}
	}
	return ostr.str();
}
