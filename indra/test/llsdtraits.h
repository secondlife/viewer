#ifndef LLSDTRAITS_H
#define LLSDTRAITS_H

#include "llsd.h"
#include "llstring.h"

template<class T>
class LLSDTraits
{
 protected:
	typedef T (LLSD::*Getter)() const;
	
	LLSD::Type type;
	Getter getter;
	
 public:
	LLSDTraits();
	
	T get(const LLSD& actual)
		{
			return (actual.*getter)();
		}
	
	bool checkType(const LLSD& actual)
		{
			return actual.type() == type;
		}
};

template<> inline
LLSDTraits<LLSD::Boolean>::LLSDTraits()
	: type(LLSD::TypeBoolean), getter(&LLSD::asBoolean)
{ }

template<> inline
LLSDTraits<LLSD::Integer>::LLSDTraits()
	: type(LLSD::TypeInteger), getter(&LLSD::asInteger)
{ }

template<> inline
LLSDTraits<LLSD::Real>::LLSDTraits()
	: type(LLSD::TypeReal), getter(&LLSD::asReal)
{ }

template<> inline
LLSDTraits<LLSD::UUID>::LLSDTraits()
	: type(LLSD::TypeUUID), getter(&LLSD::asUUID)
{ }

template<> inline
LLSDTraits<LLSD::String>::LLSDTraits()
	: type(LLSD::TypeString), getter(&LLSD::asString)
{ }

template<>
class LLSDTraits<LLString> : public LLSDTraits<LLSD::String>
{ };

template<>
class LLSDTraits<const char*> : public LLSDTraits<LLSD::String>
{ };

template<> inline
LLSDTraits<LLSD::Date>::LLSDTraits()
	: type(LLSD::TypeDate), getter(&LLSD::asDate)
{ }

template<> inline
LLSDTraits<LLSD::URI>::LLSDTraits()
	: type(LLSD::TypeURI), getter(&LLSD::asURI)
{ }

template<> inline
LLSDTraits<LLSD::Binary>::LLSDTraits()
	: type(LLSD::TypeBinary), getter(&LLSD::asBinary)
{ }

#endif // LLSDTRAITS_H
