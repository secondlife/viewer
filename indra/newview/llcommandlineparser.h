/** 
 * @file llcommandlineparser.h
 * @brief LLCommandLineParser class declaration
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

#ifndef LL_LLCOMMANDLINEPARSER_H
#define LL_LLCOMMANDLINEPARSER_H

#include <boost/function/function1.hpp>

// *NOTE:Mani The following is a forward decl of 
// boost::program_options::command_line_parser
// "yay" c++!
namespace boost
{
    namespace program_options
    {
        template <class charT> class basic_command_line_parser;
        typedef basic_command_line_parser<char> command_line_parser;
    }
}


/** 
 * @class LLCommandLineParser
 * @brief Handle defining and parsing the command line.
 */
class LLCommandLineParser
{
public:
    typedef std::vector< std::string > token_vector_t;
    
    /**
     * @brief Add a value-less option to the command line description.
     * @param option_name The long name of the cmd-line option. 
     * @param description The text description of the option usage.
     */
    void addOptionDesc(
                       const std::string& option_name, 
                       boost::function1<void, const token_vector_t&> notify_callback = 0,
                       unsigned int num_tokens = 0,
                       const std::string& description = LLStringUtil::null,
                       const std::string& short_name = LLStringUtil::null,
                       bool composing = false,
                       bool positional = false,
                       bool last_option = false);
    
    /** 
     * @brief Parse the command line given by argc/argv.
     */
    bool parseCommandLine(int argc, char **argv);
    
    /** 
     * @brief Parse the command line contained by the given file.
     */
    bool parseCommandLineString(const std::string& str);
    
    /** 
     * @brief Parse the command line contained by the given file.
     */
    bool parseCommandLineFile(const std::basic_istream< char >& file);
    
    /** 
     * @brief Call callbacks associated with option descriptions.
     * 
     * Use this to handle the results of parsing. 
     */
    bool notify();
    
    /** @brief Print a description of the configured options.
     *
     * Use this to print a description of options to the
     * given ostream. Useful for displaying usage info.
     */
    std::ostream& printOptionsDesc(std::ostream& os) const;
    
    /** @brief Manual option setting accessors.
     * 
     * Use these to retrieve get the values set for an option.
     * getOption will return an empty value if the option isn't
     * set. 
     */
    bool hasOption(const std::string& name) const;
    const token_vector_t& getOption(const std::string& name) const;
    
    /** @brief Print the list of configured options.
     */
    void printOptions() const;
    
    /** @bried Get the error message, if it exists.
     */
    const std::string& getErrorMessage() const { return mErrorMsg; }
    
    /** @brief Add a custom parser func to the parser.
     *  
     * Use this method to add a custom parser for parsing values
     * the the simple parser may not handle.
     * it will be applied to each parameter before the
     * default parser gets a chance.
     * The parser_func takes an input string, and should return a
     * name/value pair as the result.
     */
    typedef boost::function1<std::pair<std::string, std::string>, const std::string&> parser_func;
    void setCustomParser(parser_func f) { mExtraParser = f; }
    
private:
    bool parseAndStoreResults(boost::program_options::command_line_parser& clp);
    std::string mErrorMsg;
    parser_func mExtraParser;
};

inline std::ostream& operator<<(std::ostream& out, const LLCommandLineParser& clp)
{
    return clp.printOptionsDesc(out);
}

class LLControlGroup; 

/** 
 * @class LLControlGroupCLP
 * @brief Uses the CLP to configure an LLControlGroup
 *
 * 
 */
class LLControlGroupCLP : public LLCommandLineParser
{
public:
    /**
     * @brief Configure the command line parser according the given config file.
     *
     * @param config_filename The name of the XML based LLSD config file. 
     * @param clp A reference to the command line parser object to configure.
     *
     * *FIX:Mani Specify config file format.
     */
    void configure(const std::string& config_filename, 
                   LLControlGroup* controlGroup);
};

#endif // LL_LLCOMMANDLINEPARSER_H
