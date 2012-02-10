/** 
 * @file llinitparam.cpp
 * @brief parameter block abstraction for creating complex objects and 
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llinitparam.h"


namespace LLInitParam
{
	//
	// Param
	//
	Param::Param(BaseBlock* enclosing_block)
	:	mIsProvided(false)
	{
		const U8* my_addr = reinterpret_cast<const U8*>(this);
		const U8* block_addr = reinterpret_cast<const U8*>(enclosing_block);
		mEnclosingBlockOffset = 0x7FFFffff & (U32)(my_addr - block_addr);
	}

	//
	// ParamDescriptor
	//
	ParamDescriptor::ParamDescriptor(param_handle_t p, 
									merge_func_t merge_func, 
									deserialize_func_t deserialize_func, 
									serialize_func_t serialize_func,
									validation_func_t validation_func,
									inspect_func_t inspect_func,
									S32 min_count,
									S32 max_count)
	:	mParamHandle(p),
		mMergeFunc(merge_func),
		mDeserializeFunc(deserialize_func),
		mSerializeFunc(serialize_func),
		mValidationFunc(validation_func),
		mInspectFunc(inspect_func),
		mMinCount(min_count),
		mMaxCount(max_count),
		mUserData(NULL)
	{}

	ParamDescriptor::ParamDescriptor()
	:	mParamHandle(0),
		mMergeFunc(NULL),
		mDeserializeFunc(NULL),
		mSerializeFunc(NULL),
		mValidationFunc(NULL),
		mInspectFunc(NULL),
		mMinCount(0),
		mMaxCount(0),
		mUserData(NULL)
	{}

	ParamDescriptor::~ParamDescriptor()
	{
		delete mUserData;
	}

	//
	// Parser
	//
	Parser::~Parser()
	{}

	void Parser::parserWarning(const std::string& message)
	{
		if (mParseSilently) return;
		llwarns << message << llendl;
	}
	
	void Parser::parserError(const std::string& message)
	{
		if (mParseSilently) return;
		llerrs << message << llendl;
	}


	//
	// BlockDescriptor
	//
	void BlockDescriptor::aggregateBlockData(BlockDescriptor& src_block_data) 
	{
		mNamedParams.insert(src_block_data.mNamedParams.begin(), src_block_data.mNamedParams.end());
		std::copy(src_block_data.mUnnamedParams.begin(), src_block_data.mUnnamedParams.end(), std::back_inserter(mUnnamedParams));
		std::copy(src_block_data.mValidationList.begin(), src_block_data.mValidationList.end(), std::back_inserter(mValidationList));
		std::copy(src_block_data.mAllParams.begin(), src_block_data.mAllParams.end(), std::back_inserter(mAllParams));
	}

	BlockDescriptor::BlockDescriptor()
	:	mMaxParamOffset(0),
		mInitializationState(UNINITIALIZED),
		mCurrentBlockPtr(NULL)
	{}

	// called by each derived class in least to most derived order
	void BaseBlock::init(BlockDescriptor& descriptor, BlockDescriptor& base_descriptor, size_t block_size)
	{
		descriptor.mCurrentBlockPtr = this;
		descriptor.mMaxParamOffset = block_size;

		switch(descriptor.mInitializationState)
		{
		case BlockDescriptor::UNINITIALIZED:
			// copy params from base class here
			descriptor.aggregateBlockData(base_descriptor);

			descriptor.mInitializationState = BlockDescriptor::INITIALIZING;
			break;
		case BlockDescriptor::INITIALIZING:
			descriptor.mInitializationState = BlockDescriptor::INITIALIZED;
			break;
		case BlockDescriptor::INITIALIZED:
			// nothing to do
			break;
		}
	}

	param_handle_t BaseBlock::getHandleFromParam(const Param* param) const
	{
		const U8* param_address = reinterpret_cast<const U8*>(param);
		const U8* baseblock_address = reinterpret_cast<const U8*>(this);
		return (param_address - baseblock_address);
	}

	bool BaseBlock::submitValue(Parser::name_stack_t& name_stack, Parser& p, bool silent)
	{
		if (!deserializeBlock(p, std::make_pair(name_stack.begin(), name_stack.end()), true))
		{
			if (!silent)
			{
				p.parserWarning(llformat("Failed to parse parameter \"%s\"", p.getCurrentElementName().c_str()));
			}
			return false;
		}
		return true;
	}


	bool BaseBlock::validateBlock(bool emit_errors) const
	{
		const BlockDescriptor& block_data = mostDerivedBlockDescriptor();
		for (BlockDescriptor::param_validation_list_t::const_iterator it = block_data.mValidationList.begin(); it != block_data.mValidationList.end(); ++it)
		{
			const Param* param = getParamFromHandle(it->first);
			if (!it->second(param))
			{
				if (emit_errors)
				{
					llwarns << "Invalid param \"" << getParamName(block_data, param) << "\"" << llendl;
				}
				return false;
			}
		}
		return true;
	}

	void BaseBlock::serializeBlock(Parser& parser, Parser::name_stack_t& name_stack, const LLInitParam::BaseBlock* diff_block) const
	{
		// named param is one like LLView::Params::follows
		// unnamed param is like LLView::Params::rect - implicit
		const BlockDescriptor& block_data = mostDerivedBlockDescriptor();

		for (BlockDescriptor::param_list_t::const_iterator it = block_data.mUnnamedParams.begin(); 
			it != block_data.mUnnamedParams.end(); 
			++it)
		{
			param_handle_t param_handle = (*it)->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::serialize_func_t serialize_func = (*it)->mSerializeFunc;
			if (serialize_func)
			{
				const Param* diff_param = diff_block ? diff_block->getParamFromHandle(param_handle) : NULL;
				// each param descriptor remembers its serial number
				// so we can inspect the same param under different names
				// and see that it has the same number
				name_stack.push_back(std::make_pair("", true));
				serialize_func(*param, parser, name_stack, diff_param);
				name_stack.pop_back();
			}
		}

		for(BlockDescriptor::param_map_t::const_iterator it = block_data.mNamedParams.begin();
			it != block_data.mNamedParams.end();
			++it)
		{
			param_handle_t param_handle = it->second->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::serialize_func_t serialize_func = it->second->mSerializeFunc;
			if (serialize_func && param->anyProvided())
			{
				// Ensure this param has not already been serialized
				// Prevents <rect> from being serialized as its own tag.
				bool duplicate = false;
				for (BlockDescriptor::param_list_t::const_iterator it2 = block_data.mUnnamedParams.begin(); 
					it2 != block_data.mUnnamedParams.end(); 
					++it2)
				{
					if (param_handle == (*it2)->mParamHandle)
					{
						duplicate = true;
						break;
					}
				}

				//FIXME: for now, don't attempt to serialize values under synonyms, as current parsers
				// don't know how to detect them
				if (duplicate) 
				{
					continue;
				}

				name_stack.push_back(std::make_pair(it->first, !duplicate));
				const Param* diff_param = diff_block ? diff_block->getParamFromHandle(param_handle) : NULL;
				serialize_func(*param, parser, name_stack, diff_param);
				name_stack.pop_back();
			}
		}
	}

	bool BaseBlock::inspectBlock(Parser& parser, Parser::name_stack_t name_stack, S32 min_count, S32 max_count) const
	{
		// named param is one like LLView::Params::follows
		// unnamed param is like LLView::Params::rect - implicit
		const BlockDescriptor& block_data = mostDerivedBlockDescriptor();

		for (BlockDescriptor::param_list_t::const_iterator it = block_data.mUnnamedParams.begin(); 
			it != block_data.mUnnamedParams.end(); 
			++it)
		{
			param_handle_t param_handle = (*it)->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::inspect_func_t inspect_func = (*it)->mInspectFunc;
			if (inspect_func)
			{
				name_stack.push_back(std::make_pair("", true));
				inspect_func(*param, parser, name_stack, (*it)->mMinCount, (*it)->mMaxCount);
				name_stack.pop_back();
			}
		}

		for(BlockDescriptor::param_map_t::const_iterator it = block_data.mNamedParams.begin();
			it != block_data.mNamedParams.end();
			++it)
		{
			param_handle_t param_handle = it->second->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::inspect_func_t inspect_func = it->second->mInspectFunc;
			if (inspect_func)
			{
				// Ensure this param has not already been inspected
				bool duplicate = false;
				for (BlockDescriptor::param_list_t::const_iterator it2 = block_data.mUnnamedParams.begin(); 
					it2 != block_data.mUnnamedParams.end(); 
					++it2)
				{
					if (param_handle == (*it2)->mParamHandle)
					{
						duplicate = true;
						break;
					}
				}

				name_stack.push_back(std::make_pair(it->first, !duplicate));
				inspect_func(*param, parser, name_stack, it->second->mMinCount, it->second->mMaxCount);
				name_stack.pop_back();
			}
		}

		return true;
	}

	bool BaseBlock::deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack_range, bool ignored)
	{
		BlockDescriptor& block_data = mostDerivedBlockDescriptor();
		bool names_left = name_stack_range.first != name_stack_range.second;

		bool new_name = names_left
						? name_stack_range.first->second
						: true;

		if (names_left)
		{
			const std::string& top_name = name_stack_range.first->first;

			ParamDescriptor::deserialize_func_t deserialize_func = NULL;
			Param* paramp = NULL;

			BlockDescriptor::param_map_t::iterator found_it = block_data.mNamedParams.find(top_name);
			if (found_it != block_data.mNamedParams.end())
			{
				// find pointer to member parameter from offset table
				paramp = getParamFromHandle(found_it->second->mParamHandle);
				deserialize_func = found_it->second->mDeserializeFunc;
					
				Parser::name_stack_range_t new_name_stack(name_stack_range.first, name_stack_range.second);
				++new_name_stack.first;
				if (deserialize_func(*paramp, p, new_name_stack, new_name))
				{
					// value is no longer new, we know about it now
					name_stack_range.first->second = false;
					return true;
				}
				else
				{
					return false;
				}
			}
		}

		// try to parse unnamed parameters, in declaration order
		for ( BlockDescriptor::param_list_t::iterator it = block_data.mUnnamedParams.begin(); 
			it != block_data.mUnnamedParams.end(); 
			++it)
		{
			Param* paramp = getParamFromHandle((*it)->mParamHandle);
			ParamDescriptor::deserialize_func_t deserialize_func = (*it)->mDeserializeFunc;

			if (deserialize_func && deserialize_func(*paramp, p, name_stack_range, new_name))
			{
				return true;
			}
		}

		// if no match, and no names left on stack, this is just an existence assertion of this block
		// verify by calling readValue with NoParamValue type, an inherently unparseable type
		if (!names_left)
		{
			Flag no_value;
			return p.readValue(no_value);
		}

		return false;
	}

	//static 
	void BaseBlock::addParam(BlockDescriptor& block_data, const ParamDescriptorPtr in_param, const char* char_name)
	{
		// create a copy of the param descriptor in mAllParams
		// so other data structures can store a pointer to it
		block_data.mAllParams.push_back(in_param);
		ParamDescriptorPtr param(block_data.mAllParams.back());

		std::string name(char_name);
		if ((size_t)param->mParamHandle > block_data.mMaxParamOffset)
		{
			llerrs << "Attempted to register param with block defined for parent class, make sure to derive from LLInitParam::Block<YOUR_CLASS, PARAM_BLOCK_BASE_CLASS>" << llendl;
		}

		if (name.empty())
		{
			block_data.mUnnamedParams.push_back(param);
		}
		else
		{
			// don't use insert, since we want to overwrite existing entries
			block_data.mNamedParams[name] = param;
		}

		if (param->mValidationFunc)
		{
			block_data.mValidationList.push_back(std::make_pair(param->mParamHandle, param->mValidationFunc));
		}
	}

	void BaseBlock::addSynonym(Param& param, const std::string& synonym)
	{
		BlockDescriptor& block_data = mostDerivedBlockDescriptor();
		if (block_data.mInitializationState == BlockDescriptor::INITIALIZING)
		{
			param_handle_t handle = getHandleFromParam(&param);
			
			// check for invalid derivation from a paramblock (i.e. without using
			// Block<T, Base_Class>
			if ((size_t)handle > block_data.mMaxParamOffset)
			{
				llerrs << "Attempted to register param with block defined for parent class, make sure to derive from LLInitParam::Block<YOUR_CLASS, PARAM_BLOCK_BASE_CLASS>" << llendl;
			}

			ParamDescriptorPtr param_descriptor = findParamDescriptor(param);
			if (param_descriptor)
			{
				if (synonym.empty())
				{
					block_data.mUnnamedParams.push_back(param_descriptor);
				}
				else
				{
					block_data.mNamedParams[synonym] = param_descriptor;
				}
			}
		}
	}

	const std::string& BaseBlock::getParamName(const BlockDescriptor& block_data, const Param* paramp) const
	{
		param_handle_t handle = getHandleFromParam(paramp);
		for (BlockDescriptor::param_map_t::const_iterator it = block_data.mNamedParams.begin(); it != block_data.mNamedParams.end(); ++it)
		{
			if (it->second->mParamHandle == handle)
			{
				return it->first;
			}
		}

		return LLStringUtil::null;
	}

	ParamDescriptorPtr BaseBlock::findParamDescriptor(const Param& param)
	{
		param_handle_t handle = getHandleFromParam(&param);
		BlockDescriptor& descriptor = mostDerivedBlockDescriptor();
		BlockDescriptor::all_params_list_t::iterator end_it = descriptor.mAllParams.end();
		for (BlockDescriptor::all_params_list_t::iterator it = descriptor.mAllParams.begin();
			it != end_it;
			++it)
		{
			if ((*it)->mParamHandle == handle) return *it;
		}
		return ParamDescriptorPtr();
	}

	// take all provided params from other and apply to self
	// NOTE: this requires that "other" is of the same derived type as this
	bool BaseBlock::mergeBlock(BlockDescriptor& block_data, const BaseBlock& other, bool overwrite)
	{
		bool some_param_changed = false;
		BlockDescriptor::all_params_list_t::const_iterator end_it = block_data.mAllParams.end();
		for (BlockDescriptor::all_params_list_t::const_iterator it = block_data.mAllParams.begin();
			it != end_it;
			++it)
		{
			const Param* other_paramp = other.getParamFromHandle((*it)->mParamHandle);
			ParamDescriptor::merge_func_t merge_func = (*it)->mMergeFunc;
			if (merge_func)
			{
				Param* paramp = getParamFromHandle((*it)->mParamHandle);
				llassert(paramp->mEnclosingBlockOffset == (*it)->mParamHandle);
				some_param_changed |= merge_func(*paramp, *other_paramp, overwrite);
			}
		}
		return some_param_changed;
	}
}
