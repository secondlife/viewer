/** 
 * @file llsrv.h
 * @brief Wrapper for DNS SRV record lookups
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSRV_H
#define LL_LLSRV_H

class LLSRV;

class LLSRVRecord
{
	friend class LLSRV;

protected:
	U16 mPriority;
	U16 mWeight;
	std::string mTarget;
	U16 mPort;

	LLSRVRecord(U16 priority, U16 weight, const std::string& target,
				U16 port) :
		mPriority(priority),
		mWeight(weight),
		mTarget(target),
		mPort(port) {
	}
	
public:
	U16 priority() const { return mPriority; }
	U16 weight() const { return mWeight; }
	const std::string& target() const { return mTarget; }
	U16 port() const { return mPort; }
};
	
class LLSRV
{
public:
	static std::vector<LLSRVRecord> query(const std::string& name);
	static std::vector<std::string> rewriteURI(const std::string& uri);
};

#endif // LL_LLSRV_H
