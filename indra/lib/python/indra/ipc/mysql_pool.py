"""\
@file mysql_pool.py
@brief Thin wrapper around eventlet.db_pool that chooses MySQLdb and Tpool.

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

import MySQLdb
from eventlet import db_pool

class DatabaseConnector(db_pool.DatabaseConnector):
    def __init__(self, credentials, *args, **kwargs):
        super(DatabaseConnector, self).__init__(MySQLdb, credentials,
                                                conn_pool=db_pool.ConnectionPool,
                                                *args, **kwargs)

    # get is extended relative to eventlet.db_pool to accept a port argument
    def get(self, host, dbname, port=3306):
        key = (host, dbname, port)
        if key not in self._databases:
            new_kwargs = self._kwargs.copy()
            new_kwargs['db'] = dbname
            new_kwargs['host'] = host
            new_kwargs['port'] = port
            new_kwargs.update(self.credentials_for(host))
            dbpool = ConnectionPool(*self._args, **new_kwargs)
            self._databases[key] = dbpool

        return self._databases[key]

class ConnectionPool(db_pool.TpooledConnectionPool):
    """A pool which gives out saranwrapped MySQLdb connections from a pool
    """

    def __init__(self, *args, **kwargs):
        super(ConnectionPool, self).__init__(MySQLdb, *args, **kwargs)

    def get(self):
        conn = super(ConnectionPool, self).get()
        # annotate the connection object with the details on the
        # connection; this is used elsewhere to check that you haven't
        # suddenly changed databases in midstream while making a
        # series of queries on a connection.
        arg_names = ['host','user','passwd','db','port','unix_socket','conv','connect_timeout',
         'compress', 'named_pipe', 'init_command', 'read_default_file', 'read_default_group',
         'cursorclass', 'use_unicode', 'charset', 'sql_mode', 'client_flag', 'ssl',
         'local_infile']
        # you could have constructed this connectionpool with a mix of
        # keyword and non-keyword arguments, but we want to annotate
        # the connection object with a dict so it's easy to check
        # against so here we are converting the list of non-keyword
        # arguments (in self._args) into a dict of keyword arguments,
        # and merging that with the actual keyword arguments
        # (self._kwargs).  The arg_names variable lists the
        # constructor arguments for MySQLdb Connection objects.
        converted_kwargs = dict([ (arg_names[i], arg) for i, arg in enumerate(self._args) ])
        converted_kwargs.update(self._kwargs)
        conn.connection_parameters = converted_kwargs
        return conn

