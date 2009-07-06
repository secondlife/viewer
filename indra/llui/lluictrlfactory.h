/** 
 * @file lluictrlfactory.h
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LLUICTRLFACTORY_H
#define LLUICTRLFACTORY_H

#include "llcallbackmap.h"
#include "llinitparam.h"
#include "llxmlnode.h"
#include "llfasttimer.h"

#include <boost/function.hpp>
#include <iosfwd>
#include <stack>

class LLPanel;
class LLFloater;
class LLView;

class LLXUIParser : public LLInitParam::Parser, public LLSingleton<LLXUIParser>
{
LOG_CLASS(LLXUIParser);

protected:
	LLXUIParser();
	friend class LLSingleton<LLXUIParser>;
public:
	typedef LLInitParam::Parser::name_stack_t name_stack_t;

	/*virtual*/ std::string getCurrentElementName();
	/*virtual*/ void parserWarning(const std::string& message);
	/*virtual*/ void parserError(const std::string& message);

	void readXUI(LLXMLNodePtr node, LLInitParam::BaseBlock& block, bool silent=false);
	void writeXUI(LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const LLInitParam::BaseBlock* diff_block = NULL);

private:
	typedef std::list<std::pair<std::string, bool> >	token_list_t;

	bool readXUIImpl(LLXMLNodePtr node, const std::string& scope, LLInitParam::BaseBlock& block);
	bool readAttributes(LLXMLNodePtr nodep, LLInitParam::BaseBlock& block);

	//reader helper functions
	bool readBoolValue(void* val_ptr);
	bool readStringValue(void* val_ptr);
	bool readU8Value(void* val_ptr);
	bool readS8Value(void* val_ptr);
	bool readU16Value(void* val_ptr);
	bool readS16Value(void* val_ptr);
	bool readU32Value(void* val_ptr);
	bool readS32Value(void* val_ptr);
	bool readF32Value(void* val_ptr);
	bool readF64Value(void* val_ptr);
	bool readColor4Value(void* val_ptr);
	bool readUIColorValue(void* val_ptr);
	bool readUUIDValue(void* val_ptr);
	bool readSDValue(void* val_ptr);

	//writer helper functions
	bool writeBoolValue(const void* val_ptr, const name_stack_t&);
	bool writeStringValue(const void* val_ptr, const name_stack_t&);
	bool writeU8Value(const void* val_ptr, const name_stack_t&);
	bool writeS8Value(const void* val_ptr, const name_stack_t&);
	bool writeU16Value(const void* val_ptr, const name_stack_t&);
	bool writeS16Value(const void* val_ptr, const name_stack_t&);
	bool writeU32Value(const void* val_ptr, const name_stack_t&);
	bool writeS32Value(const void* val_ptr, const name_stack_t&);
	bool writeF32Value(const void* val_ptr, const name_stack_t&);
	bool writeF64Value(const void* val_ptr, const name_stack_t&);
	bool writeColor4Value(const void* val_ptr, const name_stack_t&);
	bool writeUIColorValue(const void* val_ptr, const name_stack_t&);
	bool writeUUIDValue(const void* val_ptr, const name_stack_t&);
	bool writeSDValue(const void* val_ptr, const name_stack_t&);

	LLXMLNodePtr getNode(const name_stack_t& stack);

private:
	Parser::name_stack_t			mNameStack;
	LLXMLNodePtr					mCurReadNode;
	// Root of the widget XML sub-tree, for example, "line_editor"
	LLXMLNodePtr					mWriteRootNode;
	S32								mLastWriteGeneration;
	LLXMLNodePtr					mLastWrittenChild;
	S32								mCurReadDepth;
};

// global static instance for registering all widget types
typedef boost::function<LLView* (LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)> LLWidgetCreatorFunc;

typedef LLRegistry<std::string, LLWidgetCreatorFunc> widget_registry_t;

template <typename DERIVED_TYPE>
class LLWidgetRegistry : public LLRegistrySingleton<std::string, LLWidgetCreatorFunc, DERIVED_TYPE>
{
public:
	typedef LLRegistrySingleton<std::string, LLWidgetCreatorFunc, DERIVED_TYPE> super_t;
	// local static instance for registering a particular widget
	template<typename T, typename PARAM_BLOCK = typename T::Params>
	class Register : public super_t::StaticRegistrar
	{
	public:
		// register with either the provided builder, or the generic templated builder
		Register(const char* tag, LLWidgetCreatorFunc func = NULL);
	};

protected:
	LLWidgetRegistry() {}
};

class LLDefaultWidgetRegistry : public LLWidgetRegistry<LLDefaultWidgetRegistry>
{
protected:
	LLDefaultWidgetRegistry() {}
	friend class LLSingleton<LLDefaultWidgetRegistry>;
};

struct LLCompareTypeID
{
	bool operator()(const std::type_info* lhs, const std::type_info* rhs) const
	{
		return lhs->before(*rhs);
	}
};


class LLWidgetTemplateRegistry 
:	public LLRegistrySingleton<const std::type_info*, std::string, LLWidgetTemplateRegistry, LLCompareTypeID>
{};

// function used to create new default widgets via LLView::getChild<T>
typedef LLView* (*dummy_widget_creator_func_t)(const std::string&);

// used to register factory functions for default widget instances
class LLDummyWidgetRegistry
:	public LLRegistrySingleton<const std::type_info*, dummy_widget_creator_func_t, LLDummyWidgetRegistry, LLCompareTypeID>
{};

extern LLFastTimer::DeclareTimer FTM_WIDGET_SETUP;
extern LLFastTimer::DeclareTimer FTM_WIDGET_CONSTRUCTION;
extern LLFastTimer::DeclareTimer FTM_INIT_FROM_PARAMS;

class LLUICtrlFactory : public LLSingleton<LLUICtrlFactory>
{
private:
	friend class LLSingleton<LLUICtrlFactory>;
	LLUICtrlFactory();
	~LLUICtrlFactory();

	// only partial specialization allowed in inner classes, so use extra dummy parameter
	template <typename T, int DUMMY>
	class ParamDefaults : public LLSingleton<ParamDefaults<T, DUMMY> > 
	{
	public:
		ParamDefaults()
		{
			// recursively initialize from base class param block
			((typename T::base_block_t&)mPrototype).fillFrom(ParamDefaults<typename T::base_block_t, DUMMY>::instance().get());
			// after initializing base classes, look up template file for this param block
			std::string* param_block_tag = LLWidgetTemplateRegistry::instance().getValue(&typeid(T));
			if (param_block_tag)
			{
				LLUICtrlFactory::loadWidgetTemplate(*param_block_tag, mPrototype);
			}
		}

		const T& get() { return mPrototype; }

	private:
		T mPrototype;
	};

	// base case for recursion, there are NO base classes of LLInitParam::BaseBlock
	template<int DUMMY>
	class ParamDefaults<LLInitParam::BaseBlock, DUMMY> : public LLSingleton<ParamDefaults<LLInitParam::BaseBlock, DUMMY> >
	{
	public:
		const LLInitParam::BaseBlock& get() { return mBaseBlock; }
	private:
		LLInitParam::BaseBlock mBaseBlock;
	};

public:

	template<typename T>
	static const T& getDefaultParams()
	{
		//#pragma message("Generating ParamDefaults")
		return ParamDefaults<T, 0>::instance().get();
	}

	void buildFloater(LLFloater* floaterp, const std::string &filename, BOOL open_floater = TRUE, LLXMLNodePtr output_node = NULL);
	LLFloater* buildFloaterFromXML(const std::string& filename, BOOL open_floater = TRUE);
	BOOL buildPanel(LLPanel* panelp, const std::string &filename, LLXMLNodePtr output_node = NULL);

	// Does what you want for LLFloaters and LLPanels
	// Returns 0 on success
	S32 saveToXML(LLView* viewp, const std::string& filename);

	std::string getCurFileName() { return mFileNames.empty() ? "" : mFileNames.back(); }

	static BOOL getAttributeColor(LLXMLNodePtr node, const std::string& name, LLColor4& color);

	LLPanel* createFactoryPanel(const std::string& name);

	void pushFactoryFunctions(const LLCallbackMap::map_t* map);
	void popFactoryFunctions();

	template<typename T>
	static T* create(typename T::Params& params, LLView* parent = NULL)
	{
		//#pragma message("Generating LLUICtrlFactory::create")
		params.fillFrom(ParamDefaults<typename T::Params, 0>::instance().get());
		//S32 foo = "test";

		if (!params.validateBlock())
		{
			llwarns << getInstance()->getCurFileName() << ": Invalid parameter block for " << typeid(T).name() << llendl;
		}
		T* widget = new T(params);
		widget->initFromParams(params);
		if (parent)
			widget->setParent(parent);
		return widget;
	}

	LLView* createFromXML(LLXMLNodePtr node, LLView* parent, const std::string& filename,  LLXMLNodePtr output_node, const widget_registry_t&  );

	static const widget_registry_t& getWidgetRegistry(LLView*);
	
	template<typename T>
	static T* createFromFile(const std::string &filename, LLView *parent, LLXMLNodePtr output_node = NULL)
	{
		//#pragma message("Generating LLUICtrlFactory::createFromFile")
		T* widget = NULL;

		std::string skinned_filename = findSkinnedFilename(filename);
		getInstance()->mFileNames.push_back(skinned_filename);
		{
			LLXMLNodePtr root_node;

			//if exporting, only load the language being exported, 			
			//instead of layering localized version on top of english			
			if (output_node)			
			{					
				if (!LLUICtrlFactory::getLocalizedXMLNode(filename, root_node))				
				{							
					llwarns << "Couldn't parse XUI file: " <<  filename  << llendl;					
					goto fail;				
				}
			}
			else if (!LLUICtrlFactory::getLayeredXMLNode(filename, root_node))
			{
				llwarns << "Couldn't parse XUI file: " << skinned_filename << llendl;
				goto fail;
			}
			
			LLView* view = getInstance()->createFromXML(root_node, parent, filename, output_node, getWidgetRegistry(parent));
			if (view)
			{
				widget = dynamic_cast<T*>(view);
				// not of right type, so delete it
				if (!widget) 
				{
					delete view;
					view = NULL;
				}
			
			}
		}
fail:
		getInstance()->mFileNames.pop_back();
		return widget;
	}

	template<class T>
	static T* getDefaultWidget(const std::string& name)
	{
		dummy_widget_creator_func_t* dummy_func = LLDummyWidgetRegistry::instance().getValue(&typeid(T));
		return dummy_func ? dynamic_cast<T*>((*dummy_func)(name)) : NULL;
	}

	template <class T> 
	static LLView* createDefaultWidget(const std::string& name) 
	{
		typename T::Params params;
		params.name(name);
		
		return create<T>(params);
	}

	template<typename T, typename PARAM_BLOCK>
	static T* defaultBuilder(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)
	{
		LLFastTimer timer(FTM_WIDGET_SETUP);

		//#pragma message("Generating LLUICtrlFactory::defaultBuilder")
		PARAM_BLOCK params(getDefaultParams<PARAM_BLOCK>());

		LLXUIParser::instance().readXUI(node, params);

		if (output_node)
		{
			// We always want to output top-left coordinates
			PARAM_BLOCK output_params(params);
			T::setupParamsForExport(output_params, parent);
			// Export only the differences between this any default params
			PARAM_BLOCK default_params(getDefaultParams<PARAM_BLOCK>());
			output_node->setName(node->getName()->mString);
			LLXUIParser::instance().writeXUI(
				output_node, output_params, &default_params);
		}

		// Apply layout transformations, usually munging rect
		T::setupParams(params, parent);

		if (!params.validateBlock())
		{
			llwarns << getInstance()->getCurFileName() << ": Invalid parameter block for " << typeid(T).name() << llendl;
		}
		T* widget;
		{
			LLFastTimer timer(FTM_WIDGET_CONSTRUCTION);
			widget = new T(params);	
		}
		{
			LLFastTimer timer(FTM_INIT_FROM_PARAMS);
			widget->initFromParams(params);
		}

		if (parent)
		{
			S32 tab_group = params.tab_group.isProvided() ? params.tab_group() : -1;
			setCtrlParent(widget, parent, tab_group);
		}

		createChildren(widget, node, output_node);

		if (!widget->postBuild())
		{
			delete widget;
			return NULL;
		}

		return widget;
	}

	static void createChildren(LLView* viewp, LLXMLNodePtr node, LLXMLNodePtr output_node = NULL);

	static bool getLayeredXMLNode(const std::string &filename, LLXMLNodePtr& root);
	
	static bool getLocalizedXMLNode(const std::string &xui_filename, LLXMLNodePtr& root);

	static void loadWidgetTemplate(const std::string& widget_tag, LLInitParam::BaseBlock& block);

private:
	//static void setCtrlValue(LLView* view, LLXMLNodePtr node);
	static void setCtrlParent(LLView* view, LLView* parent, S32 tab_group);

	// Avoid directly using LLUI and LLDir in the template code
	static std::string findSkinnedFilename(const std::string& filename);

	typedef std::deque<const LLCallbackMap::map_t*> factory_stack_t;
	factory_stack_t					mFactoryStack;

	LLPanel*		mDummyPanel;
	std::vector<std::string>	mFileNames;
};

// this is here to make gcc happy with reference to LLUICtrlFactory
template<typename DERIVED>
template<typename T, typename PARAM_BLOCK> 
LLWidgetRegistry<DERIVED>::Register<T, PARAM_BLOCK>::Register(const char* tag, LLWidgetCreatorFunc func)
:	LLWidgetRegistry<DERIVED>::StaticRegistrar(tag, func.empty() ? (LLWidgetCreatorFunc)&LLUICtrlFactory::defaultBuilder<T, PARAM_BLOCK> : func)
{
	// associate parameter block type with template .xml file
	LLWidgetTemplateRegistry::instance().defaultRegistrar().add(&typeid(PARAM_BLOCK), tag);
	// associate widget type with factory function
	LLDummyWidgetRegistry::instance().defaultRegistrar().add(&typeid(T), &LLUICtrlFactory::createDefaultWidget<T>);
}


typedef boost::function<LLPanel* (void)> LLPannelClassCreatorFunc;

// local static instance for registering a particular panel class

class LLRegisterPanelClass
:	public LLSingleton< LLRegisterPanelClass >
{
public:
	// reigister with either the provided builder, or the generic templated builder
	void addPanelClass(const std::string& tag,LLPannelClassCreatorFunc func)
	{
		mPannelClassesNames[tag] = func;
	}

	LLPanel* createPanelClass(const std::string& tag)
	{
		param_name_map_t::iterator iT =  mPannelClassesNames.find(tag);
		if(iT == mPannelClassesNames.end())
			return 0;
		return iT->second();
	}
	template<typename T>
	static T* defaultPanelClassBuilder()
	{
		T* pT = new T();
		return pT;
	}

private:
	typedef std::map< std::string, LLPannelClassCreatorFunc> param_name_map_t;
	
	param_name_map_t mPannelClassesNames;
};


// local static instance for registering a particular panel class
template<typename T>
class LLRegisterPanelClassWrapper
:	public LLRegisterPanelClass
{
public:
	// reigister with either the provided builder, or the generic templated builder
	LLRegisterPanelClassWrapper(const std::string& tag);
};


template<typename T>
LLRegisterPanelClassWrapper<T>::LLRegisterPanelClassWrapper(const std::string& tag) 
{
	LLRegisterPanelClass::instance().addPanelClass(tag,&LLRegisterPanelClass::defaultPanelClassBuilder<T>);
}


#endif //LLUICTRLFACTORY_H
