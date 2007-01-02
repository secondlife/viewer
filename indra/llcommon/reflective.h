/** 
 * @file reflective.h
 * @author Babbage
 * @date 2006-05-15
 * @brief Interface that must be implemented by all reflective classes.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_REFLECTIVE_H
#define LL_REFLECTIVE_H

class LLMetaClass;
class LLReflective
{
public:
	LLReflective();
	virtual ~LLReflective();
	
	virtual const LLMetaClass& getMetaClass() const = 0;
};

#endif // LL_REFLECTIVE_H
