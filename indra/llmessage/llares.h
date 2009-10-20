/** 
 * @file llares.h
 * @author Bryan O'Sullivan
 * @date 2007-08-15
 * @brief Wrapper for asynchronous DNS lookups.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLARES_H
#define LL_LLARES_H

#ifdef LL_WINDOWS
// ares.h is broken on windows in that it depends on types defined in ws2tcpip.h
// we need to include them first to work around it, but the headers issue warnings
# pragma warning(push)
# pragma warning(disable:4996)
# include <winsock2.h>
# include <ws2tcpip.h>
# pragma warning(pop)
#endif

#ifdef LL_STANDALONE
# include <ares.h>
#else
# include <ares/ares.h>
#endif

#include "llpointer.h"
#include "llrefcount.h"
#include "lluri.h"

#include <boost/shared_ptr.hpp>

class LLQueryResponder;
class LLAresListener;

/**
 * @brief Supported DNS RR types.
 */
enum LLResType
{
	RES_INVALID = 0,			/**< Cookie. */
	RES_A = 1,					/**< "A" record. IPv4 address. */
	RES_NS = 2,					/**< "NS" record. Authoritative server. */
	RES_CNAME = 5,				/**< "CNAME" record. Canonical name. */
	RES_PTR = 12,				/**< "PTR" record. Domain name pointer. */
	RES_AAAA = 28,				/**< "AAAA" record. IPv6 Address. */
	RES_SRV = 33,				/**< "SRV" record. Server Selection. */
	RES_MAX = 65536				/**< Sentinel; RR types are 16 bits wide. */
};

/**
 * @class LLDnsRecord
 * @brief Base class for all DNS RR types.
 */
class LLDnsRecord : public LLRefCount
{
protected:
	friend class LLQueryResponder;

	LLResType mType;
	std::string mName;
	unsigned mTTL;
	
	virtual int parse(const char *buf, size_t len, const char *pos,
					  size_t rrlen) = 0;

	LLDnsRecord(LLResType type, const std::string &name, unsigned ttl);
	
public:
	/**
	 * @brief Record name.
	 */
	const std::string &name() const { return mName; }

	/**
	 * @brief Time-to-live value, in seconds.
	 */
	unsigned ttl() const { return mTTL; }

	/**
	 * @brief RR type.
	 */
	LLResType type() const { return mType; }
};

/**
 * @class LLAddrRecord
 * @brief Base class for address-related RRs.
 */
class LLAddrRecord : public LLDnsRecord
{
protected:
	friend class LLQueryResponder;

	LLAddrRecord(LLResType type, const std::string &name, unsigned ttl);

	union 
	{
		sockaddr sa;
		sockaddr_in sin;
		sockaddr_in6 sin6;
	} mSA;

	socklen_t mSize;

public:
	/**
	 * @brief Generic socket address.
	 */
	const sockaddr &addr() const { return mSA.sa; }

	/**
	 * @brief Size of the socket structure.
	 */
	socklen_t size() const { return mSize; }
};

/**
 * @class LLARecord
 * @brief A RR, for IPv4 addresses.
 */
class LLARecord : public LLAddrRecord
{
protected:
	friend class LLQueryResponder;

	LLARecord(const std::string &name, unsigned ttl);

	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);

public:
	/**
	 * @brief Socket address.
	 */
	const sockaddr_in &addr_in() const { return mSA.sin; }
};

/**
 * @class LLAaaaRecord
 * @brief AAAA RR, for IPv6 addresses.
 */
class LLAaaaRecord : public LLAddrRecord
{
protected:
	friend class LLQueryResponder;

	LLAaaaRecord(const std::string &name, unsigned ttl);

	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);

public:
	/**
	 * @brief Socket address.
	 */
	const sockaddr_in6 &addr_in6() const { return mSA.sin6; }
};
	
/**
 * @class LLHostRecord
 * @brief Base class for host-related RRs.
 */
class LLHostRecord : public LLDnsRecord
{
protected:
	LLHostRecord(LLResType type, const std::string &name, unsigned ttl);

	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);

	std::string mHost;

public:
	/**
	 * @brief Host name.
	 */
	const std::string &host() const { return mHost; }
};
	
/**
 * @class LLCnameRecord
 * @brief CNAME RR.
 */
class LLCnameRecord : public LLHostRecord
{
protected:
	friend class LLQueryResponder;

	LLCnameRecord(const std::string &name, unsigned ttl);
};
	
/**
 * @class LLPtrRecord
 * @brief PTR RR.
 */
class LLPtrRecord : public LLHostRecord
{
protected:
	friend class LLQueryResponder;

	LLPtrRecord(const std::string &name, unsigned ttl);
};

/**
 * @class LLSrvRecord
 * @brief SRV RR.
 */
class LLSrvRecord : public LLHostRecord
{
protected:
	U16 mPriority;
	U16 mWeight;
	U16 mPort;

	int parse(const char *buf, size_t len, const char *pos, size_t rrlen);

public:
	LLSrvRecord(const std::string &name, unsigned ttl);

	/**
	 * @brief Service priority.
	 */
	U16 priority() const { return mPriority; }

	/**
	 * @brief Service weight.
	 */
	U16 weight() const { return mWeight; }

	/**
	 * @brief Port number of service.
	 */
	U16 port() const { return mPort; }

	/**
	 * @brief Functor for sorting SRV records by priority.
	 */
	struct ComparePriorityLowest
	{
		bool operator()(const LLSrvRecord& lhs, const LLSrvRecord& rhs)
		{
			return lhs.mPriority < rhs.mPriority;
		}
	};
};
	
/**
 * @class LLNsRecord
 * @brief NS RR.
 */
class LLNsRecord : public LLHostRecord
{
public:
	LLNsRecord(const std::string &name, unsigned ttl);
};

class LLQueryResponder;

/**
 * @class LLAres
 * @brief Asynchronous address resolver.
 */
class LLAres
{
public:
    /**
	 * @class HostResponder
	 * @brief Base class for responding to hostname lookups.
	 * @see LLAres::getHostByName
	 */
	class HostResponder : public LLRefCount
	{
	public:
		virtual ~HostResponder();

		virtual void hostResult(const hostent *ent);
		virtual void hostError(int code);
	};

    /**
	 * @class NameInfoResponder
	 * @brief Base class for responding to address lookups.
	 * @see LLAres::getNameInfo
	 */
	class NameInfoResponder : public LLRefCount
	{
	public:
		virtual ~NameInfoResponder();

		virtual void nameInfoResult(const char *node, const char *service);
		virtual void nameInfoError(int code);
	};

    /**
	 * @class QueryResponder
	 * @brief Base class for responding to custom searches.
	 * @see LLAres::search
	 */
	class QueryResponder : public LLRefCount
	{
	public:
		virtual ~QueryResponder();

		virtual void queryResult(const char *buf, size_t len);
		virtual void queryError(int code);
	};

	class SrvResponder;
	class UriRewriteResponder;
		
	LLAres();

	~LLAres();

	/**
	 * Cancel all outstanding requests.  The error methods of the
	 * corresponding responders will be called, with ARES_ETIMEOUT.
	 */
	void cancel();
	
	/**
	 * Look up the address of a host.
	 *
	 * @param name name of host to look up
	 * @param resp responder to call with result
	 * @param family AF_INET for IPv4 addresses, AF_INET6 for IPv6
	 */
	void getHostByName(const std::string &name, HostResponder *resp,
					   int family = AF_INET) {
		getHostByName(name.c_str(), resp, family);
	}

	/**
	 * Look up the address of a host.
	 *
	 * @param name name of host to look up
	 * @param resp responder to call with result
	 * @param family AF_INET for IPv4 addresses, AF_INET6 for IPv6
	 */
	void getHostByName(const char *name, HostResponder *resp,
					   int family = PF_INET);

	/**
	 * Look up the name associated with a socket address.
	 *
	 * @param sa socket address to look up
	 * @param salen size of socket address
	 * @param flags flags to use
	 * @param resp responder to call with result
	 */
	void getNameInfo(const struct sockaddr &sa, socklen_t salen, int flags,
					 NameInfoResponder *resp);

	/**
	 * Look up SRV (service location) records for a service name.
	 *
	 * @param name service name (e.g. "_https._tcp.login.agni.lindenlab.com")
	 * @param resp responder to call with result
	 */
	void getSrvRecords(const std::string &name, SrvResponder *resp);

	/**
	 * Rewrite a URI, using SRV (service location) records for its
	 * protocol if available.  If no SRV records are published, the
	 * existing URI is handed to the responder.
	 *
	 * @param uri URI to rewrite
	 * @param resp responder to call with result
	 */
	void rewriteURI(const std::string &uri,
					UriRewriteResponder *resp);

	/**
	 * Start a custom search.
	 *
	 * @param query query to make
	 * @param type type of query to perform
	 * @param resp responder to call with result
	 */
	void search(const std::string &query, LLResType type,
				QueryResponder *resp);

	/**
	 * Process any outstanding queries.  This method takes an optional
	 * timeout parameter (specified in microseconds).  If provided, it
	 * will block the calling thread for that length of time to await
	 * possible responses. A timeout of zero will return immediately
	 * if there are no responses or timeouts to process.
	 *
	 * @param timeoutUsecs number of microseconds to block before timing out
	 * @return whether any responses were processed
	 */
	bool process(U64 timeoutUsecs = 0);

	/**
	 * Process all outstanding queries, blocking the calling thread
	 * until all have either been responded to or timed out.
	 *
	 * @return whether any responses were processed
	 */
	bool processAll();
	
	/**
	 * Expand a DNS-encoded compressed string into a normal string.
	 *
	 * @param encoded the encoded name (null-terminated)
	 * @param abuf the response buffer in which the string is embedded
	 * @param alen the length of the response buffer
	 * @param s the string into which to place the result
	 * @return ARES_SUCCESS on success, otherwise an error indicator
	 */
	static int expandName(const char *encoded, const char *abuf, size_t alen,
						  std::string &s) {
		size_t ignore;
		return expandName(encoded, abuf, alen, s, ignore);
	}
	
	static int expandName(const char *encoded, const char *abuf, size_t alen,
						  std::string &s, size_t &enclen);

	/**
	 * Return a string describing an error code.
	 */
	static const char *strerror(int code);

	bool isInitialized(void) { return mInitSuccess; }

protected:
	ares_channel chan_;
	bool mInitSuccess;
    // boost::scoped_ptr would actually fit the requirement better, but it
    // can't handle incomplete types as boost::shared_ptr can.
    boost::shared_ptr<LLAresListener> mListener;
};
	
/**
 * An ordered collection of DNS resource records.
 */
typedef std::vector<LLPointer<LLDnsRecord> > dns_rrs_t;

/**
 * @class LLQueryResponder
 * @brief Base class for friendly handling of DNS query responses.
 *
 * This class parses a DNS response and represents it in a friendly
 * manner.
 *
 * @see LLDnsRecord
 * @see LLARecord
 * @see LLNsRecord
 * @see LLCnameRecord
 * @see LLPtrRecord
 * @see LLAaaaRecord
 * @see LLSrvRecord
 */
class LLQueryResponder : public LLAres::QueryResponder
{
protected:
	int mResult;
	std::string mQuery;
	LLResType mType;
	
	dns_rrs_t mAnswers;
	dns_rrs_t mAuthorities;
	dns_rrs_t mAdditional;

	/**
	 * Parse a single RR.
	 */
	int parseRR(const char *buf, size_t len, const char *&pos,
				LLPointer<LLDnsRecord> &r);
	/**
	 * Parse one section of a response.
	 */
	int parseSection(const char *buf, size_t len,
					 size_t count, const char *& pos, dns_rrs_t &rrs);

	void queryResult(const char *buf, size_t len);
	virtual void querySuccess();

public:
	LLQueryResponder();

	/**
	 * Indicate whether the response could be parsed successfully.
	 */
	bool valid() const { return mResult == ARES_SUCCESS; }

	/**
	 * The more detailed result of parsing the response.
	 */
	int result() const { return mResult; }
	
	/**
	 * Return the query embedded in the response.
	 */
	const std::string &query() const { return mQuery; }

	/**
	 * Return the contents of the "answers" section of the response.
	 */
	const dns_rrs_t &answers() const { return mAnswers; }

	/**
	 * Return the contents of the "authorities" section of the
	 * response.
	 */
	const dns_rrs_t &authorities() const { return mAuthorities; }

	/**
	 * Return the contents of the "additional records" section of the
	 * response.
	 */
	const dns_rrs_t &additional() const { return mAdditional; }
};

/**
 * @class LLAres::SrvResponder
 * @brief Class for handling SRV query responses.
 */
class LLAres::SrvResponder : public LLQueryResponder
{
public:
	friend void LLAres::getSrvRecords(const std::string &name,
									  SrvResponder *resp);
	void querySuccess();
	void queryError(int code);

	virtual void srvResult(const dns_rrs_t &ents);
	virtual void srvError(int code);
};

/**
 * @class LLAres::UriRewriteResponder
 * @brief Class for handling URI rewrites based on SRV records.
 */
class LLAres::UriRewriteResponder : public LLQueryResponder
{
protected:
	LLURI mUri;

public:
	friend void LLAres::rewriteURI(const std::string &uri,
								   UriRewriteResponder *resp);
	void querySuccess();
	void queryError(int code);

	virtual void rewriteResult(const std::vector<std::string> &uris);
};
	
/**
 * Singleton responder.
 */
extern LLAres *gAres;

/**
 * Set up the singleton responder.  It's safe to call this more than
 * once from within a single thread, but this function is not
 * thread safe.
 */
extern LLAres *ll_init_ares();

#endif // LL_LLARES_H
