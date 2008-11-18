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

import copy
import errno
import os
import traceback
import time
import types

from os.path import dirname, getmtime, join, realpath
from indra.base import llsd

_g_config = None

class IndraConfig(object):
    """
    IndraConfig loads a 'indra' xml configuration file
    and loads into memory.  This representation in memory
    can get updated to overwrite values or add new values.

    The xml configuration file is considered a live file and changes
    to the file are checked and reloaded periodically.  If a value had
    been overwritten via the update or set method, the loaded values
    from the file are ignored (the values from the update/set methods
    override)
    """
    def __init__(self, indra_config_file):
        self._indra_config_file = indra_config_file
        self._reload_check_interval = 30 # seconds
        self._last_check_time = 0
        self._last_mod_time = 0

        self._config_overrides = {}
        self._config_file_dict = {}
        self._combined_dict = {}

        self._load()

    def _load(self):
        # if you initialize the IndraConfig with None, no attempt
        # is made to load any files
        if self._indra_config_file is None:
            return

        config_file = open(self._indra_config_file)
        self._config_file_dict = llsd.parse(config_file.read())
        self._combine_dictionaries()
        config_file.close()

        self._last_mod_time = self._get_last_modified_time()
        self._last_check_time = time.time() # now

    def _get_last_modified_time(self):
        """
        Returns the mtime (last modified time) of the config file,
        if such exists.
        """
        if self._indra_config_file is not None:
            return os.path.getmtime(self._indra_config_file)

        return 0

    def _combine_dictionaries(self):
        self._combined_dict = {}
        self._combined_dict.update(self._config_file_dict)
        self._combined_dict.update(self._config_overrides)

    def _reload_if_necessary(self):
        now = time.time()

        if (now - self._last_check_time) > self._reload_check_interval:
            self._last_check_time = now
            try:
                modtime = self._get_last_modified_time()
                if modtime > self._last_mod_time:
                    self._load()
            except OSError, e:
                if e.errno == errno.ENOENT: # file not found
                    # someone messed with our internal state
                    # or removed the file

                    print 'WARNING: Configuration file has been removed ' + (self._indra_config_file)
                    print 'Disabling reloading of configuration file.'

                    traceback.print_exc()

                    self._indra_config_file = None
                    self._last_check_time = 0
                    self._last_mod_time = 0
                else:
                    raise  # pass the exception along to the caller

    def __getitem__(self, key):
        self._reload_if_necessary()

        return self._combined_dict[key]

    def get(self, key, default = None):
        try:
            return self.__getitem__(key)
        except KeyError:
            return default

    def __setitem__(self, key, value):
        """
        Sets the value of the config setting of key to be newval

        Once any key/value pair is changed via the set method,
        that key/value pair will remain set with that value until
        change via the update or set method
        """
        self._config_overrides[key] = value
        self._combine_dictionaries()

    def set(self, key, newval):
        return self.__setitem__(key, newval)

    def update(self, new_conf):
        """
        Load an XML file and apply its map as overrides or additions
        to the existing config.  Update can be a file or a dict.

        Once any key/value pair is changed via the update method,
        that key/value pair will remain set with that value until
        change via the update or set method
        """
        if isinstance(new_conf, dict):
            overrides = new_conf
        else:
            # assuming that it is a filename
            config_file = open(new_conf)
            overrides = llsd.parse(config_file.read())
            config_file.close()
  
        self._config_overrides.update(overrides)
        self._combine_dictionaries()

    def as_dict(self):
        """
        Returns immutable copy of the IndraConfig as a dictionary
        """
        return copy.deepcopy(self._combined_dict)

def load(config_xml_file = None):
    global _g_config

    load_default_files = config_xml_file is None
    if load_default_files:
        ## going from:
        ## "/opt/linden/indra/lib/python/indra/base/config.py"
        ## to:
        ## "/opt/linden/etc/indra.xml"
        config_xml_file = realpath(
            dirname(realpath(__file__)) + "../../../../../../etc/indra.xml")

    try:
        _g_config = IndraConfig(config_xml_file)
    except IOError:
        # Failure to load passed in file
        # or indra.xml default file
        if load_default_files:
            try:
                config_xml_file = realpath(
                    dirname(realpath(__file__)) + "../../../../../../etc/globals.xml")
                _g_config = IndraConfig(config_xml_file)
                return
            except IOError:
                # Failure to load globals.xml
                # fall to code below
                pass

        # Either failed to load passed in file
        # or failed to load all default files
        _g_config = IndraConfig(None)

def dump(indra_xml_file, indra_cfg = None, update_in_mem=False):
    '''
    Dump config contents into a file
    Kindof reverse of load.
    Optionally takes a new config to dump.
    Does NOT update global config unless requested.
    '''
    global _g_config

    if not indra_cfg:
        if _g_config is None:
            return

        indra_cfg = _g_config.as_dict()

    if not indra_cfg:
        return

    config_file = open(indra_xml_file, 'w')
    _config_xml = llsd.format_xml(indra_cfg)
    config_file.write(_config_xml)
    config_file.close()

    if update_in_mem:
        update(indra_cfg)

def update(new_conf):
    global _g_config

    if _g_config is None:
        # To keep with how this function behaved
        # previously, a call to update
        # before the global is defined
        # make a new global config which does not
        # load data from a file.
        _g_config = IndraConfig(None)

    return _g_config.update(new_conf)

def get(key, default = None):
    global _g_config

    if _g_config is None:
        load()

    return _g_config.get(key, default)

def set(key, newval):
    """
    Sets the value of the config setting of key to be newval

    Once any key/value pair is changed via the set method,
    that key/value pair will remain set with that value until
    change via the update or set method or program termination
    """
    global _g_config

    if _g_config is None:
        _g_config = IndraConfig(None)

    _g_config.set(key, newval)

def get_config():
    global _g_config
    return _g_config
