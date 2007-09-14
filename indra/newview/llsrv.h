/** 
 * @file llsrv.h
 * @brief Wrapper for DNS SRV record lookups
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSRV_H
#define LL_LLSRV_H

class LLSRV
{
public:
	static std::vector<std::string> rewriteURI(const std::string& uri);
};

#endif // LL_LLSRV_H
