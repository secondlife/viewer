// DOCTEST_SKIP_AUTOGEN: hand-maintained doctest suite; see docs/testing/doctest_quickstart.md.
// ---------------------------------------------------------------------------
// Deterministic HttpHeaders coverage with no runtime network or file access.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"

#include "httpheaders.h"

#include "http_header_norm.h"

#include <vector>

using namespace LLCore;
using namespace llcorehttp_test;

namespace
{
HeaderList to_header_list(const HttpHeaders::container_t& container)
{
    HeaderList list;
    list.reserve(container.size());
    for (const auto& entry : container)
    {
        list.emplace_back(entry.first, entry.second);
    }
    return list;
}
} // namespace

TEST_SUITE("httpheaders_test")
{
    TEST_CASE("normalize helpers")
    {
        CHECK_EQ(normalize_header_name(" Content-Type "), "content-type");
        CHECK_EQ(normalize_header_name("ACCEPT"), "accept");

        CHECK_EQ(normalize_header_value("  text/html  "), "text/html");
        CHECK_EQ(normalize_header_value("text/html;   charset=UTF-8"),
                 "text/html; charset=UTF-8");

        std::string folded = "line\r\n\tcontinued\r\n more";
        CHECK_EQ(unfold_legacy_lines(folded), "line continued more");
        CHECK_EQ(normalize_header_value(folded), "line continued more");
    }

    TEST_CASE("appendNormal canonicalization")
    {
        HttpHeaders::ptr_t headers(new HttpHeaders());
        static char line1[] = " AcCePT : image/yourfacehere";
        static char line2[] = " next : \t\tlinejunk \t";
        static char line3[] = "FancY-PANTs::plop:-neuf-=vleem=";
        static char line4[] = "all-talk-no-walk:";
        static char line5[] = ":all-talk-no-walk";
        static char line6[] = "  :";
        static char line7[] = " \toskdgioasdghaosdghoowg28342908tg8902hg0hwedfhqew890v7qh0wdebv78q0wdevbhq>?M>BNM<ZV>?NZ? \t";
        static char line8[] = "binary:ignorestuffontheendofthis";

        headers->appendNormal(line1, sizeof(line1) - 1);
        headers->appendNormal(line2, sizeof(line2) - 1);
        headers->appendNormal(line3, sizeof(line3) - 1);
        headers->appendNormal(line4, sizeof(line4) - 1);
        headers->appendNormal(line5, sizeof(line5) - 1);
        headers->appendNormal(line6, sizeof(line6) - 1);
        headers->appendNormal(line7, sizeof(line7) - 1);
        headers->appendNormal(line8, 13); // apenas prefixo

        auto canonical = canonicalize_headers(to_header_list(headers->getContainerTESTONLY()));

        HeaderList expected = {
            {"accept", "image/yourfacehere"},
            {"next", "linejunk"},
            {"fancy-pants", ":plop:-neuf-=vleem="},
            {"all-talk-no-walk", ""},
            {"", "all-talk-no-walk"},
            {"", ""},
            {"oskdgioasdghaosdghoowg28342908tg8902hg0hwedfhqew890v7qh0wdebv78q0wdevbhq>?m>bnm<zv>?nz?", ""},
            {"binary", "ignore"}
        };

        CHECK_EQ(canonical.size(), expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i)
        {
            CAPTURE(i);
            LL_CHECK_EQ_STR(canonical[i].first, expected[i].first);
            LL_CHECK_EQ_STR(canonical[i].second, expected[i].second);
        }
    }

    TEST_CASE("duplicate merge policy")
    {
        HttpHeaders::ptr_t headers(new HttpHeaders());
        headers->append("Accept", "text/html");
        headers->append("accept", "application/json");
        headers->append("Set-Cookie", "a=1");
        headers->append("set-cookie", "b=2");
        headers->append("Cache-Control", "no-cache");
        headers->append("Cache-Control", "max-age=100");

        auto canonical = canonicalize_headers(to_header_list(headers->getContainerTESTONLY()));
        auto buckets = merge_duplicates(canonical);
        auto flattened = collapse_merged(buckets);

        HeaderList expected = {
            {"accept", "text/html, application/json"},
            {"set-cookie", "a=1"},
            {"set-cookie", "b=2"},
            {"cache-control", "no-cache, max-age=100"}
        };

        CHECK_EQ(flattened.size(), expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i)
        {
            CAPTURE(i);
            LL_CHECK_EQ_STR(flattened[i].first, expected[i].first);
            LL_CHECK_EQ_STR(flattened[i].second, expected[i].second);
        }
    }

    TEST_CASE("legacy folding handled via helper")
    {
        const std::string folded = "Subject: first line\r\n\tsecond line\r\n third line";
        const HeaderList raw = {{"Subject", folded}};
        auto canonical = canonicalize_headers(raw);
        CHECK_EQ(canonical.size(), 1U);
        LL_CHECK_EQ_STR(canonical[0].first, "subject");
        LL_CHECK_EQ_STR(canonical[0].second, "Subject: first line second line third line");
    }
}
