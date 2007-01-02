/** 
 * @file reflectivet.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_REFLECTIVET_H
#define LL_REFLECTIVET_H

#include "reflective.h"

template <class T>
class LLReflectiveT : public LLReflective
{
public:

	LLReflectiveT(const T& value) : mValue(value) {;}
	virtual ~LLReflectiveT() {;}
	
	virtual const LLMetaClass& getMetaClass() const {return LLMetaClassT<LLReflectiveT<T> >::instance();}
	
	const T& getValue() const {return mValue;}
	
private:

	T mValue;
};

#endif
