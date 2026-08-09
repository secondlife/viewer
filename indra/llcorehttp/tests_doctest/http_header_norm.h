/**
 * @file http_header_norm.h
 * @date   2025-02-18
 * @brief Helpers for deterministic HTTP header normalization tests (doctest only)
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace llcorehttp_test
{
using HeaderList = std::vector<std::pair<std::string, std::string>>;
using HeaderBuckets = std::vector<HeaderList>;

// Normalize header names to lower-case and trim surrounding whitespace.
inline std::string normalize_header_name(const std::string& input)
{
    std::string result = input;

    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    auto begin = std::find_if(result.begin(), result.end(), not_space);
    auto end = std::find_if(result.rbegin(), result.rend(), not_space).base();
    if (begin < end)
    {
        result.assign(begin, end);
    }
    else
    {
        result.clear();
    }

    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return result;
}

// RFC 7230 style folding removal and trimming for header values.
inline std::string normalize_header_value(const std::string& input)
{
    std::string result;
    result.reserve(input.size());
    bool in_ws = false;

    for (std::size_t i = 0; i < input.size(); ++i)
    {
        char ch = input[i];
        if (ch == '\r' && (i + 1) < input.size() && input[i + 1] == '\n')
        {
            // treat CRLF followed by WSP as a single space
            i += 1;
            in_ws = true;
            continue;
        }
        if (ch == '\n')
        {
            in_ws = true;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(ch)))
        {
            in_ws = true;
            continue;
        }

        if (in_ws && !result.empty())
        {
            result.push_back(' ');
        }
        in_ws = false;
        result.push_back(ch);
    }

    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    auto begin = std::find_if(result.begin(), result.end(), not_space);
    auto end = std::find_if(result.rbegin(), result.rend(), not_space).base();
    if (begin < end)
    {
        return std::string(begin, end);
    }
    return std::string();
}

inline std::string unfold_legacy_lines(const std::string& input)
{
    return normalize_header_value(input);
}

// Canonicalize a raw header list: normalize names to lower-case and
// normalize values (trimming/folding whitespace).
inline HeaderList canonicalize_headers(const HeaderList& input)
{
    HeaderList out;
    out.reserve(input.size());

    for (const auto& header : input)
    {
        const std::string& name = header.first;
        const std::string& value = header.second;

        std::string canonical_name = normalize_header_name(name);
        std::string canonical_value = normalize_header_value(value);

        out.emplace_back(std::move(canonical_name), std::move(canonical_value));
    }

    return out;
}

// Group canonical headers into buckets by name. Some headers (like
// Accept, Cache-Control) have values merged, others (e.g. Set-Cookie)
// remain separate buckets.
inline HeaderBuckets merge_duplicates(const HeaderList& canonical)
{
    HeaderBuckets buckets;

    for (const auto& entry : canonical)
    {
        const std::string& name = entry.first;
        const std::string& value = entry.second;

        const bool mergeable =
            (name == "accept") ||
            (name == "cache-control");

        if (!mergeable || buckets.empty())
        {
            HeaderList bucket;
            bucket.emplace_back(name, value);
            buckets.push_back(std::move(bucket));
            continue;
        }

        HeaderList& current = buckets.back();
        if (!current.empty() && current.front().first == name)
        {
            current.emplace_back(name, value);
        }
        else
        {
            HeaderList bucket;
            bucket.emplace_back(name, value);
            buckets.push_back(std::move(bucket));
        }
    }

    return buckets;
}

// Flatten buckets back into a header list, merging mergeable buckets
// into single comma-separated values.
inline HeaderList collapse_merged(const HeaderBuckets& buckets)
{
    HeaderList out;

    for (const auto& bucket : buckets)
    {
        if (bucket.empty())
        {
            continue;
        }

        if (bucket.size() == 1)
        {
            out.push_back(bucket.front());
            continue;
        }

        const std::string& name = bucket.front().first;
        std::string combined = bucket.front().second;
        for (std::size_t i = 1; i < bucket.size(); ++i)
        {
            combined += ", ";
            combined += bucket[i].second;
        }
        out.emplace_back(name, combined);
    }

    return out;
}

}
