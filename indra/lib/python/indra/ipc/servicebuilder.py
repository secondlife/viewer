"""\
@file servicebuilder.py
@author Phoenix
@brief Class which will generate service urls.

$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007-2009, Linden Research, Inc.

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

from indra.base import config
from indra.ipc import llsdhttp
from indra.ipc import russ

# *NOTE: agent presence relies on this variable existing and being current, it is a huge hack
services_config = {}
try:
    services_config = llsdhttp.get(config.get('services-config'))
except:
    pass

_g_builder = None
def build(name, context={}, **kwargs):
    """ Convenience method for using a global, singleton, service builder.  Pass arguments either via a dict or via python keyword arguments, or both!

    Example use:
     > context = {'channel':'Second Life Release', 'version':'1.18.2.0'}
     > servicebuilder.build('version-manager-version', context)
       'http://int.util.vaak.lindenlab.com/channel/Second%20Life%20Release/1.18.2.0'
     > servicebuilder.build('version-manager-version', channel='Second Life Release', version='1.18.2.0')
       'http://int.util.vaak.lindenlab.com/channel/Second%20Life%20Release/1.18.2.0'
     > servicebuilder.build('version-manager-version', context, version='1.18.1.2')
       'http://int.util.vaak.lindenlab.com/channel/Second%20Life%20Release/1.18.1.2'
    """
    global _g_builder
    if _g_builder is None:
        _g_builder = ServiceBuilder()
    return _g_builder.buildServiceURL(name, context, **kwargs)

class ServiceBuilder(object):
    def __init__(self, services_definition = services_config):
        """\
        @brief
        @brief Create a ServiceBuilder.
        @param services_definition Complete services definition, services.xml.
        """
        # no need to keep a copy of the services section of the
        # complete services definition, but it doesn't hurt much.
        self.services = services_definition['services']
        self.builders = {}
        for service in self.services:
            service_builder = service.get('service-builder')
            if not service_builder:
                continue
            if isinstance(service_builder, dict):
                # We will be constructing several builders
                for name, builder in service_builder.items():
                    full_builder_name = service['name'] + '-' + name
                    self.builders[full_builder_name] = builder
            else:
                self.builders[service['name']] = service_builder

    def buildServiceURL(self, name, context={}, **kwargs):
        """\
        @brief given the environment on construction, return a service URL.
        @param name The name of the service.
        @param context A dict of name value lookups for the service.
        @param kwargs Any keyword arguments are treated as members of the
            context, this allows you to be all 31337 by writing shit like:
            servicebuilder.build('name', param=value)
        @returns Returns the 
        """
        context = context.copy()  # shouldn't modify the caller's dictionary
        context.update(kwargs)
        base_url = config.get('services-base-url')
        svc_path = russ.format(self.builders[name], context)
        return base_url + svc_path


def on_in(query_name, host_key, schema_key):
    """\
    @brief Constructs an on/in snippet (for running named queries)
    from a schema name and two keys referencing values stored in
    indra.xml.

    @param query_name Name of the query.
    @param host_key Logical name of destination host.  Will be
        looked up in indra.xml.
    @param schema_key Logical name of destination schema.  Will
        be looked up in indra.xml.
    """
    return "on/config:%s/in/config:%s/%s" % (host_key.strip('/'),
                                             schema_key.strip('/'),
                                             query_name.lstrip('/'))

