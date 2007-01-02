/** 
 * @file metaproperty.h
 * @author Babbage
 * @date 2006-05-15
 * @brief Reflective meta information describing a property of a class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_METAPROPERTY_H
#define LL_METAPROPERTY_H

#include "stdtypes.h"
#include "llsd.h"
#include "reflective.h"

class LLMetaClass;
class LLReflective;
class LLMetaProperty
{
public:
	LLMetaProperty(const std::string& name, const LLMetaClass& object_class);
  virtual ~LLMetaProperty();
  
  // Get property name.
  const std::string& getName() const {return mName;}

  // Get value of this property.
  virtual const LLReflective* get(const LLReflective* object) const = 0;
  
  // Set value of this property.
  // virtual void set(LLReflective* object, const LLReflective* value) = 0;
  
  // Get value of this property as LLSD. Default returns undefined LLSD.
  virtual LLSD getLLSD(const LLReflective* object) const = 0;
  
  // Get the MetaClass of legal values of this property.
  // const LLMetaClass& getValueMetaClass();
  
  // Get the meta class that this property is a member of.
  const LLMetaClass& getObjectMetaClass() const;

protected:

  // Check object is instance of object class, throw exception if not.
  void checkObjectClass(const LLReflective* object) const;

private:

	std::string mName;
	const LLMetaClass& mObjectClass;
};

#endif // LL_METAPROPERTY_H
