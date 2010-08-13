/** 
 * @file llares.cpp
 * @author Bryan O'Sullivan
 * @date 2007-08-15
 * @brief Wrapper for asynchronous DNS lookups.
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

#include "linden_common.h"
#include "llares.h"

#include <ares_dns.h>
#include <ares_version.h>

#include "apr_portable.h"
#include "apr_network_io.h"
#include "apr_poll.h"

#include "llapr.h"
#include "llareslistener.h"

#if defined(LL_WINDOWS)
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
# define ns_c_in 1
# define NS_HFIXEDSZ     12      /* #/bytes of fixed data in header */
# define NS_QFIXEDSZ     4       /* #/bytes of fixed data in query */
# define NS_RRFIXEDSZ    10      /* #/bytes of fixed data in r record */
#else
# include <arpa/nameser.h>
#endif

LLAres::HostResponder::~HostResponder()
{
}

void LLAres::HostResponder::hostResult(const hostent *ent)
{
	llinfos << "LLAres::HostResponder::hostResult not implemented" << llendl;
}

void LLAres::HostResponder::hostError(int code)
{
	llinfos << "LLAres::HostResponder::hostError " << code << ": "
			<< LLAres::strerror(code) << llendl;
}

LLAres::NameInfoResponder::~NameInfoResponder()
{
}

void LLAres::NameInfoResponder::nameInfoResult(const char *node,
											   const char *service)
{
	llinfos << "LLAres::NameInfoResponder::nameInfoResult not implemented"
			<< llendl;
}

void LLAres::NameInfoResponder::nameInfoError(int code)
{
	llinfos << "LLAres::NameInfoResponder::nameInfoError " << code << ": "
			<< LLAres::strerror(code) << llendl;
}

LLAres::QueryResponder::~QueryResponder()
{
}

void LLAres::QueryResponder::queryResult(const char *buf, size_t len)
{
	llinfos << "LLAres::QueryResponder::queryResult not implemented"
			<< llendl;
}

void LLAres::QueryResponder::queryError(int code)
{
	llinfos << "LLAres::QueryResponder::queryError " << code << ": "
			<< LLAres::strerror(code) << llendl;
}

LLAres::LLAres() :
    chan_(NULL),
    mInitSuccess(false),
    mListener(new LLAresListener(this))
{
	if (ares_library_init( ARES_LIB_INIT_ALL ) != ARES_SUCCESS ||
		ares_init(&chan_) != ARES_SUCCESS)
	{
		llwarns << "Could not succesfully initialize ares!" << llendl;
		return;
	}

	mInitSuccess = true;
}

LLAres::~LLAres()
{
	ares_destroy(chan_);
	ares_library_cleanup();
}

void LLAres::cancel()
{
	ares_cancel(chan_);
}

static void host_callback_1_5(void *arg, int status, int timeouts,
							  struct hostent *ent)
{
	LLPointer<LLAres::HostResponder> *resp =
		(LLPointer<LLAres::HostResponder> *) arg;

	if (status == ARES_SUCCESS)
	{
		(*resp)->hostResult(ent);
	} else {
		(*resp)->hostError(status);
	}

	delete resp;
}

#if ARES_VERSION_MAJOR == 1 && ARES_VERSION_MINOR == 4
static void host_callback(void *arg, int status, struct hostent *ent)
{
	host_callback_1_5(arg, status, 0, ent);
}
#else
# define host_callback host_callback_1_5
#endif

void LLAres::getHostByName(const char *name, HostResponder *resp,
						   int family)
{
	ares_gethostbyname(chan_, name, family, host_callback,
					   new LLPointer<LLAres::HostResponder>(resp));
}
	
void LLAres::getSrvRecords(const std::string &name, SrvResponder *resp)
{
	search(name, RES_SRV, resp);
}
	
void LLAres::rewriteURI(const std::string &uri, UriRewriteResponder *resp)
{
	llinfos << "Rewriting " << uri << llendl;

	resp->mUri = LLURI(uri);
	search("_" + resp->mUri.scheme() + "._tcp." + resp->mUri.hostName(),
		   RES_SRV, resp);
}

LLQueryResponder::LLQueryResponder()
	: LLAres::QueryResponder(),
	  mResult(ARES_ENODATA),
	  mType(RES_INVALID)
{
}

int LLQueryResponder::parseRR(const char *buf, size_t len, const char *&pos,
							  LLPointer<LLDnsRecord> &r)
{
	std::string rrname;
	size_t enclen;
	int ret;

	// RR name.

	ret = LLAres::expandName(pos, buf, len, rrname, enclen);
	if (ret != ARES_SUCCESS)
	{
		return ret;
	}
		
	pos += enclen;

	if (pos + NS_RRFIXEDSZ > buf + len)
	{
		return ARES_EBADRESP;
	}

	int rrtype = DNS_RR_TYPE(pos);
	int rrclass = DNS_RR_CLASS(pos);
	int rrttl = DNS_RR_TTL(pos);
	int rrlen = DNS_RR_LEN(pos);
		
	if (rrclass != ns_c_in)
	{
		return ARES_EBADRESP;
	}

	pos += NS_RRFIXEDSZ;

	if (pos + rrlen > buf + len)
	{
		return ARES_EBADRESP;
	}

	switch (rrtype)
	{
	case RES_A:
		r = new LLARecord(rrname, rrttl);
		break;
	case RES_NS:
		r = new LLNsRecord(rrname, rrttl);
		break;
	case RES_CNAME:
		r = new LLCnameRecord(rrname, rrttl);
		break;
	case RES_PTR:
		r = new LLPtrRecord(rrname, rrttl);
		break;
	case RES_AAAA:
		r = new LLAaaaRecord(rrname, rrttl);
		break;
	case RES_SRV:
		r = new LLSrvRecord(rrname, rrttl);
		break;
	default:
		llinfos << "LLQueryResponder::parseRR got unknown RR type " << rrtype
				<< llendl;
		return ARES_EBADRESP;
	}

	ret = r->parse(buf, len, pos, rrlen);

	if (ret == ARES_SUCCESS)
	{
		pos += rrlen;
	} else {
		r = NULL;
	}
		
	return ret;
}

int LLQueryResponder::parseSection(const char *buf, size_t len,
								   size_t count, const char *&pos,
								   dns_rrs_t &rrs)
{
	int ret = ARES_SUCCESS;
	
	for (size_t i = 0; i < count; i++)
	{
		LLPointer<LLDnsRecord> r;
		ret = parseRR(buf, len, pos, r);
		if (ret != ARES_SUCCESS)
		{
			break;
		}
		rrs.push_back(r);
	}

	return ret;
}

void LLQueryResponder::queryResult(const char *buf, size_t len)
{
	const char *pos = buf;
	int qdcount = DNS_HEADER_QDCOUNT(pos);
	int ancount = DNS_HEADER_ANCOUNT(pos);
	int nscount = DNS_HEADER_NSCOUNT(pos);
	int arcount = DNS_HEADER_ARCOUNT(pos);
	int ret;

	if (qdcount == 0 || ancount + nscount + arcount == 0)
	{
		ret = ARES_ENODATA;
		goto bail;
	}

	pos += NS_HFIXEDSZ;

	for (int i = 0; i < qdcount; i++)
	{
		std::string ignore;
		size_t enclen;

		ret = LLAres::expandName(pos, buf, len, i == 0 ? mQuery : ignore,
								 enclen);
		if (ret != ARES_SUCCESS)
		{
			goto bail;
		}

		pos += enclen;

		if (i == 0)
		{
			int t = DNS_QUESTION_TYPE(pos);
			switch (t)
			{
			case RES_A:
			case RES_NS:
			case RES_CNAME:
			case RES_PTR:
			case RES_AAAA:
			case RES_SRV:
				mType = (LLResType) t;
				break;
			default:
				llinfos << "Cannot grok query type " << t << llendl;
				ret = ARES_EBADQUERY;
				goto bail;
			}
		}

		pos += NS_QFIXEDSZ;
		if (pos > buf + len)
		{
			ret = ARES_EBADRESP;
			goto bail;
		}
	}
	
	ret = parseSection(buf, len, ancount, pos, mAnswers);
	if (ret != ARES_SUCCESS)
	{
		goto bail;
	}

	ret = parseSection(buf, len, nscount, pos, mAuthorities);
	if (ret != ARES_SUCCESS)
	{
		goto bail;
	}

	ret = parseSection(buf, len, arcount, pos, mAdditional);

bail:
	mResult = ret;
	if (mResult == ARES_SUCCESS)
	{
		querySuccess();
	} else {
		queryError(mResult);
	}
}

void LLQueryResponder::querySuccess()
{
	llinfos << "LLQueryResponder::queryResult not implemented" << llendl;
}

void LLAres::SrvResponder::querySuccess()
{
	if (mType == RES_SRV)
	{
		srvResult(mAnswers);
	} else {
		srvError(ARES_EBADRESP);
	}
}

void LLAres::SrvResponder::queryError(int code)
{
	srvError(code);
}

void LLAres::SrvResponder::srvResult(const dns_rrs_t &ents)
{
	llinfos << "LLAres::SrvResponder::srvResult not implemented" << llendl;

	for (size_t i = 0; i < ents.size(); i++)
	{
		const LLSrvRecord *s = (const LLSrvRecord *) ents[i].get();

		llinfos << "[" << i << "] " << s->host() << ":" << s->port()
				<< " priority " << s->priority()
				<< " weight " << s->weight()
				<< llendl;
	}
}

void LLAres::SrvResponder::srvError(int code)
{
	llinfos << "LLAres::SrvResponder::srvError " << code << ": "
			<< LLAres::strerror(code) << llendl;
}

static void nameinfo_callback_1_5(void *arg, int status, int timeouts,
								  char *node, char *service)
{
	LLPointer<LLAres::NameInfoResponder> *resp =
		(LLPointer<LLAres::NameInfoResponder> *) arg;

	if (status == ARES_SUCCESS)
	{
		(*resp)->nameInfoResult(node, service);
	} else {
		(*resp)->nameInfoError(status);
	}

	delete resp;
}

#if ARES_VERSION_MAJOR == 1 && ARES_VERSION_MINOR == 4
static void nameinfo_callback(void *arg, int status, char *node, char *service)
{
	nameinfo_callback_1_5(arg, status, 0, node, service);
}
#else
# define nameinfo_callback nameinfo_callback_1_5
#endif

void LLAres::getNameInfo(const struct sockaddr &sa, socklen_t salen, int flags,
						 NameInfoResponder *resp)
{
	ares_getnameinfo(chan_, &sa, salen, flags, nameinfo_callback,
					 new LLPointer<NameInfoResponder>(resp));
}

static void search_callback_1_5(void *arg, int status, int timeouts,
								unsigned char *abuf, int alen)
{
	LLPointer<LLAres::QueryResponder> *resp =
		(LLPointer<LLAres::QueryResponder> *) arg;

	if (status == ARES_SUCCESS)
	{
		(*resp)->queryResult((const char *) abuf, alen);
	} else {
		(*resp)->queryError(status);
	}

	delete resp;
}

#if ARES_VERSION_MAJOR == 1 && ARES_VERSION_MINOR == 4
static void search_callback(void *arg, int status, unsigned char *abuf,
							int alen)
{
	search_callback_1_5(arg, status, 0, abuf, alen);
}
#else
# define search_callback search_callback_1_5
#endif

void LLAres::search(const std::string &query, LLResType type,
					QueryResponder *resp)
{
	ares_search(chan_, query.c_str(), ns_c_in, type, search_callback,
				new LLPointer<QueryResponder>(resp));
}

bool LLAres::process(U64 timeout)
{
	if (!gAPRPoolp)
	{
		ll_init_apr();
	}

	ares_socket_t socks[ARES_GETSOCK_MAXNUM];
	apr_pollfd_t aprFds[ARES_GETSOCK_MAXNUM];
	apr_int32_t nsds = 0;	
	int nactive = 0;
	int bitmask;

	bitmask = ares_getsock(chan_, socks, ARES_GETSOCK_MAXNUM);

	if (bitmask == 0)
	{
		return nsds > 0;
	}

	apr_status_t status;
	LLAPRPool pool;
	status = pool.getStatus() ;
	ll_apr_assert_status(status);

	for (int i = 0; i < ARES_GETSOCK_MAXNUM; i++)
	{
		if (ARES_GETSOCK_READABLE(bitmask, i))
		{
			aprFds[nactive].reqevents = APR_POLLIN | APR_POLLERR;
		}
		else if (ARES_GETSOCK_WRITABLE(bitmask, i))
		{
			aprFds[nactive].reqevents = APR_POLLOUT | APR_POLLERR;
		} else {
			continue;
		}

		apr_socket_t *aprSock = NULL;

		status = apr_os_sock_put(&aprSock, (apr_os_sock_t *) &socks[i], pool.getAPRPool());
		if (status != APR_SUCCESS)
		{
			ll_apr_warn_status(status);
			return nsds > 0;
		}

		aprFds[nactive].desc.s = aprSock;
		aprFds[nactive].desc_type = APR_POLL_SOCKET;
		aprFds[nactive].p = pool.getAPRPool();
		aprFds[nactive].rtnevents = 0;
		aprFds[nactive].client_data = &socks[i];

		nactive++;
	}

	if (nactive > 0)
	{
		status = apr_poll(aprFds, nactive, &nsds, timeout);

		if (status != APR_SUCCESS && status != APR_TIMEUP)
		{
			ll_apr_warn_status(status);
		}

		for (int i = 0; i < nactive; i++)
		{
			int evts = aprFds[i].rtnevents;
			int ifd = (evts & (APR_POLLIN | APR_POLLERR))
				? *((int *) aprFds[i].client_data) : ARES_SOCKET_BAD;
			int ofd = (evts & (APR_POLLOUT | APR_POLLERR))
				? *((int *) aprFds[i].client_data) : ARES_SOCKET_BAD;
					
			ares_process_fd(chan_, ifd, ofd);
		}
	}

	return nsds > 0;
}

bool LLAres::processAll()
{
	bool anyProcessed = false, ret;

	do {
		timeval tv;

		ret = ares_timeout(chan_, NULL, &tv) != NULL;

		if (ret)
		{
			ret = process(tv.tv_sec * 1000000LL + tv.tv_usec);
			anyProcessed |= ret;
		}
	} while (ret);

	return anyProcessed;
}

int LLAres::expandName(const char *encoded, const char *abuf, size_t alen,
					   std::string &s, size_t &enclen)
{
	char *t;
	int ret;
	long e;
	
	ret = ares_expand_name((const unsigned char *) encoded,
						   (const unsigned char *) abuf, alen, &t, &e);
	if (ret == ARES_SUCCESS)
	{
		s.assign(t);
		enclen = e;
		ares_free_string(t);
	}
	return ret;
}

const char *LLAres::strerror(int code)
{
	return ares_strerror(code);
}

LLAres *gAres;

LLAres *ll_init_ares()
{
	if (gAres == NULL)
	{
		gAres = new LLAres();
	}
	return gAres;
}

LLDnsRecord::LLDnsRecord(LLResType type, const std::string &name,
						 unsigned ttl)
	: LLRefCount(),
	  mType(type),
	  mName(name),
	  mTTL(ttl)
{
}

LLHostRecord::LLHostRecord(LLResType type, const std::string &name,
						   unsigned ttl)
	: LLDnsRecord(type, name, ttl)
{
}

int LLHostRecord::parse(const char *buf, size_t len, const char *pos,
						size_t rrlen)
{
	int ret;

	ret = LLAres::expandName(pos, buf, len, mHost);
	if (ret != ARES_SUCCESS)
	{
		goto bail;
	}
	
	ret = ARES_SUCCESS;

bail:
	return ret;
}

LLCnameRecord::LLCnameRecord(const std::string &name, unsigned ttl)
	: LLHostRecord(RES_CNAME, name, ttl)
{
}

LLPtrRecord::LLPtrRecord(const std::string &name, unsigned ttl)
	: LLHostRecord(RES_PTR, name, ttl)
{
}

LLAddrRecord::LLAddrRecord(LLResType type, const std::string &name,
			   unsigned ttl)
	: LLDnsRecord(type, name, ttl),

	  mSize(0)
{
}

LLARecord::LLARecord(const std::string &name, unsigned ttl)
	: LLAddrRecord(RES_A, name, ttl)
{
}

int LLARecord::parse(const char *buf, size_t len, const char *pos,
					 size_t rrlen)
{
	int ret;

	if (rrlen != sizeof(mSA.sin.sin_addr.s_addr))
	{
		ret = ARES_EBADRESP;
		goto bail;
	}
	
	memset(&mSA, 0, sizeof(mSA));
	memcpy(&mSA.sin.sin_addr.s_addr, pos, rrlen);
	mSA.sin.sin_family = AF_INET6;
	mSize = sizeof(mSA.sin);
	
	ret = ARES_SUCCESS;

bail:
	return ret;
}

LLAaaaRecord::LLAaaaRecord(const std::string &name, unsigned ttl)
	: LLAddrRecord(RES_AAAA, name, ttl)
{
}

int LLAaaaRecord::parse(const char *buf, size_t len, const char *pos,
						size_t rrlen)
{
	int ret;

	if (rrlen != sizeof(mSA.sin6.sin6_addr))
	{
		ret = ARES_EBADRESP;
		goto bail;
	}
	
	memset(&mSA, 0, sizeof(mSA));
	memcpy(&mSA.sin6.sin6_addr.s6_addr, pos, rrlen);
	mSA.sin6.sin6_family = AF_INET6;
	mSize = sizeof(mSA.sin6);
	
	ret = ARES_SUCCESS;

bail:
	return ret;
}

LLSrvRecord::LLSrvRecord(const std::string &name, unsigned ttl)
	: LLHostRecord(RES_SRV, name, ttl),

	  mPriority(0),
	  mWeight(0),
	  mPort(0)
{
}

int LLSrvRecord::parse(const char *buf, size_t len, const char *pos,
					   size_t rrlen)
{
	int ret;

	if (rrlen < 6)
	{
		ret = ARES_EBADRESP;
		goto bail;
	}

	memcpy(&mPriority, pos, 2);
	memcpy(&mWeight, pos + 2, 2);
	memcpy(&mPort, pos + 4, 2);

	mPriority = ntohs(mPriority);
	mWeight = ntohs(mWeight);
	mPort = ntohs(mPort);

	ret = LLHostRecord::parse(buf, len, pos + 6, rrlen - 6);
	
bail:
	return ret;
}

LLNsRecord::LLNsRecord(const std::string &name, unsigned ttl)
	: LLHostRecord(RES_NS, name, ttl)
{
}

void LLAres::UriRewriteResponder::queryError(int code)
{
	std::vector<std::string> uris;
	uris.push_back(mUri.asString());
	rewriteResult(uris);
}

void LLAres::UriRewriteResponder::querySuccess()
{
	std::vector<std::string> uris;

	if (mType != RES_SRV)
	{
		goto bail;
	}
		
	for (size_t i = 0; i < mAnswers.size(); i++)
	{
		const LLSrvRecord *r = (const LLSrvRecord *) mAnswers[i].get();

		if (r->type() == RES_SRV)
		{
			// Check the domain in the response to ensure that it's
			// the same as the domain in the request, so that bad guys
			// can't forge responses that point to their own login
			// servers with their own certificates.

			// Hard-coding the domain to check here is a bit of a
			// hack.  Hoist it to an outer caller if anyone ever needs
			// this functionality on other domains.

			static const std::string domain(".lindenlab.com");
			const std::string &host = r->host();

			std::string::size_type s = host.find(domain) + domain.length();
				
			if (s != host.length() && s != host.length() - 1)
			{
				continue;
			}
			
			LLURI uri(mUri.scheme(),
					  mUri.userName(),
					  mUri.password(),
					  r->host(),
					  mUri.defaultPort() ? r->port() : mUri.hostPort(),
					  mUri.escapedPath(),
					  mUri.escapedQuery());
			uris.push_back(uri.asString());
		}
	}

	if (!uris.empty())
	{
		goto done;
	}

bail:
	uris.push_back(mUri.asString());

done:
	rewriteResult(uris);
}

void LLAres::UriRewriteResponder::rewriteResult(
	const std::vector<std::string> &uris)
{
	llinfos << "LLAres::UriRewriteResponder::rewriteResult not implemented"
			<< llendl;

	for (size_t i = 0; i < uris.size(); i++)
	{
		llinfos << "[" << i << "] " << uris[i] << llendl;
	}
}
