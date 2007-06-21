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

vector<LLSRVRecord> LLSRV::query(const string& queryName)
{
	unsigned char response[16384];
	vector<LLSRVRecord> recs;
	char name[1024];
	int len;
	
	len = res_query(queryName.c_str(), ns_c_in, ns_t_srv, response,
					sizeof(response));

	if (len == -1)
	{
		llinfos << "Query failed for " << queryName << llendl;
		return recs;
	}
	else if (len > (int) sizeof(response))
	{
		llinfos << "Response too big for " << queryName
				<< " (capacity " << sizeof(response)
				<< ", response " << len << ")" << llendl;
		return recs;
	}

    // We "should" be using libresolv's ns_initparse and ns_parserr to
    // parse the result of our query.  However, libresolv isn't
    // packaged correctly on Linux (as of BIND 9), so neither of these
    // functions is available without statically linking against
    // libresolv.  Ugh!  So we parse the response ourselves.

	const unsigned char *pos = response + sizeof(HEADER);
	const unsigned char *end = response + len;
	const HEADER *hdr = (const HEADER *) response;

	// Skip over the query embedded in the response.

	for (int q = ntohs(hdr->qdcount); q > 0; --q)
	{
		len = dn_expand(response, end, pos, name, sizeof(name));

		if (len == -1)
		{
			llinfos << "Could not expand queried name in RR response" << llendl;
			return recs;
		}
		
		pos += len + NS_QFIXEDSZ;
	}
	
	for (int a = ntohs(hdr->ancount); a > 0; --a)
	{
		static const ns_rr *rr;

		len = dn_expand(response, end, pos, name, sizeof(name) - 1);
		if (len == -1)
		{
			llinfos << "Could not expand response name in RR response" << llendl;
			return recs;
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
			return recs;
		}

		recs.push_back(LLSRVRecord(prio, weight, name, port));
	}

	// There are likely to be more records in the response, but we
	// don't care about those, at least for now.

	return recs;
}

#endif // LL_WINDOWS

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
