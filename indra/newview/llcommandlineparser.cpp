/**
 * @file llcommandlineparser.cpp
 * @brief The LLCommandLineParser class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llcommandlineparser.h"
#include "llexception.h"

// *NOTE: The boost::lexical_cast generates 
// the warning C4701(local used with out assignment) in VC7.1.
// Disable the warning for the boost includes.
#if _MSC_VER
#   pragma warning(push)
#   pragma warning( disable : 4701 )
#else
// NOTE: For the other platforms?
#endif

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/assign/list_of.hpp>

#if _MSC_VER
#   pragma warning(pop)
#endif

#include "llsdserialize.h"
#include "llerror.h"
#include "stringize.h"
#include "llexception.h"
#include <string>
#include <set>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "llcontrol.h"

namespace po = boost::program_options;

// *NOTE:MEP - Currently the boost object reside in file scope.
// This has a couple of negatives, they are always around and 
// there can be only one instance of each. 
// The plus is that the boost-ly-ness of this implementation is 
// hidden from the rest of the world. 
// Its importatnt to realize that multiple LLCommandLineParser objects 
// will all have this single repository of option escs and parsed options.
// This could be good or bad, and probably won't matter for most use cases.
namespace 
{
    // List of command-line switches that can't map-to settings variables.
    // Going forward, we want every new command-line switch to map-to some
    // settings variable. This list is used to validate that.
    const std::set<std::string> unmapped_options = boost::assign::list_of
        ("help")
        ("set")
        ("setdefault")
        ("settings")
        ("sessionsettings")
        ("usersessionsettings")
    ;

    po::options_description gOptionsDesc;
    po::positional_options_description gPositionalOptions;
    po::variables_map gVariableMap;

    const LLCommandLineParser::token_vector_t gEmptyValue;

    void read_file_into_string(std::string& str, const std::basic_istream < char >& file)
    {
        std::ostringstream oss;
        oss << file.rdbuf();
        str = oss.str();
    }

    bool gPastLastOption = false;
}

class LLCLPError : public LLException {
public:
    LLCLPError(const std::string& what) : LLException(what) {}
};

class LLCLPLastOption : public LLException {
public:
    LLCLPLastOption(const std::string& what) : LLException(what) {}
};

class LLCLPValue : public po::value_semantic_codecvt_helper<char> 
{
    unsigned mMinTokens;
    unsigned mMaxTokens;
    bool mIsComposing;
    typedef boost::function1<void, const LLCommandLineParser::token_vector_t&> notify_callback_t;
    notify_callback_t mNotifyCallback;
    bool mLastOption;

public:
    LLCLPValue() :
        mMinTokens(0),
        mMaxTokens(0),
        mIsComposing(false),
        mLastOption(false)
        {}
      
    virtual ~LLCLPValue() {};

    void setMinTokens(unsigned c) 
    {
        mMinTokens = c;
    }

    void setMaxTokens(unsigned c) 
    {
        mMaxTokens = c;
    }

    void setComposing(bool c)
    {
        mIsComposing = c;
    }

    void setLastOption(bool c)
    {
        mLastOption = c;
    }

    void setNotifyCallback(notify_callback_t f)
    {
        mNotifyCallback = f;
    }

    // Overrides to support the value_semantic interface.
    virtual std::string name() const 
    { 
        const std::string arg("arg");
        const std::string args("args");
        return (max_tokens() > 1) ? args : arg; 
    }

    virtual unsigned min_tokens() const
    {
        return mMinTokens;
    }

    virtual unsigned max_tokens() const 
    {
        return mMaxTokens;
    }

    virtual bool is_composing() const 
    {
        return mIsComposing;
    }

    // Needed for boost 1.42
    virtual bool is_required() const
    {
        return false; // All our command line options are optional.
    }

    virtual bool apply_default(boost::any& value_store) const
    {
        return false; // No defaults.
    }

    virtual void notify(const boost::any& value_store) const
    {
        const LLCommandLineParser::token_vector_t* value =
            boost::any_cast<const LLCommandLineParser::token_vector_t>(&value_store);
        if(mNotifyCallback) 
        {
           mNotifyCallback(*value);
        }
    }

protected:
    void xparse(boost::any& value_store,
         const std::vector<std::string>& new_tokens) const
    {
        if(gPastLastOption)
        {
            LLTHROW(LLCLPLastOption("Don't parse no more!"));
        }

        // Error checks. Needed?
        if (!value_store.empty() && !is_composing()) 
        {
            LLTHROW(LLCLPError("Non composing value with multiple occurences."));
        }
        if (new_tokens.size() < min_tokens() || new_tokens.size() > max_tokens())
        {
            LLTHROW(LLCLPError("Illegal number of tokens specified."));
        }
        
        if(value_store.empty())
        {
            value_store = boost::any(LLCommandLineParser::token_vector_t());
        }
        LLCommandLineParser::token_vector_t* tv = 
            boost::any_cast<LLCommandLineParser::token_vector_t>(&value_store); 
       
        for(unsigned i = 0; i < new_tokens.size() && i < mMaxTokens; ++i)
        {
            tv->push_back(new_tokens[i]);
        }

        if(mLastOption)
        {
            gPastLastOption = true;
        }
    }
};

//----------------------------------------------------------------------------
// LLCommandLineParser defintions
//----------------------------------------------------------------------------
void LLCommandLineParser::addOptionDesc(const std::string& option_name, 
                                        boost::function1<void, const token_vector_t&> notify_callback,
                                        unsigned int token_count,
                                        const std::string& description,
                                        const std::string& short_name,
                                        bool composing,
                                        bool positional,
                                        bool last_option)
{
    // Compose the name for boost::po. 
    // It takes the format "long_name, short name"
    const std::string comma(",");
    std::string boost_option_name = option_name;
    if(short_name != LLStringUtil::null)
    {
        boost_option_name += comma;
        boost_option_name += short_name;
    }
   
    LLCLPValue* value_desc = new LLCLPValue();
    value_desc->setMinTokens(token_count);
    value_desc->setMaxTokens(token_count);
    value_desc->setComposing(composing);
    value_desc->setLastOption(last_option);

    boost::shared_ptr<po::option_description> d(
            new po::option_description(boost_option_name.c_str(), 
                                    value_desc, 
                                    description.c_str()));

    if(!notify_callback.empty())
    {
        value_desc->setNotifyCallback(notify_callback);
    }

    gOptionsDesc.add(d);

    if(positional)
    {
        gPositionalOptions.add(boost_option_name.c_str(), token_count);
    }
}

bool LLCommandLineParser::parseAndStoreResults(po::command_line_parser& clp)
{
    try
    {
        clp.options(gOptionsDesc);
        clp.positional(gPositionalOptions);
        // SNOW-626: Boost 1.42 erroneously added allow_guessing to the default style
        // (see http://groups.google.com/group/boost-list/browse_thread/thread/545d7bf98ff9bb16?fwc=2&pli=1)
        // Remove allow_guessing from the default style, because that is not allowed
        // when we have options that are a prefix of other options (aka, --help and --helperuri).
        clp.style((po::command_line_style::default_style & ~po::command_line_style::allow_guessing)
                  | po::command_line_style::allow_long_disguise);
        if(mExtraParser)
        {
            clp.extra_parser(mExtraParser);
        }
            
        po::basic_parsed_options<char> opts = clp.run();
        po::store(opts, gVariableMap);
    }
    catch(po::error& e)
    {
        LL_WARNS() << "Caught Error:" << e.what() << LL_ENDL;
        mErrorMsg = e.what();
        return false;
    }
    catch(LLCLPError& e)
    {
        LL_WARNS() << "Caught Error:" << e.what() << LL_ENDL;
        mErrorMsg = e.what();
        return false;
    }
    catch(LLCLPLastOption&) 
    {
        // This exception means a token was read after an option 
        // that must be the last option was reached (see url and slurl options)

        // boost::po will have stored a malformed option. 
        // All such options will be removed below.
        // The last option read, the last_option option, and its value
        // are put into the error message.
        std::string last_option;
        std::string last_value;
        for(po::variables_map::iterator i = gVariableMap.begin(); i != gVariableMap.end();)
        {
            po::variables_map::iterator tempI = i++;
            if(tempI->second.empty())
            {
                gVariableMap.erase(tempI);
            }
            else
            {
                last_option = tempI->first;
                LLCommandLineParser::token_vector_t* tv = 
                    boost::any_cast<LLCommandLineParser::token_vector_t>(&(tempI->second.value())); 
                if(!tv->empty())
                {
                    last_value = (*tv)[tv->size()-1];
                }
            }
        }

        // Continue without parsing.
        std::ostringstream msg;
        msg << "Caught Error: Found options after last option: " 
            << last_option << " "
            << last_value;

        LL_WARNS() << msg.str() << LL_ENDL;
        mErrorMsg = msg.str();
        return false;
    } 
    return true;
}

bool LLCommandLineParser::parseCommandLine(int argc, char **argv)
{
    po::command_line_parser clp(argc, argv);
    return parseAndStoreResults(clp);
}

// TODO:
// - Break out this funky parsing logic into separate method
// - Unit-test it with tests like LLStringUtil::getTokens() (the command-line
//   overload that supports quoted tokens)
// - Unless this logic offers significant semantic benefits, replace it with
//   LLStringUtil::getTokens(). This would fix a known bug: you cannot --set a
//   string-valued variable to the empty string, because empty strings are
//   eliminated below.

bool LLCommandLineParser::parseCommandLineString(const std::string& str)
{
    std::string cmd_line_string("");
    if (!str.empty())
    {
        bool add_last_c = true;
        S32 last_c_pos = str.size() - 1; //don't get out of bounds on pos+1, last char will be processed separately
        for (S32 pos = 0; pos < last_c_pos; ++pos)
        {
            cmd_line_string.append(&str[pos], 1);
            if (str[pos] == '\\')
            {
                cmd_line_string.append("\\", 1);
                if (str[pos + 1] == '\\')
                {
                    ++pos;
                    add_last_c = (pos != last_c_pos);
                }
            }
        }
        if (add_last_c)
        {
            cmd_line_string.append(&str[last_c_pos], 1);
            if (str[last_c_pos] == '\\')
            {
                cmd_line_string.append("\\", 1);
            }
        }
    }

    std::vector<std::string> tokens;
    try
    {
        // Split the string content into tokens
        const char* escape_chars = "\\";
        const char* separator_chars = "\r\n ";
        const char* quote_chars = "\"'";
        boost::escaped_list_separator<char> sep(escape_chars, separator_chars, quote_chars);
        boost::tokenizer< boost::escaped_list_separator<char> > tok(cmd_line_string, sep);
        // std::copy(tok.begin(), tok.end(), std::back_inserter(tokens));
        for (boost::tokenizer< boost::escaped_list_separator<char> >::iterator i = tok.begin();
            i != tok.end();
            ++i)
        {
            if (0 != i->size())
            {
                tokens.push_back(*i);
            }
        }
    }
    catch (...)
    {
        CRASH_ON_UNHANDLED_EXCEPTION(STRINGIZE("Unexpected crash while parsing: " << str));
    }

    po::command_line_parser clp(tokens);
    return parseAndStoreResults(clp);
        
}

bool LLCommandLineParser::parseCommandLineFile(const std::basic_istream < char >& file)
{
    std::string args;
    read_file_into_string(args, file);

    return parseCommandLineString(args);
}

bool LLCommandLineParser::notify()
{
    try
    {
        po::notify(gVariableMap);
        return true;
    }
    catch (const LLCLPError& e)
    {
        LL_WARNS() << "Caught Error: " << e.what() << LL_ENDL;
        mErrorMsg = e.what();
        return false;
    }
}

void LLCommandLineParser::printOptions() const
{
    for(po::variables_map::iterator i = gVariableMap.begin(); i != gVariableMap.end(); ++i)
    {
        std::string name = i->first;
        token_vector_t values = i->second.as<token_vector_t>();
        std::ostringstream oss;
        oss << name << ": ";
        for(token_vector_t::iterator t_itr = values.begin(); t_itr != values.end(); ++t_itr)
        {
            oss << t_itr->c_str() << " ";
        }
        LL_INFOS() << oss.str() << LL_ENDL;
    }
}

std::ostream& LLCommandLineParser::printOptionsDesc(std::ostream& os) const
{
    return os << gOptionsDesc;
}

bool LLCommandLineParser::hasOption(const std::string& name) const
{
    return gVariableMap.count(name) > 0;
}

const LLCommandLineParser::token_vector_t& LLCommandLineParser::getOption(const std::string& name) const
{
    if(hasOption(name))
    {
        return gVariableMap[name].as<token_vector_t>();
    }

    return gEmptyValue;
}

//----------------------------------------------------------------------------
// LLControlGroupCLP defintions
//----------------------------------------------------------------------------
namespace {
LLCommandLineParser::token_vector_t::value_type
onevalue(const std::string& option,
         const LLCommandLineParser::token_vector_t& value)
{
    if (value.empty())
    {
        // What does it mean when the user specifies a command-line switch
        // that requires a value, but omits the value? Complain.
        LLTHROW(LLCLPError(STRINGIZE("No value specified for --" << option << "!")));
    }
    else if (value.size() > 1)
    {
        LL_WARNS() << "Ignoring extra tokens specified for --"
                << option << "." << LL_ENDL; 
    }
    return value[0];
}

void badvalue(const std::string& option,
              const std::string& varname,
              const std::string& type,
              const std::string& value)
{
    // If the user passes an unusable value for a command-line switch, it
    // seems like a really bad idea to just ignore it, even with a log
    // warning.
    LLTHROW(LLCLPError(STRINGIZE("Invalid value specified by command-line switch '" << option
                                 << "' for variable '" << varname << "' of type " << type
                                 << ": '" << value << "'")));
}

template <typename T>
T convertTo(const std::string& option,
            const std::string& varname,
            const LLCommandLineParser::token_vector_t::value_type& value)
{
    try
    {
        return boost::lexical_cast<T>(value);
    }
    catch (const boost::bad_lexical_cast&)
    {
        badvalue(option, varname, typeid(T).name(), value);
        // bogus return; compiler unaware that badvalue() won't return
        return T();
    }
}

void setControlValueCB(const LLCommandLineParser::token_vector_t& value, 
                       const std::string& option, 
                       LLControlVariable* ctrl)
{
    // *FIX: Do semantic conversion here.
    // LLSD (ImplString) Is no good for doing string to type conversion for...
    // booleans
    // compound types
    // ?...

    if(NULL != ctrl)
    {
        switch(ctrl->type())
        {
        case TYPE_BOOLEAN:
            if (value.empty())
            {
                // Boolean-valued command-line switches are unusual. If you
                // simply specify the switch without an explicit value, we can
                // infer you mean 'true'.
                ctrl->setValue(LLSD(true), false);
            }
            else
            {
                // Only call onevalue() AFTER handling value.empty() case!
                std::string token(onevalue(option, value));
            
                // There's a token. check the string for true/false/1/0 etc.
                BOOL result = false;
                BOOL gotSet = LLStringUtil::convertToBOOL(token, result);
                if (gotSet)
                {
                    ctrl->setValue(LLSD(result), false);
                }
                else
                {
                    badvalue(option, ctrl->getName(), "bool", token);
                }
            }
            break;

        case TYPE_U32:
        {
            std::string token(onevalue(option, value));
            // To my surprise, for an unsigned target, lexical_cast() doesn't
            // complain about an input string such as "-17". In that case, you
            // get a very large positive result. So for U32, make sure there's
            // no minus sign!
            if (token.find('-') == std::string::npos)
            {
                ctrl->setValue(LLSD::Integer(convertTo<U32>(option, ctrl->getName(), token)),
                               false);
            }
            else
            {
                badvalue(option, ctrl->getName(), "unsigned", token);
            }
            break;
        }

        case TYPE_S32:
            ctrl->setValue(convertTo<S32>(option, ctrl->getName(),
                                          onevalue(option, value)), false);
            break;

        case TYPE_F32:
            ctrl->setValue(convertTo<F32>(option, ctrl->getName(),
                                          onevalue(option, value)), false);
            break;

        // It appears that no one has yet tried to define a command-line
        // switch mapped to a settings variable of TYPE_VEC3, TYPE_VEC3D,
        // TYPE_RECT, TYPE_COL4, TYPE_COL3. Such types would certainly seem to
        // call for a bit of special handling here...
        default:
            {
                // For the default types, let llsd do the conversion.
                if(value.size() > 1 && ctrl->isType(TYPE_LLSD))
                {
                    // Assume its an array...
                    LLSD llsdArray;
                    for(unsigned int i = 0; i < value.size(); ++i)
                    {
                        LLSD llsdValue;
                        llsdValue.assign(LLSD::String(value[i]));
                        llsdArray.set(i, llsdValue);
                    }

                    ctrl->setValue(llsdArray, false);
                }
                else
                {
                    ctrl->setValue(onevalue(option, value), false);
                }
            }
            break;
        }
    }
    else
    {
        // This isn't anything a user can affect -- it's a misconfiguration on
        // the part of the coder. Rub the coder's nose in the problem right
        // away so even preliminary testing will surface it.
        LL_ERRS() << "Command Line option --" << option
               << " maps to unknown setting!" << LL_ENDL;
    }
}
} // anonymous namespace

void LLControlGroupCLP::configure(const std::string& config_filename, LLControlGroup* controlGroup)
{
    // This method reads the llsd based config file, and uses it to set 
    // members of a control group.
    LLSD clpConfigLLSD;
    
    llifstream input_stream;
    input_stream.open(config_filename.c_str(), std::ios::in | std::ios::binary);

    if(input_stream.is_open())
    {
        LLSDSerialize::fromXML(clpConfigLLSD, input_stream);
        for(LLSD::map_iterator option_itr = clpConfigLLSD.beginMap(); 
            option_itr != clpConfigLLSD.endMap(); 
            ++option_itr)
        {
            LLSD::String long_name = option_itr->first;
            LLSD option_params = option_itr->second;
            
            std::string desc("n/a");
            if(option_params.has("desc"))
            {
                desc = option_params["desc"].asString();
            }
            
            std::string short_name = LLStringUtil::null;
            if(option_params.has("short"))
            {
                short_name = option_params["short"].asString();
            }

            unsigned int token_count = 0;
            if(option_params.has("count"))
            {
                token_count = option_params["count"].asInteger();
            }

            bool composing = false;
            if(option_params.has("compose"))
            {
                composing = option_params["compose"].asBoolean();
            }

            bool positional = false;
            if(option_params.has("positional"))
            {
                positional = option_params["positional"].asBoolean();
            }

            bool last_option = false;
            if(option_params.has("last_option"))
            {
                last_option = option_params["last_option"].asBoolean();
            }

            boost::function1<void, const token_vector_t&> callback;
            if (! option_params.has("map-to"))
            {
                // If this option isn't mapped to a settings variable, is it
                // one of the ones for which that's unreasonable, or did
                // someone carelessly add a new option? (Make all these
                // configuration errors fatal so a maintainer will catch them
                // right away.)
                std::set<std::string>::const_iterator found = unmapped_options.find(long_name);
                if (found == unmapped_options.end())
                {
                    LL_ERRS() << "New command-line option " << long_name
                           << " should map-to a variable in settings.xml" << LL_ENDL;
                }
            }
            else                    // option specifies map-to
            {
                std::string controlName = option_params["map-to"].asString();
                if (! controlGroup)
                {
                    LL_ERRS() << "Must pass gSavedSettings to LLControlGroupCLP::configure() for "
                           << long_name << " (map-to " << controlName << ")" << LL_ENDL;
                }

                LLControlVariable* ctrl = controlGroup->getControl(controlName);
                if (! ctrl)
                {
                    LL_ERRS() << "Option " << long_name << " specifies map-to " << controlName
                           << " which does not exist" << LL_ENDL;
                }

                callback = boost::bind(setControlValueCB, _1, long_name, ctrl);
            }

            this->addOptionDesc(
                long_name, 
                callback,
                token_count, 
                desc, 
                short_name, 
                composing,
                positional,
                last_option);
        }
    }
}
