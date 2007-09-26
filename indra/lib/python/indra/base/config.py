"""\
@file config.py
@brief Utility module for parsing and accessing the indra.xml config file.

Copyright (c) 2006-2007, Linden Research, Inc.
$License$
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

def as_dict():
    global _g_config_dict
    return _g_config_dict
