/** 
 * @file metaclass.h
 * @author Babbage
 * @date 2006-05-15
 * @brief Reflective meta information describing a class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_METACLASS_H
#define LL_METACLASS_H

#include <string>
#include <map>

#include "stdtypes.h"

class LLReflective;
class LLMetaProperty;
class LLMetaMethod;
class LLMetaClass
{
public:

	LLMetaClass();
	virtual ~LLMetaClass();
  
	// Create instance of this MetaClass. NULL if class is abstract. 
	// Gives ownership of returned object.
	// virtual LLReflective* create() const = 0;
  
	// Returns named property or NULL.
	const LLMetaProperty* findProperty(const std::string& name) const;
  
	// Add property to metaclass. Takes ownership of given property.
	void addProperty(const LLMetaProperty* property);
	
	typedef std::map<std::string, const LLMetaProperty*>::const_iterator PropertyIterator;
	
	U32 getPropertyCount() const;
	
	PropertyIterator beginProperties() const;
	PropertyIterator endProperties() const;
  
	// Returns named property or NULL.
	// const LLMetaMethod* findMethod(const std::string& name) const;
  
	// Add method to metaclass. Takes ownership of given method.
	// void addMethod(const LLMetaMethod* method);
  
	// Find MetaClass by name. NULL if name is unknown.
	// static LLMetaClass* findClass(const std::string& name);
	
	// True if object is instance of this meta class.
	bool isInstance(const LLReflective* object) const;
  
private:

	typedef std::map<std::string, const LLMetaProperty*> PropertyMap;
	PropertyMap mProperties;
};

#endif // LL_METACLASS_H
