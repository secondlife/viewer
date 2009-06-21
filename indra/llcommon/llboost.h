/** 
 * @file llboost.h
 * @brief helper object & functions for use with boost
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#ifndef LL_LLBOOST_H
#define LL_LLBOOST_H

#include <boost/tokenizer.hpp>

// boost_tokenizer typedef
/* example usage:
	boost_tokenizer tokens(input_string, boost::char_separator<char>(" \t\n"));
	for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string tok = *token_iter;
		process_token_string( tok );
	}
*/
typedef boost::tokenizer<boost::char_separator<char> > boost_tokenizer;

// Useful combiner for boost signals that retturn a vool (e.g. validation)
//  returns false if any of the callbacks return false
struct boost_boolean_combiner
{
	typedef bool result_type;
	template<typename InputIterator>
	bool operator()(InputIterator first, InputIterator last) const
	{
		bool res = true;
		while (first != last)
			res &= *first++;
		return res;
	}
};

#endif // LL_LLBOOST_H
