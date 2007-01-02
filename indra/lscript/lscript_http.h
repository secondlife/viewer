/** 
 * @file lscript_http.h
 * @brief LSL HTTP keys
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Keys used in LSL HTTP function <key,value> pair lists.

#ifndef LSCRIPT_HTTP_H
#define LSCRIPT_HTTP_H

enum LLScriptHTTPRequestParameterKey
{
  HTTP_METHOD,
  HTTP_MIMETYPE,
  HTTP_BODY_MAXLENGTH,
  HTTP_VERIFY_CERT
};

enum LLScriptHTTPResponseMetadataKey
{
	HTTP_BODY_TRUNCATED
};

#endif
