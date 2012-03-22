/*
 *  LLCalcParser.h
 *  Copyright 2008 Aimee Walton.
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2008, Linden Research, Inc.
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
 *
 */

#ifndef LL_CALCPARSER_H
#define LL_CALCPARSER_H

#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_error_handling.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
using namespace boost::spirit::classic;

#include "llcalc.h"
#include "llmath.h"

struct LLCalcParser : grammar<LLCalcParser>
{
	LLCalcParser(F32& result, LLCalc::calc_map_t* constants, LLCalc::calc_map_t* vars) :
		mResult(result), mConstants(constants), mVariables(vars) {};
	
	struct value_closure : closure<value_closure, F32>
	{
		member1 value;
	};
	
	template <typename ScannerT>
	struct definition
	{
		// Rule declarations
		rule<ScannerT> statement, identifier;
		rule<ScannerT, value_closure::context_t> expression, term,
			power, 
			unary_expr, 
			factor, 
			unary_func, 
			binary_func,
			group;

		// start() should return the starting symbol
		rule<ScannerT> const& start() const { return statement; }
		
		definition(LLCalcParser const& self)
		{
			using namespace phoenix;
			
			assertion<std::string> assert_domain("Domain error");
//			assertion<std::string> assert_symbol("Unknown symbol");
			assertion<std::string> assert_syntax("Syntax error");
			
			identifier =
				lexeme_d[(alpha_p | '_') >> *(alnum_p | '_')]
			;
			
			group =
				'(' >> expression[group.value = arg1] >> assert_syntax(ch_p(')'))
			;

			unary_func =
				((str_p("SIN") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_sin)(self,arg1)]) |
				 (str_p("COS") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_cos)(self,arg1)]) |
				 (str_p("TAN") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_tan)(self,arg1)]) |
				 (str_p("ASIN") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_asin)(self,arg1)]) |
				 (str_p("ACOS") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_acos)(self,arg1)]) |
				 (str_p("ATAN") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_atan)(self,arg1)]) |
				 (str_p("SQRT") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_sqrt)(self,arg1)]) |
				 (str_p("LOG") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_log)(self,arg1)]) |
				 (str_p("EXP") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_exp)(self,arg1)]) |
				 (str_p("ABS") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_fabs)(self,arg1)]) |
				 (str_p("FLR") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_floor)(self,arg1)]) |
				 (str_p("CEIL") >> '(' >> expression[unary_func.value = bind(&LLCalcParser::_ceil)(self,arg1)])
				) >> assert_syntax(ch_p(')'))
			;
			
			binary_func =
				((str_p("ATAN2") >> '(' >> expression[binary_func.value = arg1] >> ',' >>
				  expression[binary_func.value = bind(&LLCalcParser::_atan2)(self, binary_func.value, arg1)]) |
				 (str_p("MIN") >> '(' >> expression[binary_func.value = arg1] >> ',' >> 
				  expression[binary_func.value = bind(&LLCalcParser::_min)(self, binary_func.value, arg1)]) |
				 (str_p("MAX") >> '(' >> expression[binary_func.value = arg1] >> ',' >> 
				  expression[binary_func.value = bind(&LLCalcParser::_max)(self, binary_func.value, arg1)])
				) >> assert_syntax(ch_p(')'))
			;
			
			// *TODO: Localisation of the decimal point?
			// Problem, LLLineEditor::postvalidateFloat accepts a comma when appropriate
			// for the current locale. However to do that here could clash with using
			// the comma as a separator when passing arguments to functions.
			factor =
				(ureal_p[factor.value = arg1] |
				 group[factor.value = arg1] |
				 unary_func[factor.value = arg1] |
				 binary_func[factor.value = arg1] |
				 // Lookup throws an Unknown Symbol error if it is unknown, while this works fine,
				 // would be "neater" to handle symbol lookup from here with an assertive parser.
//				 constants_p[factor.value = arg1]|
				 identifier[factor.value = bind(&LLCalcParser::lookup)(self, arg1, arg2)]
				) >>
				// Detect and throw math errors.
				assert_domain(eps_p(bind(&LLCalcParser::checkNaN)(self, factor.value)))
			;

			unary_expr =
				!ch_p('+') >> factor[unary_expr.value = arg1] |
				'-' >> factor[unary_expr.value = -arg1]
			;
			
			power =
				unary_expr[power.value = arg1] >>
				*('^' >> assert_syntax(unary_expr[power.value = bind(&powf)(power.value, arg1)]))
			;
			
			term =
				power[term.value = arg1] >>
				*(('*' >> assert_syntax(power[term.value *= arg1])) |
				  ('/' >> assert_syntax(power[term.value /= arg1])) |
				  ('%' >> assert_syntax(power[term.value = bind(&fmodf)(term.value, arg1)]))
				)
			;
			
			expression =
				assert_syntax(term[expression.value = arg1]) >>
				*(('+' >> assert_syntax(term[expression.value += arg1])) |
				  ('-' >> assert_syntax(term[expression.value -= arg1]))
				)
			;

			statement =
				!ch_p('=') >> ( expression )[var(self.mResult) = arg1] >> (end_p)
			;
		}
	};
	
private:
	// Member functions for semantic actions
	F32	lookup(const std::string::iterator&, const std::string::iterator&) const;
	F32 _min(const F32& a, const F32& b) const { return llmin(a, b); }
	F32 _max(const F32& a, const F32& b) const { return llmax(a, b); }
	
	bool checkNaN(const F32& a) const { return !llisnan(a); }
	
	//FIX* non ambigious function fix making SIN() work for calc -Cryogenic Blitz
	F32 _sin(const F32& a) const { return sin(DEG_TO_RAD * a); }
	F32 _cos(const F32& a) const { return cos(DEG_TO_RAD * a); }
	F32 _tan(const F32& a) const { return tan(DEG_TO_RAD * a); }
	F32 _asin(const F32& a) const { return asin(a * RAD_TO_DEG); }
	F32 _acos(const F32& a) const { return acos(a * RAD_TO_DEG); }
	F32 _atan(const F32& a) const { return atan(a * RAD_TO_DEG); }
	F32 _sqrt(const F32& a) const { return sqrt(a); }
	F32 _log(const F32& a) const { return log(a); }
	F32 _exp(const F32& a) const { return exp(a); }
	F32 _fabs(const F32& a) const { return fabs(a); }
	F32 _floor(const F32& a) const { return (F32)llfloor(a); }
	F32 _ceil(const F32& a) const { return llceil(a); }

	F32 _atan2(const F32& a,const F32& b) const { return atan2(a,b); }



	LLCalc::calc_map_t* mConstants;
	LLCalc::calc_map_t* mVariables;
//	LLCalc::calc_map_t* mUserVariables;
	
	F32&		mResult;
};

#endif // LL_CALCPARSER_H
