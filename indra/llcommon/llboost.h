/** 
 * @file llboost.h
 * @brief helper object & functions for use with boost
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#endif // LL_LLBOOST_H
