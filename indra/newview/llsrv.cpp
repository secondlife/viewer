/** 
 * @file llsrv.cpp
 * @brief Wrapper for DNS SRV record lookups
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llsrv.h"

using namespace std;

#if LL_WINDOWS

#undef UNICODE
#include <winsock2.h>
#include <windns.h>

vector<LLSRVRecord> LLSRV::query(const string& name)
{
	vector<LLSRVRecord> recs;
	DNS_RECORD *rec;
	DNS_STATUS status;

	status = DnsQuery(name.c_str(), DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &rec, NULL);
	if (!status)
	{
		for (DNS_RECORD *cur = rec; cur != NULL; cur = cur->pNext)
		{
			if (cur->wType != DNS_TYPE_SRV)
			{
				continue;
			}
			recs.push_back(LLSRVRecord(cur->Data.Srv.wPriority,
									   cur->Data.Srv.wWeight,
									   cur->Data.Srv.pNameTarget,
									   cur->Data.Srv.wPort));
		}
		DnsRecordListFree(rec, DnsFreeRecordListDeep);
	}

	return recs;
}

#else // !LL_WINDOWS

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>
#include <resolv.h>

#include <netdb.h>

#ifdef HOMEGROWN_RESPONSE_PARSER

// We ought to be using libresolv's ns_initparse and ns_parserr to
// parse the result of our query.  However, libresolv isn't packaged
// correctly on Linux (as of BIND 9), so neither of these functions is
// available without statically linking against libresolv.  Ugh!  This
// fallback function is available if we need to parse the response
// ourselves without relying too much on libresolv.  It is NOT THE
// DEFAULT.

vector<LLSRVRecord> LLSRV::parseResponse(const unsigned char *response,
										 int resp_len)
{
	vector<LLSRVRecord> recs;

	const unsigned char *pos = response + sizeof(HEADER);
	const unsigned char *end = response + resp_len;
	const HEADER *hdr = (const HEADER *) response;
	char name[1024];

	// Skip over the query embedded in the response.

	for (int q = ntohs(hdr->qdcount); q > 0; --q)
	{
		int len = dn_expand(response, end, pos, name, sizeof(name));

		if (len == -1)
		{
			llinfos << "Could not expand queried name in RR response"
					<< llendl;
			goto bail;
		}
		
		pos += len + NS_QFIXEDSZ;
	}
	
	for (int a = ntohs(hdr->ancount); a > 0; --a)
	{
		static const ns_rr *rr;

		int len = dn_expand(response, end, pos, name, sizeof(name) - 1);
		if (len == -1)
		{
			llinfos << "Could not expand response name in RR response"
					<< llendl;
			goto bail;
		}

		// Skip over the resource name and headers we don't care about.

		pos += len + sizeof(rr->type) + sizeof(rr->rr_class) +
			sizeof(rr->ttl) + sizeof(rr->rdlength);
		
		U16 prio;
		U16 weight;
		U16 port;

		NS_GET16(prio, pos);
		NS_GET16(weight, pos);
		NS_GET16(port, pos);
		
		len = dn_expand(response, end, pos, name, sizeof(name) - 1);

		if (len == -1)
		{
			llinfos << "Could not expand name in RR response" << llendl;
			goto bail;
		}

		recs.push_back(LLSRVRecord(prio, weight, name, port));
	}

	// There are likely to be more records in the response, but we
	// don't care about those, at least for now.
bail:
	return reorder(recs);
}

#else // HOMEGROWN_RESPONSE_PARSER

// This version of the response parser is the one to use if libresolv
// is available and behaving itself.

vector<LLSRVRecord> LLSRV::parseResponse(const unsigned char *response,
										 int resp_len)
{
	vector<LLSRVRecord> recs;
	ns_msg hdr;

	if (ns_initparse(response, resp_len, &hdr))
	{
		llinfos << "Could not parse response" << llendl;
		goto bail;
	}
	
	for (int i = 0; i < ns_msg_count(hdr, ns_s_an); i++)
	{
		ns_rr rr;
		
		if (ns_parserr(&hdr, ns_s_an, i, &rr))
		{
			llinfos << "Could not parse RR" << llendl;
			goto bail;
		}

		if (ns_rr_type(rr) != ns_t_srv)
		{
			continue;
		}

		const unsigned char *pos = ns_rr_rdata(rr);
		U16 prio, weight, port;
		char name[1024];
		int ret;
		
		NS_GET16(prio, pos);
		NS_GET16(weight, pos);
		NS_GET16(port, pos);
		
		ret = dn_expand(ns_msg_base(hdr), ns_msg_end(hdr), pos,
						name, sizeof(name));

		if (ret == -1)
		{
			llinfos << "Could not decompress name" << llendl;
			goto bail;
		}

		recs.push_back(LLSRVRecord(prio, weight, name, port));
	}
	
bail:
	return reorder(recs);
}

#endif // HOMEGROWN_RESPONSE_PARSER

vector<LLSRVRecord> LLSRV::query(const string& queryName)
{
	unsigned char response[16384];
	vector<LLSRVRecord> recs;
	int len;
	
	len = res_query(queryName.c_str(), ns_c_in, ns_t_srv, response,
					sizeof(response));

	if (len == -1)
	{
		llinfos << "Query failed for " << queryName << llendl;
		goto bail;
	}
	else if (len > (int) sizeof(response))
	{
		llinfos << "Response too big for " << queryName
				<< " (capacity " << sizeof(response)
				<< ", response " << len << ")" << llendl;
		goto bail;
	}

	recs = parseResponse(response, len);
bail:
	return reorder(recs);
}

#endif // LL_WINDOWS

// Implement the algorithm specified in RFC 2782 for dealing with RRs
// of differing priorities and weights.
vector<LLSRVRecord> LLSRV::reorder(vector<LLSRVRecord>& recs)
{
	typedef list<const LLSRVRecord *> reclist_t;
	typedef map<U16, reclist_t> bucket_t;
	vector<LLSRVRecord> newRecs;
	bucket_t buckets;

	// Don't rely on the DNS server to shuffle responses.
	
	random_shuffle(recs.begin(), recs.end());

	for (vector<LLSRVRecord>::const_iterator iter = recs.begin();
		 iter != recs.end(); ++iter)
	{
		buckets[iter->priority()].push_back(&*iter);
	}
	
	// Priorities take precedence over weights.

	for (bucket_t::iterator iter = buckets.begin();
		 iter != buckets.end(); ++iter)
	{
		reclist_t& myPrio = iter->second;
		reclist_t r;

		// RRs with weight zero go to the front of the intermediate
		// list, so they'll have little chance of being chosen.
		// Larger weights have a higher likelihood of selection.

		for (reclist_t::iterator i = myPrio.begin(); i != myPrio.end(); )
		{
			if ((*i)->weight() == 0)
			{
				r.push_back(*i);
				i = myPrio.erase(i);
			} else {
				++i;
			}
		}

		r.insert(r.end(), myPrio.begin(), myPrio.end());
		
		while (!r.empty())
		{
			U32 total = 0;

			for (reclist_t::const_iterator i = r.begin(); i != r.end(); ++i)
			{
				total += (*i)->weight();
			}

			U32 target = total > 1 ? (rand() % total) : 0;
			U32 partial = 0;
			
			for (reclist_t::iterator i = r.begin(); i != r.end(); )
			{
				partial += (*i)->weight();
				if (partial >= target)
				{
					newRecs.push_back(**i);
					i = r.erase(i);
				} else {
					++i;
				}
			}
		}
	}
	
	// Order RRs by lowest numeric priority.  The stable sort
	// preserves the weight choices we made above.

	stable_sort(newRecs.begin(), newRecs.end(),
				LLSRVRecord::ComparePriorityLowest());

	return newRecs;
}

vector<string> LLSRV::rewriteURI(const string& uriStr)
{
	LLURI uri(uriStr);
	const string& scheme = uri.scheme();
	llinfos << "Rewriting " << uriStr << llendl;
	string serviceName("_" + scheme + "._tcp." + uri.hostName());
	llinfos << "Querying for " << serviceName << llendl;
	vector<LLSRVRecord> srvs(LLSRV::query(serviceName));
	vector<string> rewritten;

	if (srvs.empty())
	{
		llinfos << "No query results; using " << uriStr << llendl;
		rewritten.push_back(uriStr);
	}
	else
	{
		vector<LLSRVRecord>::const_iterator iter;
		size_t maxSrvs = 3;
		size_t i;

		llinfos << "Got " << srvs.size() << " results" << llendl;
		for (iter = srvs.begin(); iter != srvs.end(); ++iter)
		{
			lldebugs << "host " << iter->target() << ':' << iter->port()
					 << " prio " << iter->priority()
					 << " weight " << iter->weight()
					 << llendl;
		}

		if (srvs.size() > maxSrvs)
		{
			llinfos << "Clamping to " << maxSrvs << llendl;
		}

		for (iter = srvs.begin(), i = 0;
			 iter != srvs.end() && i < maxSrvs; ++iter, ++i)
		{
			LLURI newUri(scheme,
						 uri.userName(),
						 uri.password(),
						 iter->target(),
						 uri.defaultPort() ? iter->port() : uri.hostPort(),
						 uri.escapedPath(),
						 uri.escapedQuery());
			string newUriStr(newUri.asString());

			llinfos << "Rewrite[" << i << "] " << newUriStr << llendl;

			rewritten.push_back(newUriStr);
		}
	}

	return rewritten;
}
