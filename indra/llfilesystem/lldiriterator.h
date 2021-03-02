/**
 * @file lldiriterator.h
 * @brief Iterator through directory entries matching the search pattern.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLDIRITERATOR_H
#define LL_LLDIRITERATOR_H

#include "linden_common.h"

/**
 * Class LLDirIterator
 *
 * Iterates through directory entries matching the search pattern.
 */
class LLDirIterator
{
public:
	/**
	 * Constructs LLDirIterator object to search for glob pattern
	 * matches in a directory.
	 *
	 * @param dirname - name of a directory to search in.
	 * @param mask - search pattern, a glob expression
	 *
	 * Wildcards supported in glob expressions:
	 * --------------------------------------------------------------
	 * | Wildcard 	| Matches										|
	 * --------------------------------------------------------------
	 * | 	* 		|zero or more characters						|
	 * | 	?		|exactly one character							|
	 * | [abcde]	|exactly one character listed					|
	 * | [a-e]		|exactly one character in the given range		|
	 * | [!abcde]	|any character that is not listed				|
	 * | [!a-e]		|any character that is not in the given range	|
	 * | {abc,xyz}	|exactly one entire word in the options given	|
	 * --------------------------------------------------------------
	 */
	LLDirIterator(const std::string &dirname, const std::string &mask);

	~LLDirIterator();

	/**
	 * Searches for the next directory entry matching the glob mask
	 * specified upon iterator construction.
	 * Returns true if a match is found, sets fname
	 * parameter to the name of the matched directory entry and
	 * increments the iterator position.
	 *
	 * Typical usage:
	 * <code>
	 * LLDirIterator iter(directory, pattern);
	 * if ( iter.next(scanResult) )
	 * </code>
	 *
	 * @param fname - name of the matched directory entry.
	 * @return true if a match is found, false otherwise.
	 */
	bool next(std::string &fname);

protected:
	class Impl;
	Impl* mImpl;
};

#endif //LL_LLDIRITERATOR_H
