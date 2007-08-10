"""\
@file mysql_pool.py
@brief Uses saranwrap to implement a pool of nonblocking database connections to a mysql server.

Copyright (c) 2007, Linden Research, Inc.
$License$
"""

import os

from eventlet.pools import Pool
from eventlet.processes import DeadProcess
from indra.ipc import saranwrap

import MySQLdb

class ConnectionPool(Pool):
    """A pool which gives out saranwrapped MySQLdb connections from a pool
    """
    def __init__(self, min_size = 0, max_size = 4, *args, **kwargs):
        self._args = args
        self._kwargs = kwargs
        Pool.__init__(self, min_size, max_size)

    def create(self):
        return saranwrap.wrap(MySQLdb).connect(*self._args, **self._kwargs)

    def put(self, conn):
        # poke the process to see if it's dead or None
        try:
            status = saranwrap.status(conn)
        except (AttributeError, DeadProcess), e:
            conn = self.create()

        if conn:
            Pool.put(self, conn)
        else:
            self.current_size -= 1
