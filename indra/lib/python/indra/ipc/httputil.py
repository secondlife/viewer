
import warnings

warnings.warn("indra.ipc.httputil has been deprecated; use eventlet.httpc instead", DeprecationWarning, 2)

from eventlet.httpc import *


makeConnection = make_connection
