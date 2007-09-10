"""\
@file servicebuilder.py
@author Phoenix
@brief Class which will generate service urls.

Copyright (c) 2007, Linden Research, Inc.
$License$
"""

from indra.base import config
from indra.ipc import llsdhttp
from indra.ipc import russ

services_config = {}
try:
    services_config = llsdhttp.get(config.get('services-config'))
except:
    pass

# convenience method for when you can't be arsed to make your own object (i.e. all the time)
_g_builder = None
def build(name, context):
    global _g_builder
    if _g_builder is None:
        _g_builder = ServiceBuilder()
    _g_builder.buildServiceURL(name, context)

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

    def buildServiceURL(self, name, context):
        """\
        @brief given the environment on construction, return a service URL.
        @param name The name of the service.
        @param context A dict of name value lookups for the service.
        @returns Returns the 
        """
        base_url = config.get('services-base-url')
        svc_path = russ.format(self.builders[name], context)
        return base_url + svc_path
