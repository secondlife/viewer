/** 
 * @file metapropertyt.h
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
inline const LLReflective* LLMetaPropertyT<LLUUID>::get(const LLReflective* object) const
{
	checkObjectClass(object);
	return NULL;
}

template <>
inline const LLReflective* LLMetaPropertyT<bool>::get(const LLReflective* object) const
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
inline LLSD LLMetaPropertyT<LLUUID>::getLLSD(const LLReflective* object) const
{
	return *(getProperty(object));
}

template <>
inline LLSD LLMetaPropertyT<bool>::getLLSD(const LLReflective* object) const
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
