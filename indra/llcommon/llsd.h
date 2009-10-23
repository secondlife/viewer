/** 
 * @file llsd.h
 * @brief LLSD flexible data system.
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

#ifndef LL_LLSD_NEW_H
#define LL_LLSD_NEW_H

#include <map>
#include <string>
#include <vector>

#include "stdtypes.h"

#include "lldate.h"
#include "lluri.h"
#include "lluuid.h"

/**
	LLSD provides a flexible data system similar to the data facilities of
	dynamic languages like Perl and Python.  It is created to support exchange
	of structured data between loosly coupled systems.  (Here, "loosly coupled"
	means not compiled together into the same module.)
	
	Data in such exchanges must be highly tollerant of changes on either side
	such as:
			- recompilation
			- implementation in a different langauge
			- addition of extra parameters
			- execution of older versions (with fewer parameters)

	To this aim, the C++ API of LLSD strives to be very easy to use, and to
	default to "the right thing" whereever possible.  It is extremely tollerant
	of errors and unexpected situations.
	
	The fundimental class is LLSD.  LLSD is a value holding object.  It holds
	one value that is either undefined, one of the scalar types, or a map or an
	array.  LLSD objects have value semantics (copying them copies the value,
	though it can be considered efficient, due to shareing.), and mutable.

	Undefined is the singular value given to LLSD objects that are not
	initialized with any data.  It is also used as the return value for
	operations that return an LLSD,
	
	The sclar data types are:
		- Boolean	- true or false
		- Integer	- a 32 bit signed integer
		- Real		- a 64 IEEE 754 floating point value
		- UUID		- a 128 unique value
		- String	- a sequence of zero or more Unicode chracters
		- Date		- an absolute point in time, UTC,
						with resolution to the second
		- URI		- a String that is a URI
		- Binary	- a sequence of zero or more octets (unsigned bytes)
	
	A map is a dictionary mapping String keys to LLSD values.  The keys are
	unique within a map, and have only one value (though that value could be
	an LLSD array).
	
	An array is a sequence of zero or more LLSD values.
	
	@nosubgrouping
*/

class LL_COMMON_API LLSD
{
public:
		LLSD();		///< initially Undefined
		~LLSD();	///< this class may NOT be subclassed

	/** @name Copyable and Assignable */
	//@{
		LLSD(const LLSD&);
		void assign(const LLSD& other);
		LLSD& operator=(const LLSD& other)	{ assign(other); return *this; }

	//@}

	void clear();	///< resets to Undefined


	/** @name Scalar Types
	    The scalar types, and how they map onto C++
	*/
	//@{
		typedef bool			Boolean;
		typedef S32				Integer;
		typedef F64				Real;
		typedef std::string		String;
		typedef LLUUID			UUID;
		typedef LLDate			Date;
		typedef LLURI			URI;
		typedef std::vector<U8>	Binary;
	//@}
	
	/** @name Scalar Constructors */
	//@{
		LLSD(Boolean);
		LLSD(Integer);
		LLSD(Real);
		LLSD(const String&);
		LLSD(const UUID&);
		LLSD(const Date&);
		LLSD(const URI&);
		LLSD(const Binary&);
	//@}

	/** @name Convenience Constructors */
	//@{
		LLSD(F32); // F32 -> Real
	//@}
	
	/** @name Scalar Assignment */
	//@{
		void assign(Boolean);
		void assign(Integer);
		void assign(Real);
		void assign(const String&);
		void assign(const UUID&);
		void assign(const Date&);
		void assign(const URI&);
		void assign(const Binary&);
		
		LLSD& operator=(Boolean v)			{ assign(v); return *this; }
		LLSD& operator=(Integer v)			{ assign(v); return *this; }
		LLSD& operator=(Real v)				{ assign(v); return *this; }
		LLSD& operator=(const String& v)	{ assign(v); return *this; }
		LLSD& operator=(const UUID& v)		{ assign(v); return *this; }
		LLSD& operator=(const Date& v)		{ assign(v); return *this; }
		LLSD& operator=(const URI& v)		{ assign(v); return *this; }
		LLSD& operator=(const Binary& v)	{ assign(v); return *this; }
	//@}

	/**
		@name Scalar Accessors
		@brief Fetch a scalar value, converting if needed and possible
		
		Conversion among the basic types, Boolean, Integer, Real and String, is
		fully defined.  Each type can be converted to another with a reasonable
		interpretation.  These conversions can be used as a convenience even
		when you know the data is in one format, but you want it in another.  Of
		course, many of these conversions lose information.

		Note: These conversions are not the same as Perl's.  In particular, when
		converting a String to a Boolean, only the empty string converts to
		false.  Converting the String "0" to Boolean results in true.

		Conversion to and from UUID, Date, and URI is only defined to and from
		String.  Conversion is defined to be information preserving for valid
		values of those types.  These conversions can be used when one needs to
		convert data to or from another system that cannot handle these types
		natively, but can handle strings.

		Conversion to and from Binary isn't defined.

		Conversion of the Undefined value to any scalar type results in a
		reasonable null or zero value for the type.
	*/
	//@{
		Boolean	asBoolean() const;
		Integer	asInteger() const;
		Real	asReal() const;
		String	asString() const;
		UUID	asUUID() const;
		Date	asDate() const;
		URI		asURI() const;
		Binary	asBinary() const;

		operator Boolean() const	{ return asBoolean(); }
		operator Integer() const	{ return asInteger(); }
		operator Real() const		{ return asReal(); }
		operator String() const		{ return asString(); }
		operator UUID() const		{ return asUUID(); }
		operator Date() const		{ return asDate(); }
		operator URI() const		{ return asURI(); }
		operator Binary() const		{ return asBinary(); }

		// This is needed because most platforms do not automatically
		// convert the boolean negation as a bool in an if statement.
		bool operator!() const {return !asBoolean();}
	//@}
	
	/** @name Character Pointer Helpers
		These are helper routines to make working with char* the same as easy as
		working with strings.
	 */
	//@{
		LLSD(const char*);
		void assign(const char*);
		LLSD& operator=(const char* v)	{ assign(v); return *this; }
	//@}
	
	/** @name Map Values */
	//@{
		static LLSD emptyMap();
		
		bool has(const String&) const;
		LLSD get(const String&) const;
		LLSD& insert(const String&, const LLSD&);
		void erase(const String&);
		
		LLSD& operator[](const String&);
		LLSD& operator[](const char* c)			{ return (*this)[String(c)]; }
		const LLSD& operator[](const String&) const;
		const LLSD& operator[](const char* c) const	{ return (*this)[String(c)]; }
	//@}
	
	/** @name Array Values */
	//@{
		static LLSD emptyArray();
		
		LLSD get(Integer) const;
		void set(Integer, const LLSD&);
		LLSD& insert(Integer, const LLSD&);
		void append(const LLSD&);
		void erase(Integer);
		
		const LLSD& operator[](Integer) const;
		LLSD& operator[](Integer);
	//@}

	/** @name Iterators */
	//@{
		int size() const;

		typedef std::map<String, LLSD>::iterator		map_iterator;
		typedef std::map<String, LLSD>::const_iterator	map_const_iterator;
		
		map_iterator		beginMap();
		map_iterator		endMap();
		map_const_iterator	beginMap() const;
		map_const_iterator	endMap() const;
		
		typedef std::vector<LLSD>::iterator			array_iterator;
		typedef std::vector<LLSD>::const_iterator	array_const_iterator;
		
		array_iterator			beginArray();
		array_iterator			endArray();
		array_const_iterator	beginArray() const;
		array_const_iterator	endArray() const;
	//@}
	
	/** @name Type Testing */
	//@{
		enum Type {
			TypeUndefined,
			TypeBoolean,
			TypeInteger,
			TypeReal,
			TypeString,
			TypeUUID,
			TypeDate,
			TypeURI,
			TypeBinary,
			TypeMap,
			TypeArray
		};
		
		Type type() const;
		
		bool isUndefined() const	{ return type() == TypeUndefined; }
		bool isDefined() const		{ return type() != TypeUndefined; }
		bool isBoolean() const		{ return type() == TypeBoolean; }
		bool isInteger() const		{ return type() == TypeInteger; }
		bool isReal() const			{ return type() == TypeReal; }
		bool isString() const		{ return type() == TypeString; }
		bool isUUID() const			{ return type() == TypeUUID; }
		bool isDate() const			{ return type() == TypeDate; }
		bool isURI() const			{ return type() == TypeURI; }
		bool isBinary() const		{ return type() == TypeBinary; }
		bool isMap() const			{ return type() == TypeMap; }
		bool isArray() const		{ return type() == TypeArray; }
	//@}

	/** @name Automatic Cast Protection
		These are not implemented on purpose.  Without them, C++ can perform
		some conversions that are clearly not what the programmer intended.
		
		If you get a linker error about these being missing, you have made
		mistake in your code.  DO NOT IMPLEMENT THESE FUNCTIONS as a fix.
		
		All of thse problems stem from trying to support char* in LLSD or in
		std::string.  There are too many automatic casts that will lead to
		using an arbitrary pointer or scalar type to std::string.
	 */
	//@{
		LLSD(const void*);				///< construct from aribrary pointers
		void assign(const void*);		///< assign from arbitrary pointers
		LLSD& operator=(const void*);	///< assign from arbitrary pointers
		
		bool has(Integer) const;		///< has only works for Maps
	//@}
	
	/** @name Implementation */
	//@{
public:
		class Impl;
private:
		Impl* impl;
	//@}
	
	/** @name Unit Testing Interface */
	//@{
public:
		static U32 allocationCount();	///< how many Impls have been made
		static U32 outstandingCount();	///< how many Impls are still alive
	//@}

private:
	/** @name Debugging Interface */
	//@{
		/// Returns XML version of llsd -- only to be called from debugger
		static const char *dumpXML(const LLSD &llsd);

		/// Returns Notation version of llsd -- only to be called from debugger
		static const char *dump(const LLSD &llsd);
	//@}
};

struct llsd_select_bool : public std::unary_function<LLSD, LLSD::Boolean>
{
	LLSD::Boolean operator()(const LLSD& sd) const
	{
		return sd.asBoolean();
	}
};
struct llsd_select_integer : public std::unary_function<LLSD, LLSD::Integer>
{
	LLSD::Integer operator()(const LLSD& sd) const
	{
		return sd.asInteger();
	}
};
struct llsd_select_real : public std::unary_function<LLSD, LLSD::Real>
{
	LLSD::Real operator()(const LLSD& sd) const
	{
		return sd.asReal();
	}
};
struct llsd_select_float : public std::unary_function<LLSD, F32>
{
	F32 operator()(const LLSD& sd) const
	{
		return (F32)sd.asReal();
	}
};
struct llsd_select_uuid : public std::unary_function<LLSD, LLSD::UUID>
{
	LLSD::UUID operator()(const LLSD& sd) const
	{
		return sd.asUUID();
	}
};
struct llsd_select_string : public std::unary_function<LLSD, LLSD::String>
{
	LLSD::String operator()(const LLSD& sd) const
	{
		return sd.asString();
	}
};

LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLSD& llsd);

/** QUESTIONS & TO DOS
	- Would Binary be more convenient as usigned char* buffer semantics?
	- Should Binary be convertable to/from String, and if so how?
		- as UTF8 encoded strings (making not like UUID<->String)
		- as Base64 or Base96 encoded (making like UUID<->String)
	- Conversions to std::string and LLUUID do not result in easy assignment
		to std::string, std::string or LLUUID due to non-unique conversion paths
*/

#endif // LL_LLSD_NEW_H
