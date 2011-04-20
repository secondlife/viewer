/** 
 * @file llsdutil.cpp
 * @author Phoenix
 * @date 2006-05-24
 * @brief Implementation of classes, functions, etc, for using structured data.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llsdutil.h"

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>	// for htonl
#elif LL_LINUX || LL_SOLARIS
#	include <netinet/in.h>
#elif LL_DARWIN
#	include <arpa/inet.h>
#endif

#include "llsdserialize.h"
#include "stringize.h"
#include "is_approx_equal_fraction.h"

#include <map>
#include <set>
#include <boost/range.hpp>

// U32
LLSD ll_sd_from_U32(const U32 val)
{
	std::vector<U8> v;
	U32 net_order = htonl(val);

	v.resize(4);
	memcpy(&(v[0]), &net_order, 4);		/* Flawfinder: ignore */

	return LLSD(v);
}

U32 ll_U32_from_sd(const LLSD& sd)
{
	U32 ret;
	std::vector<U8> v = sd.asBinary();
	if (v.size() < 4)
	{
		return 0;
	}
	memcpy(&ret, &(v[0]), 4);		/* Flawfinder: ignore */
	ret = ntohl(ret);
	return ret;
}

//U64
LLSD ll_sd_from_U64(const U64 val)
{
	std::vector<U8> v;
	U32 high, low;

	high = (U32)(val >> 32);
	low = (U32)val;
	high = htonl(high);
	low = htonl(low);

	v.resize(8);
	memcpy(&(v[0]), &high, 4);		/* Flawfinder: ignore */
	memcpy(&(v[4]), &low, 4);		/* Flawfinder: ignore */

	return LLSD(v);
}

U64 ll_U64_from_sd(const LLSD& sd)
{
	U32 high, low;
	std::vector<U8> v = sd.asBinary();

	if (v.size() < 8)
	{
		return 0;
	}

	memcpy(&high, &(v[0]), 4);		/* Flawfinder: ignore */
	memcpy(&low, &(v[4]), 4);		/* Flawfinder: ignore */
	high = ntohl(high);
	low = ntohl(low);

	return ((U64)high) << 32 | low;
}

// IP Address (stored in net order in a U32, so don't need swizzling)
LLSD ll_sd_from_ipaddr(const U32 val)
{
	std::vector<U8> v;

	v.resize(4);
	memcpy(&(v[0]), &val, 4);		/* Flawfinder: ignore */

	return LLSD(v);
}

U32 ll_ipaddr_from_sd(const LLSD& sd)
{
	U32 ret;
	std::vector<U8> v = sd.asBinary();
	if (v.size() < 4)
	{
		return 0;
	}
	memcpy(&ret, &(v[0]), 4);		/* Flawfinder: ignore */
	return ret;
}

// Converts an LLSD binary to an LLSD string
LLSD ll_string_from_binary(const LLSD& sd)
{
	std::vector<U8> value = sd.asBinary();
	std::string str;
	str.resize(value.size());
	memcpy(&str[0], &value[0], value.size());
	return str;
}

// Converts an LLSD string to an LLSD binary
LLSD ll_binary_from_string(const LLSD& sd)
{
	std::vector<U8> binary_value;

	std::string string_value = sd.asString();
	for (std::string::iterator iter = string_value.begin();
		 iter != string_value.end(); ++iter)
	{
		binary_value.push_back(*iter);
	}

	binary_value.push_back('\0');

	return binary_value;
}

char* ll_print_sd(const LLSD& sd)
{
	const U32 bufferSize = 10 * 1024;
	static char buffer[bufferSize];
	std::ostringstream stream;
	//stream.rdbuf()->pubsetbuf(buffer, bufferSize);
	stream << LLSDOStreamer<LLSDXMLFormatter>(sd);
	stream << std::ends;
	strncpy(buffer, stream.str().c_str(), bufferSize);
	buffer[bufferSize - 1] = '\0';
	return buffer;
}

char* ll_pretty_print_sd_ptr(const LLSD* sd)
{
	if (sd)
	{
		return ll_pretty_print_sd(*sd);
	}
	return NULL;
}

char* ll_pretty_print_sd(const LLSD& sd)
{
	const U32 bufferSize = 10 * 1024;
	static char buffer[bufferSize];
	std::ostringstream stream;
	//stream.rdbuf()->pubsetbuf(buffer, bufferSize);
	stream << LLSDOStreamer<LLSDXMLFormatter>(sd, LLSDFormatter::OPTIONS_PRETTY);
	stream << std::ends;
	strncpy(buffer, stream.str().c_str(), bufferSize);
	buffer[bufferSize - 1] = '\0';
	return buffer;
}

//compares the structure of an LLSD to a template LLSD and stores the
//"valid" values in a 3rd LLSD.  Default values pulled from the template
//if the tested LLSD does not contain the key/value pair.
//Excess values in the test LLSD are ignored in the resultant_llsd.
//If the llsd to test has a specific key to a map and the values
//are not of the same type, false is returned or if the LLSDs are not
//of the same value.  Ordering of arrays matters
//Otherwise, returns true
BOOL compare_llsd_with_template(
	const LLSD& llsd_to_test,
	const LLSD& template_llsd,
	LLSD& resultant_llsd)
{
	if (
		llsd_to_test.isUndefined() &&
		template_llsd.isDefined() )
	{
		resultant_llsd = template_llsd;
		return TRUE;
	}
	else if ( llsd_to_test.type() != template_llsd.type() )
	{
		resultant_llsd = LLSD();
		return FALSE;
	}

	if ( llsd_to_test.isArray() )
	{
		//they are both arrays
		//we loop over all the items in the template
		//verifying that the to_test has a subset (in the same order)
		//any shortcoming in the testing_llsd are just taken
		//to be the rest of the template
		LLSD data;
		LLSD::array_const_iterator test_iter;
		LLSD::array_const_iterator template_iter;

		resultant_llsd = LLSD::emptyArray();
		test_iter = llsd_to_test.beginArray();

		for (
			template_iter = template_llsd.beginArray();
			(template_iter != template_llsd.endArray() &&
			 test_iter != llsd_to_test.endArray());
			++template_iter)
		{
			if ( !compare_llsd_with_template(
					 *test_iter,
					 *template_iter,
					 data) )
			{
				resultant_llsd = LLSD();
				return FALSE;
			}
			else
			{
				resultant_llsd.append(data);
			}

			++test_iter;
		}

		//so either the test or the template ended
		//we do another loop now to the end of the template
		//grabbing the default values
		for (;
			 template_iter != template_llsd.endArray();
			 ++template_iter)
		{
			resultant_llsd.append(*template_iter);
		}
	}
	else if ( llsd_to_test.isMap() )
	{
		//now we loop over the keys of the two maps
		//any excess is taken from the template
		//excess is ignored in the test
		LLSD value;
		LLSD::map_const_iterator template_iter;

		resultant_llsd = LLSD::emptyMap();
		for (
			template_iter = template_llsd.beginMap();
			template_iter != template_llsd.endMap();
			++template_iter)
		{
			if ( llsd_to_test.has(template_iter->first) )
			{
				//the test LLSD has the same key
				if ( !compare_llsd_with_template(
						 llsd_to_test[template_iter->first],
						 template_iter->second,
						 value) )
				{
					resultant_llsd = LLSD();
					return FALSE;
				}
				else
				{
					resultant_llsd[template_iter->first] = value;
				}
			}
			else
			{
				//test llsd doesn't have it...take the
				//template as default value
				resultant_llsd[template_iter->first] =
					template_iter->second;
			}
		}
	}
	else
	{
		//of same type...take the test llsd's value
		resultant_llsd = llsd_to_test;
	}


	return TRUE;
}

/*****************************************************************************
*   Helpers for llsd_matches()
*****************************************************************************/
// raw data used for LLSD::Type lookup
struct Data
{
    LLSD::Type type;
    const char* name;
} typedata[] =
{
#define def(type) { LLSD::type, #type + 4 }
    def(TypeUndefined),
    def(TypeBoolean),
    def(TypeInteger),
    def(TypeReal),
    def(TypeString),
    def(TypeUUID),
    def(TypeDate),
    def(TypeURI),
    def(TypeBinary),
    def(TypeMap),
    def(TypeArray)
#undef  def
};

// LLSD::Type lookup class into which we load the above static data
class TypeLookup
{
    typedef std::map<LLSD::Type, std::string> MapType;

public:
    TypeLookup()
    {
        for (const Data *di(boost::begin(typedata)), *dend(boost::end(typedata)); di != dend; ++di)
        {
            mMap[di->type] = di->name;
        }
    }

    std::string lookup(LLSD::Type type) const
    {
        MapType::const_iterator found = mMap.find(type);
        if (found != mMap.end())
        {
            return found->second;
        }
        return STRINGIZE("<unknown LLSD type " << type << ">");
    }

private:
    MapType mMap;
};

// static instance of the lookup class
static const TypeLookup sTypes;

// describe a mismatch; phrasing may want tweaking
const std::string op(" required instead of ");

// llsd_matches() wants to identify specifically where in a complex prototype
// structure the mismatch occurred. This entails passing a prefix string,
// empty for the top-level call. If the prototype contains an array of maps,
// and the mismatch occurs in the second map in a key 'foo', we want to
// decorate the returned string with: "[1]['foo']: etc." On the other hand, we
// want to omit the entire prefix -- including colon -- if the mismatch is at
// top level. This helper accepts the (possibly empty) recursively-accumulated
// prefix string, returning either empty or the original string with colon
// appended.
static std::string colon(const std::string& pfx)
{
    if (pfx.empty())
        return pfx;
    return pfx + ": ";
}

// param type for match_types
typedef std::vector<LLSD::Type> TypeVector;

// The scalar cases in llsd_matches() use this helper. In most cases, we can
// accept not only the exact type specified in the prototype, but also other
// types convertible to the expected type. That implies looping over an array
// of such types. If the actual type doesn't match any of them, we want to
// provide a list of acceptable conversions as well as the exact type, e.g.:
// "Integer (or Boolean, Real, String) required instead of UUID". Both the
// implementation and the calling logic are simplified by separating out the
// expected type from the convertible types.
static std::string match_types(LLSD::Type expect, // prototype.type()
                               const TypeVector& accept, // types convertible to that type
                               LLSD::Type actual,        // type we're checking
                               const std::string& pfx)   // as for llsd_matches
{
    // Trivial case: if the actual type is exactly what we expect, we're good.
    if (actual == expect)
        return "";

    // For the rest of the logic, build up a suitable error string as we go so
    // we only have to make a single pass over the list of acceptable types.
    // If we detect success along the way, we'll simply discard the partial
    // error string.
    std::ostringstream out;
    out << colon(pfx) << sTypes.lookup(expect);

    // If there are any convertible types, append that list.
    if (! accept.empty())
    {
        out << " (";
        const char* sep = "or ";
        for (TypeVector::const_iterator ai(accept.begin()), aend(accept.end());
             ai != aend; ++ai, sep = ", ")
        {
            // Don't forget to return success if we match any of those types...
            if (actual == *ai)
                return "";
            out << sep << sTypes.lookup(*ai);
        }
        out << ')';
    }
    // If we got this far, it's because 'actual' was not one of the acceptable
    // types, so we must return an error. 'out' already contains colon(pfx)
    // and the formatted list of acceptable types, so just append the mismatch
    // phrase and the actual type.
    out << op << sTypes.lookup(actual);
    return out.str();
}

// see docstring in .h file
std::string llsd_matches(const LLSD& prototype, const LLSD& data, const std::string& pfx)
{
    // An undefined prototype means that any data is valid.
    // An undefined slot in an array or map prototype means that any data
    // may fill that slot.
    if (prototype.isUndefined())
        return "";
    // A prototype array must match a data array with at least as many
    // entries. Moreover, every prototype entry must match the
    // corresponding data entry.
    if (prototype.isArray())
    {
        if (! data.isArray())
        {
            return STRINGIZE(colon(pfx) << "Array" << op << sTypes.lookup(data.type()));
        }
        if (data.size() < prototype.size())
        {
            return STRINGIZE(colon(pfx) << "Array size " << prototype.size() << op
                             << "Array size " << data.size());
        }
        for (LLSD::Integer i = 0; i < prototype.size(); ++i)
        {
            std::string match(llsd_matches(prototype[i], data[i], STRINGIZE('[' << i << ']')));
            if (! match.empty())
            {
                return match;
            }
        }
        return "";
    }
    // A prototype map must match a data map. Every key in the prototype
    // must have a corresponding key in the data map; every value in the
    // prototype must match the corresponding key's value in the data.
    if (prototype.isMap())
    {
        if (! data.isMap())
        {
            return STRINGIZE(colon(pfx) << "Map" << op << sTypes.lookup(data.type()));
        }
        // If there are a number of keys missing from the data, it would be
        // frustrating to a coder to discover them one at a time, with a big
        // build each time. Enumerate all missing keys.
        std::ostringstream out;
        out << colon(pfx);
        const char* init = "Map missing keys: ";
        const char* sep = init;
        for (LLSD::map_const_iterator mi = prototype.beginMap(); mi != prototype.endMap(); ++mi)
        {
            if (! data.has(mi->first))
            {
                out << sep << mi->first;
                sep = ", ";
            }
        }
        // So... are we missing any keys?
        if (sep != init)
        {
            return out.str();
        }
        // Good, the data block contains all the keys required by the
        // prototype. Now match the prototype entries.
        for (LLSD::map_const_iterator mi2 = prototype.beginMap(); mi2 != prototype.endMap(); ++mi2)
        {
            std::string match(llsd_matches(mi2->second, data[mi2->first],
                                           STRINGIZE("['" << mi2->first << "']")));
            if (! match.empty())
            {
                return match;
            }
        }
        return "";
    }
    // A String prototype can match String, Boolean, Integer, Real, UUID,
    // Date and URI, because any of these can be converted to String.
    if (prototype.isString())
    {
        static LLSD::Type accept[] =
        {
            LLSD::TypeBoolean,
            LLSD::TypeInteger,
            LLSD::TypeReal,
            LLSD::TypeUUID,
            LLSD::TypeDate,
            LLSD::TypeURI
        };
        return match_types(prototype.type(),
                           TypeVector(boost::begin(accept), boost::end(accept)),
                           data.type(),
                           pfx);
    }
    // Boolean, Integer, Real match each other or String. TBD: ensure that
    // a String value is numeric.
    if (prototype.isBoolean() || prototype.isInteger() || prototype.isReal())
    {
        static LLSD::Type all[] =
        {
            LLSD::TypeBoolean,
            LLSD::TypeInteger,
            LLSD::TypeReal,
            LLSD::TypeString
        };
        // Funny business: shuffle the set of acceptable types to include all
        // but the prototype's type. Get the acceptable types in a set.
        std::set<LLSD::Type> rest(boost::begin(all), boost::end(all));
        // Remove the prototype's type because we pass that separately.
        rest.erase(prototype.type());
        return match_types(prototype.type(),
                           TypeVector(rest.begin(), rest.end()),
                           data.type(),
                           pfx);
    }
    // UUID, Date and URI match themselves or String.
    if (prototype.isUUID() || prototype.isDate() || prototype.isURI())
    {
        static LLSD::Type accept[] =
        {
            LLSD::TypeString
        };
        return match_types(prototype.type(),
                           TypeVector(boost::begin(accept), boost::end(accept)),
                           data.type(),
                           pfx);
    }
    // We don't yet know the conversion semantics associated with any new LLSD
    // data type that might be added, so until we've been extended to handle
    // them, assume it's strict: the new type matches only itself. (This is
    // true of Binary, which is why we don't handle that case separately.) Too
    // bad LLSD doesn't define isConvertible(Type to, Type from).
    return match_types(prototype.type(), TypeVector(), data.type(), pfx);
}

bool llsd_equals(const LLSD& lhs, const LLSD& rhs, unsigned bits)
{
    // We're comparing strict equality of LLSD representation rather than
    // performing any conversions. So if the types aren't equal, the LLSD
    // values aren't equal.
    if (lhs.type() != rhs.type())
    {
        return false;
    }

    // Here we know both types are equal. Now compare values.
    switch (lhs.type())
    {
    case LLSD::TypeUndefined:
        // Both are TypeUndefined. There's nothing more to know.
        return true;

    case LLSD::TypeReal:
        // This is where the 'bits' argument comes in handy. If passed
        // explicitly, it means to use is_approx_equal_fraction() to compare.
        if (bits >= 0)
        {
            return is_approx_equal_fraction(lhs.asReal(), rhs.asReal(), bits);
        }
        // Otherwise we compare bit representations, and the usual caveats
        // about comparing floating-point numbers apply. Omitting 'bits' when
        // comparing Real values is only useful when we expect identical bit
        // representation for a given Real value, e.g. for integer-valued
        // Reals.
        return (lhs.asReal() == rhs.asReal());

#define COMPARE_SCALAR(type)                                    \
    case LLSD::Type##type:                                      \
        /* LLSD::URI has operator!=() but not operator==() */   \
        /* rely on the optimizer for all others */              \
        return (! (lhs.as##type() != rhs.as##type()))

    COMPARE_SCALAR(Boolean);
    COMPARE_SCALAR(Integer);
    COMPARE_SCALAR(String);
    COMPARE_SCALAR(UUID);
    COMPARE_SCALAR(Date);
    COMPARE_SCALAR(URI);
    COMPARE_SCALAR(Binary);

#undef COMPARE_SCALAR

    case LLSD::TypeArray:
    {
        LLSD::array_const_iterator
            lai(lhs.beginArray()), laend(lhs.endArray()),
            rai(rhs.beginArray()), raend(rhs.endArray());
        // Compare array elements, walking the two arrays in parallel.
        for ( ; lai != laend && rai != raend; ++lai, ++rai)
        {
            // If any one array element is unequal, the arrays are unequal.
            if (! llsd_equals(*lai, *rai, bits))
                return false;
        }
        // Here we've reached the end of one or the other array. They're equal
        // only if they're BOTH at end: that is, if they have equal length too.
        return (lai == laend && rai == raend);
    }

    case LLSD::TypeMap:
    {
        // Build a set of all rhs keys.
        std::set<LLSD::String> rhskeys;
        for (LLSD::map_const_iterator rmi(rhs.beginMap()), rmend(rhs.endMap());
             rmi != rmend; ++rmi)
        {
            rhskeys.insert(rmi->first);
        }
        // Now walk all the lhs keys.
        for (LLSD::map_const_iterator lmi(lhs.beginMap()), lmend(lhs.endMap());
             lmi != lmend; ++lmi)
        {
            // Try to erase this lhs key from the set of rhs keys. If rhs has
            // no such key, the maps are unequal. erase(key) returns count of
            // items erased.
            if (rhskeys.erase(lmi->first) != 1)
                return false;
            // Both maps have the current key. Compare values.
            if (! llsd_equals(lmi->second, rhs[lmi->first], bits))
                return false;
        }
        // We've now established that all the lhs keys have equal values in
        // both maps. The maps are equal unless rhs contains a superset of
        // those keys.
        return rhskeys.empty();
    }

    default:
        // We expect that every possible type() value is specifically handled
        // above. Failing to extend this switch to support a new LLSD type is
        // an error that must be brought to the coder's attention.
        LL_ERRS("llsd_equals") << "llsd_equals(" << lhs << ", " << rhs << ", " << bits << "): "
            "unknown type " << lhs.type() << LL_ENDL;
        return false;               // pacify the compiler
    }
}
