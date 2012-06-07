/** 
 * @file llsd.h
 * @brief LLSD flexible data system.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
	of structured data between loosely coupled systems.  (Here, "loosely coupled"
	means not compiled together into the same module.)
	
	Data in such exchanges must be highly tolerant of changes on either side
	such as:
			- recompilation
			- implementation in a different langauge
			- addition of extra parameters
			- execution of older versions (with fewer parameters)

	To this aim, the C++ API of LLSD strives to be very easy to use, and to
	default to "the right thing" wherever possible.  It is extremely tolerant
	of errors and unexpected situations.
	
	The fundamental class is LLSD.  LLSD is a value holding object.  It holds
	one value that is either undefined, one of the scalar types, or a map or an
	array.  LLSD objects have value semantics (copying them copies the value,
	though it can be considered efficient, due to sharing), and mutable.

	Undefined is the singular value given to LLSD objects that are not
	initialized with any data.  It is also used as the return value for
	operations that return an LLSD.
	
	The scalar data types are:
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
	
	Thread Safety

	In general, these LLSD classes offer *less* safety than STL container
	classes.  Implementations prior to this one were unsafe even when
	completely unrelated LLSD trees were in two threads due to reference
	sharing of special 'undefined' values that participated in the reference
	counting mechanism.

	The dereference-before-refcount and aggressive tree sharing also make
	it impractical to share an LLSD across threads.  A strategy of passing
	ownership or a copy to another thread is still difficult due to a lack
	of a cloning interface but it can be done with some care.

	One way of transferring ownership is as follows:

		void method(const LLSD input) {
		...
		LLSD * xfer_tree = new LLSD();
		{
			// Top-level values
			(* xfer_tree)['label'] = "Some text";
			(* xfer_tree)['mode'] = APP_MODE_CONSTANT;

			// There will be a second-level
			LLSD subtree(LLSD::emptyMap());
			(* xfer_tree)['subtree'] = subtree;

			// Do *not* copy from LLSD objects via LLSD
			// intermediaries.  Only use plain-old-data
			// types as intermediaries to prevent reference
			// sharing.
			subtree['value1'] = input['value1'].asInteger();
			subtree['value2'] = input['value2'].asString();

			// Close scope and drop 'subtree's reference.
			// Only xfer_tree has a reference to the second
			// level data.
		}
		...
		// Transfer the LLSD pointer to another thread.  Ownership
		// transfers, this thread no longer has a reference to any
		// part of the xfer_tree and there's nothing to free or
		// release here.  Receiving thread does need to delete the
		// pointer when it is done with the LLSD.  Transfer
		// mechanism must perform correct data ordering operations
		// as dictated by architecture.
		other_thread.sendMessageAndPointer("Take This", xfer_tree);
		xfer_tree = NULL;


	Avoid this pattern which provides half of a race condition:
	
		void method(const LLSD input) {
		...
		LLSD xfer_tree(LLSD::emptyMap());
		xfer_tree['label'] = "Some text";
		xfer_tree['mode'] = APP_MODE_CONSTANT;
		...
		other_thread.sendMessageAndPointer("Take This", xfer_tree);

	
	@nosubgrouping
*/

// Normally undefined, used for diagnostics
//#define LLSD_DEBUG_INFO	1

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
		These are helper routines to make working with char* as easy as
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
		void insert(const String&, const LLSD&);
		void erase(const String&);
		LLSD& with(const String&, const LLSD&);
		
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
		void insert(Integer, const LLSD&);
		void append(const LLSD&);
		void erase(Integer);
		LLSD& with(Integer, const LLSD&);
		
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
			TypeUndefined = 0,
			TypeBoolean,
			TypeInteger,
			TypeReal,
			TypeString,
			TypeUUID,
			TypeDate,
			TypeURI,
			TypeBinary,
			TypeMap,
			TypeArray,
			TypeLLSDTypeEnd,
			TypeLLSDTypeBegin = TypeUndefined,
			TypeLLSDNumTypes = (TypeLLSDTypeEnd - TypeLLSDTypeBegin)
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
		
		All of these problems stem from trying to support char* in LLSD or in
		std::string.  There are too many automatic casts that will lead to
		using an arbitrary pointer or scalar type to std::string.
	 */
	//@{
		LLSD(const void*);				///< construct from aribrary pointers
		void assign(const void*);		///< assign from arbitrary pointers
		LLSD& operator=(const void*);	///< assign from arbitrary pointers
		
		bool has(Integer) const;		///< has() only works for Maps
	//@}
	
	/** @name Implementation */
	//@{
public:
		class Impl;
private:
		Impl* impl;
		friend class LLSD::Impl;
	//@}

private:
	/** @name Debugging Interface */
	//@{
		/// Returns XML version of llsd -- only to be called from debugger
		static const char *dumpXML(const LLSD &llsd);

		/// Returns Notation version of llsd -- only to be called from debugger
		static const char *dump(const LLSD &llsd);
	//@}

public:

	static std::string		typeString(Type type);		// Return human-readable type as a string
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

namespace llsd
{

#ifdef LLSD_DEBUG_INFO
/** @name Unit Testing Interface */
//@{
	LL_COMMON_API void dumpStats(const LLSD&);	///< Output information on object and usage

	/// @warn THE FOLLOWING COUNTS WILL NOT BE ACCURATE IN A MULTI-THREADED
	/// ENVIRONMENT.
	///
	/// These counts track LLSD::Impl (hidden) objects.
	LL_COMMON_API U32 allocationCount();	///< how many Impls have been made
	LL_COMMON_API U32 outstandingCount();	///< how many Impls are still alive

	/// These counts track LLSD (public) objects.
	LL_COMMON_API extern S32 sLLSDAllocationCount;	///< Number of LLSD objects ever created
	LL_COMMON_API extern S32 sLLSDNetObjects;		///< Number of LLSD objects that exist
#endif
//@}

} // namespace llsd

/** QUESTIONS & TO DOS
	- Would Binary be more convenient as unsigned char* buffer semantics?
	- Should Binary be convertible to/from String, and if so how?
		- as UTF8 encoded strings (making not like UUID<->String)
		- as Base64 or Base96 encoded (making like UUID<->String)
	- Conversions to std::string and LLUUID do not result in easy assignment
		to std::string, std::string or LLUUID due to non-unique conversion paths
*/

#endif // LL_LLSD_NEW_H
