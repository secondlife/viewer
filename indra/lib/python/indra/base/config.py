"""\
@file config.py
@brief Utility module for parsing and accessing the indra.xml config file.

$LicenseInfo:firstyear=2006&license=mit$

Copyright (c) 2006-2007, Linden Research, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
$/LicenseInfo$
"""

from os.path import dirname, join, realpath
import types
from indra.base import llsd

_g_config_dict = None

def load(indra_xml_file=None):
    global _g_config_dict
    if _g_config_dict == None:
        if indra_xml_file is None:
            ## going from:
            ## "/opt/linden/indra/lib/python/indra/base/config.py"
            ## to:
            ## "/opt/linden/etc/indra.xml"
            indra_xml_file = realpath(
                dirname(realpath(__file__)) + "../../../../../../etc/indra.xml")
        config_file = file(indra_xml_file)
        _g_config_dict = llsd.LLSD().parse(config_file.read())
        config_file.close()
        #print "loaded config from",indra_xml_file,"into",_g_config_dict

def dump(indra_xml_file, indra_cfg={}, update_in_mem=False):
    '''
    Dump config contents into a file
    Kindof reverse of load.
    Optionally takes a new config to dump.
    Does NOT update global config unless requested.
    '''
    global _g_config_dict
    if not indra_cfg:
        indra_cfg = _g_config_dict
    if not indra_cfg:
        return
    config_file = open(indra_xml_file, 'w')
    _config_xml = llsd.format_xml(indra_cfg)
    config_file.write(_config_xml)
    config_file.close()
    if update_in_mem:
        update(indra_cfg)

def update(new_conf):
    """Load an XML file and apply its map as overrides or additions
    to the existing config.  The dataserver does this with indra.xml
    and dataserver.xml."""
    global _g_config_dict
    if _g_config_dict == None:
        _g_config_dict = {}
    if isinstance(new_conf, dict):
        overrides = new_conf
    else:
        config_file = file(new_conf)
        overrides = llsd.LLSD().parse(config_file.read())
        config_file.close()
        
    _g_config_dict.update(overrides)

def get(key, default = None):
    global _g_config_dict
    if _g_config_dict == None:
        load()
    return _g_config_dict.get(key, default)

def set(key, newval):
    global _g_config_dict
    if _g_config_dict == None:
        load()
    _g_config_dict[key] = newval

def as_dict():
    global _g_config_dict
    return _g_config_dict
