/** 
 * @file metapropertyt.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_METAPROPERTYT_H
#define LL_METAPROPERTYT_H

#include "llsd.h"
#include "llstring.h"
#include "metaclasst.h"
#include "metaproperty.h"
#include "reflectivet.h"

template<class TProperty>
class LLMetaPropertyT : public LLMetaProperty
{
public:
	
	virtual ~LLMetaPropertyT() {;}
	
	// Get value of this property. Gives ownership of returned value.
	virtual const LLReflective* get(const LLReflective* object) const
	{
		checkObjectClass(object);
		return getProperty(object);
	}
  
	// Set value of this property.
	/*virtual void set(LLReflective* object, const LLReflective* value)
	{
		// TODO: Babbage: Check types.
		ref(object) = static_cast<const LLReflectiveT<TProperty>* >(value)->getValue();
	}*/
	
	// Get value of this property as LLSD.
	virtual LLSD getLLSD(const LLReflective* object) const
	{
		return LLSD();
	}

protected:

	LLMetaPropertyT(const std::string& name, const LLMetaClass& object_class) : LLMetaProperty(name, object_class) {;}

	virtual const TProperty* getProperty(const LLReflective* object) const = 0;
};

template <>
inline const LLReflective* LLMetaPropertyT<S32>::get(const LLReflective* object) const
{
	checkObjectClass(object);
	return NULL;
}

template <>
inline const LLReflective* LLMetaPropertyT<std::string>::get(const LLReflective* object) const
{
	checkObjectClass(object);
	return NULL;
}

template <>
inline const LLReflective* LLMetaPropertyT<LLString>::get(const LLReflective* object) const
{
	checkObjectClass(object);
	return NULL;
}

template <>
inline const LLReflective* LLMetaPropertyT<LLUUID>::get(const LLReflective* object) const
{
	checkObjectClass(object);
	return NULL;
}

template <>
inline LLSD LLMetaPropertyT<S32>::getLLSD(const LLReflective* object) const
{
	return *(getProperty(object));
}

template <>
inline LLSD LLMetaPropertyT<std::string>::getLLSD(const LLReflective* object) const
{
	return *(getProperty(object));
}

template <>
inline LLSD LLMetaPropertyT<LLString>::getLLSD(const LLReflective* object) const
{
	return *(getProperty(object));
}

template <>
inline LLSD LLMetaPropertyT<LLUUID>::getLLSD(const LLReflective* object) const
{
	return *(getProperty(object));
}

template<class TObject, class TProperty>
class LLMetaPropertyTT : public LLMetaPropertyT<TProperty>
{
public:

	LLMetaPropertyTT(const std::string& name, const LLMetaClass& object_class, TProperty (TObject::*property)) : 
	  LLMetaPropertyT<TProperty>(name, object_class), mProperty(property) {;}

protected:

	// Get void* to property.
	virtual const TProperty* getProperty(const LLReflective* object) const
	{
		const TObject* typed_object = static_cast<const TObject*>(object);
		return &(typed_object->*mProperty);
	};

private:

	TProperty (TObject::*mProperty);
};

template<class TObject, class TProperty>
class LLMetaPropertyPtrTT : public LLMetaPropertyT<TProperty>
{
public:

	LLMetaPropertyPtrTT(const std::string& name, const LLMetaClass& object_class, TProperty* (TObject::*property)) : 
	  LLMetaPropertyT<TProperty>(name, object_class), mProperty(property) {;}

protected:

	// Get void* to property.
	virtual const TProperty* getProperty(const LLReflective* object) const
	{
		const TObject* typed_object = static_cast<const TObject*>(object);
		return typed_object->*mProperty;
	};

private:

	TProperty* (TObject::*mProperty);
};

// Utility function to simplify the registration of members.
template<class TObject, class TProperty>
void reflectProperty(LLMetaClass& meta_class, const std::string& name, TProperty (TObject::*property))
{
	typedef LLMetaPropertyTT<TObject, TProperty> PropertyType;
	const LLMetaProperty* meta_property = new PropertyType(name, meta_class, property);
	meta_class.addProperty(meta_property);
}

// Utility function to simplify the registration of ptr properties.
template<class TObject, class TProperty>
void reflectPtrProperty(LLMetaClass& meta_class, const std::string& name, TProperty* (TObject::*property))
{
	typedef LLMetaPropertyPtrTT<TObject, TProperty> PropertyType;
	const LLMetaProperty* meta_property = new PropertyType(name, meta_class, property);
	meta_class.addProperty(meta_property);
}

#endif // LL_METAPROPERTYT_H
