"""\
@file russ.py
@brief Recursive URL Substitution Syntax helpers
@author Phoenix

Many details on how this should work is available on the wiki:
https://wiki.secondlife.com/wiki/Recursive_URL_Substitution_Syntax

Adding features to this should be reflected in that page in the
implementations section.

Copyright (c) 2007, Linden Research, Inc.
$License$
"""

import urllib
from indra.ipc import llsdhttp

class UnbalancedBraces(Exception):
    pass

class UnknownDirective(Exception):
    pass

class BadDirective(Exception):
    pass

def format_value_for_path(value):
    if type(value) in [list, tuple]:
        # *NOTE: treat lists as unquoted path components so that the quoting
        # doesn't get out-of-hand.  This is a workaround for the fact that
        # russ always quotes, even if the data it's given is already quoted,
        # and it's not safe to simply unquote a path directly, so if we want
        # russ to substitute urls parts inside other url parts we always
        # have to do so via lists of unquoted path components.
        return '/'.join([urllib.quote(str(item)) for item in value])
    else:
        return urllib.quote(str(value))

def format(format_str, context):
    """@brief Format format string according to rules for RUSS.
@see https://osiris.lindenlab.com/mediawiki/index.php/Recursive_URL_Substitution_Syntax
@param format_str The input string to format.
@param context A map used for string substitutions.
@return Returns the formatted string. If no match, the braces remain intact.
"""
    while True:
        #print "format_str:", format_str
        all_matches = _find_sub_matches(format_str)
        if not all_matches:
            break
        substitutions = 0
        while True:
            matches = all_matches.pop()
            # we work from right to left to make sure we do not
            # invalidate positions earlier in format_str
            matches.reverse()
            for pos in matches:
                # Use index since _find_sub_matches should have raised
                # an exception, and failure to find now is an exception.
                end = format_str.index('}', pos)
                #print "directive:", format_str[pos+1:pos+5]
                if format_str[pos + 1] == '$':
                    value = context.get(format_str[pos + 2:end])
                    if value is not None:
                        value = format_value_for_path(value)
                elif format_str[pos + 1] == '%':
                    value = _build_query_string(
                        context.get(format_str[pos + 2:end]))
                elif format_str[pos+1:pos+5] == 'http' or format_str[pos+1:pos+5] == 'file':
                    value = _fetch_url_directive(format_str[pos + 1:end])
                else:
                    raise UnknownDirective, format_str[pos:end + 1]
                if not value == None:
                    format_str = format_str[:pos]+str(value)+format_str[end+1:]
                    substitutions += 1

            # If there were any substitutions at this depth, re-parse
            # since this may have revealed new things to substitute
            if substitutions:
                break
            if not all_matches:
                break

        # If there were no substitutions at all, and we have exhausted
        # the possible matches, bail.
        if not substitutions:
            break
    return format_str

def _find_sub_matches(format_str):
    """@brief Find all of the substitution matches.
@param format_str the RUSS conformant format string.	
@return Returns an array of depths of arrays of positional matches in input.
"""
    depth = 0
    matches = []
    for pos in range(len(format_str)):
        if format_str[pos] == '{':
            depth += 1
            if not len(matches) == depth:
                matches.append([])
            matches[depth - 1].append(pos)
            continue
        if format_str[pos] == '}':
            depth -= 1
            continue
    if not depth == 0:
        raise UnbalancedBraces, format_str
    return matches

def _build_query_string(query_dict):
    """\
    @breif given a dict, return a query string. utility wrapper for urllib.
    @param query_dict input query dict
    @returns Returns an urlencoded query string including leading '?'.
    """
    if query_dict:
        return '?' + urllib.urlencode(query_dict)
    else:
        return ''

def _fetch_url_directive(directive):
    "*FIX: This only supports GET"
    commands = directive.split('|')
    resource = llsdhttp.get(commands[0])
    if len(commands) == 3:
        resource = _walk_resource(resource, commands[2])
    return resource

def _walk_resource(resource, path):
    path = path.split('/')
    for child in path:
        if not child:
            continue
        resource = resource[child]
    return resource
